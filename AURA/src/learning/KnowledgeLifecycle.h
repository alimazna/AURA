#ifndef AURA_LEARNING_KNOWLEDGELIFECYCLE_H
#define AURA_LEARNING_KNOWLEDGELIFECYCLE_H

#include "learning/KnowledgeObject.h"

#include <cstdint>

namespace aura {
namespace learning {

// Explicit knowledge lifecycle transition validator (Master V2-06).
//
// Phase 3 self-learning. Encodes exactly the documented transitions; there are no
// hidden transitions. Only listed moves are allowed:
//
//   OBSERVED             -> SUSPECTED
//   SUSPECTED            -> UNDER_INVESTIGATION | REFUTED
//   UNDER_INVESTIGATION  -> SUPPORTED
//   SUPPORTED            -> VALIDATED | CONTRADICTED
//   VALIDATED            -> OPERATIONAL_KNOWLEDGE | AGING
//   CONTRADICTED         -> UNDER_INVESTIGATION      (context split / more research)
//   AGING                -> REVALIDATION
//   REVALIDATION         -> SUPPORTED | REFUTED
//
// A same-state call is an idempotent no-op and is allowed. Any other move is
// rejected. Pure and deterministic: no state is stored or mutated here.
class KnowledgeLifecycle {
public:
    static bool is_terminal(KnowledgeStatus status) noexcept {
        return status == KnowledgeStatus::REFUTED;
    }

    static bool can_transition(KnowledgeStatus from, KnowledgeStatus to) noexcept {
        if (from == to) return true;  // idempotent no-op
        switch (from) {
            case KnowledgeStatus::OBSERVED:
                return to == KnowledgeStatus::SUSPECTED;
            case KnowledgeStatus::SUSPECTED:
                return to == KnowledgeStatus::UNDER_INVESTIGATION ||
                       to == KnowledgeStatus::REFUTED;
            case KnowledgeStatus::UNDER_INVESTIGATION:
                return to == KnowledgeStatus::SUPPORTED;
            case KnowledgeStatus::SUPPORTED:
                return to == KnowledgeStatus::VALIDATED ||
                       to == KnowledgeStatus::CONTRADICTED;
            case KnowledgeStatus::VALIDATED:
                return to == KnowledgeStatus::OPERATIONAL_KNOWLEDGE ||
                       to == KnowledgeStatus::AGING;
            case KnowledgeStatus::OPERATIONAL_KNOWLEDGE:
                return to == KnowledgeStatus::AGING;  // operational knowledge can age
            case KnowledgeStatus::CONTRADICTED:
                return to == KnowledgeStatus::UNDER_INVESTIGATION;
            case KnowledgeStatus::AGING:
                return to == KnowledgeStatus::REVALIDATION;
            case KnowledgeStatus::REVALIDATION:
                return to == KnowledgeStatus::SUPPORTED ||
                       to == KnowledgeStatus::REFUTED;
            case KnowledgeStatus::REFUTED:
                return false;  // terminal (history retained, never erased)
        }
        return false;
    }

    // Advances a copy of an object to `to` when the transition is legal, returning
    // nullptr otherwise. The original is untouched. No permission is granted by a
    // status change (V2-10); this only validates structure.
    static const KnowledgeObject* advance(const KnowledgeObject& from, KnowledgeStatus to,
                                          KnowledgeObject& out) noexcept {
        if (!can_transition(from.status, to)) return nullptr;
        out = from;
        out.status = to;
        return &out;
    }
};

}  // namespace learning
}  // namespace aura

#endif  // AURA_LEARNING_KNOWLEDGELIFECYCLE_H
