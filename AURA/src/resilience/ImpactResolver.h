#ifndef AURA_RESILIENCE_IMPACTRESOLVER_H
#define AURA_RESILIENCE_IMPACTRESOLVER_H

#include "resilience/CapabilityId.h"
#include "resilience/DegradationImpact.h"
#include "resilience/DependencyGraph.h"

#include <algorithm>
#include <vector>

namespace aura {
namespace resilience {

// Maps a capability failure to its capability impact.
//
// RS-0018 / Phase 0.5 resilience foundation. Given a failed capability, produces
// the exact set of affected capabilities (the failure plus its transitive
// dependents) and the capabilities that remain available, using the explicit
// dependency graph rather than an implied one (V3-15). The exact failed
// capability is always reported, so a failed timeframe is never hidden behind an
// aggregate status. Deterministic: the impact set is produced in graph order.
class ImpactResolver {
public:
    explicit ImpactResolver(const DependencyGraph& graph) : graph_(graph) {}

    DegradationImpact resolve(const CapabilityId& failed) const {
        DegradationImpact::capability_list affected;
        if (failed.valid()) affected.push_back(failed);
        for (const CapabilityId& dependent : graph_.reachable_dependents(failed)) {
            if (dependent != failed) affected.push_back(dependent);
        }

        DegradationImpact::capability_list available;
        for (const CapabilityId& node : graph_.nodes()) {
            if (!contains(affected, node)) available.push_back(node);
        }
        return DegradationImpact(failed, std::move(affected), std::move(available));
    }

private:
    static bool contains(const std::vector<CapabilityId>& v, const CapabilityId& id) {
        return std::find(v.begin(), v.end(), id) != v.end();
    }

    const DependencyGraph& graph_;
};

}  // namespace resilience
}  // namespace aura

#endif  // AURA_RESILIENCE_IMPACTRESOLVER_H
