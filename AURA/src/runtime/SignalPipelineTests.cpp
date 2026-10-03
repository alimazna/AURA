// Conceptual Phase 5 — Signal verification artifact.
//
// Proves the signal pipeline over the already-approved manifest Phase 1 engines.
// Exercises real code paths (no mocks):
//   - deterministic signal identity (V3-23 DecisionID) and reproducibility
//   - eligibility constraints gate the signal (no silent permission)
//   - H4 structural authority + M15 operational trigger preserved
//   - score and confidence are deterministic ranking values, never probabilities
//   - the risk gate enforces quality/validity constraints
//   - shadow-only execution: every fill/position is_live == false, proposal
//     is_order == false, and there is no live order path
//   - the shadow ledger is append-only and idempotent, preserving provenance
//   - attempted live execution is blocked/unavailable (negative test)
// Exits non-zero on any failure.

#include "foundation/DataQualityState.h"
#include "foundation/EventId.h"
#include "foundation/EventMetadata.h"
#include "foundation/EventType.h"
#include "foundation/SchemaVersion.h"
#include "foundation/Timestamp.h"
#include "runtime/AdapterManager.h"
#include "runtime/BarFinalizer.h"
#include "runtime/ConfidenceEngine.h"
#include "runtime/EligibilityEngine.h"
#include "runtime/FeatureEngine.h"
#include "runtime/MarketQualityEngine.h"
#include "runtime/PositionSimulator.h"
#include "runtime/RegimeEngine.h"
#include "runtime/RiskEngine.h"
#include "runtime/ShadowExecutionEngine.h"
#include "runtime/ShadowLedger.h"
#include "runtime/SignalEngine.h"
#include "runtime/StructureEngine.h"
#include "runtime/TimeframeStateStore.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace aura::runtime;
using aura::foundation::DataQualityState;
using aura::foundation::SchemaVersion;
using aura::foundation::Timestamp;

static int g_failures = 0;
static void check(bool condition, const char* what) {
    if (!condition) {
        std::printf("FAIL: %s\n", what);
        ++g_failures;
    }
}

static MarketBar bar(Timeframe tf, int i, double base, bool closed = true) {
    MarketBar b;
    b.timeframe = tf;
    b.open_time = Timestamp::from_seconds(static_cast<std::int64_t>(i) * 60);
    b.close_time = Timestamp::from_seconds(static_cast<std::int64_t>(i + 1) * 60);
    b.open = base;
    b.high = base + 1.0;
    b.low = base - 0.5;
    b.close = base + 0.8;
    b.volume = 100.0;
    b.closed = closed;
    return b;
}

static std::vector<MarketBar> series(Timeframe tf, int n, double base0, double step) {
    std::vector<MarketBar> v;
    for (int i = 0; i < n; ++i) v.push_back(bar(tf, i, base0 + step * i));
    return v;
}

struct Fixture {
    FeatureEngine features;
    StructureEngine structure_engine;
    RegimeEngine regime_engine;
    EligibilityEngine eligibility;
    TimeframeStateStore store;

    FeatureSet h4_features;
    FeatureSet m15_features;
    StructureContext h4_structure;
    RegimeVerdict regime;
    EligibilityVerdict verdict;
    MarketQualityVerdict market_quality;

    Fixture() {
        h4_features = features.compute(series(Timeframe::H4, 20, 100.0, 1.0), Timeframe::H4);
        m15_features = features.compute(series(Timeframe::M15, 20, 100.0, 1.0), Timeframe::M15);
        h4_structure = structure_engine.compute(h4_features);
        regime = regime_engine.classify(h4_structure, h4_features);
        verdict = eligibility.evaluate(m15_features, regime, DataQualityState::VALID);

        // Populate the store's last processed M15 bar deterministically so the
        // signal can attach a closed-bar identity.
        BarFinalizer finalizer;
        FinalizedBar fb;
        MarketBar last = bar(Timeframe::M15, 999, 150.0);
        if (finalizer.finalize(make_event(last, 1), fb)) store.advance(fb);

        AdapterManager adapters;
        for (const Timeframe tf : all_timeframes()) {
            adapters.report_bar(tf, last.close_time, 1);
        }
        market_quality = MarketQualityEngine().evaluate(adapters);
    }

    static MarketEvent make_event(const MarketBar& b, std::uint64_t seq) {
        MarketEvent e;
        e.bar = b;
        e.metadata = aura::foundation::EventMetadata(
            aura::foundation::EventId::from_string("evt-" + std::to_string(seq)),
            aura::foundation::EventType::MARKET_DATA, "XAUUSD", "mt5", "ea-1", b.close_time,
            b.close_time, seq, SchemaVersion::from_string("1.0.0"));
        return e;
    }
};

static void test_signal_determinism_and_authority() {
    Fixture f;
    check(f.verdict.decision == EligibilityDecision::ELIGIBLE, "fixture is ELIGIBLE");
    check(f.verdict.timeframe == Timeframe::M15, "operational trigger is M15");
    check(f.h4_structure.timeframe == Timeframe::H4 && f.h4_structure.valid,
          "structural authority is H4");

    const SignalEngine engine(StrategyFamily::S03_SOVEREIGN_BREAKOUT,
                              aura::foundation::Version::from_string("1.0.0"),
                              aura::foundation::Version::from_string("1.0.0"));
    const Signal s1 = engine.generate(f.verdict, f.regime, f.store, "XAUUSD");
    const Signal s2 = engine.generate(f.verdict, f.regime, f.store, "XAUUSD");
    check(s1.valid && s2.valid, "signal generated");
    check(s1.trigger_timeframe == Timeframe::M15, "signal trigger timeframe is M15 (operational)");
    check(s1.direction == SignalDirection::LONG, "uptrend yields LONG");
    check(s1.decision_id == s2.decision_id && s1.signal_id == s2.signal_id,
          "signal identity deterministic across identical runs");
    check(!s1.decision_id.empty(), "decision id computed (V3-23)");
    check(s1.closed_bar.timeframe() == Timeframe::M15, "signal binds to a closed M15 bar (no lookahead)");

    // A different symbol must yield a different decision identity.
    const Signal other = engine.generate(f.verdict, f.regime, f.store, "XAUUSDX");
    check(other.valid && other.decision_id != s1.decision_id,
          "identity depends on symbol (provenance preserved)");
}

static void test_eligibility_gates_signal() {
    Fixture f;
    const SignalEngine engine(StrategyFamily::S03_SOVEREIGN_BREAKOUT,
                              aura::foundation::Version::from_string("1.0.0"),
                              aura::foundation::Version::from_string("1.0.0"));

    // Ineligible verdict -> no signal (no silent permission).
    EligibilityVerdict ineligible = f.verdict;
    ineligible.decision = EligibilityDecision::INELIGIBLE;
    check(!engine.generate(ineligible, f.regime, f.store, "XAUUSD").valid,
          "ineligible verdict produces no signal");

    // Ranging regime -> ineligible -> no signal.
    const auto flat = series(Timeframe::H4, 20, 100.0, 0.0);
    FeatureEngine features;
    StructureEngine se;
    RegimeEngine re;
    const auto flat_structure = se.compute(features.compute(flat, Timeframe::H4));
    const RegimeVerdict flat_regime = re.classify(flat_structure, features.compute(flat, Timeframe::H4));
    const EligibilityVerdict flat_verdict =
        f.eligibility.evaluate(f.m15_features, flat_regime, DataQualityState::VALID);
    check(flat_verdict.decision == EligibilityDecision::INELIGIBLE, "ranging regime ineligible");
    check(!engine.generate(flat_verdict, flat_regime, f.store, "XAUUSD").valid,
          "ranging regime produces no signal");
}

static void test_score_confidence_are_not_probabilities() {
    Fixture f;
    const SignalEngine engine(StrategyFamily::S03_SOVEREIGN_BREAKOUT,
                              aura::foundation::Version::from_string("1.0.0"),
                              aura::foundation::Version::from_string("1.0.0"));
    const Signal signal = engine.generate(f.verdict, f.regime, f.store, "XAUUSD");

    ScoreEngine scorer;
    const SignalScore sc1 = scorer.score_with_direction(signal, f.regime, f.m15_features);
    const SignalScore sc2 = scorer.score_with_direction(signal, f.regime, f.m15_features);
    check(sc1.valid && sc1.score >= 0.0 && sc1.score <= 1.0, "score bounded in [0,1]");
    check(sc1.score == sc2.score, "score deterministic across runs");

    ConfidenceEngine conf;
    const ConfidenceValue c1 = conf.evaluate(sc1, DataQualityState::VALID);
    const ConfidenceValue c2 = conf.evaluate(sc1, DataQualityState::VALID);
    check(c1.valid && c1.confidence >= 0.0 && c1.confidence <= 1.0, "confidence bounded in [0,1]");
    check(c1.confidence == c2.confidence, "confidence deterministic across runs");
    check(conf.evaluate(sc1, DataQualityState::DEGRADED).confidence == 0.0,
          "non-VALID data cannot yield full confidence (no fabricated confidence)");
}

static void test_risk_gate_and_shadow_only() {
    Fixture f;
    const SignalEngine engine(StrategyFamily::S03_SOVEREIGN_BREAKOUT,
                              aura::foundation::Version::from_string("1.0.0"),
                              aura::foundation::Version::from_string("1.0.0"));
    const Signal signal = engine.generate(f.verdict, f.regime, f.store, "XAUUSD");
    ScoreEngine scorer;
    ConfidenceEngine conf;
    const ConfidenceValue confidence =
        conf.evaluate(scorer.score(signal, f.regime, f.m15_features), DataQualityState::VALID);

    // Market quality must be usable for a proposal.
    const MarketQualityVerdict unusable;  // default: not usable
    RiskEngine risk(100000.0, 0.01);
    check(!risk.propose(signal, confidence, unusable, 5.0).valid,
          "risk proposal refused when market quality not usable");

    // A usable quality + positive ATR yields a deterministic, non-order proposal.
    const MarketQualityVerdict usable = f.market_quality;
    check(usable.usable, "fixture market quality usable");
    const RiskProposal p1 = risk.propose(signal, confidence, usable, 5.0);
    const RiskProposal p2 = risk.propose(signal, confidence, usable, 5.0);
    check(p1.valid && p1.position_size > 0.0, "risk proposal valid with coherent inputs");
    check(p1.is_order == false, "risk proposal is explicitly NOT an order");
    check(p1.position_size == p2.position_size && p1.proposal_id == p2.proposal_id,
          "risk proposal deterministic");

    // Zero/negative ATR -> refused, not fabricated.
    check(!risk.propose(signal, confidence, usable, 0.0).valid, "zero ATR refused by risk gate");

    // Shadow execution: fill and position are never live.
    ShadowExecutionEngine shadow(ExecutionCostModel{0.1, 1.0, 0.0}, 2.0);
    const SimulatedFill fill = shadow.simulate(p1, 150.0, Timestamp::from_seconds(1000));
    check(fill.valid, "shadow fill simulated");
    check(fill.is_live == false, "shadow fill is never live");
    check(shadow.simulate(p1, 150.0, Timestamp::from_seconds(1000)).fill_id == fill.fill_id,
          "shadow fill deterministic");

    PositionSimulator positions;
    const SimulatedPosition position = positions.open(fill, Timestamp::from_seconds(1001));
    check(position.valid && position.is_live == false, "simulated position is never live");
    const SimulatedPosition closed = positions.close(position, position.size, 155.0,
                                                     Timestamp::from_seconds(1100), 1.0);
    check(closed.state == PositionState::CLOSED, "position closes deterministically");
    check(closed.is_live == false, "closed position is never live");
    check(!positions.events().empty(), "position lifecycle is auditable");

    // Negative test: live execution is unavailable, not merely disabled by policy.
    const LiveExecutionResult live = shadow.request_live_execution(p1, 150.0);
    check(!live.executed, "live execution request is blocked/unavailable");
    check(live.reason == LiveExecutionBlockReason::SHADOW_ONLY,
          "live execution blocked because the system is shadow-only");
}

static void test_ledger_append_only_idempotent() {
    Fixture f;
    const SignalEngine engine(StrategyFamily::S03_SOVEREIGN_BREAKOUT,
                              aura::foundation::Version::from_string("1.0.0"),
                              aura::foundation::Version::from_string("1.0.0"));
    const Signal signal = engine.generate(f.verdict, f.regime, f.store, "XAUUSD");
    ScoreEngine scorer;
    ConfidenceEngine conf;
    const ConfidenceValue confidence =
        conf.evaluate(scorer.score(signal, f.regime, f.m15_features), DataQualityState::VALID);
    RiskEngine risk(100000.0, 0.01);
    const RiskProposal proposal = risk.propose(signal, confidence, f.market_quality, 5.0);

    ShadowLedger ledger;
    check(ledger.record_signal(signal, Timestamp::from_seconds(1000)), "signal recorded");
    const std::size_t size = ledger.size();
    check(ledger.record_signal(signal, Timestamp::from_seconds(1000)),
          "duplicate signal record is idempotent success");
    check(ledger.size() == size, "idempotent append does not duplicate");

    // Provenance: the ledger preserves the decision identity chain.
    bool found = false;
    for (const LedgerEntry& e : ledger.ordered()) {
        if (e.type == LedgerEntryType::SIGNAL && e.decision_id == signal.decision_id) found = true;
    }
    check(found, "ledger preserves decision provenance");

    // Shadow fill + position recorded, still append-only.
    ShadowExecutionEngine shadow(ExecutionCostModel{0.1, 1.0, 0.0});
    const SimulatedFill fill = shadow.simulate(proposal, 150.0, Timestamp::from_seconds(1000));
    check(ledger.record_fill(fill, Timestamp::from_seconds(1001)), "fill recorded");
    check(!ledger.record_fill(SimulatedFill{}, Timestamp::from_seconds(1001)),
          "invalid fill rejected by ledger");

    // Deterministic entry identity.
    check(ShadowLedger::make_entry_id(LedgerEntryType::SIGNAL, signal.decision_id, "x") ==
              ShadowLedger::make_entry_id(LedgerEntryType::SIGNAL, signal.decision_id, "x"),
          "ledger entry identity deterministic");
}

int main() {
    test_signal_determinism_and_authority();
    test_eligibility_gates_signal();
    test_score_confidence_are_not_probabilities();
    test_risk_gate_and_shadow_only();
    test_ledger_append_only_idempotent();

    if (g_failures == 0) {
        std::printf("Conceptual Phase 5 signal-pipeline tests: PASS\n");
        return 0;
    }
    std::printf("Conceptual Phase 5 signal-pipeline tests: %d FAILURE(S)\n", g_failures);
    return 1;
}
