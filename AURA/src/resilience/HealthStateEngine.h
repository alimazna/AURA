#ifndef AURA_RESILIENCE_HEALTHSTATEENGINE_H
#define AURA_RESILIENCE_HEALTHSTATEENGINE_H

#include "foundation/ServiceState.h"
#include "resilience/CapabilityRegistry.h"
#include "resilience/DependencyGraph.h"
#include "resilience/HealthSnapshot.h"

#include <map>

namespace aura {
namespace resilience {

// Derives aggregate capability health from service health.
//
// RS-0011 / Phase 0.5 resilience foundation. A capability is ONLINE only when its
// owning service is ONLINE; it is never fabricated healthy. Dependencies are
// propagated: a capability whose declared dependency is not operational cannot be
// reported ONLINE, so dependency failure degrades dependents rather than being
// hidden. UNKNOWN service health (no snapshot, or an invalid snapshot) yields
// BLOCKED rather than a fabricated healthy state. Aggregation is deterministic:
// pure functions of the supplied snapshots, registry and graph; no clock, no I/O,
// no hidden state.
class HealthStateEngine {
public:
    using snapshot_map = std::map<ServiceDescriptor, HealthSnapshot>;

    HealthStateEngine(const CapabilityRegistry& registry, const DependencyGraph& graph)
        : registry_(registry), graph_(graph) {}

    // Aggregate state of a capability given observed service health.
    foundation::ServiceState capability_state(const CapabilityId& capability,
                                              const snapshot_map& snapshots) const {
        const CapabilityDescriptor* descriptor = registry_.find(capability);
        if (descriptor == nullptr) return foundation::ServiceState::BLOCKED;

        const foundation::ServiceState own = service_state(descriptor->service(), snapshots);
        if (!foundation::is_operational(own)) return own;

        // Own service is ONLINE; a non-operational declared dependency degrades the
        // capability rather than being ignored.
        for (const CapabilityId& dependency : graph_.dependencies_of(capability)) {
            const CapabilityDescriptor* dep = registry_.find(dependency);
            if (dep == nullptr) return foundation::ServiceState::BLOCKED;
            if (!foundation::is_operational(service_state(dep->service(), snapshots))) {
                return foundation::ServiceState::DEGRADED;
            }
        }
        return foundation::ServiceState::ONLINE;
    }

    // State of a service from its own snapshot only. Absent or invalid health is
    // BLOCKED, never a fabricated ONLINE.
    foundation::ServiceState service_state(const ServiceDescriptor& service,
                                           const snapshot_map& snapshots) const {
        const auto it = snapshots.find(service);
        if (it == snapshots.end() || !it->second.valid()) return foundation::ServiceState::BLOCKED;
        return it->second.state();
    }

private:
    const CapabilityRegistry& registry_;
    const DependencyGraph& graph_;
};

}  // namespace resilience
}  // namespace aura

#endif  // AURA_RESILIENCE_HEALTHSTATEENGINE_H
