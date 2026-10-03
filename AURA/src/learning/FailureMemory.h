#ifndef AURA_LEARNING_FAILUREMEMORY_H
#define AURA_LEARNING_FAILUREMEMORY_H

#include "foundation/EntityId.h"
#include "foundation/Timestamp.h"
#include "observation/FailureDetectionEngine.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace aura {
namespace learning {

// A recorded failure observation used as learning memory.
//
// `outcome_known_at` is the time the failure's consequences became knowable; it
// is kept distinct from `observed_at` so that a later-diagnosed failure cannot be
// folded into an earlier knowledge statement (no future-label leakage).
struct FailureMemoryEntry {
    foundation::EntityId failure_id{};
    observation::FailureCategory category{observation::FailureCategory::UNKNOWN};
    foundation::ErrorCode code{foundation::ErrorCode::DATA_ERROR};
    foundation::ErrorSeverity severity{foundation::ErrorSeverity::INFO};
    std::string context{};
    foundation::Timestamp observed_at{};
    foundation::Timestamp outcome_known_at{};
};

// Aggregated, deterministic failure memory summary.
struct FailurePattern {
    observation::FailureCategory category{observation::FailureCategory::UNKNOWN};
    foundation::ErrorCode code{foundation::ErrorCode::DATA_ERROR};
    std::size_t occurrences{0};
    foundation::Timestamp first_seen{};
    foundation::Timestamp last_seen{};
};

// Failure memory (Master V2 "every failure becomes future research knowledge").
//
// Phase 3 self-learning. Converts structured foundation::ErrorRecord objects
// (OB-0003 / V3-27) into append-only learning memory and aggregates recurring
// patterns. A failure whose consequences postdate the query time is excluded, and
// such exclusions are counted, so causality is explicit. Never erases history.
// Pure/deterministic apart from the caller-supplied identity; no I/O.
class FailureMemory {
public:
    FailureMemory() = default;

    // Adopts a structured error record into memory. Uses the record's own stable
    // error_id as the failure identity; a failure with no identity is rejected.
    // `code` is supplied by the caller because foundation::ErrorRecord folds the
    // canonical code into its component string rather than exposing it as a field
    // (V3-27 shape). Idempotent on identity.
    bool remember(const foundation::ErrorRecord& record, observation::FailureCategory category,
                  foundation::ErrorCode code, std::string context,
                  foundation::Timestamp observed_at, foundation::Timestamp outcome_known_at) {
        if (!record.valid()) return false;
        if (by_id_.find(record.error_id()) != by_id_.end()) return true;  // idempotent duplicate
        FailureMemoryEntry entry;
        entry.failure_id = record.error_id();
        entry.category = category;
        entry.code = code;
        entry.severity = record.severity();
        entry.context = std::move(context);
        entry.observed_at = observed_at;
        entry.outcome_known_at = outcome_known_at;
        order_.push_back(entry.failure_id);
        by_id_.emplace(entry.failure_id, std::move(entry));
        return true;
    }

    // Aggregates patterns using only failures whose outcome was known at or before
    // `as_of`. `excluded_future` counts the failures held back for causality.
    std::vector<FailurePattern> patterns(foundation::Timestamp as_of,
                                         std::size_t& excluded_future) const {
        std::map<std::pair<int, int>, FailurePattern> acc;
        excluded_future = 0;
        for (const foundation::EntityId& id : order_) {
            const FailureMemoryEntry& e = by_id_.at(id);
            if (e.outcome_known_at > as_of) {
                excluded_future += 1;
                continue;
            }
            const std::pair<int, int> key{static_cast<int>(e.category), static_cast<int>(e.code)};
            FailurePattern& p = acc[key];
            p.category = e.category;
            p.code = e.code;
            if (p.occurrences == 0) {
                p.first_seen = e.observed_at;
                p.last_seen = e.observed_at;
            } else {
                if (e.observed_at < p.first_seen) p.first_seen = e.observed_at;
                if (e.observed_at > p.last_seen) p.last_seen = e.observed_at;
            }
            p.occurrences += 1;
        }
        std::vector<FailurePattern> out;
        out.reserve(acc.size());
        for (auto& entry : acc) out.push_back(entry.second);
        return out;
    }

    std::size_t size() const noexcept { return by_id_.size(); }

private:
    std::vector<foundation::EntityId> order_{};
    std::map<foundation::EntityId, FailureMemoryEntry> by_id_{};
};

}  // namespace learning
}  // namespace aura

#endif  // AURA_LEARNING_FAILUREMEMORY_H
