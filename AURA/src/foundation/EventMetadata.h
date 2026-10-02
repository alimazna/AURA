#ifndef AURA_FOUNDATION_EVENTMETADATA_H
#define AURA_FOUNDATION_EVENTMETADATA_H

#include "foundation/EventId.h"
#include "foundation/EventType.h"
#include "foundation/SchemaVersion.h"
#include "foundation/Timestamp.h"

#include <cstdint>
#include <string>
#include <utility>

namespace aura {
namespace foundation {

// Immutable event metadata (V3-23, V3-24).
//
// FND-0013 / Phase 0 immutable foundation. Mirrors the Master V3 minimum event
// metadata. `event_time` and `receive_time` are kept distinct because the Master
// distinguishes temporal concepts and forbids future information entering a
// decision (V3-24); this type does not compare or enforce that ordering — it only
// preserves both fields so later layers can.
class EventMetadata {
public:
    EventMetadata() = default;

    EventMetadata(EventId event_id, EventType event_type, std::string symbol, std::string source,
                  std::string source_instance, Timestamp event_time, Timestamp receive_time,
                  std::uint64_t sequence_id, SchemaVersion schema)
        : event_id_(std::move(event_id)),
          event_type_(event_type),
          symbol_(std::move(symbol)),
          source_(std::move(source)),
          source_instance_(std::move(source_instance)),
          event_time_(event_time),
          receive_time_(receive_time),
          sequence_id_(sequence_id),
          schema_(schema) {}

    const EventId& event_id() const noexcept { return event_id_; }
    EventType event_type() const noexcept { return event_type_; }
    const std::string& symbol() const noexcept { return symbol_; }
    const std::string& source() const noexcept { return source_; }
    const std::string& source_instance() const noexcept { return source_instance_; }
    Timestamp event_time() const noexcept { return event_time_; }
    Timestamp receive_time() const noexcept { return receive_time_; }
    std::uint64_t sequence_id() const noexcept { return sequence_id_; }
    const SchemaVersion& schema_version() const noexcept { return schema_; }

    // Structurally addressable only when it carries an identity and a known,
    // non-UNKNOWN type. Deeper checks (clock anomaly, duplicate, unknown source)
    // belong to receivers, not to this metadata value.
    bool valid() const noexcept {
        return event_id_.valid() && event_type_ != EventType::UNKNOWN;
    }

private:
    EventId event_id_{};
    EventType event_type_{EventType::UNKNOWN};
    std::string symbol_{};
    std::string source_{};
    std::string source_instance_{};
    Timestamp event_time_{};
    Timestamp receive_time_{};
    std::uint64_t sequence_id_{0};
    SchemaVersion schema_{};
};

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_EVENTMETADATA_H
