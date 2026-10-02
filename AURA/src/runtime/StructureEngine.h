#ifndef AURA_RUNTIME_STRUCTUREENGINE_H
#define AURA_RUNTIME_STRUCTUREENGINE_H

#include "runtime/AdapterManager.h"
#include "runtime/FeatureEngine.h"

#include <cstdint>
#include <string_view>

namespace aura {
namespace runtime {

// H4 structural context (V3-29: H4 is primary structural authority).
//
// RT-0007 / Phase 1 deterministic runtime. Derived from closed H4 features only.
// TREND/RANGE are descriptive structural labels; UNKNOWN means structure could
// not be established and must not be treated as a neutral trend or range.
enum class StructureState : std::uint8_t {
    UNKNOWN = 0,
    TREND_UP,
    TREND_DOWN,
    RANGE,
};

constexpr std::string_view to_string(StructureState state) noexcept {
    switch (state) {
        case StructureState::UNKNOWN:    return "UNKNOWN";
        case StructureState::TREND_UP:   return "TREND_UP";
        case StructureState::TREND_DOWN: return "TREND_DOWN";
        case StructureState::RANGE:      return "RANGE";
    }
    return "UNKNOWN";
}

// Structural context derived from the structural timeframe.
struct StructureContext {
    Timeframe timeframe{Timeframe::UNKNOWN};
    StructureState state{StructureState::UNKNOWN};
    BarIdentity based_on{};
    double range_position{0.0};  // 0 at window low, 1 at window high
    bool valid{false};
};

// Derives H4 structural context deterministically.
//
// RT-0007 / Phase 1 deterministic runtime. Consumes the H4 feature set (closed
// data only) and classifies structure. A non-H4 timeframe is refused so the H4
// structural authority (V3-29) cannot be silently overridden by another
// timeframe; an invalid feature set yields UNKNOWN. Deterministic and pure.
class StructureEngine {
public:
    StructureEngine() = default;

    StructureContext compute(const FeatureSet& h4_features) const {
        StructureContext context;
        context.timeframe = h4_features.timeframe;
        context.based_on = h4_features.based_on;

        // Structural authority is H4; refuse any other timeframe rather than
        // substituting it.
        if (h4_features.timeframe != Timeframe::H4) return context;
        if (!h4_features.valid) return context;

        context.valid = true;
        const double span = h4_features.highest_high - h4_features.lowest_low;
        context.range_position =
            span > 0.0 ? (h4_features.last_close - h4_features.lowest_low) / span : 0.5;

        const double threshold = h4_features.atr;
        if (threshold <= 0.0) {
            context.state = StructureState::RANGE;
        } else if (h4_features.return_n > 0.0 && h4_features.last_close > h4_features.highest_high - threshold) {
            context.state = StructureState::TREND_UP;
        } else if (h4_features.return_n < 0.0 && h4_features.last_close < h4_features.lowest_low + threshold) {
            context.state = StructureState::TREND_DOWN;
        } else {
            context.state = StructureState::RANGE;
        }
        return context;
    }
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_STRUCTUREENGINE_H
