#ifndef AURA_FOUNDATION_PERSISTENCETRANSACTION_H
#define AURA_FOUNDATION_PERSISTENCETRANSACTION_H

#include "foundation/EntityId.h"
#include "foundation/HashDigest.h"
#include "foundation/PersistenceRecordMetadata.h"

#include <cstddef>
#include <vector>

namespace aura {
namespace foundation {

// Persistence transaction concept (V3-22, V3-27).
//
// PER-0004 / Phase 0 persistence foundation. A transaction groups appends so the
// result is atomic and auditable. The Master requires that a partial failure be
// explicit rather than silently accepted, and that retrying an identical
// transaction is idempotent. This type is an immutable description of the work to
// be performed; it does not itself write storage or own a store handle.
class PersistenceTransaction {
public:
    // One pending append: the record metadata plus the digest of its payload.
    struct Entry {
        PersistenceRecordMetadata metadata;
        HashDigest payload_digest;
    };

    PersistenceTransaction() = default;

    explicit PersistenceTransaction(EntityId transaction_id)
        : transaction_id_(std::move(transaction_id)) {}

    const EntityId& transaction_id() const noexcept { return transaction_id_; }

    // Appends a pending entry. Duplicate record identities within one transaction
    // are rejected by returning false rather than silently overwriting.
    bool add(const PersistenceRecordMetadata& metadata, const HashDigest& payload_digest) {
        for (const auto& entry : entries_) {
            if (entry.metadata.record_id() == metadata.record_id()) return false;
        }
        entries_.push_back(Entry{metadata, payload_digest});
        return true;
    }

    std::size_t size() const noexcept { return entries_.size(); }
    bool empty() const noexcept { return entries_.empty(); }
    const std::vector<Entry>& entries() const noexcept { return entries_; }

    // A transaction is applicable only when it has its own identity and at least
    // one entry.
    bool applicable() const noexcept { return transaction_id_.valid() && !entries_.empty(); }

private:
    EntityId transaction_id_{};
    std::vector<Entry> entries_{};
};

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_PERSISTENCETRANSACTION_H
