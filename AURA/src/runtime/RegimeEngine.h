#ifndef AURA_RUNTIME_REGIMEENGINE_H
#define AURA_RUNTIME_REGIMEENGINE_H

#include "runtime/AdapterManager.h"
#include "runtime/FeatureEngine.h"
#include "runtime/StructureEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace aura {
namespace runtime {

// Market regime classification (V3-05: regime before strategy eligibility).
//
// RT-0008 / Phase 1 deterministic runtime. Regime is a descriptive classification
// derived from closed data. The accompanying score is a deterministic ranking
// value only and is never a probability (V3-31). UNKNOWN means regime could not
// be established and must not be treated as neutral.
enum class Regime : std::uint8_t {
    UNKNOWN = 0,
    TRENDING_UP,
    TRENDING_DOWN,
    RANGING,
    TRANSITIONAL,
};

constexpr std::string_view to_string(Regime regime) noexcept {
    switch (regime) {
        case Regime::UNKNOWN:       return "UNKNOWN";
        case Regime::TRENDING_UP:   return "TRENDING_UP";
        case Regime::TRENDING_DOWN: return "TRENDING_DOWN";
        case Regime::RANGING:       return "RANGING";
        case Regime::TRANSITIONAL:  return "TRANSITIONAL";
    }
    return "UNKNOWN";
}

// Regime verdict with a deterministic ranking score (not a probability).
struct RegimeVerdict {
    Timeframe timeframe{Timeframe::UNKNOWN};
    Regime regime{Regime::UNKNOWN};
    BarIdentity based_on{};
    double score{0.0};  // deterministic ranking/quality value, NOT a probability
    bool valid{false};
};

// Classifies regime deterministically from closed data.
//
// RT-0008 / Phase 1 deterministic runtime. Uses the structural context
// (RT-0007) and the operational feature set; a disagreement between structure and
// trend magnitude is classified TRANSITIONAL rather than silently resolved. The
// score is explicitly a ranking value, never labelled or treated as a probability
// (V3-31). An invalid input yields UNKNOWN. Deterministic and pure.
class RegimeEngine {
public:
    RegimeEngine() = default;

    RegimeVerdict classify(const StructureContext& structure, const FeatureSet& features) const {
        RegimeVerdict verdict;
        verdict.timeframe = features.timeframe;
        verdict.based_on = features.based_on;
        if (!structure.valid || !features.valid) return verdict;
        if (structure.timeframe != features.timeframe) return verdict;

        verdict.valid = true;
        const double span = features.highest_high - features.lowest_low;
        const double normalised_move = span > 0.0 ? features.return_n * features.last_close / span : 0.0;

        switch (structure.state) {
            case StructureState::TREND_UP:
                verdict.regime = normalised_move >= 0.0 ? Regime::TRENDING_UP : Regime::TRANSITIONAL;
                break;
            case StructureState::TREND_DOWN:
                verdict.regime = normalised_move <= 0.0 ? Regime::TRENDING_DOWN : Regime::TRANSITIONAL;
                break;
            case StructureState::RANGE:
                verdict.regime = Regime::RANGING;
                break;
            case StructureState::UNKNOWN:
                return verdict;
        }

        // Deterministic ranking score in [0,1]; not a probability.
        double score = 0.0;
        if (features.atr > 0.0 && span > 0.0) {
            score = std::min(1.0, std::abs(features.return_n) * features.last_close / span);
        }
        verdict.score = score;
        return verdict;
    }
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_REGIMEENGINE_H
