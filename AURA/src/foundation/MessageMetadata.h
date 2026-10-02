#ifndef AURA_FOUNDATION_MESSAGEMETADATA_H
#define AURA_FOUNDATION_MESSAGEMETADATA_H

#include "foundation/EntityId.h"
#include "foundation/HashDigest.h"
#include "foundation/ProtocolVersion.h"
#include "foundation/SchemaVersion.h"
#include "foundation/Timestamp.h"

#include <string>
#include <utility>

namespace aura {
namespace foundation {

// Immutable message metadata (V3-23).
//
// FND-0010 / Phase 0 immutable foundation. Mirrors the Master V3 minimum message
// metadata. The payload itself is intentionally not modelled here; a message
// carries only its addressing/envelope fields plus an optional checksum digest
// reference. Receivers must reject or quarantine messages whose protocol/schema
// is unknown, whose identity is empty, or whose timestamps are anomalous; this
// type only carries the fields needed for those checks.
class MessageMetadata {
public:
    MessageMetadata() = default;

    MessageMetadata(EntityId message_id, ProtocolVersion protocol, SchemaVersion schema,
                    Timestamp timestamp, std::string source, std::string destination,
                    HashDigest checksum = {})
        : message_id_(std::move(message_id)),
          protocol_(protocol),
          schema_(schema),
          timestamp_(timestamp),
          source_(std::move(source)),
          destination_(std::move(destination)),
          checksum_(checksum) {}

    const EntityId& message_id() const noexcept { return message_id_; }
    const ProtocolVersion& protocol_version() const noexcept { return protocol_; }
    const SchemaVersion& schema_version() const noexcept { return schema_; }
    Timestamp timestamp() const noexcept { return timestamp_; }
    const std::string& source() const noexcept { return source_; }
    const std::string& destination() const noexcept { return destination_; }
    // Optional checksum/hash of the payload (V3-23); absent means "not provided",
    // which a receiver may treat as a quarantine condition depending on policy.
    const HashDigest& checksum() const noexcept { return checksum_; }

    // A message is structurally addressable only when it has an identity and
    // both endpoints. Protocol/schema compatibility, checksum verification and
    // timestamp anomaly checks are receiver-side semantic checks (V3-23), not
    // properties of this carrier.
    bool valid() const noexcept {
        return message_id_.valid() && !source_.empty() && !destination_.empty();
    }

private:
    EntityId message_id_{};
    ProtocolVersion protocol_{};
    SchemaVersion schema_{};
    Timestamp timestamp_{};
    std::string source_{};
    std::string destination_{};
    HashDigest checksum_{};
};

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_MESSAGEMETADATA_H
