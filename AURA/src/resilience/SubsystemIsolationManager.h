#ifndef AURA_RESILIENCE_SUBSYSTEMISOLATIONMANAGER_H
#define AURA_RESILIENCE_SUBSYSTEMISOLATIONMANAGER_H

#include "foundation/ServiceState.h"
#include "resilience/CapabilityId.h"
#include "resilience/CapabilityRegistry.h"
#include "resilience/DependencyGraph.h"

#include <algorithm>
#include <vector>

namespace aura {
namespace resilience {

// Decision describing which capabilities an isolation would disable.
//
// RS-0014 / Phase 0.5 resilience foundation. Isolation disables only the failed
// capability and the capabilities that depend on it; every independent capability
// remains available. The two sets are disjoint and their union is the registry's
// capability set, so an independent capability can never be silently disabled
// (V3-13). This is a value decision only: it disables nothing at runtime and
// performs no I/O.
struct IsolationDecision {
    CapabilityId failed{};
    std::vector<CapabilityId> disabled{};   // failed + transitive dependents
    std::vector<CapabilityId> available{};  // independent capabilities
    bool valid{false};
};

// Computes capability isolation decisions from the dependency graph.
//
// RS-0014 / Phase 0.5 resilience foundation. Deterministic and pure: given the
// same registry and graph it produces the same decision. It keeps safe
// independent capabilities running by construction (they are placed in
// `available`) and disables only dependents of the failed capability.
class SubsystemIsolationManager {
public:
    SubsystemIsolationManager(const CapabilityRegistry& registry, const DependencyGraph& graph)
        : registry_(registry), graph_(graph) {}

    IsolationDecision isolate(const CapabilityId& failed) const {
        IsolationDecision decision;
        decision.failed = failed;
        if (!registry_.contains(failed)) return decision;

        // Failed capability plus everything that transitively depends on it.
        decision.disabled.push_back(failed);
        for (const CapabilityId& dependent : graph_.reachable_dependents(failed)) {
            if (dependent != failed) decision.disabled.push_back(dependent);
        }
        std::sort(decision.disabled.begin(), decision.disabled.end());

        for (const auto& entry : registry_.capabilities()) {
            const CapabilityId& capability = entry.first;
            if (!std::binary_search(decision.disabled.begin(), decision.disabled.end(), capability)) {
                decision.available.push_back(capability);
            }
        }
        decision.valid = true;
        return decision;
    }

    // True when `capability` would be disabled by isolating `failed`.
    bool is_disabled_by(const CapabilityId& failed, const CapabilityId& capability) const {
        const IsolationDecision decision = isolate(failed);
        return std::binary_search(decision.disabled.begin(), decision.disabled.end(), capability);
    }

private:
    const CapabilityRegistry& registry_;
    const DependencyGraph& graph_;
};

}  // namespace resilience
}  // namespace aura

#endif  // AURA_RESILIENCE_SUBSYSTEMISOLATIONMANAGER_H
