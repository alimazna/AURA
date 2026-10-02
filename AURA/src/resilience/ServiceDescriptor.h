#ifndef AURA_RESILIENCE_SERVICEDESCRIPTOR_H
#define AURA_RESILIENCE_SERVICEDESCRIPTOR_H

#include "foundation/ServiceState.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace aura {
namespace resilience {

// Immutable descriptor of a supervised service/subsystem.
//
// RS-0001 / Phase 0.5 resilience foundation. A descriptor names a supervised
// service and carries its declared default/initial state vocabulary. It is a
// static, descriptive contract only: it performs no supervision, monitoring,
// transition, recovery, isolation, scheduling or I/O, and it exposes no mutator.
//
// Identity is a non-empty, byte-exact, stable name. RS-0001 depends only on
// FND-0004 ServiceState, so the descriptor carries no EntityId and grants no
// runtime authority. The default state is descriptive and is NOT a safety
// guarantee: a service is usable only when it is actually ONLINE
// (is_operational), never because a descriptor says so.
class ServiceDescriptor {
public:
    using name_type = std::string;

    ServiceDescriptor() = default;

    // Constructs a descriptor from a non-empty name and its declared default
    // state. An empty name yields the invalid (default) descriptor; there is no
    // implicit construction from a bare string.
    explicit ServiceDescriptor(name_type name, foundation::ServiceState default_state)
        : name_(std::move(name)), default_state_(default_state) {}

    // A descriptor is valid only when it carries a non-empty name.
    bool valid() const noexcept { return !name_.empty(); }
    explicit operator bool() const noexcept { return valid(); }

    const name_type& name() const noexcept { return name_; }
    std::string_view view() const noexcept { return name_; }

    // Declared default/initial state vocabulary. Descriptive only; it does not
    // assert that the service is currently operational.
    foundation::ServiceState default_state() const noexcept { return default_state_; }

    friend bool operator==(const ServiceDescriptor& a, const ServiceDescriptor& b) noexcept {
        return a.name_ == b.name_ && a.default_state_ == b.default_state_;
    }
    friend bool operator!=(const ServiceDescriptor& a, const ServiceDescriptor& b) noexcept {
        return !(a == b);
    }
    friend bool operator<(const ServiceDescriptor& a, const ServiceDescriptor& b) noexcept {
        if (a.name_ != b.name_) return a.name_ < b.name_;
        return a.default_state_ < b.default_state_;
    }

    std::size_t hash() const noexcept {
        const std::size_t h = std::hash<name_type>{}(name_);
        const std::size_t s = std::hash<std::uint8_t>{}(
            static_cast<std::uint8_t>(default_state_));
        return h ^ (s + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
    }

private:
    name_type name_{};
    foundation::ServiceState default_state_{foundation::ServiceState::STARTING};
};

}  // namespace resilience
}  // namespace aura

namespace std {
template <>
struct hash<aura::resilience::ServiceDescriptor> {
    std::size_t operator()(const aura::resilience::ServiceDescriptor& d) const noexcept {
        return d.hash();
    }
};
}  // namespace std

#endif  // AURA_RESILIENCE_SERVICEDESCRIPTOR_H
