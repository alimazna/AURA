#ifndef AURA_RESILIENCE_CAPABILITYDESCRIPTOR_H
#define AURA_RESILIENCE_CAPABILITYDESCRIPTOR_H

#include "resilience/CapabilityId.h"
#include "resilience/ServiceDescriptor.h"

#include <cstddef>
#include <string_view>

namespace aura {
namespace resilience {

// Immutable descriptor binding a capability to its owning service.
//
// RS-0003 / Phase 0.5 resilience foundation. A capability is an identifiable
// function exposed to the dependency graph; it is always owned by exactly one
// supervised service (RS-0001). Criticality is referenced explicitly by the
// owning service's identity and is never defaulted: a descriptor that does not
// name a valid capability and a valid owning service is not usable. This is a
// descriptive contract only: no registration, lookup, health evaluation or I/O.
class CapabilityDescriptor {
public:
    CapabilityDescriptor() = default;

    CapabilityDescriptor(CapabilityId capability, ServiceDescriptor service)
        : capability_(capability), service_(service) {}

    // Usable only when the capability and its owning service are both valid.
    bool valid() const noexcept { return capability_.valid() && service_.valid(); }
    explicit operator bool() const noexcept { return valid(); }

    const CapabilityId& capability() const noexcept { return capability_; }
    const ServiceDescriptor& service() const noexcept { return service_; }

    friend bool operator==(const CapabilityDescriptor& a, const CapabilityDescriptor& b) noexcept {
        return a.capability_ == b.capability_ && a.service_ == b.service_;
    }
    friend bool operator!=(const CapabilityDescriptor& a, const CapabilityDescriptor& b) noexcept {
        return !(a == b);
    }
    friend bool operator<(const CapabilityDescriptor& a, const CapabilityDescriptor& b) noexcept {
        if (a.capability_ != b.capability_) return a.capability_ < b.capability_;
        return a.service_ < b.service_;
    }

    std::size_t hash() const noexcept {
        const std::size_t h = capability_.hash();
        return h ^ (service_.hash() + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
    }

private:
    CapabilityId capability_{};
    ServiceDescriptor service_{};
};

}  // namespace resilience
}  // namespace aura

namespace std {
template <>
struct hash<aura::resilience::CapabilityDescriptor> {
    std::size_t operator()(const aura::resilience::CapabilityDescriptor& d) const noexcept {
        return d.hash();
    }
};
}  // namespace std

#endif  // AURA_RESILIENCE_CAPABILITYDESCRIPTOR_H
