// Phase 1 deterministic-runtime behavioural tests.
//
// Test artifact for the Phase 1 runtime contracts (RT-0001..RT-0020). It
// exercises real code paths (no mocks) and asserts the phase's acceptance
// properties: deterministic ordering, no lookahead, no repaint, independent
// per-stream state, deterministic decision identity, shadow-only execution
// (never a live order), append-only idempotent ledger, and reproducible replay.
// Exits non-zero on any failure.

#include "foundation/DataQualityState.h"
#include "foundation/EventId.h"
#include "foundation/EventMetadata.h"
#include "foundation/EventType.h"
#include "foundation/EntityId.h"
#include "foundation/HashDigest.h"
#include "foundation/IPersistenceStore.h"
#include "foundation/PersistenceRecordMetadata.h"
#include "foundation/PersistenceStatus.h"
#include "foundation/SchemaVersion.h"
#include "foundation/Timestamp.h"
#include "foundation/Version.h"
#include "runtime/AdapterManager.h"
#include "runtime/BarFinalizer.h"
#include "runtime/ConfidenceEngine.h"
#include "runtime/DataBus.h"
#include "runtime/DataValidator.h"
#include "runtime/EligibilityEngine.h"
#include "runtime/FeatureEngine.h"
#include "runtime/MacroContextEngine.h"
#include "runtime/MarketQualityEngine.h"
#include "runtime/PositionSimulator.h"
#include "runtime/ReconciliationEngine.h"
#include "runtime/RegimeEngine.h"
#include "runtime/ReplayEngine.h"
#include "runtime/RiskEngine.h"
#include "runtime/ScoreEngine.h"
#include "runtime/ShadowExecutionEngine.h"
#include "runtime/ShadowLedger.h"
#include "runtime/SignalEngine.h"
#include "runtime/StructureEngine.h"
#include "runtime/TimeframeStateStore.h"

#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

using namespace aura::runtime;
using aura::foundation::DataQualityState;
using aura::foundation::EventId;
using aura::foundation::EventMetadata;
using aura::foundation::EventType;
using aura::foundation::SchemaVersion;
using aura::foundation::Timestamp;
using aura::foundation::Version;

static int g_failures = 0;
static void check(bool condition, const char* what) {
    if (!condition) {
        std::printf("FAIL: %s\n", what);
        ++g_failures;
    }
}

// In-memory append-only store used to exercise ledger persistence idempotency.
class MemoryStore final : public aura::foundation::IPersistenceStore {
public:
    aura::foundation::PersistenceStatus append(
        const aura::foundation::PersistenceRecordMetadata& metadata,
        const aura::foundation::HashDigest& payload_digest) override {
        const auto it = records_.find(metadata.record_id());
        if (it != records_.end()) {
            return it->second == payload_digest ? aura::foundation::PersistenceStatus::OK
                                                : aura::foundation::PersistenceStatus::CONFLICT;
        }
        records_.emplace(metadata.record_id(), payload_digest);
        return aura::foundation::PersistenceStatus::OK;
    }
    aura::foundation::PersistenceStatus contains(const aura::foundation::EntityId& id) const override {
        return records_.find(id) != records_.end() ? aura::foundation::PersistenceStatus::OK
                                                   : aura::foundation::PersistenceStatus::NOT_FOUND;
    }
    aura::foundation::PersistenceStatus flush() override {
        return aura::foundation::PersistenceStatus::OK;
    }
    std::size_t size() const { return records_.size(); }

private:
    std::map<aura::foundation::EntityId, aura::foundation::HashDigest> records_;
};

static MarketBar bar(Timeframe tf, Timestamp open_t, Timestamp close_t, double o, double h, double l,
                     double c, bool closed = true) {
    MarketBar b;
    b.timeframe = tf;
    b.open_time = open_t;
    b.close_time = close_t;
    b.open = o;
    b.high = h;
    b.low = l;
    b.close = c;
    b.volume = 100.0;
    b.closed = closed;
    return b;
}

static MarketEvent event(const MarketBar& b, const std::string& symbol, std::uint64_t seq,
                         Timestamp receive = Timestamp{}) {
    MarketEvent e;
    e.bar = b;
    const Timestamp et = b.close_time;
    const Timestamp rt = receive.nanoseconds() == 0 ? b.close_time : receive;
    e.metadata = EventMetadata(EventId::from_string("evt-" + symbol + "-" + std::to_string(seq)),
                               EventType::MARKET_DATA, symbol, "mt5", "ea-1", et, rt, seq,
                               SchemaVersion::from_string("1.0.0"));
    return e;
}

static void test_adapter_independence() {
    AdapterManager adapters;
    check(adapters.stream_count() == 9, "adapter tracks nine streams");
    check(!adapters.all_healthy(), "streams not healthy before data");

    check(adapters.report_bar(Timeframe::M15, Timestamp::from_seconds(100), 1), "M15 bar recorded");
    check(adapters.report_bar(Timeframe::H4, Timestamp::from_seconds(200), 1), "H4 bar recorded");
    check(adapters.healthy_count() == 2, "two streams healthy");

    // Fail M15; H4 must remain healthy (independent stream state).
    check(adapters.report_error(Timeframe::M15, "gap"), "M15 error recorded");
    check(adapters.status(Timeframe::M15)->state == aura::foundation::ServiceState::DEGRADED,
          "M15 degraded");
    check(adapters.status(Timeframe::H4)->healthy(), "H4 unaffected by M15 failure");

    // Recover M15; H4's bar time must be untouched.
    const Timestamp h4_before = adapters.status(Timeframe::H4)->last_bar_time;
    check(adapters.report_bar(Timeframe::M15, Timestamp::from_seconds(101), 2), "M15 recovered");
    check(adapters.status(Timeframe::H4)->last_bar_time == h4_before,
          "M15 recovery does not overwrite H4 state");
    check(adapters.status(Timeframe::M15)->healthy(), "M15 healthy after recovery");

    // Disconnect one stream only.
    check(adapters.report_disconnected(Timeframe::D1, "no data"), "D1 disconnected");
    check(adapters.status(Timeframe::D1)->state == aura::foundation::ServiceState::OFFLINE,
          "D1 offline");
    check(adapters.status(Timeframe::M15)->healthy(), "M15 still healthy after D1 disconnect");

    // Unknown timeframe is rejected, not defaulted.
    check(!adapters.report_bar(Timeframe::UNKNOWN, Timestamp::from_seconds(1), 1),
          "unknown timeframe rejected");
    check(timeframe_from_string("M15") == Timeframe::M15, "timeframe label parses");
    check(timeframe_from_string("ZZ") == Timeframe::UNKNOWN, "bad timeframe label -> UNKNOWN");
}

static void test_databus_causality() {
    DataBus bus;
    // Future-dated event (event_time > receive_time) must be rejected: no lookahead.
    const MarketBar b1 = bar(Timeframe::M15, Timestamp::from_seconds(60), Timestamp::from_seconds(120),
                             1.0, 1.1, 0.9, 1.05);
    MarketEvent future = event(b1, "XAUUSD", 1, Timestamp::from_seconds(100));
    check(!bus.publish(future), "future-dated event rejected (no lookahead)");

    // Forming bar must be rejected: no repaint.
    MarketBar forming = b1;
    forming.closed = false;
    check(!bus.publish(event(forming, "XAUUSD", 2)), "forming bar rejected (no repaint)");

    // Valid closed events published out of order.
    const MarketBar b2 = bar(Timeframe::M15, Timestamp::from_seconds(120), Timestamp::from_seconds(180),
                             1.05, 1.2, 1.0, 1.15);
    const MarketBar b3 = bar(Timeframe::H4, Timestamp::from_seconds(0), Timestamp::from_seconds(60),
                             1.0, 1.1, 0.95, 1.08);
    check(bus.publish(event(b2, "XAUUSD", 5)), "later M15 event published");
    check(bus.publish(event(b3, "XAUUSD", 3)), "earlier H4 event published");
    const std::vector<MarketEvent> ordered = bus.drain_ordered();
    check(ordered.size() == 2, "two events drained");
    check(ordered[0].metadata.event_time() <= ordered[1].metadata.event_time(),
          "drain is in deterministic event_time order");
    check(bus.empty(), "bus empty after drain");
}

static void test_validation_and_finalization() {
    DataValidator validator;
    check(validator.validate(bar(Timeframe::M15, Timestamp::from_seconds(0), Timestamp::from_seconds(60),
                                 1.0, 1.1, 0.9, 1.05)).quality == DataQualityState::VALID,
          "well-formed bar is VALID");
    check(validator.validate(bar(Timeframe::M15, Timestamp::from_seconds(0), Timestamp::from_seconds(60),
                                 1.0, 0.8, 0.9, 1.05)).quality == DataQualityState::INVALID,
          "high below low is INVALID");
    check(validator.validate(bar(Timeframe::M15, Timestamp::from_seconds(0), Timestamp::from_seconds(60),
                                 1.0, 1.0, 1.0, 1.0)).quality == DataQualityState::DEGRADED,
          "zero-range bar is DEGRADED");
    check(!aura::foundation::is_usable(DataQualityState::UNKNOWN), "UNKNOWN is not usable");

    // Finalizer: no repaint, deterministic identity, per-timeframe independence.
    BarFinalizer finalizer;
    FinalizedBar out;
    check(finalizer.finalize(event(bar(Timeframe::M15, Timestamp::from_seconds(0), Timestamp::from_seconds(60),
                                     1.0, 1.1, 0.9, 1.05), "XAUUSD", 1), out),
          "first M15 bar finalized");
    const BarIdentity first = out.identity;
    // Same close_time again -> repaint attempt rejected.
    check(!finalizer.finalize(event(bar(Timeframe::M15, Timestamp::from_seconds(0), Timestamp::from_seconds(60),
                                     1.0, 1.2, 0.9, 1.15), "XAUUSD", 2), out),
          "re-finalizing same close_time rejected (no repaint)");
    // H4 independent.
    check(finalizer.finalize(event(bar(Timeframe::H4, Timestamp::from_seconds(0), Timestamp::from_seconds(60),
                                     1.0, 1.1, 0.9, 1.05), "XAUUSD", 3), out),
          "H4 bar finalized independently");
    check(out.identity.timeframe() == Timeframe::H4, "H4 identity carries H4");
    check(first.timeframe() == Timeframe::M15, "M15 identity carries M15");
    check(first != out.identity, "different timeframes have different identity");
    check(first.canonical_string().find("M15") != std::string::npos, "identity encodes timeframe");
}

static void test_state_store() {
    TimeframeStateStore store;
    BarFinalizer finalizer;
    FinalizedBar out;
    finalizer.finalize(event(bar(Timeframe::M15, Timestamp::from_seconds(0), Timestamp::from_seconds(60),
                                 1.0, 1.1, 0.9, 1.05), "XAUUSD", 1), out);
    check(store.advance(out), "advance first bar");
    finalizer.finalize(event(bar(Timeframe::M15, Timestamp::from_seconds(60), Timestamp::from_seconds(120),
                                 1.05, 1.2, 1.0, 1.15), "XAUUSD", 2), out);
    check(store.advance(out), "advance second bar");
    check(store.last_processed(Timeframe::M15)->sequence == 2, "sequence advanced to 2");

    // Regression attempt must be refused (no time travel).
    TimeframeProgress stale;
    stale.timeframe = Timeframe::M15;
    stale.last_processed = BarIdentity("XAUUSD", Timeframe::M15, Timestamp::from_seconds(60));
    stale.last_close = Timestamp::from_seconds(60);
    check(!store.restore(stale), "restore cannot regress a timeframe");

    // Encode/decode round-trip.
    TimeframeProgress decoded;
    check(decode_progress(encode_progress(*store.last_processed(Timeframe::M15)), decoded),
          "progress round-trips through text");
    check(decoded.timeframe == Timeframe::M15 && decoded.sequence == 2, "decoded progress matches");

    // Persistence idempotency.
    MemoryStore memory;
    const SchemaVersion schema = SchemaVersion::from_string("1.0.0");
    const auto s1 = store.persist(memory, schema, Timestamp::from_seconds(1000));
    check(aura::foundation::is_success(s1), "persist succeeds");
    const std::size_t count = memory.size();
    const auto s2 = store.persist(memory, schema, Timestamp::from_seconds(1000));
    check(aura::foundation::is_success(s2), "re-persist succeeds");
    check(memory.size() == count, "re-persist is idempotent (no duplicate records)");
}

static void test_pipeline() {
    // Build a rising M15 series and a rising H4 series so structure/regime are
    // trending up.
    std::vector<MarketBar> m15;
    std::vector<MarketBar> h4;
    for (int i = 0; i < 20; ++i) {
        const double base = 100.0 + i;
        m15.push_back(bar(Timeframe::M15, Timestamp::from_seconds(i * 60), Timestamp::from_seconds((i + 1) * 60),
                          base, base + 1.0, base - 0.5, base + 0.8));
    }
    for (int i = 0; i < 20; ++i) {
        const double base = 100.0 + i * 2.0;
        h4.push_back(bar(Timeframe::H4, Timestamp::from_seconds(i * 240), Timestamp::from_seconds((i + 1) * 240),
                         base, base + 3.0, base - 1.0, base + 2.5));
    }

    FeatureEngine features;
    const FeatureSet h4f = features.compute(h4, Timeframe::H4);
    const FeatureSet m15f = features.compute(m15, Timeframe::M15);
    check(h4f.valid && m15f.valid, "feature sets valid");

    // A still-forming bar poisons the feature set (no lookahead).
    std::vector<MarketBar> mixed = m15;
    mixed.push_back(bar(Timeframe::M15, Timestamp::from_seconds(1200), Timestamp::from_seconds(1260),
                        120.0, 121.0, 119.0, 120.5, false));
    check(!features.compute(mixed, Timeframe::M15).valid, "forming bar refuses feature computation");

    StructureEngine structure_engine;
    const StructureContext structure = structure_engine.compute(h4f);
    check(structure.valid && structure.state == StructureState::TREND_UP, "H4 structure is TREND_UP");
    // Structure authority is H4: an M15 feature set is refused.
    check(!structure_engine.compute(m15f).valid, "structure refuses non-H4 timeframe");

    RegimeEngine regime_engine;
    const RegimeVerdict regime = regime_engine.classify(structure, h4f);
    check(regime.valid && regime.regime == Regime::TRENDING_UP, "H4 regime is TRENDING_UP");
    check(regime.score >= 0.0 && regime.score <= 1.0, "regime score bounded (not a probability)");

    // Eligibility gating on data quality.
    EligibilityEngine eligibility;
    check(eligibility.evaluate(m15f, regime, DataQualityState::VALID).decision ==
              EligibilityDecision::ELIGIBLE,
          "valid data + trending regime is ELIGIBLE");
    check(eligibility.evaluate(m15f, regime, DataQualityState::UNKNOWN).decision ==
              EligibilityDecision::INELIGIBLE,
          "UNKNOWN quality is INELIGIBLE");
    check(eligibility.evaluate(m15f, regime, DataQualityState::STALE).decision ==
              EligibilityDecision::INELIGIBLE,
          "STALE quality is INELIGIBLE");

    // Signal identity determinism (V3-23).
    TimeframeStateStore store;
    BarFinalizer finalizer;
    FinalizedBar fb;
    finalizer.finalize(event(m15.back(), "XAUUSD", 20), fb);
    store.advance(fb);

    const EligibilityVerdict eligible = eligibility.evaluate(m15f, regime, DataQualityState::VALID);
    SignalEngine signal_engine(StrategyFamily::S01_SECULAR_TREND_FOLLOWING, Version(1, 0, 0),
                               Version(1, 0, 0));
    const Signal s1 = signal_engine.generate(eligible, regime, store, "XAUUSD");
    const Signal s2 = signal_engine.generate(eligible, regime, store, "XAUUSD");
    check(s1.valid && s1.direction == SignalDirection::LONG, "signal generated LONG");
    check(s1.decision_id == s2.decision_id, "decision id is deterministic");
    check(s1.signal_id == s2.signal_id, "signal id is deterministic");
    // A different configuration version must change the decision id.
    SignalEngine other_config(StrategyFamily::S01_SECULAR_TREND_FOLLOWING, Version(1, 0, 0),
                              Version(2, 0, 0));
    check(other_config.generate(eligible, regime, store, "XAUUSD").decision_id != s1.decision_id,
          "decision id depends on configuration version");

    // Score / confidence are ranking values, never probabilities.
    ScoreEngine score_engine;
    const SignalScore score = score_engine.score(s1, regime, m15f);
    check(score.valid && score.score >= 0.0 && score.score <= 1.0, "score bounded");
    ConfidenceEngine confidence_engine;
    const ConfidenceValue confidence = confidence_engine.evaluate(score, DataQualityState::VALID);
    check(confidence.valid, "confidence valid");
    check(confidence_engine.evaluate(score, DataQualityState::UNKNOWN).confidence == 0.0,
          "non-VALID quality yields zero confidence");

    // Market quality from adapters.
    AdapterManager adapters;
    adapters.report_bar(Timeframe::M15, Timestamp::from_seconds(100), 1);
    MarketQualityEngine market_quality_engine;
    MarketQualityVerdict mq = market_quality_engine.evaluate(adapters);
    check(mq.quality == MarketQuality::UNKNOWN && !mq.usable,
          "unassessed streams -> market quality UNKNOWN (not usable)");
    for (const Timeframe tf : all_timeframes()) adapters.report_bar(tf, Timestamp::from_seconds(100), 1);
    mq = market_quality_engine.evaluate(adapters);
    check(mq.quality == MarketQuality::GOOD && mq.usable, "all-valid streams -> market quality GOOD");

    // Risk proposal is not an order.
    RiskEngine risk_engine(100000.0, 0.01);
    const RiskProposal proposal = risk_engine.propose(s1, confidence, mq, m15f.atr, 2.0);
    check(proposal.valid && !proposal.is_order, "risk proposal produced and is not an order");
    check(proposal.position_size > 0.0, "proposal has positive size");
    check(risk_engine.propose(s1, confidence, MarketQualityVerdict{}, m15f.atr, 2.0).valid == false,
          "no proposal without usable market quality");

    // Shadow execution: simulated fill only, never live.
    ShadowExecutionEngine shadow(ExecutionCostModel{0.01, 0.02, 0.0}, 1000000.0);
    const SimulatedFill fill = shadow.simulate(proposal, m15f.last_close, Timestamp::from_seconds(2000));
    check(fill.valid && !fill.is_live, "shadow fill is valid and never live");
    check(fill.filled_size > 0.0, "shadow fill has size");
    check(fill.commission > 0.0, "shadow fill models commission");

    // Partial fill.
    ShadowExecutionEngine partial_shadow(ExecutionCostModel{0.0, 0.0, 0.0}, proposal.position_size / 2.0);
    const SimulatedFill partial = partial_shadow.simulate(proposal, m15f.last_close, Timestamp::from_seconds(2000));
    check(partial.partial && partial.filled_size < proposal.position_size, "partial fill modelled");

    // Position lifecycle: open, partial close, full close.
    PositionSimulator positions;
    const SimulatedPosition opened = positions.open(fill, Timestamp::from_seconds(2000));
    check(opened.valid && opened.state == PositionState::OPEN && !opened.is_live, "position opened");
    const SimulatedPosition half = positions.close(opened, opened.size / 2.0, m15f.last_close + 1.0,
                                                   Timestamp::from_seconds(3000));
    check(half.state == PositionState::PARTIALLY_CLOSED, "position partially closed");
    const SimulatedPosition closed = positions.close(half, half.size, m15f.last_close + 2.0,
                                                     Timestamp::from_seconds(4000));
    check(closed.state == PositionState::CLOSED && closed.size == 0.0, "position fully closed");
    check(positions.events().size() == 3, "three lifecycle events recorded");

    // Reconciliation: matched vs diverged.
    ReconciliationEngine reconciliation(0.5);
    check(reconciliation.reconcile(fill, fill.fill_price).outcome == ReconciliationOutcome::MATCHED,
          "in-tolerance price reconciles MATCHED");
    const ReconciliationResult diverged = reconciliation.reconcile(fill, fill.fill_price + 10.0);
    check(diverged.outcome == ReconciliationOutcome::DIVERGED && !diverged.trustworthy,
          "out-of-tolerance price DIVERGED and not trustworthy");
}

static void test_ledger_and_replay() {
    ShadowLedger ledger;
    MemoryStore memory;
    const SchemaVersion schema = SchemaVersion::from_string("1.0.0");

    // Deterministic entry identity.
    const auto id1 = ShadowLedger::make_entry_id(LedgerEntryType::SIGNAL, {}, "p");
    const auto id2 = ShadowLedger::make_entry_id(LedgerEntryType::SIGNAL, {}, "p");
    check(id1.valid() && id1 == id2, "ledger entry identity is deterministic");

    check(ledger.record(LedgerEntryType::SIGNAL, {}, Timestamp::from_seconds(1), "payload-a"),
          "append first record");
    const std::size_t size = ledger.size();
    check(ledger.record(LedgerEntryType::SIGNAL, {}, Timestamp::from_seconds(1), "payload-a"),
          "re-append identical record is idempotent success");
    check(ledger.size() == size, "idempotent append does not duplicate");
    check(ledger.record(LedgerEntryType::SIGNAL, {}, Timestamp::from_seconds(1), "payload-b"),
          "different payload is a distinct record");
    check(ledger.size() == size + 1, "distinct record appended");

    const auto s = ledger.persist(memory, schema);
    check(aura::foundation::is_success(s), "ledger persist succeeds");
    const std::size_t stored = memory.size();
    const auto s2 = ledger.persist(memory, schema);
    check(aura::foundation::is_success(s2), "ledger re-persist succeeds");
    check(memory.size() == stored, "ledger persist is idempotent");

    // Deterministic replay: identical input -> identical fingerprint.
    std::vector<MarketEvent> events;
    for (int i = 0; i < 10; ++i) {
        const double base = 100.0 + i;
        events.push_back(event(bar(Timeframe::M15, Timestamp::from_seconds(i * 60),
                                   Timestamp::from_seconds((i + 1) * 60), base, base + 1.0,
                                   base - 0.5, base + 0.8),
                               "XAUUSD", static_cast<std::uint64_t>(i)));
    }
    // Add a future-dated and a forming event that replay must reject.
    MarketEvent future = event(bar(Timeframe::M15, Timestamp::from_seconds(600), Timestamp::from_seconds(660),
                                   110.0, 111.0, 109.0, 110.5),
                               "XAUUSD", 100, Timestamp::from_seconds(500));
    events.push_back(future);
    MarketBar forming = bar(Timeframe::M15, Timestamp::from_seconds(660), Timestamp::from_seconds(720),
                            111.0, 112.0, 110.0, 111.5, false);
    events.push_back(event(forming, "XAUUSD", 101));

    ReplayEngine replay;
    const ReplayResult r1 = replay.replay(events);
    const ReplayResult r2 = replay.replay(events);
    check(r1.valid && r1.finalized == 10, "replay finalized ten closed bars");
    check(r1.rejected == 2, "replay rejected future-dated and forming events");
    check(r1.fingerprint == r2.fingerprint, "replay fingerprint is deterministic");
    check(replay.reproducible(events), "replay is reproducible");

    // Replay through a ledger appends idempotently.
    ShadowLedger replay_ledger;
    replay.replay(events, &replay_ledger, Timestamp::from_seconds(9000));
    const std::size_t ledger_size = replay_ledger.size();
    replay.replay(events, &replay_ledger, Timestamp::from_seconds(9000));
    check(replay_ledger.size() == ledger_size, "replayed ledger writes are idempotent");
}

int main() {
    test_adapter_independence();
    test_databus_causality();
    test_validation_and_finalization();
    test_state_store();
    test_pipeline();
    test_ledger_and_replay();

    if (g_failures == 0) {
        std::printf("Phase 1 runtime behavioural tests: PASS\n");
        return 0;
    }
    std::printf("Phase 1 runtime behavioural tests: %d FAILURE(S)\n", g_failures);
    return 1;
}
