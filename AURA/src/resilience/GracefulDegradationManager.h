#ifndef AURA_RESILIENCE_GRACEFULDEGRADATIONMANAGER_H
#define AURA_RESILIENCE_GRACEFULDEGRADATIONMANAGER_H

#include "resilience/CapabilityId.h"
#include "resilience/CapabilityRegistry.h"
#include "resilience/CriticalityPolicy.h"
#include "resilience/DegradationImpact.h"
#include "resilience/SubsystemIsolationManager.h"

namespace aura {
namespace resilience {

// Result of applying the graceful-degradation policy to a failed capability.
//
// RS-0015 / Phase 0.5 resilience foundation. Records the exact failed capability,
// the capabilities that are disabled, those that remain available, and whether
// the failure is tolerable. A capability that is not explicitly classified as
// NON_CRITICAL is never assumed safe: CRITICAL and UNCLASSIFIED both require
// protection. This is a decision record only: it invokes no Guardian, changes no
// runtime state, fabricates no data and performs no I/O.
struct DegradationDecision {
    CapabilityId failed{};
    Criticality criticality{Criticality::UNCLASSIFIED};
    bool tolerable{false};        // true only for explicitly NON_CRITICAL failure
    bool requires_protection{false};  // CRITICAL or UNCLASSIFIED
    DegradationImpact impact{};
    bool valid{false};
};

// Applies graceful-degradation policy deterministically.
//
// RS-0015 / Phase 0.5 resilience foundation. Combines the explicit criticality
// policy with subsystem isolation: a tolerable (NON_CRITICAL) failure disables
// only dependents and keeps independent capabilities running; a CRITICAL or
// UNCLASSIFIED failure additionally requires protection (surfaced for the
// Guardian), so the manager never fabricates a healthy state or silently
// continues. Pure and deterministic: same inputs, same decision.
class GracefulDegradationManager {
public:
    GracefulDegradationManager(const CapabilityRegistry& registry, const CriticalityPolicy& policy,
                               const SubsystemIsolationManager& isolation)
        : registry_(registry), policy_(policy), isolation_(isolation) {}

    DegradationDecision apply(const CapabilityId& failed) const {
        DegradationDecision decision;
        decision.failed = failed;
        if (!registry_.contains(failed)) return decision;

        decision.criticality = policy_.classify(failed);
        decision.tolerable = decision.criticality == Criticality::NON_CRITICAL;
        decision.requires_protection = !decision.tolerable;

        const IsolationDecision isolation = isolation_.isolate(failed);
        decision.impact = DegradationImpact(failed, isolation.disabled, isolation.available);
        decision.valid = true;
        return decision;
    }

private:
    const CapabilityRegistry& registry_;
    const CriticalityPolicy& policy_;
    const SubsystemIsolationManager& isolation_;
};

}  // namespace resilience
}  // namespace aura

#endif  // AURA_RESILIENCE_GRACEFULDEGRADATIONMANAGER_H
