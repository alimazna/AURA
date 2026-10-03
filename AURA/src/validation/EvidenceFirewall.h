#ifndef AURA_VALIDATION_EVIDENCEFIREWALL_H
#define AURA_VALIDATION_EVIDENCEFIREWALL_H

#include "foundation/EntityId.h"
#include "research/Experiment.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace aura {
namespace validation {

// Evidence firewall decision (Master 15.2).
enum class HoldoutDecision : std::uint8_t {
    DENIED = 0,
    GRANTED,
    BUDGET_EXHAUSTED,
    INVALID_REQUEST,
};

constexpr std::string_view to_string(HoldoutDecision d) noexcept {
    switch (d) {
        case HoldoutDecision::DENIED:            return "DENIED";
        case HoldoutDecision::GRANTED:           return "GRANTED";
        case HoldoutDecision::BUDGET_EXHAUSTED:  return "BUDGET_EXHAUSTED";
        case HoldoutDecision::INVALID_REQUEST:   return "INVALID_REQUEST";
    }
    return "DENIED";
}

// A holdout query request. The research engine never reads raw holdout data; it
// submits a candidate hash, experiment definition, artifact hash and evaluator
// version, and the service returns a predefined output.
struct HoldoutRequest {
    std::string candidate_hash{};
    std::string experiment_definition{};
    std::string artifact_hash{};
    std::string evaluator_version{};
    research::EvidenceZone zone{research::EvidenceZone::E3_LOCKED_HOLDOUT};

    bool valid() const noexcept {
        return !candidate_hash.empty() && !experiment_definition.empty() &&
               !artifact_hash.empty() && !evaluator_version.empty();
    }
};

// Immutable audit entry for every holdout query.
struct HoldoutAuditEntry {
    foundation::EntityId query_id{};
    std::string candidate_hash{};
    std::string experiment_definition{};
    std::string artifact_hash{};
    std::string evaluator_version{};
    HoldoutDecision decision{HoldoutDecision::DENIED};
};

// Evidence firewall / holdout service (Master 15.1/15.2).
//
// Phase 6 validation. Enforces the protected design: no raw holdout access (only
// predefined outputs), an immutable query audit, a query budget, and a
// contamination transition. Once locked-holdout evidence is consumed the
// candidate's contamination state advances and a return to the same optimization
// loop with that evidence is refused. Deterministic; no raw data ever crosses.
class EvidenceFirewall {
public:
    explicit EvidenceFirewall(std::int64_t query_budget) : query_budget_(query_budget) {}

    std::int64_t remaining_budget() const noexcept { return query_budget_ - queries_used_; }
    std::int64_t queries_used() const noexcept { return queries_used_; }

    // Evaluates a request. A malformed request is INVALID_REQUEST (not a granted
    // query). Held-out (holdout) access additionally requires the budget and
    // contaminates: a request for locked holdout on a candidate already
    // contaminated is denied.
    HoldoutDecision query(const HoldoutRequest& request, research::ContaminationState& contamination,
                          HoldoutAuditEntry& audit_out) {
        audit_out = HoldoutAuditEntry{};
        audit_out.candidate_hash = request.candidate_hash;
        audit_out.experiment_definition = request.experiment_definition;
        audit_out.artifact_hash = request.artifact_hash;
        audit_out.evaluator_version = request.evaluator_version;

        if (!request.valid()) {
            audit_out.decision = HoldoutDecision::INVALID_REQUEST;
            queries_used_ += 1;
            return HoldoutDecision::INVALID_REQUEST;
        }
        // contamination is monotone: contaminated candidates cannot re-query holdout
        if (request.zone == research::EvidenceZone::E3_LOCKED_HOLDOUT &&
            contamination == research::ContaminationState::CONTAMINATED) {
            audit_out.decision = HoldoutDecision::DENIED;
            queries_used_ += 1;
            return HoldoutDecision::DENIED;
        }
        if (queries_used_ >= query_budget_) {
            audit_out.decision = HoldoutDecision::BUDGET_EXHAUSTED;
            queries_used_ += 1;
            return HoldoutDecision::BUDGET_EXHAUSTED;
        }
        queries_used_ += 1;
        audit_out.decision = HoldoutDecision::GRANTED;
        // Exposing locked holdout advances contamination, never silently resets.
        if (request.zone == research::EvidenceZone::E3_LOCKED_HOLDOUT) {
            contamination = research::ContaminationState::HOLDOUT_EXPOSED;
        } else if (request.zone == research::EvidenceZone::E2_OUT_OF_SAMPLE &&
                   contamination == research::ContaminationState::CLEAN) {
            contamination = research::ContaminationState::OOS_EXPOSED;
        }
        return HoldoutDecision::GRANTED;
    }

private:
    std::int64_t query_budget_{0};
    std::int64_t queries_used_{0};
};

// Contamination state machine (Master 15.3). Monotone: forward-only. Returns
// false when a move is not a legal forward transition (no silent reset).
inline bool transition_contamination(research::ContaminationState from,
                                     research::ContaminationState to) noexcept {
    using S = research::ContaminationState;
    if (from == to) return true;  // idempotent
    switch (from) {
        case S::CLEAN:
            return to == S::VALIDATED || to == S::OOS_EXPOSED || to == S::HOLDOUT_EXPOSED ||
                   to == S::RETIRED;
        case S::VALIDATED:
            return to == S::OOS_EXPOSED || to == S::HOLDOUT_EXPOSED || to == S::RETIRED;
        case S::OOS_EXPOSED:
            return to == S::HOLDOUT_EXPOSED || to == S::CONTAMINATED || to == S::RETIRED;
        case S::HOLDOUT_EXPOSED:
            return to == S::CONTAMINATED || to == S::RETIRED;
        case S::CONTAMINATED:
            return to == S::RETIRED;
        case S::RETIRED:
            return false;
    }
    return false;
}

}  // namespace validation
}  // namespace aura

#endif  // AURA_VALIDATION_EVIDENCEFIREWALL_H
