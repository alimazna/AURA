#ifndef AURA_FOUNDATION_PERSISTENCERECORDMETADATA_H
#define AURA_FOUNDATION_PERSISTENCERECORDMETADATA_H

#include "foundation/EntityId.h"
#include "foundation/SchemaVersion.h"
#include "foundation/Timestamp.h"

#include <utility>

namespace aura {
namespace foundation {

// Immutable metadata attached to every persisted record (V3-22, V3-27).
//
// PER-0002 / Phase 0 persistence foundation. Every persisted record must be
// identifiable, schema-versioned and time-stamped so it remains auditable and
// reproducible. This type carries provenance only; it does not read or write
// storage.
class PersistenceRecordMetadata {
public:
    PersistenceRecordMetadata() = default;

    PersistenceRecordMetadata(EntityId record_id, SchemaVersion schema, Timestamp written_at)
        : record_id_(std::move(record_id)), schema_(schema), written_at_(written_at) {}

    const EntityId& record_id() const noexcept { return record_id_; }
    const SchemaVersion& schema_version() const noexcept { return schema_; }
    Timestamp written_at() const noexcept { return written_at_; }

    // A record is auditable only when it has an identity.
    bool auditable() const noexcept { return record_id_.valid(); }

private:
    EntityId record_id_{};
    SchemaVersion schema_{};
    Timestamp written_at_{};
};

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_PERSISTENCERECORDMETADATA_H
