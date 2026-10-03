#ifndef AURA_VALIDATION_STATISTICALCONTROLS_H
#define AURA_VALIDATION_STATISTICALCONTROLS_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace aura {
namespace validation {

// Multiple-testing evidence tools (Master 14.3).
enum class StatisticalTool : std::uint8_t {
    PBO = 0,
    DSR,
    WHITE_REALITY_CHECK,
    SUPERIOR_PREDICTIVE_ABILITY,
    PURGED_CPCV,
    WALK_FORWARD,
    MONTE_CARLO_STRESS,
};

constexpr std::string_view to_string(StatisticalTool t) noexcept {
    switch (t) {
        case StatisticalTool::PBO:                       return "PBO";
        case StatisticalTool::DSR:                       return "DSR";
        case StatisticalTool::WHITE_REALITY_CHECK:       return "WHITE_REALITY_CHECK";
        case StatisticalTool::SUPERIOR_PREDICTIVE_ABILITY: return "SUPERIOR_PREDICTIVE_ABILITY";
        case StatisticalTool::PURGED_CPCV:               return "PURGED_CPCV";
        case StatisticalTool::WALK_FORWARD:              return "WALK_FORWARD";
        case StatisticalTool::MONTE_CARLO_STRESS:        return "MONTE_CARLO_STRESS";
    }
    return "PBO";
}

// A reported statistical result. Values are provided by the trusted evaluator;
// this layer only records and gates them.
struct StatisticalEvidence {
    StatisticalTool tool{StatisticalTool::PBO};
    double value{0.0};
    bool available{false};
    std::string notes{};
};

// Statistical controls gate (Master 14.3).
//
// Phase 6 validation. Records multiple-testing results and gates review on their
// availability. The tool NEVER states a profitability or calibrated-probability
// claim: a present, in-threshold result is necessary evidence, not a guarantee.
// Deterministic; no I/O.
class StatisticalControls {
public:
    // True when every required tool has an available result. Missing evidence is
    // not treated as passing.
    static bool evidence_complete(const std::vector<StatisticalTool>& required,
                                  const std::vector<StatisticalEvidence>& evidence) noexcept {
        for (StatisticalTool t : required) {
            bool found = false;
            for (const StatisticalEvidence& e : evidence) {
                if (e.tool == t && e.available) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        return true;
    }

    // PBO is lower-is-better (probability of backtest overfitting); DSR is
    // higher-is-better. This only reports whether the threshold is met; it makes
    // no claim of future performance.
    static bool within_threshold(const StatisticalEvidence& e, double threshold) noexcept {
        switch (e.tool) {
            case StatisticalTool::PBO:
                return e.value <= threshold;
            case StatisticalTool::DSR:
                return e.value >= threshold;
            default:
                return true;  // other tools carry no universal threshold here
        }
    }
};

}  // namespace validation
}  // namespace aura

#endif  // AURA_VALIDATION_STATISTICALCONTROLS_H
