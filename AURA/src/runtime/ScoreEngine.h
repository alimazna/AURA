#ifndef AURA_RUNTIME_SCOREENGINE_H
#define AURA_RUNTIME_SCOREENGINE_H

#include "runtime/FeatureEngine.h"
#include "runtime/RegimeEngine.h"
#include "runtime/SignalEngine.h"

#include <algorithm>
#include <cmath>

namespace aura {
namespace runtime {

// Deterministic signal score.
//
// RT-0011 / Phase 1 deterministic runtime. A score is a deterministic
// ranking/quality value in [0,1]. It is explicitly NOT a probability (V3-31):
// the type, the field name and the comments never call it one, and no
// calibration is implied. UNKNOWN (score unavailable) is distinct from a low
// score and is never treated as neutral.
struct SignalScore {
    foundation::HashDigest decision_id{};
    double score{0.0};
    bool valid{false};
};

// Scores a signal deterministically.
//
// RT-0011 / Phase 1 deterministic runtime. Pure function of the signal, regime
// and features; same inputs always yield the same score. No randomness, no
// clock, no I/O. The result is a ranking value only, never a probability.
class ScoreEngine {
public:
    ScoreEngine() = default;

    SignalScore score(const Signal& signal, const RegimeVerdict& regime,
                      const FeatureSet& features) const {
        SignalScore result;
        result.decision_id = signal.decision_id;
        if (!signal.valid || !regime.valid || !features.valid) return result;

        // Bounded, deterministic blend of regime ranking score and trend
        // magnitude. Deliberately not normalised into a probability.
        const double magnitude = features.atr > 0.0
                                     ? std::min(1.0, std::abs(features.return_n) * features.last_close /
                                                         (features.atr * 10.0))
                                     : 0.0;
        double value = 0.5 * std::clamp(regime.score, 0.0, 1.0) + 0.5 * magnitude;
        value = std::clamp(value, 0.0, 1.0);

        result.score = value;
        result.valid = true;
        return result;
    }

    // Convenience: direction agreement contributes to the score deterministically.
    SignalScore score_with_direction(const Signal& signal, const RegimeVerdict& regime,
                                     const FeatureSet& features) const {
        SignalScore result = score(signal, regime, features);
        if (!result.valid) return result;
        if (signal.direction == SignalDirection::LONG && features.return_n < 0.0) result.score *= 0.5;
        if (signal.direction == SignalDirection::SHORT && features.return_n > 0.0) result.score *= 0.5;
        return result;
    }
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_SCOREENGINE_H
