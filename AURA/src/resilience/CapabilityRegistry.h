#ifndef AURA_RESILIENCE_CAPABILITYREGISTRY_H
#define AURA_RESILIENCE_CAPABILITYREGISTRY_H

#include "resilience/CapabilityDescriptor.h"
#include "resilience/CapabilityId.h"
#include "resilience/DependencyDescriptor.h"

#include <cstddef>
#include <map>
#include <utility>
#include <vector>

namespace aura {
namespace resilience {

// Registry of capabilities and their declared dependencies.
//
// RS-0009 / Phase 0.5 resilience foundation. Acts as the single source of
// capability metadata: a capability descriptor is registered once, and its
// declared dependency edges are recorded explicitly. Registration is
// deterministic and never silently overwrites: registering an invalid descriptor
// or a duplicate capability id fails and leaves existing metadata unchanged.
// Dependencies must reference already-registered capabilities, so no implicit
// dependency is created. This is a metadata container only: it evaluates no
// health, resolves no impact and performs no I/O.
class CapabilityRegistry {
public:
    using descriptor_map = std::map<CapabilityId, CapabilityDescriptor>;
    using edge_list = std::vector<DependencyDescriptor>;

    CapabilityRegistry() = default;

    // Registers a capability. Returns false, with no change, when the descriptor
    // is invalid or the capability id is already registered.
    bool register_capability(CapabilityDescriptor descriptor) {
        if (!descriptor.valid()) return false;
        const auto result = capabilities_.emplace(descriptor.capability(), std::move(descriptor));
        return result.second;
    }

    // Records a directed dependency edge between two registered capabilities.
    // Returns false, with no change, when the edge is invalid, already present, or
    // references an unregistered capability.
    bool add_dependency(DependencyDescriptor edge) {
        if (!edge.valid()) return false;
        if (!contains(edge.dependent()) || !contains(edge.dependency())) return false;
        for (const DependencyDescriptor& existing : dependencies_) {
            if (existing == edge) return false;
        }
        dependencies_.push_back(std::move(edge));
        return true;
    }

    bool contains(const CapabilityId& capability) const {
        return capabilities_.find(capability) != capabilities_.end();
    }

    // Returns the descriptor for `capability`, or nullptr when unregistered. A
    // missing capability is not a silent default; callers must decide explicitly.
    const CapabilityDescriptor* find(const CapabilityId& capability) const {
        const auto it = capabilities_.find(capability);
        return it == capabilities_.end() ? nullptr : &it->second;
    }

    std::size_t size() const noexcept { return capabilities_.size(); }
    bool empty() const noexcept { return capabilities_.empty(); }
    std::size_t dependency_count() const noexcept { return dependencies_.size(); }

    const descriptor_map& capabilities() const noexcept { return capabilities_; }
    const edge_list& dependencies() const noexcept { return dependencies_; }

    // Declared capabilities that `capability` directly depends on, sorted.
    std::vector<CapabilityId> dependencies_of(const CapabilityId& capability) const {
        std::vector<CapabilityId> result;
        for (const DependencyDescriptor& edge : dependencies_) {
            if (edge.dependent() == capability) result.push_back(edge.dependency());
        }
        return result;
    }

private:
    descriptor_map capabilities_{};
    edge_list dependencies_{};
};

}  // namespace resilience
}  // namespace aura

#endif  // AURA_RESILIENCE_CAPABILITYREGISTRY_H
