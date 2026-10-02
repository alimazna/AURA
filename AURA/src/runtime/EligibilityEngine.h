#ifndef AURA_RUNTIME_ELIGIBILITYENGINE_H
#define AURA_RUNTIME_ELIGIBILITYENGINE_H

#include "foundation/DataQualityState.h"
#include "runtime/AdapterManager.h"
#include "runtime/FeatureEngine.h"
#include "runtime/RegimeEngine.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace aura {
namespace runtime {

// Strategy eligibility decision (V3-05: regime before strategy eligibility).
//
// RT-0009 / Phase 1 deterministic runtime. UNKNOWN is not eligible; an
// unestablished condition must not be treated as permission.
enum class EligibilityDecision : std::uint8_t {
    UNKNOWN = 0,
    ELIGIBLE,
    INELIGIBLE,
};

constexpr std::string_view to_string(EligibilityDecision decision) noexcept {
    switch (decision) {
        case EligibilityDecision::UNKNOWN:    return "UNKNOWN";
        case EligibilityDecision::ELIGIBLE:   return "ELIGIBLE";
        case EligibilityDecision::INELIGIBLE: return "INELIGIBLE";
    }
    return "UNKNOWN";
}

// Eligibility verdict with explicit reasons for every gate that blocked.
struct EligibilityVerdict {
    Timeframe timeframe{Timeframe::UNKNOWN};
    BarIdentity based_on{};
    EligibilityDecision decision{EligibilityDecision::UNKNOWN};
    std::vector<std::string> reasons{};
    bool valid{false};
};

// Gates strategy eligibility on data quality and regime.
//
// RT-0009 / Phase 1 deterministic runtime. A strategy is eligible only when the
// operational timeframe's data quality is VALID (V3-25: quality propagates into
// eligibility, not merely logs), the regime is valid and trending, and the
// feature set is valid. UNKNOWN/INVALID data or an unestablished regime yields
// INELIGIBLE with reasons, never silent permission. Deterministic and pure.
class EligibilityEngine {
public:
    EligibilityEngine() = default;

    EligibilityVerdict evaluate(const FeatureSet& features, const RegimeVerdict& regime,
                                foundation::DataQualityState quality,
                                Timeframe operational_timeframe = Timeframe::M15) const {
        EligibilityVerdict verdict;
        verdict.timeframe = features.timeframe;
        verdict.based_on = features.based_on;

        if (!features.valid || !regime.valid) {
            verdict.decision = EligibilityDecision::UNKNOWN;
            verdict.reasons.push_back("feature/regime input not valid");
            return verdict;
        }
        verdict.valid = true;

        if (features.timeframe != operational_timeframe) {
            verdict.decision = EligibilityDecision::INELIGIBLE;
            verdict.reasons.push_back("not the operational timeframe");
        }
        if (!foundation::is_usable(quality)) {
            verdict.decision = EligibilityDecision::INELIGIBLE;
            verdict.reasons.push_back(std::string("data quality not VALID: ") +
                                      std::string(foundation::to_string(quality)));
        }
        if (regime.regime == Regime::UNKNOWN) {
            verdict.decision = EligibilityDecision::INELIGIBLE;
            verdict.reasons.push_back("regime unknown");
        }
        if (regime.regime == Regime::RANGING || regime.regime == Regime::TRANSITIONAL) {
            verdict.decision = EligibilityDecision::INELIGIBLE;
            verdict.reasons.push_back(std::string("regime not eligible: ") +
                                      std::string(to_string(regime.regime)));
        }

        if (verdict.reasons.empty()) verdict.decision = EligibilityDecision::ELIGIBLE;
        return verdict;
    }

    bool eligible(const EligibilityVerdict& verdict) const noexcept {
        return verdict.decision == EligibilityDecision::ELIGIBLE;
    }
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_ELIGIBILITYENGINE_H
