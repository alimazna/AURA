#ifndef AURA_RESILIENCE_DEPENDENCYDESCRIPTOR_H
#define AURA_RESILIENCE_DEPENDENCYDESCRIPTOR_H

#include "resilience/CapabilityId.h"

#include <cstddef>

namespace aura {
namespace resilience {

// Immutable directed capability dependency edge.
//
// RS-0004 / Phase 0.5 resilience foundation. A dependency edge is directed:
// `dependent` requires `dependency`. The direction is explicit and is never
// inferred; an edge is usable only when both endpoints are valid and distinct.
// A self-edge (dependent == dependency) is invalid because a capability cannot
// depend on itself. This type encodes an edge only: cycle detection and impact
// resolution belong to the dependency graph (RS-0010) and resolver (RS-0018).
class DependencyDescriptor {
public:
    DependencyDescriptor() = default;

    DependencyDescriptor(CapabilityId dependent, CapabilityId dependency)
        : dependent_(dependent), dependency_(dependency) {}

    // Usable only when both endpoints are valid and not the same capability.
    bool valid() const noexcept {
        return dependent_.valid() && dependency_.valid() && dependent_ != dependency_;
    }
    explicit operator bool() const noexcept { return valid(); }

    // The capability that requires the other.
    const CapabilityId& dependent() const noexcept { return dependent_; }
    // The capability that is required.
    const CapabilityId& dependency() const noexcept { return dependency_; }

    // True when the edge points from the given capability to itself, i.e. a
    // self-dependency, which is not a valid edge.
    bool is_self_edge() const noexcept {
        return dependent_.valid() && dependent_ == dependency_;
    }

    friend bool operator==(const DependencyDescriptor& a, const DependencyDescriptor& b) noexcept {
        return a.dependent_ == b.dependent_ && a.dependency_ == b.dependency_;
    }
    friend bool operator!=(const DependencyDescriptor& a, const DependencyDescriptor& b) noexcept {
        return !(a == b);
    }
    friend bool operator<(const DependencyDescriptor& a, const DependencyDescriptor& b) noexcept {
        if (a.dependent_ != b.dependent_) return a.dependent_ < b.dependent_;
        return a.dependency_ < b.dependency_;
    }

    std::size_t hash() const noexcept {
        const std::size_t h = dependent_.hash();
        return h ^ (dependency_.hash() + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
    }

private:
    CapabilityId dependent_{};
    CapabilityId dependency_{};
};

}  // namespace resilience
}  // namespace aura

namespace std {
template <>
struct hash<aura::resilience::DependencyDescriptor> {
    std::size_t operator()(const aura::resilience::DependencyDescriptor& d) const noexcept {
        return d.hash();
    }
};
}  // namespace std

#endif  // AURA_RESILIENCE_DEPENDENCYDESCRIPTOR_H
