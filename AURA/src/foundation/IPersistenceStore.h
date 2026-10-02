#ifndef AURA_FOUNDATION_IPERSISTENCESTORE_H
#define AURA_FOUNDATION_IPERSISTENCESTORE_H

#include "foundation/EntityId.h"
#include "foundation/HashDigest.h"
#include "foundation/PersistenceRecordMetadata.h"
#include "foundation/PersistenceStatus.h"

namespace aura {
namespace foundation {

// Persistence store interface contract (V3-22).
//
// PER-0003 / Phase 0 persistence foundation. The store is append-oriented: it
// records identified, schema-versioned entries and never rewrites history. Every
// operation returns a PersistenceStatus so failure is surfaced rather than
// swallowed, and is idempotent on the record identity: appending an entry that
// already exists must not create a duplicate or corrupt prior state. This
// interface has no higher-layer dependency and performs no recovery.
class IPersistenceStore {
public:
    virtual ~IPersistenceStore() = default;

    // Appends an entry. Idempotent on metadata.record_id(): re-appending an
    // existing identity with the same payload digest returns OK without creating
    // a second copy; a conflicting payload for the same identity returns CONFLICT.
    virtual PersistenceStatus append(const PersistenceRecordMetadata& metadata,
                                     const HashDigest& payload_digest) = 0;

    // Reports whether an entry with this identity is already stored.
    virtual PersistenceStatus contains(const EntityId& record_id) const = 0;

    // Forces durable persistence of all previously accepted appends. Failure here
    // is a safety-relevant event and must not be reported as OK.
    virtual PersistenceStatus flush() = 0;
};

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_IPERSISTENCESTORE_H
