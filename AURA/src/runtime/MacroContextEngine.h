#ifndef AURA_RUNTIME_MACROCONTEXTENGINE_H
#define AURA_RUNTIME_MACROCONTEXTENGINE_H

#include "runtime/AdapterManager.h"
#include "runtime/FeatureEngine.h"
#include "runtime/TimeframeStateStore.h"

#include <cstdint>
#include <string_view>

namespace aura {
namespace runtime {

// Long-horizon macro context (V3-29: D1/W1/MN1 are long-horizon context).
//
// RT-0013 / Phase 1 deterministic runtime. Descriptive bias derived from closed
// higher-timeframe data. UNKNOWN means macro context could not be established and
// must not be treated as neutral.
enum class MacroBias : std::uint8_t {
    UNKNOWN = 0,
    BULLISH,
    BEARISH,
    NEUTRAL,
};

constexpr std::string_view to_string(MacroBias bias) noexcept {
    switch (bias) {
        case MacroBias::UNKNOWN: return "UNKNOWN";
        case MacroBias::BULLISH: return "BULLISH";
        case MacroBias::BEARISH: return "BEARISH";
        case MacroBias::NEUTRAL: return "NEUTRAL";
    }
    return "UNKNOWN";
}

struct MacroContext {
    MacroBias bias{MacroBias::UNKNOWN};
    BarIdentity based_on{};
    Timeframe timeframe{Timeframe::UNKNOWN};
    bool valid{false};
};

// Derives higher-timeframe macro context deterministically.
//
// RT-0013 / Phase 1 deterministic runtime. Consumes only closed higher-timeframe
// features (D1/W1/MN1); a lower timeframe is refused so macro context cannot be
// derived from operational data (no lookahead across the authority model). An
// invalid feature set yields UNKNOWN. Deterministic and pure.
class MacroContextEngine {
public:
    MacroContextEngine() = default;

    MacroContext compute(const FeatureSet& higher_timeframe_features) const {
        MacroContext context;
        context.timeframe = higher_timeframe_features.timeframe;
        context.based_on = higher_timeframe_features.based_on;

        const Timeframe tf = higher_timeframe_features.timeframe;
        if (tf != Timeframe::D1 && tf != Timeframe::W1 && tf != Timeframe::MN1) return context;
        if (!higher_timeframe_features.valid) return context;

        context.valid = true;
        if (higher_timeframe_features.return_n > 0.0) context.bias = MacroBias::BULLISH;
        else if (higher_timeframe_features.return_n < 0.0) context.bias = MacroBias::BEARISH;
        else context.bias = MacroBias::NEUTRAL;
        return context;
    }
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_MACROCONTEXTENGINE_H
