#ifndef AURA_EVOLUTION_CANDIDATE_H
#define AURA_EVOLUTION_CANDIDATE_H

#include "foundation/EntityId.h"
#include "foundation/Timestamp.h"
#include "research/Experiment.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace aura {
namespace evolution {

// Candidate lifecycle (Master 18). No hidden transitions.
enum class CandidateState : std::uint8_t {
    PROPOSED = 0,
    SCHEMA_VALIDATED,
    SANDBOX_READY,
    EXPERIMENTAL,
    VALIDATING,
    PASSED,
    FAILED,
    INCONCLUSIVE,
    REVIEW_READY,
    HUMAN_APPROVED,
    HUMAN_REJECTED,
    MORE_RESEARCH,
    SHADOW_READY,
    SHADOW_ACTIVE,
    PROMOTION_READY,
    PROMOTED,
    ABORTED,
    MONITORED,
    STABLE,
    ROLLED_BACK,
    RETIRED,
};

constexpr std::string_view to_string(CandidateState s) noexcept {
    switch (s) {
        case CandidateState::PROPOSED:         return "PROPOSED";
        case CandidateState::SCHEMA_VALIDATED: return "SCHEMA_VALIDATED";
        case CandidateState::SANDBOX_READY:    return "SANDBOX_READY";
        case CandidateState::EXPERIMENTAL:     return "EXPERIMENTAL";
        case CandidateState::VALIDATING:       return "VALIDATING";
        case CandidateState::PASSED:           return "PASSED";
        case CandidateState::FAILED:           return "FAILED";
        case CandidateState::INCONCLUSIVE:     return "INCONCLUSIVE";
        case CandidateState::REVIEW_READY:     return "REVIEW_READY";
        case CandidateState::HUMAN_APPROVED:   return "HUMAN_APPROVED";
        case CandidateState::HUMAN_REJECTED:   return "HUMAN_REJECTED";
        case CandidateState::MORE_RESEARCH:    return "MORE_RESEARCH";
        case CandidateState::SHADOW_READY:     return "SHADOW_READY";
        case CandidateState::SHADOW_ACTIVE:    return "SHADOW_ACTIVE";
        case CandidateState::PROMOTION_READY:  return "PROMOTION_READY";
        case CandidateState::PROMOTED:         return "PROMOTED";
        case CandidateState::ABORTED:          return "ABORTED";
        case CandidateState::MONITORED:        return "MONITORED";
        case CandidateState::STABLE:           return "STABLE";
        case CandidateState::ROLLED_BACK:      return "ROLLED_BACK";
        case CandidateState::RETIRED:          return "RETIRED";
    }
    return "UNKNOWN";
}

// Mutation/chgange class (Master 12.2).
enum class ChangeType : std::uint8_t {
    NONE = 0,
    PARAMETER,
    RULE,
    FEATURE,
    STRATEGY,
    RISK,
    EXECUTION,
    RESEARCH_METHOD,
    EVALUATOR,
    ARCHITECTURE,
    META,
};

constexpr std::string_view to_string(ChangeType c) noexcept {
    switch (c) {
        case ChangeType::NONE:            return "NONE";
        case ChangeType::PARAMETER:       return "PARAMETER";
        case ChangeType::RULE:            return "RULE";
        case ChangeType::FEATURE:         return "FEATURE";
        case ChangeType::STRATEGY:        return "STRATEGY";
        case ChangeType::RISK:            return "RISK";
        case ChangeType::EXECUTION:       return "EXECUTION";
        case ChangeType::RESEARCH_METHOD: return "RESEARCH_METHOD";
        case ChangeType::EVALUATOR:       return "EVALUATOR";
        case ChangeType::ARCHITECTURE:    return "ARCHITECTURE";
        case ChangeType::META:            return "META";
    }
    return "NONE";
}

// Candidate schema (Master 19, relevant subset). Immutable research artifact.
// It carries its own identity, explicit parent lineage, change description and
// validation/approval/contamination state. It has no path to mutate production:
// promotion is a separate governance action (Phase 7).
struct Candidate {
    foundation::EntityId candidate_id{};
    std::string parent_version{};
    std::string parent_hash{};
    std::string campaign_id{};
    foundation::EntityId hypothesis_id{};
    foundation::EntityId failure_id{};
    std::string reason{};
    ChangeType change_type{ChangeType::NONE};
    std::vector<std::string> changed_components{};
    std::string parameter_set{};
    std::string code_commit{};
    std::string evaluator_version{};
    std::string control_version{};
    std::int64_t trial_number{0};
    std::string random_seed_policy{};
    std::string provenance_digest{};
    std::string rollback_target{};
    CandidateState state{CandidateState::PROPOSED};
    research::ContaminationState contamination{research::ContaminationState::CLEAN};
    // Monotonic revision within a candidate identity; incremented on each
    // lifecycle advance (append-only history).
    std::uint32_t revision{0};
    foundation::Timestamp created_at{};

    bool valid() const noexcept {
        return candidate_id.valid() && !parent_version.empty() && change_type != ChangeType::NONE;
    }
};

inline foundation::EntityId make_candidate_id(std::string_view parent_version,
                                              std::string_view parameter_set,
                                              std::string_view digest) {
    return foundation::EntityId("CAND|" + std::string(parent_version) + "|" +
                                std::string(parameter_set) + "|" + std::string(digest));
}

// Explicit candidate lifecycle transition validator. Encodes exactly the Master 18
// graph plus the terminal/abort branches. Same-state is an idempotent no-op.
class CandidateLifecycle {
public:
    static bool is_terminal(CandidateState s) noexcept {
        return s == CandidateState::RETIRED;
    }

    static bool can_transition(CandidateState from, CandidateState to) noexcept {
        if (from == to) return true;
        switch (from) {
            case CandidateState::PROPOSED:
                return to == CandidateState::SCHEMA_VALIDATED;
            case CandidateState::SCHEMA_VALIDATED:
                return to == CandidateState::SANDBOX_READY;
            case CandidateState::SANDBOX_READY:
                return to == CandidateState::EXPERIMENTAL;
            case CandidateState::EXPERIMENTAL:
                return to == CandidateState::VALIDATING;
            case CandidateState::VALIDATING:
                return to == CandidateState::PASSED || to == CandidateState::FAILED ||
                       to == CandidateState::INCONCLUSIVE;
            case CandidateState::PASSED:
                return to == CandidateState::REVIEW_READY;
            case CandidateState::FAILED:
            case CandidateState::INCONCLUSIVE:
                return to == CandidateState::MORE_RESEARCH || to == CandidateState::ABORTED;
            case CandidateState::MORE_RESEARCH:
                return to == CandidateState::EXPERIMENTAL;
            case CandidateState::REVIEW_READY:
                return to == CandidateState::HUMAN_APPROVED || to == CandidateState::HUMAN_REJECTED ||
                       to == CandidateState::MORE_RESEARCH;
            case CandidateState::HUMAN_REJECTED:
                return to == CandidateState::ABORTED;
            case CandidateState::HUMAN_APPROVED:
                return to == CandidateState::SHADOW_READY;
            case CandidateState::SHADOW_READY:
                return to == CandidateState::SHADOW_ACTIVE;
            case CandidateState::SHADOW_ACTIVE:
                return to == CandidateState::PROMOTION_READY || to == CandidateState::ABORTED;
            case CandidateState::PROMOTION_READY:
                // Promotion itself is a separate governance action (Phase 7).
                return to == CandidateState::PROMOTED || to == CandidateState::ABORTED;
            case CandidateState::PROMOTED:
                return to == CandidateState::MONITORED;
            case CandidateState::MONITORED:
                return to == CandidateState::STABLE || to == CandidateState::ROLLED_BACK ||
                       to == CandidateState::RETIRED;
            case CandidateState::ROLLED_BACK:
                return to == CandidateState::RETIRED;
            case CandidateState::STABLE:
            case CandidateState::ABORTED:
                return to == CandidateState::RETIRED;
            case CandidateState::RETIRED:
                return false;  // terminal
        }
        return false;
    }
};

}  // namespace evolution
}  // namespace aura

#endif  // AURA_EVOLUTION_CANDIDATE_H
