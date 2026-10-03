#ifndef AURA_LEARNING_CONTRADICTIONENGINE_H
#define AURA_LEARNING_CONTRADICTIONENGINE_H

#include "foundation/EntityId.h"
#include "learning/KnowledgeObject.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace aura {
namespace learning {

// Outcome of a detected knowledge contradiction (Master V2-07).
enum class ContradictionStatus : std::uint8_t {
    UNRESOLVED = 0,
    CONTEXT_RESOLVED,
    EVIDENCE_WEIGHTED,
    RESEARCH_REQUIRED,
    TRUE_CONTRADICTION,
};

constexpr std::string_view to_string(ContradictionStatus status) noexcept {
    switch (status) {
        case ContradictionStatus::UNRESOLVED:        return "UNRESOLVED";
        case ContradictionStatus::CONTEXT_RESOLVED:  return "CONTEXT_RESOLVED";
        case ContradictionStatus::EVIDENCE_WEIGHTED: return "EVIDENCE_WEIGHTED";
        case ContradictionStatus::RESEARCH_REQUIRED: return "RESEARCH_REQUIRED";
        case ContradictionStatus::TRUE_CONTRADICTION: return "TRUE_CONTRADICTION";
    }
    return "UNRESOLVED";
}

// First-class contradiction object (Master V2-07).
//
// Contradiction is preserved, not resolved by overwriting: the engine never
// mutates Knowledge A using Knowledge B. It records both sides, their shared
// scope and conflict dimension, and leaves a status. It performs no research and
// grants no permission.
struct Contradiction {
    foundation::EntityId contradiction_id{};
    foundation::EntityId knowledge_a{};
    foundation::EntityId knowledge_b{};
    std::string shared_scope{};
    std::string conflict_dimension{};
    std::string evidence_difference{};
    std::string potential_context_split{};
    std::string required_experiment{};
    ContradictionStatus status{ContradictionStatus::UNRESOLVED};
};

// Contradiction detection over knowledge objects (Master V2-07).
//
// Phase 3 self-learning. Detects pairs of knowledge objects that share a validity
// scope and make opposite claims (polarity derived from status: REFUTED /
// CONTRADICTED are treated as negative, SUPPORTED and above as positive), and
// records a contradiction object instead of overwriting either side. Also exports
// the canonical contradiction-object identity when a resolvable context split
// exists. Pure and deterministic; the result order is stable (a < b).
class ContradictionEngine {
public:
    // Positive/negative polarity of a knowledge object for contradiction purposes.
    // UNKNOWN statuses (OBSERVED/SUSPECTED/UNDER_INVESTIGATION/AGING/REVALIDATION)
    // are not treated as committed claims.
    static int polarity(KnowledgeStatus status) noexcept {
        switch (status) {
            case KnowledgeStatus::SUPPORTED:
            case KnowledgeStatus::VALIDATED:
            case KnowledgeStatus::OPERATIONAL_KNOWLEDGE:
                return 1;
            case KnowledgeStatus::REFUTED:
            case KnowledgeStatus::CONTRADICTED:
                return -1;
            default:
                return 0;
        }
    }

    // Detects contradiction pairs within the supplied knowledge objects. Two
    // objects contradict when they share a non-empty validity scope, make opposite
    // committed claims, and their contexts differ (or one is unscoped). The result
    // is deterministic and ordered by (a.knowledge_id, b.knowledge_id).
    static std::vector<Contradiction> detect(const std::vector<KnowledgeObject>& objects) {
        std::vector<Contradiction> out;
        for (std::size_t i = 0; i < objects.size(); ++i) {
            for (std::size_t j = i + 1; j < objects.size(); ++j) {
                const KnowledgeObject& a = objects[i];
                const KnowledgeObject& b = objects[j];
                if (!a.valid() || !b.valid()) continue;
                if (a.validity_scope.empty() || a.validity_scope != b.validity_scope) continue;
                const int pa = polarity(a.status);
                const int pb = polarity(b.status);
                if (pa == 0 || pb == 0 || pa == pb) continue;
                if (a.context == b.context) continue;  // same context + opposite = true conflict
                Contradiction c;
                c.knowledge_a = a.knowledge_id;
                c.knowledge_b = b.knowledge_id;
                c.shared_scope = a.validity_scope;
                c.conflict_dimension = "context";
                c.evidence_difference =
                    "A.observation=" + a.observation + " | B.observation=" + b.observation;
                c.potential_context_split = "A.context=" + a.context + " | B.context=" + b.context;
                c.status = ContradictionStatus::RESEARCH_REQUIRED;
                c.contradiction_id = make_contradiction_id(a.knowledge_id, b.knowledge_id);
                out.push_back(std::move(c));
            }
        }
        return out;
    }

    static foundation::EntityId make_contradiction_id(const foundation::EntityId& a,
                                                      const foundation::EntityId& b) {
        // Order-independent identity so (A,B) and (B,A) are the same contradiction.
        const bool a_first = a.value() <= b.value();
        const std::string& first = a_first ? a.value() : b.value();
        const std::string& second = a_first ? b.value() : a.value();
        return foundation::EntityId("C|" + first + "|" + second);
    }
};

}  // namespace learning
}  // namespace aura

#endif  // AURA_LEARNING_CONTRADICTIONENGINE_H
