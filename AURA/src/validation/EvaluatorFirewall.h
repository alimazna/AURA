#ifndef AURA_VALIDATION_EVALUATORFIREWALL_H
#define AURA_VALIDATION_EVALUATORFIREWALL_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace aura {
namespace validation {

// The trusted evaluator components a candidate may never modify (Master 16).
enum class ProtectedComponent : std::uint8_t {
    EVALUATOR = 0,
    METRIC_DEFINITIONS,
    VALIDATION_CODE,
    HOLDOUT_SERVICE,
    AUDIT_LOGGER,
    BUDGET_POLICY,
    ROLLBACK_POLICY,
    APPROVAL_POLICY,
};

constexpr std::string_view to_string(ProtectedComponent c) noexcept {
    switch (c) {
        case ProtectedComponent::EVALUATOR:          return "EVALUATOR";
        case ProtectedComponent::METRIC_DEFINITIONS: return "METRIC_DEFINITIONS";
        case ProtectedComponent::VALIDATION_CODE:    return "VALIDATION_CODE";
        case ProtectedComponent::HOLDOUT_SERVICE:    return "HOLDOUT_SERVICE";
        case ProtectedComponent::AUDIT_LOGGER:       return "AUDIT_LOGGER";
        case ProtectedComponent::BUDGET_POLICY:      return "BUDGET_POLICY";
        case ProtectedComponent::ROLLBACK_POLICY:    return "ROLLBACK_POLICY";
        case ProtectedComponent::APPROVAL_POLICY:    return "APPROVAL_POLICY";
    }
    return "EVALUATOR";
}

// A changed evaluator identity. Any intentional evaluator change creates a new
// evaluator version and a new evidence family.
struct EvaluatorVersion {
    std::string version{};
    std::string metric_registry_version{};
    std::string evidence_family{};

    bool valid() const noexcept { return !version.empty() && !evidence_family.empty(); }
};

// Evaluator firewall (Master 16).
//
// Phase 6 validation. A candidate is submitted against a fixed evaluator/evidence
// family. A candidate that requests (or whose results imply) a different evaluator
// or metric-registry version is refused: it cannot redefine the game it is judged
// by. Crossing to a new evaluator version is legal only as an explicit new
// evidence family, which resets comparability (documented, not silent).
// Deterministic; no I/O.
class EvaluatorFirewall {
public:
    // Returns true only when the candidate's evaluator and metric registry match
    // the family's fixed versions. A mismatch is refused rather than re-scored.
    static bool admits(const EvaluatorVersion& family, const EvaluatorVersion& candidate) noexcept {
        if (!family.valid() || !candidate.valid()) return false;
        return family.version == candidate.version &&
               family.metric_registry_version == candidate.metric_registry_version &&
               family.evidence_family == candidate.evidence_family;
    }

    // True when `candidate` belongs to a different (new) evidence family and so is
    // not comparable to `family`. Callers must not compare across families.
    static bool crosses_family(const EvaluatorVersion& family,
                               const EvaluatorVersion& candidate) noexcept {
        return family.evidence_family != candidate.evidence_family;
    }
};

}  // namespace validation
}  // namespace aura

#endif  // AURA_VALIDATION_EVALUATORFIREWALL_H
