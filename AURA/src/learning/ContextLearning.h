#ifndef AURA_LEARNING_CONTEXTLEARNING_H
#define AURA_LEARNING_CONTEXTLEARNING_H

#include "foundation/Timestamp.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace aura {
namespace learning {

// A single context-conditioned observation (Master V2-05 / V3-18 context learning).
//
// `outcome_known_at` is the time the outcome became knowable, kept distinct from
// when the observation occurred. Context learning must only use observations whose
// outcome was already known at the assessment time; this is the no-future-label
// invariant the caller-provided timestamp makes explicit.
struct ContextObservation {
    std::string context{};
    bool supports{false};
    foundation::Timestamp observed_at{};
    foundation::Timestamp outcome_known_at{};
};

// Deterministic context-conditioned evidence summary.
struct ContextEvidenceSummary {
    std::string context{};
    std::size_t supporting{0};
    std::size_t contradicting{0};
    std::size_t used{0};
    std::size_t excluded_future{0};
    // Deterministic bounded support ratio in [0, 1]; a ranking value, not a
    // probability. Zero when no in-window observation contributes.
    double support_ratio{0.0};
    bool point_in_time_clean{true};
};

// Context learning over observed evidence (Master V2-05 scoped knowledge).
//
// Phase 3 self-learning. Aggregates observations per context, counting only those
// whose outcome was known at or before `as_of`. Observations whose outcome became
// known after `as_of` are excluded (never used as if they had been available) and
// counted in `excluded_future`, so leakage is visible rather than silent. Pure,
// deterministic, no clock, no I/O.
class ContextLearning {
public:
    static std::vector<ContextEvidenceSummary> summarize(
        const std::vector<ContextObservation>& observations, foundation::Timestamp as_of) {
        std::map<std::string, ContextEvidenceSummary> by_context;
        for (const ContextObservation& o : observations) {
            if (o.context.empty()) continue;
            ContextEvidenceSummary& s = by_context[o.context];
            s.context = o.context;
            if (o.outcome_known_at > as_of) {
                s.excluded_future += 1;
                s.point_in_time_clean = false;
                continue;
            }
            if (o.supports) {
                s.supporting += 1;
            } else {
                s.contradicting += 1;
            }
            s.used += 1;
        }
        std::vector<ContextEvidenceSummary> out;
        out.reserve(by_context.size());
        for (auto& entry : by_context) {
            ContextEvidenceSummary& s = entry.second;
            const std::size_t denom = s.supporting + s.contradicting;
            s.support_ratio = denom == 0 ? 0.0
                                         : static_cast<double>(s.supporting) / static_cast<double>(denom);
            out.push_back(s);
        }
        return out;  // deterministic: sorted by context (std::map iteration order)
    }
};

}  // namespace learning
}  // namespace aura

#endif  // AURA_LEARNING_CONTEXTLEARNING_H
