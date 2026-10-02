#ifndef AURA_FOUNDATION_EVENTID_H
#define AURA_FOUNDATION_EVENTID_H

#include "foundation/EntityId.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace aura {
namespace foundation {

// Immutable identifier for a single event.
//
// FND-0011 / Phase 0 immutable foundation. The Master V3 requires an `event_id`
// in event metadata and mandates idempotency at relevant boundaries (V3-23).
// EventId is a distinct type over EntityId so an event identifier can never be
// silently interchanged with another entity identifier. It carries identity only:
// no dispatch, queueing, persistence or I/O.
class EventId {
public:
    EventId() = default;
    explicit EventId(EntityId id) : id_(id) {}

    static EventId from_string(std::string_view value) {
        return EventId(EntityId::from_string(value));
    }

    bool valid() const noexcept { return id_.valid(); }
    explicit operator bool() const noexcept { return valid(); }

    const EntityId& entity() const noexcept { return id_; }
    std::string_view view() const noexcept { return id_.view(); }

    // Stable serialization: the exact identity token. Round-trips through
    // from_string() and is byte-identical across processes for the same identity.
    std::string to_string() const { return std::string(id_.view()); }

    friend bool operator==(const EventId& a, const EventId& b) noexcept { return a.id_ == b.id_; }
    friend bool operator!=(const EventId& a, const EventId& b) noexcept { return !(a == b); }
    friend bool operator<(const EventId& a, const EventId& b) noexcept { return a.id_ < b.id_; }

    std::size_t hash() const noexcept { return id_.hash(); }

private:
    EntityId id_{};
};

}  // namespace foundation
}  // namespace aura

namespace std {
template <>
struct hash<aura::foundation::EventId> {
    std::size_t operator()(const aura::foundation::EventId& id) const noexcept { return id.hash(); }
};
}  // namespace std

#endif  // AURA_FOUNDATION_EVENTID_H
