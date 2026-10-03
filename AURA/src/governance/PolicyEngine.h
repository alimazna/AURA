#ifndef AURA_GOVERNANCE_POLICYENGINE_H
#define AURA_GOVERNANCE_POLICYENGINE_H

#include "evolution/Candidate.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace aura {
namespace governance {

// Minimal control (Master 38). Names the conceptual control a change type needs.
enum class Control : std::uint8_t {
    TIME_AWARE_VALIDATION = 0,
    OUT_OF_SAMPLE,
    SENSITIVITY,
    MULTIPLE_TESTING_CORRECTION,
    REGRESSION,
    REGIME_SLICES,
    LOGICAL_STATIC_CHECKS,
    POINT_IN_TIME_CHECKS,
    LINEAGE,
    LEAKAGE_TESTS,
    ABLATION_AVAILABILITY,
    FULL_CANDIDATE_PROTOCOL,
    MULTI_REGIME_ANALYSIS,
    INVARIANT_SUITE,
    WORST_CASE_TESTS,
    BROKER_MARKET_QUALITY_VALIDATION,
    DEDICATED_SIMULATION_TESTS,
    EVIDENCE_FAMILY_SEPARATION,
    STRICTER_HUMAN_GOVERNANCE,
    TRUSTED_PLANE_REVIEW,
    NEW_EVALUATOR_AND_EVIDENCE_FAMILY,
    SANDBOX_ONLY,
    STRUCTURAL_REVIEW,
    INTEGRATION_TESTS,
};

constexpr std::string_view to_string(Control c) noexcept {
    switch (c) {
        case Control::TIME_AWARE_VALIDATION:          return "TIME_AWARE_VALIDATION";
        case Control::OUT_OF_SAMPLE:                  return "OUT_OF_SAMPLE";
        case Control::SENSITIVITY:                    return "SENSITIVITY";
        case Control::MULTIPLE_TESTING_CORRECTION:    return "MULTIPLE_TESTING_CORRECTION";
        case Control::REGRESSION:                     return "REGRESSION";
        case Control::REGIME_SLICES:                  return "REGIME_SLICES";
        case Control::LOGICAL_STATIC_CHECKS:          return "LOGICAL_STATIC_CHECKS";
        case Control::POINT_IN_TIME_CHECKS:           return "POINT_IN_TIME_CHECKS";
        case Control::LINEAGE:                        return "LINEAGE";
        case Control::LEAKAGE_TESTS:                  return "LEAKAGE_TESTS";
        case Control::ABLATION_AVAILABILITY:          return "ABLATION_AVAILABILITY";
        case Control::FULL_CANDIDATE_PROTOCOL:        return "FULL_CANDIDATE_PROTOCOL";
        case Control::MULTI_REGIME_ANALYSIS:          return "MULTI_REGIME_ANALYSIS";
        case Control::INVARIANT_SUITE:                return "INVARIANT_SUITE";
        case Control::WORST_CASE_TESTS:               return "WORST_CASE_TESTS";
        case Control::BROKER_MARKET_QUALITY_VALIDATION: return "BROKER_MARKET_QUALITY_VALIDATION";
        case Control::DEDICATED_SIMULATION_TESTS:     return "DEDICATED_SIMULATION_TESTS";
        case Control::EVIDENCE_FAMILY_SEPARATION:     return "EVIDENCE_FAMILY_SEPARATION";
        case Control::STRICTER_HUMAN_GOVERNANCE:      return "STRICTER_HUMAN_GOVERNANCE";
        case Control::TRUSTED_PLANE_REVIEW:           return "TRUSTED_PLANE_REVIEW";
        case Control::NEW_EVALUATOR_AND_EVIDENCE_FAMILY: return "NEW_EVALUATOR_AND_EVIDENCE_FAMILY";
        case Control::SANDBOX_ONLY:                   return "SANDBOX_ONLY";
        case Control::STRUCTURAL_REVIEW:              return "STRUCTURAL_REVIEW";
        case Control::INTEGRATION_TESTS:              return "INTEGRATION_TESTS";
    }
    return "UNKNOWN";
}

// The policy required before a change type may run (Master 38). This table is a
// planning policy, not a claim that one suite is universally sufficient.
struct PolicyRequirement {
    evolution::ChangeType change_type{evolution::ChangeType::NONE};
    std::vector<Control> minimum_controls{};
    std::vector<Control> additional_controls{};
    bool requires_human_approval{false};
    bool sandbox_only{false};
};

// Policy engine (Master 38/39 trusted control plane).
//
// Phase 7 governance. Selects the validation policy for a change type *before* the
// candidate runs. It never runs the candidate, never promotes, and never modifies
// runtime. Deterministic pure function of the change type.
class PolicyEngine {
public:
    static PolicyRequirement requirement_for(evolution::ChangeType type) {
        using evolution::ChangeType;
        PolicyRequirement r;
        r.change_type = type;
        switch (type) {
            case ChangeType::PARAMETER:
                r.minimum_controls = {Control::TIME_AWARE_VALIDATION, Control::OUT_OF_SAMPLE,
                                      Control::SENSITIVITY};
                r.additional_controls = {Control::MULTIPLE_TESTING_CORRECTION};
                break;
            case ChangeType::RULE:
                r.minimum_controls = {Control::REGRESSION, Control::REGIME_SLICES,
                                      Control::OUT_OF_SAMPLE};
                r.additional_controls = {Control::LOGICAL_STATIC_CHECKS};
                break;
            case ChangeType::FEATURE:
                r.minimum_controls = {Control::POINT_IN_TIME_CHECKS, Control::LINEAGE,
                                      Control::LEAKAGE_TESTS};
                r.additional_controls = {Control::ABLATION_AVAILABILITY};
                break;
            case ChangeType::STRATEGY:
                r.minimum_controls = {Control::FULL_CANDIDATE_PROTOCOL};
                r.additional_controls = {Control::MULTI_REGIME_ANALYSIS};
                break;
            case ChangeType::RISK:
                r.minimum_controls = {Control::INVARIANT_SUITE, Control::WORST_CASE_TESTS};
                r.requires_human_approval = true;
                break;
            case ChangeType::EXECUTION:
                r.minimum_controls = {Control::BROKER_MARKET_QUALITY_VALIDATION};
                r.additional_controls = {Control::DEDICATED_SIMULATION_TESTS};
                break;
            case ChangeType::RESEARCH_METHOD:
                r.minimum_controls = {Control::EVIDENCE_FAMILY_SEPARATION};
                r.additional_controls = {Control::STRICTER_HUMAN_GOVERNANCE};
                r.requires_human_approval = true;
                break;
            case ChangeType::EVALUATOR:
                r.minimum_controls = {Control::TRUSTED_PLANE_REVIEW};
                r.additional_controls = {Control::NEW_EVALUATOR_AND_EVIDENCE_FAMILY};
                r.requires_human_approval = true;
                break;
            case ChangeType::ARCHITECTURE:
                r.minimum_controls = {Control::SANDBOX_ONLY};
                r.additional_controls = {Control::STRUCTURAL_REVIEW, Control::INTEGRATION_TESTS};
                r.sandbox_only = true;
                break;
            case ChangeType::META:
                r.minimum_controls = {Control::EVIDENCE_FAMILY_SEPARATION};
                r.additional_controls = {Control::STRICTER_HUMAN_GOVERNANCE};
                r.requires_human_approval = true;
                r.sandbox_only = true;
                break;
            case ChangeType::NONE:
                break;
        }
        return r;
    }
};

}  // namespace governance
}  // namespace aura

#endif  // AURA_GOVERNANCE_POLICYENGINE_H
