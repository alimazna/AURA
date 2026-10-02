#ifndef AURA_RESILIENCE_CAPABILITYID_H
#define AURA_RESILIENCE_CAPABILITYID_H

#include "foundation/EntityId.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace aura {
namespace resilience {

// Immutable identifier for a capability exposed to the dependency graph.
//
// RS-0002 / Phase 0.5 resilience foundation. A distinct type over
// foundation::EntityId so a capability identifier can never be silently
// interchanged with a service, event or other entity identifier. It carries
// identity only: no registration, lookup, persistence or I/O.
class CapabilityId {
public:
    CapabilityId() = default;
    explicit CapabilityId(foundation::EntityId id) : id_(id) {}

    static CapabilityId from_string(std::string_view value) {
        return CapabilityId(foundation::EntityId::from_string(value));
    }

    bool valid() const noexcept { return id_.valid(); }
    explicit operator bool() const noexcept { return valid(); }

    const foundation::EntityId& entity() const noexcept { return id_; }
    std::string_view view() const noexcept { return id_.view(); }

    // Stable serialization: the exact identity token; byte-identical across
    // processes and round-trips through from_string().
    std::string to_string() const { return std::string(id_.view()); }

    friend bool operator==(const CapabilityId& a, const CapabilityId& b) noexcept {
        return a.id_ == b.id_;
    }
    friend bool operator!=(const CapabilityId& a, const CapabilityId& b) noexcept {
        return !(a == b);
    }
    friend bool operator<(const CapabilityId& a, const CapabilityId& b) noexcept {
        return a.id_ < b.id_;
    }

    std::size_t hash() const noexcept { return id_.hash(); }

private:
    foundation::EntityId id_{};
};

}  // namespace resilience
}  // namespace aura

namespace std {
template <>
struct hash<aura::resilience::CapabilityId> {
    std::size_t operator()(const aura::resilience::CapabilityId& id) const noexcept {
        return id.hash();
    }
};
}  // namespace std

#endif  // AURA_RESILIENCE_CAPABILITYID_H
