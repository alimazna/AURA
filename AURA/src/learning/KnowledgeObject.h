#ifndef AURA_LEARNING_KNOWLEDGEOBJECT_H
#define AURA_LEARNING_KNOWLEDGEOBJECT_H

#include "foundation/EntityId.h"
#include "foundation/Timestamp.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace aura {
namespace learning {

// Lifecycle state of a knowledge statement (Master V2-06 / "Knowledge Lifecycle").
//
// Phase 3 self-learning. These states are descriptive only; they grant no runtime
// authority. OPERATIONAL_KNOWLEDGE means the statement may be used for *bounded
// research prioritization or policy logic subject to governance* (V2-06); it does
// NOT grant a production mutation permission (V2-10 "no automatic escalation of
// authority"). A knowledge object never becomes a configuration change by itself.
enum class KnowledgeStatus : std::uint8_t {
    OBSERVED = 0,
    SUSPECTED,
    UNDER_INVESTIGATION,
    SUPPORTED,
    VALIDATED,
    OPERATIONAL_KNOWLEDGE,
    REFUTED,
    CONTRADICTED,
    AGING,
    REVALIDATION,
};

constexpr std::string_view to_string(KnowledgeStatus status) noexcept {
    switch (status) {
        case KnowledgeStatus::OBSERVED:              return "OBSERVED";
        case KnowledgeStatus::SUSPECTED:             return "SUSPECTED";
        case KnowledgeStatus::UNDER_INVESTIGATION:   return "UNDER_INVESTIGATION";
        case KnowledgeStatus::SUPPORTED:             return "SUPPORTED";
        case KnowledgeStatus::VALIDATED:             return "VALIDATED";
        case KnowledgeStatus::OPERATIONAL_KNOWLEDGE: return "OPERATIONAL_KNOWLEDGE";
        case KnowledgeStatus::REFUTED:               return "REFUTED";
        case KnowledgeStatus::CONTRADICTED:          return "CONTRADICTED";
        case KnowledgeStatus::AGING:                 return "AGING";
        case KnowledgeStatus::REVALIDATION:          return "REVALIDATION";
    }
    return "UNKNOWN";
}

// Structured, explainable confidence assessment (Master V2-08).
//
// Confidence is a structured evidence assessment. It is explicitly NOT a
// probability and NOT a production permission (V2-08 rules). This type carries
// the named evidence components; `score()` is a deterministic bounded ranking
// value in [0, 1] derived from those components, and is documented as such so it
// cannot be silently read as a calibrated probability.
struct KnowledgeConfidence {
    double evidence_strength{0.0};
    double replication{0.0};
    double scope_coverage{0.0};
    double validation_strength{0.0};
    double contradiction{0.0};
    double selection_burden{0.0};
    double data_uncertainty{0.0};
    double recency{0.0};

    // Deterministic bounded ranking value in [0, 1]. Not a probability.
    double score() const noexcept {
        double s = evidence_strength + replication + scope_coverage + validation_strength +
                   recency - contradiction - selection_burden - data_uncertainty;
        if (s < 0.0) s = 0.0;
        if (s > 1.0) s = 1.0;
        return s;
    }

    bool operator==(const KnowledgeConfidence& o) const noexcept {
        return evidence_strength == o.evidence_strength && replication == o.replication &&
               scope_coverage == o.scope_coverage && validation_strength == o.validation_strength &&
               contradiction == o.contradiction && selection_burden == o.selection_burden &&
               data_uncertainty == o.data_uncertainty && recency == o.recency;
    }
};

// Immutable knowledge object (Master V2-05 section 59.1 canonical structure).
//
// Phase 3 self-learning. The unit of learning is a scoped KnowledgeObject, not a
// raw metric. It carries provenance (evidence/hypothesis/experiment/failure refs
// and lineage), a validity scope, timestamps and an explicit status. It is a value
// contract only: it performs no research, no persistence, no evaluation and no
// configuration mutation.
struct KnowledgeObject {
    foundation::EntityId knowledge_id{};
    std::string observation{};
    std::string context{};
    std::vector<std::string> evidence_refs{};
    std::vector<std::string> hypothesis_refs{};
    std::vector<std::string> experiment_refs{};
    std::vector<std::string> related_failures{};
    std::string conclusion{};
    KnowledgeConfidence confidence{};
    std::string validity_scope{};
    KnowledgeStatus status{KnowledgeStatus::OBSERVED};
    std::vector<std::string> contradictory_refs{};
    foundation::Timestamp first_observed_at{};
    foundation::Timestamp last_supported_at{};
    foundation::Timestamp revalidation_due_at{};
    std::vector<std::string> lineage_refs{};
    std::vector<std::string> human_decision_refs{};

    // Versioning metadata (V3-26 / V2-06 "knowledge updates must be versioned").
    // Revision 0 is the first recorded revision of a knowledge identity; a
    // superseding statement is a new revision whose lineage_refs name the prior
    // revision. Not part of the V2-05 field list; added to satisfy versioning.
    std::uint32_t revision{0};

    // The time this statement is being assessed/formed. Evidence must not postdate it.
    foundation::Timestamp assessed_at{};

    bool valid() const noexcept { return knowledge_id.valid() && !observation.empty(); }

    // Point-in-time invariant: assessment cannot precede first observation and no
    // evidence may postdate the assessment time. `latest_evidence_at` is supplied
    // by the caller because evidence timestamps are owned by other records.
    bool is_point_in_time_consistent(foundation::Timestamp latest_evidence_at) const noexcept {
        if (assessed_at < first_observed_at) return false;
        if (latest_evidence_at > assessed_at) return false;
        return true;
    }
};

// Deterministic knowledge identity (V3-23 identity discipline): a function of
// stable meaning fields (domain, observation, validity scope), never of volatile
// values such as timestamps or counts. The same statement always yields the same
// id, so revisions chain onto a stable identity.
inline foundation::EntityId make_knowledge_id(std::string_view domain,
                                              std::string_view observation,
                                              std::string_view scope) {
    std::string value = "K|";
    value.append(domain);
    value.push_back('|');
    value.append(observation);
    value.push_back('|');
    value.append(scope);
    return foundation::EntityId(std::move(value));
}

}  // namespace learning
}  // namespace aura

#endif  // AURA_LEARNING_KNOWLEDGEOBJECT_H
