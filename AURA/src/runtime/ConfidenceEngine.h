#ifndef AURA_RUNTIME_CONFIDENCEENGINE_H
#define AURA_RUNTIME_CONFIDENCEENGINE_H

#include "foundation/DataQualityState.h"
#include "runtime/ScoreEngine.h"

#include <algorithm>

namespace aura {
namespace runtime {

// Deterministic confidence value.
//
// RT-0012 / Phase 1 deterministic runtime. Confidence is a bounded deterministic
// value derived from the score and data quality. It is explicitly NOT a
// guarantee and NOT a calibrated probability (V3-31): no calibration is
// established and none is claimed. UNKNOWN (confidence unavailable) is distinct
// from low confidence.
struct ConfidenceValue {
    foundation::HashDigest decision_id{};
    double confidence{0.0};
    bool valid{false};
};

// Derives confidence deterministically.
//
// RT-0012 / Phase 1 deterministic runtime. Pure function of the score and data
// quality; same inputs always yield the same value. No randomness, no clock, no
// I/O. The output is a confidence value, never a guarantee.
class ConfidenceEngine {
public:
    ConfidenceEngine() = default;

    ConfidenceValue evaluate(const SignalScore& score, foundation::DataQualityState quality) const {
        ConfidenceValue result;
        result.decision_id = score.decision_id;
        if (!score.valid) return result;

        // Confidence is the score scaled by a deterministic data-quality factor.
        // A non-VALID quality cannot yield full confidence.
        double factor = 1.0;
        if (!foundation::is_usable(quality)) factor = 0.0;
        else factor = 1.0;

        result.confidence = std::clamp(score.score * factor, 0.0, 1.0);
        result.valid = true;
        return result;
    }
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_CONFIDENCEENGINE_H
