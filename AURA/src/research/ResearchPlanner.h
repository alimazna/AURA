#ifndef AURA_RESEARCH_RESEARCHPLANNER_H
#define AURA_RESEARCH_RESEARCHPLANNER_H

#include "foundation/Timestamp.h"
#include "research/Experiment.h"

#include <cstdint>
#include <string>
#include <vector>

namespace aura {
namespace research {

// Validation methods (Master 14.2 pipeline). Not every candidate uses every method.
enum class ValidationMethod : std::uint8_t {
    LEAKAGE_CHECKS = 0,
    TIME_AWARE_VALIDATION,
    WALK_FORWARD,
    PURGED_CPCV,
    STATISTICAL_CONTROLS,
    PBO_DSR,
    STRESS_MONTE_CARLO,
    REGRESSION,
    LOCKED_OOS_HOLDOUT,
};

// A validation plan fixed before the candidate is evaluated.
struct ValidationPlan {
    std::vector<ValidationMethod> methods{};
    bool requires_locked_oos{false};
    bool requires_holdout{false};
    bool allow_adaptive_selection{true};
    foundation::Timestamp fixed_at{};

    bool empty() const noexcept { return methods.empty(); }
};

// Research planner (Master 14.2 / INVARIANT 05).
//
// Phase 4 research plane. The planner chooses the validation protocol *before* the
// candidate is evaluated, rather than letting the candidate choose the most
// favorable test afterwards. The plan is fixed at registration time and cannot be
// silently changed after results exist (metric/validation immutability within an
// experiment family). Deterministic: the plan is a pure function of the evidence
// zone and the requested comparison class.
class ResearchPlanner {
public:
    // Deterministic protocol for a given evidence zone. E0 exploration gets only
    // leakage + time-aware checks (cheap, no OOS/holdout burn). The deeper zones
    // accumulate stricter methods. Locked OOS/holdout is reserved for E2/E3.
    static ValidationPlan plan_for(EvidenceZone zone, bool structural_change) {
        ValidationPlan p;
        p.methods.push_back(ValidationMethod::LEAKAGE_CHECKS);
        p.methods.push_back(ValidationMethod::TIME_AWARE_VALIDATION);
        p.allow_adaptive_selection = (zone == EvidenceZone::E0_EXPLORATION);

        if (zone == EvidenceZone::E1_DEVELOPMENT || zone == EvidenceZone::E2_OUT_OF_SAMPLE ||
            zone == EvidenceZone::E3_LOCKED_HOLDOUT) {
            p.methods.push_back(ValidationMethod::WALK_FORWARD);
            p.methods.push_back(ValidationMethod::STATISTICAL_CONTROLS);
            p.methods.push_back(ValidationMethod::REGRESSION);
        }
        if (structural_change || zone == EvidenceZone::E2_OUT_OF_SAMPLE ||
            zone == EvidenceZone::E3_LOCKED_HOLDOUT) {
            p.methods.push_back(ValidationMethod::PURGED_CPCV);
            p.methods.push_back(ValidationMethod::PBO_DSR);
            p.methods.push_back(ValidationMethod::STRESS_MONTE_CARLO);
        }
        if (zone == EvidenceZone::E2_OUT_OF_SAMPLE || zone == EvidenceZone::E3_LOCKED_HOLDOUT) {
            p.methods.push_back(ValidationMethod::LOCKED_OOS_HOLDOUT);
            p.requires_locked_oos = true;
            p.requires_holdout = (zone == EvidenceZone::E3_LOCKED_HOLDOUT);
        }
        return p;
    }

    // Fixes a plan for an experiment. A plan must be non-empty and is stamped with
    // the fixing time so post-hoc changes are detectable.
    static bool fix(ValidationPlan& plan, foundation::Timestamp at) {
        if (plan.empty()) return false;
        plan.fixed_at = at;
        return true;
    }

    // True when `proposed` is identical to `fixed` in the fields that matter for
    // evidential immutability. A change to the method set or locked-oos/holdout
    // requirement after fixing is rejected.
    static bool is_immutable_against(const ValidationPlan& fixed,
                                     const ValidationPlan& proposed) noexcept {
        return fixed.methods == proposed.methods &&
               fixed.requires_locked_oos == proposed.requires_locked_oos &&
               fixed.requires_holdout == proposed.requires_holdout;
    }
};

}  // namespace research
}  // namespace aura

#endif  // AURA_RESEARCH_RESEARCHPLANNER_H
