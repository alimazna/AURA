#ifndef AURA_RUNTIME_SIGNALENGINE_H
#define AURA_RUNTIME_SIGNALENGINE_H

#include "foundation/EntityId.h"
#include "foundation/HashAlgorithm.h"
#include "foundation/HashDigest.h"
#include "foundation/IHasher.h"
#include "foundation/Version.h"
#include "runtime/AdapterManager.h"
#include "runtime/EligibilityEngine.h"
#include "runtime/FeatureEngine.h"
#include "runtime/RegimeEngine.h"
#include "runtime/TimeframeStateStore.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace aura {
namespace runtime {

// Core strategy families (V3-30). The MVP uses one family at a time; the
// identifier is carried on the signal for provenance.
enum class StrategyFamily : std::uint8_t {
    UNKNOWN = 0,
    S01_SECULAR_TREND_FOLLOWING,
    S02_MULTI_YEAR_MACRO_SWING,
    S03_SOVEREIGN_BREAKOUT,
    S04_REGIME_TRANSITION,
    S05_STRUCTURAL_RETEST,
    S06_DEFENSIVE_EXIT_DE_RISKING,
};

constexpr std::string_view to_string(StrategyFamily family) noexcept {
    switch (family) {
        case StrategyFamily::UNKNOWN:                      return "UNKNOWN";
        case StrategyFamily::S01_SECULAR_TREND_FOLLOWING:  return "S01";
        case StrategyFamily::S02_MULTI_YEAR_MACRO_SWING:   return "S02";
        case StrategyFamily::S03_SOVEREIGN_BREAKOUT:       return "S03";
        case StrategyFamily::S04_REGIME_TRANSITION:        return "S04";
        case StrategyFamily::S05_STRUCTURAL_RETEST:        return "S05";
        case StrategyFamily::S06_DEFENSIVE_EXIT_DE_RISKING: return "S06";
    }
    return "UNKNOWN";
}

// Signal direction. NONE means no actionable signal was produced.
enum class SignalDirection : std::uint8_t {
    NONE = 0,
    LONG,
    SHORT,
};

constexpr std::string_view to_string(SignalDirection direction) noexcept {
    switch (direction) {
        case SignalDirection::NONE:  return "NONE";
        case SignalDirection::LONG:  return "LONG";
        case SignalDirection::SHORT: return "SHORT";
    }
    return "NONE";
}

// Deterministic signal with reproducible identity (V3-23).
//
// RT-0010 / Phase 1 deterministic runtime. `decision_id` is
// Hash(symbol, trigger timeframe, closed bar id, strategy version, configuration
// version) exactly as V3-23 prescribes; `signal_id` is a stable function of that
// digest. Both are reproducible across processes and depend only on closed data,
// so the same closed bar always yields the same identity (idempotency).
struct Signal {
    foundation::EntityId signal_id{};
    foundation::HashDigest decision_id{};
    std::string symbol{};
    Timeframe trigger_timeframe{Timeframe::UNKNOWN};
    BarIdentity closed_bar{};
    StrategyFamily family{StrategyFamily::UNKNOWN};
    SignalDirection direction{SignalDirection::NONE};
    foundation::Version strategy_version{};
    foundation::Version configuration_version{};
    bool valid{false};
};

// Generates signals for one MVP strategy family.
//
// RT-0010 / Phase 1 deterministic runtime. Emits a signal only when the
// eligibility gate (RT-0009) is ELIGIBLE; the direction is a pure function of the
// regime. Identity is computed deterministically from stable inputs (V3-23) and
// never depends on wall-clock time, arrival order or mutable state. No lookahead
// and no repaint: the trigger bar is the finalized closed bar from the timeframe
// state store (RT-0005). No I/O.
class SignalEngine {
public:
    SignalEngine(StrategyFamily family, foundation::Version strategy_version,
                 foundation::Version configuration_version)
        : family_(family),
          strategy_version_(std::move(strategy_version)),
          configuration_version_(std::move(configuration_version)) {}

    // Builds a signal from an eligibility verdict, regime and the store's last
    // processed closed bar. Returns an invalid signal when the gate is not
    // ELIGIBLE or identity inputs are missing.
    Signal generate(const EligibilityVerdict& eligibility, const RegimeVerdict& regime,
                    const TimeframeStateStore& store, const std::string& symbol) const {
        Signal signal;
        signal.symbol = symbol;
        signal.trigger_timeframe = eligibility.timeframe;
        signal.family = family_;
        signal.strategy_version = strategy_version_;
        signal.configuration_version = configuration_version_;

        if (eligibility.decision != EligibilityDecision::ELIGIBLE) return signal;
        if (!regime.valid || eligibility.timeframe == Timeframe::UNKNOWN) return signal;
        const TimeframeProgress* progress = store.last_processed(eligibility.timeframe);
        if (progress == nullptr) return signal;

        SignalDirection direction = SignalDirection::NONE;
        if (regime.regime == Regime::TRENDING_UP) direction = SignalDirection::LONG;
        else if (regime.regime == Regime::TRENDING_DOWN) direction = SignalDirection::SHORT;
        if (direction == SignalDirection::NONE) return signal;

        signal.closed_bar = progress->last_processed;
        signal.direction = direction;

        const foundation::HashDigest decision_id = compute_decision_id(signal);
        if (decision_id.empty()) return signal;
        signal.decision_id = decision_id;
        signal.signal_id = foundation::EntityId(std::string("sig|") + decision_id.to_hex() + "|" +
                                                std::string(to_string(direction)));
        signal.valid = true;
        return signal;
    }

    // DecisionID = Hash(symbol, trigger timeframe, closed bar ID, strategy
    // version, configuration version) per V3-23.
    foundation::HashDigest compute_decision_id(const Signal& signal) const {
        const std::unique_ptr<foundation::IHasher> hasher =
            foundation::create_hasher(foundation::HashAlgorithm::SHA256);
        if (hasher == nullptr) return {};
        const std::string material = signal.symbol + "|" +
                                     std::string(to_string(signal.trigger_timeframe)) + "|" +
                                     signal.closed_bar.canonical_string() + "|" +
                                     signal.strategy_version.to_string() + "|" +
                                     signal.configuration_version.to_string();
        return hasher->hash(material);
    }

    StrategyFamily family() const noexcept { return family_; }

private:
    StrategyFamily family_;
    foundation::Version strategy_version_;
    foundation::Version configuration_version_;
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_SIGNALENGINE_H
