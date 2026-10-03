#ifndef AURA_LEARNING_KNOWLEDGEDECAY_H
#define AURA_LEARNING_KNOWLEDGEDECAY_H

#include "foundation/Timestamp.h"
#include "learning/KnowledgeObject.h"

#include <cstdint>

namespace aura {
namespace learning {

// Explicit revalidation triggers (Master V2-09).
enum class RevalidationTrigger : std::uint8_t {
    NONE = 0,
    REGIME_DISTRIBUTION_SHIFT,
    LARGE_PERFORMANCE_DEGRADATION,
    BROKER_PROFILE_CHANGE,
    FEATURE_DEFINITION_CHANGE,
    EXECUTION_MODEL_CHANGE,
    LONG_TIME_SINCE_VALIDATION,
    REPEATED_CONTRADICTION,
};

constexpr std::string_view to_string(RevalidationTrigger trigger) noexcept {
    switch (trigger) {
        case RevalidationTrigger::NONE:                          return "NONE";
        case RevalidationTrigger::REGIME_DISTRIBUTION_SHIFT:     return "REGIME_DISTRIBUTION_SHIFT";
        case RevalidationTrigger::LARGE_PERFORMANCE_DEGRADATION: return "LARGE_PERFORMANCE_DEGRADATION";
        case RevalidationTrigger::BROKER_PROFILE_CHANGE:         return "BROKER_PROFILE_CHANGE";
        case RevalidationTrigger::FEATURE_DEFINITION_CHANGE:     return "FEATURE_DEFINITION_CHANGE";
        case RevalidationTrigger::EXECUTION_MODEL_CHANGE:        return "EXECUTION_MODEL_CHANGE";
        case RevalidationTrigger::LONG_TIME_SINCE_VALIDATION:    return "LONG_TIME_SINCE_VALIDATION";
        case RevalidationTrigger::REPEATED_CONTRADICTION:        return "REPEATED_CONTRADICTION";
    }
    return "NONE";
}

// Knowledge decay and revalidation (Master V2-09).
//
// Phase 3 self-learning. Knowledge is never erased when it ages; it becomes a
// research item again. This type decides, deterministically, whether an object is
// due for revalidation given the current time and an explicit set of triggers.
// Pure: no clock, no mutation.
class KnowledgeDecay {
public:
    // An object is due for revalidation when the current time has reached its
    // declared revalidation_due_at (a set, non-zero horizon) or when an explicit
    // change trigger fired. A zero/absent due time means "no scheduled horizon".
    static bool is_due(const KnowledgeObject& object, foundation::Timestamp as_of,
                       RevalidationTrigger trigger = RevalidationTrigger::NONE) noexcept {
        if (trigger != RevalidationTrigger::NONE) return true;
        if (object.revalidation_due_at.nanoseconds() == 0) return false;
        return as_of >= object.revalidation_due_at;
    }

    // The next lifecycle status implied by decay. Never deletes; only proposes
    // AGING (or REVALIDATION when already aging). Callers apply it through
    // KnowledgeLifecycle, which validates the transition.
    static KnowledgeStatus proposed_status(const KnowledgeObject& object, foundation::Timestamp as_of,
                                           RevalidationTrigger trigger =
                                               RevalidationTrigger::NONE) noexcept {
        if (!is_due(object, as_of, trigger)) return object.status;
        if (object.status == KnowledgeStatus::AGING) return KnowledgeStatus::REVALIDATION;
        if (can_age(object.status)) return KnowledgeStatus::AGING;
        return object.status;
    }

private:
    // Statuses from which aging is legal per V2-06 (VALIDATED | OPERATIONAL_KNOWLEDGE).
    static bool can_age(KnowledgeStatus status) noexcept {
        return status == KnowledgeStatus::VALIDATED ||
               status == KnowledgeStatus::OPERATIONAL_KNOWLEDGE;
    }
};

}  // namespace learning
}  // namespace aura

#endif  // AURA_LEARNING_KNOWLEDGEDECAY_H
