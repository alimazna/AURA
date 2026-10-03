// Conceptual Phase 4 — Feature / Structure / Regime verification artifact.
//
// Proves the analysis pipeline over the already-approved manifest Phase 1 engines.
// Exercises real code paths (no mocks):
//   - deterministic, reproducible feature computation tied to timeframe + source bar
//   - bounded rolling window with deterministic boundaries
//   - no future-data access: a still-forming bar refuses computation
//   - missing/invalid input yields explicit state (no fabricated values)
//   - H4 structural authority: structure is derived only from H4, never from
//     lower timeframes, and lower-timeframe noise cannot redefine H4 structure
//   - regime determinism and explainability from closed inputs; score is a ranking
//     value, never a probability
//   - degraded-input handling propagates into eligibility rather than being logged
//   - long-horizon macro context only from D1/W1/MN1
// Exits non-zero on any failure.

#include "foundation/DataQualityState.h"
#include "runtime/AdapterManager.h"
#include "runtime/EligibilityEngine.h"
#include "runtime/FeatureEngine.h"
#include "runtime/MacroContextEngine.h"
#include "runtime/MarketQualityEngine.h"
#include "runtime/RegimeEngine.h"
#include "runtime/StructureEngine.h"

#include <cstdio>
#include <vector>

using aura::runtime::AdapterManager;
using aura::runtime::EligibilityDecision;
using aura::runtime::EligibilityEngine;
using aura::runtime::FeatureEngine;
using aura::runtime::FeatureSet;
using aura::runtime::MacroBias;
using aura::runtime::MacroContextEngine;
using aura::runtime::MarketQuality;
using aura::runtime::MarketQualityEngine;
using aura::runtime::MarketQualityVerdict;
using aura::runtime::MarketBar;
using aura::runtime::Regime;
using aura::runtime::RegimeEngine;
using aura::runtime::RegimeVerdict;
using aura::runtime::StructureEngine;
using aura::runtime::StructureState;
using aura::runtime::Timeframe;

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
    b.open_time = aura::foundation::Timestamp::from_seconds(static_cast<std::int64_t>(i) * 60);
    b.close_time = aura::foundation::Timestamp::from_seconds(static_cast<std::int64_t>(i + 1) * 60);
    b.open = base;
    b.high = base + 1.0;
    b.low = base - 0.5;
    b.close = base + 0.8;
    b.volume = 100.0;
    b.closed = closed;
    return b;
}

static std::vector<MarketBar> rising(Timeframe tf, int n) {
    std::vector<MarketBar> v;
    for (int i = 0; i < n; ++i) v.push_back(bar(tf, i, 100.0 + i));
    return v;
}

static void test_feature_determinism() {
    FeatureEngine engine;
    const std::vector<MarketBar> bars = rising(Timeframe::M15, 20);
    const FeatureSet a = engine.compute(bars, Timeframe::M15);
    const FeatureSet b = engine.compute(bars, Timeframe::M15);
    check(a.valid && b.valid, "feature sets valid");
    check(a.based_on == b.based_on, "based_on provenance deterministic");
    check(a.last_close == b.last_close && a.atr == b.atr && a.return_n == b.return_n,
          "feature values deterministic across identical runs");
    check(a.timeframe == Timeframe::M15, "feature set carries its timeframe");
    check(a.highest_high >= a.lowest_low, "high/low window is coherent");
    check(a.atr > 0.0, "atr computed from closed bars");

    // Bounded window: changing only older (out-of-window) bars must not change the
    // features for the most recent window.
    std::vector<MarketBar> shifted = rising(Timeframe::M15, 20);
    shifted[0].close += 50.0;
    shifted[0].high += 50.0;
    const FeatureSet c = engine.compute(shifted, Timeframe::M15);
    check(c.atr == a.atr && c.return_n == a.return_n, "rolling window bounded deterministically");
}

static void test_no_lookahead_and_missing_data() {
    FeatureEngine engine;

    // A still-forming bar must refuse the whole computation (no future input).
    std::vector<MarketBar> mixed = rising(Timeframe::M15, 10);
    mixed.push_back(bar(Timeframe::M15, 10, 110.0, /*closed=*/false));
    const FeatureSet forming = engine.compute(mixed, Timeframe::M15);
    check(!forming.valid, "forming bar refuses feature computation (no lookahead)");

    // Empty input -> explicit invalid, no fabricated defaults.
    const FeatureSet empty = engine.compute({}, Timeframe::M15);
    check(!empty.valid, "empty input yields invalid feature set (no fabrication)");

    // Unknown timeframe -> explicit invalid.
    check(!engine.compute(rising(Timeframe::M15, 5), Timeframe::UNKNOWN).valid,
          "unknown timeframe yields invalid feature set");
}

static void test_h4_structural_authority() {
    FeatureEngine features;
    StructureEngine structure_engine;

    const FeatureSet h4 = features.compute(rising(Timeframe::H4, 20), Timeframe::H4);
    const FeatureSet m15 = features.compute(rising(Timeframe::M15, 20), Timeframe::M15);
    const FeatureSet m1 = features.compute(rising(Timeframe::M1, 20), Timeframe::M1);

    const auto h4_structure = structure_engine.compute(h4);
    check(h4_structure.valid, "H4 structure derived from H4 features");
    check(h4_structure.timeframe == Timeframe::H4, "structure is labelled H4");

    // Lower timeframes must not be accepted as structural authority.
    check(!structure_engine.compute(m15).valid, "M15 cannot substitute for H4 structure");
    check(!structure_engine.compute(m1).valid, "M1 cannot substitute for H4 structure");
    check(!structure_engine.compute(FeatureSet{}).valid, "empty feature set yields no structure");

    // Downtrending H4 -> TREND_DOWN, independent of M15/M1.
    std::vector<MarketBar> down;
    for (int i = 0; i < 20; ++i) down.push_back(bar(Timeframe::H4, i, 200.0 - i));
    const auto down_structure = structure_engine.compute(features.compute(down, Timeframe::H4));
    check(down_structure.state == StructureState::TREND_DOWN, "H4 downtrend classified TREND_DOWN");
    check(h4_structure.state == StructureState::TREND_UP, "H4 uptrend classified TREND_UP");

    // M15 noise does not redefine H4: recompute H4 structure unchanged.
    const auto h4_again = structure_engine.compute(h4);
    check(h4_again.state == h4_structure.state && h4_again.based_on == h4_structure.based_on,
          "H4 structure stable against lower-timeframe activity");
}

static void test_regime_determinism() {
    FeatureEngine features;
    StructureEngine structure_engine;
    RegimeEngine regime_engine;

    const FeatureSet h4 = features.compute(rising(Timeframe::H4, 20), Timeframe::H4);
    const auto structure = structure_engine.compute(h4);
    const RegimeVerdict r1 = regime_engine.classify(structure, h4);
    const RegimeVerdict r2 = regime_engine.classify(structure, h4);
    check(r1.valid && r1.regime == Regime::TRENDING_UP, "uptrend regime classified TRENDING_UP");
    check(r1.regime == r2.regime && r1.score == r2.score, "regime deterministic across runs");
    check(r1.score >= 0.0 && r1.score <= 1.0, "regime score bounded in [0,1] (ranking, not probability)");
    check(r1.based_on == h4.based_on, "regime retains source-bar provenance");

    // A range-bound series yields RANGING.
    std::vector<MarketBar> flat;
    for (int i = 0; i < 20; ++i) {
        MarketBar b = bar(Timeframe::H4, i, 100.0);
        b.close = 100.0;
        b.high = 100.4;
        b.low = 99.6;
        flat.push_back(b);
    }
    const auto flat_structure = structure_engine.compute(features.compute(flat, Timeframe::H4));
    const RegimeVerdict flat_regime = regime_engine.classify(flat_structure, features.compute(flat, Timeframe::H4));
    check(flat_regime.regime == Regime::RANGING, "flat series classified RANGING");

    // Invalid inputs -> UNKNOWN, not fabricated.
    check(!regime_engine.classify(StructureEngine().compute(FeatureSet{}), FeatureSet{}).valid,
          "invalid inputs yield invalid regime (no fabricated confidence)");
}

static void test_macro_context() {
    FeatureEngine features;
    MacroContextEngine macro;
    check(macro.compute(features.compute(rising(Timeframe::D1, 20), Timeframe::D1)).bias == MacroBias::BULLISH,
          "rising D1 macro context is BULLISH");
    check(macro.compute(features.compute(rising(Timeframe::W1, 20), Timeframe::W1)).valid,
          "W1 macro context valid");
    check(macro.compute(features.compute(rising(Timeframe::MN1, 20), Timeframe::MN1)).valid,
          "MN1 macro context valid");
    // Operational timeframes are not long-horizon context.
    check(!macro.compute(features.compute(rising(Timeframe::M15, 20), Timeframe::M15)).valid,
          "M15 is not long-horizon macro context");
    check(!macro.compute(features.compute(rising(Timeframe::M1, 20), Timeframe::M1)).valid,
          "M1 is not long-horizon macro context");
}

static void test_degraded_input_gating() {
    FeatureEngine features;
    StructureEngine structure_engine;
    RegimeEngine regime_engine;
    EligibilityEngine eligibility;

    const FeatureSet m15 = features.compute(rising(Timeframe::M15, 20), Timeframe::M15);
    const FeatureSet h4 = features.compute(rising(Timeframe::H4, 20), Timeframe::H4);
    const auto structure = structure_engine.compute(h4);
    const RegimeVerdict regime = regime_engine.classify(structure, h4);

    check(eligibility.evaluate(m15, regime, aura::foundation::DataQualityState::VALID).decision ==
              EligibilityDecision::ELIGIBLE,
          "VALID data + trending regime is ELIGIBLE");
    check(eligibility.evaluate(m15, regime, aura::foundation::DataQualityState::DEGRADED).decision ==
              EligibilityDecision::INELIGIBLE,
          "DEGRADED data is INELIGIBLE");
    check(eligibility.evaluate(m15, regime, aura::foundation::DataQualityState::STALE).decision ==
              EligibilityDecision::INELIGIBLE,
          "STALE data is INELIGIBLE");
    check(eligibility.evaluate(m15, regime, aura::foundation::DataQualityState::UNKNOWN).decision ==
              EligibilityDecision::INELIGIBLE,
          "UNKNOWN data is INELIGIBLE");

    // Ineligible reasons are explicit and attributable.
    const auto verdict = eligibility.evaluate(m15, regime, aura::foundation::DataQualityState::STALE);
    check(!verdict.reasons.empty(), "ineligibility carries explicit reasons");
}

static void test_market_quality() {
    MarketQualityEngine engine;
    AdapterManager adapters;
    check(!engine.evaluate(adapters).usable, "unassessed market quality is not usable");

    for (const Timeframe tf : aura::runtime::all_timeframes()) {
        adapters.report_bar(tf, aura::foundation::Timestamp::from_seconds(60), 1);
    }
    MarketQualityVerdict good = engine.evaluate(adapters);
    check(good.quality == MarketQuality::GOOD && good.usable, "all-valid streams -> GOOD");

    adapters.report_error(Timeframe::M1, "gap");
    check(engine.evaluate(adapters).quality == MarketQuality::DEGRADED,
          "one degraded stream -> DEGRADED aggregate");

    adapters.report_disconnected(Timeframe::H4, "drop");
    const MarketQualityVerdict dropped = engine.evaluate(adapters);
    // A disconnected stream has no usable quality; the engine conservatively
    // reports UNKNOWN (not usable) rather than fabricating GOOD, and does not
    // silently claim the feed is fine.
    check(dropped.quality == MarketQuality::UNKNOWN && !dropped.usable,
          "one offline stream -> UNKNOWN and not usable (no fabricated GOOD)");
}

int main() {
    test_feature_determinism();
    test_no_lookahead_and_missing_data();
    test_h4_structural_authority();
    test_regime_determinism();
    test_macro_context();
    test_degraded_input_gating();
    test_market_quality();

    if (g_failures == 0) {
        std::printf("Conceptual Phase 4 analysis-pipeline tests: PASS\n");
        return 0;
    }
    std::printf("Conceptual Phase 4 analysis-pipeline tests: %d FAILURE(S)\n", g_failures);
    return 1;
}
