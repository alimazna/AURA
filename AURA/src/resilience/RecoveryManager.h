#ifndef AURA_RESILIENCE_RECOVERYMANAGER_H
#define AURA_RESILIENCE_RECOVERYMANAGER_H

#include "foundation/RecoveryAction.h"
#include "foundation/ServiceState.h"
#include "resilience/CapabilityId.h"
#include "resilience/HealthStateEngine.h"
#include "resilience/SubsystemIsolationManager.h"

#include <vector>

namespace aura {
namespace resilience {

// Auditable recovery decision for one capability.
//
// RS-0016 / Phase 0.5 resilience foundation. Records the capability, the
// prescribed RecoveryAction, whether recovery uses a previously known-valid
// state, and a reason, so the outcome is attributable and auditable rather than
// silent. This is a decision record only: it performs no recovery, retries
// nothing, restores nothing and performs no I/O.
struct RecoveryDecision {
    CapabilityId capability{};
    foundation::RecoveryAction action{foundation::RecoveryAction::UNKNOWN};
    bool uses_known_valid_state{false};
    bool recoverable{false};
    const char* reason{""};
    bool valid{false};

    // Capabilities that an isolation of this capability had disabled and that a
    // successful recovery would restore. Empty when recovery is not planned.
    std::vector<CapabilityId> restored{};
};

// Plans subsystem recovery when possible.
//
// RS-0016 / Phase 0.5 resilience foundation. Recovery prefers a previously
// known-valid state: an unrecoverable capability (OFFLINE/BLOCKED/ERROR) is
// rolled back only when a known-valid state is available; otherwise it is
// escalated (SAFE_MODE) rather than silently retried. A DEGRADED capability is
// retried. Capabilities that are already ONLINE, RECOVERING/STARTING, or
// deliberately PAUSED require no recovery action. The decision is derived from
// observed health and is deterministic; the manager itself changes no state.
class RecoveryManager {
public:
    RecoveryManager(const HealthStateEngine& health, const SubsystemIsolationManager& isolation)
        : health_(health), isolation_(isolation) {}

    RecoveryDecision plan(const CapabilityId& capability,
                          const HealthStateEngine::snapshot_map& snapshots,
                          bool known_valid_state_available) const {
        RecoveryDecision decision;
        decision.capability = capability;
        if (!capability.valid()) return decision;

        const foundation::ServiceState state = health_.capability_state(capability, snapshots);
        decision.valid = true;

        switch (state) {
            case foundation::ServiceState::ONLINE:
                decision.action = foundation::RecoveryAction::NONE;
                decision.recoverable = true;
                decision.reason = "capability already online";
                return decision;
            case foundation::ServiceState::STARTING:
            case foundation::ServiceState::RECOVERING:
                decision.action = foundation::RecoveryAction::NONE;
                decision.recoverable = true;
                decision.reason = "recovery already in progress";
                return decision;
            case foundation::ServiceState::PAUSED:
                decision.action = foundation::RecoveryAction::NONE;
                decision.recoverable = true;
                decision.reason = "capability deliberately paused; resume is not recovery";
                return decision;
            case foundation::ServiceState::DEGRADED:
                decision.action = foundation::RecoveryAction::RETRY;
                decision.recoverable = true;
                decision.reason = "degraded; retry permitted";
                decision.restored = disabled_dependents(capability);
                return decision;
            case foundation::ServiceState::OFFLINE:
            case foundation::ServiceState::BLOCKED:
            case foundation::ServiceState::ERROR:
                break;
        }

        if (known_valid_state_available) {
            decision.action = foundation::RecoveryAction::ROLLBACK;
            decision.uses_known_valid_state = true;
            decision.recoverable = true;
            decision.reason = "rollback to known-valid state";
            decision.restored = disabled_dependents(capability);
        } else {
            decision.action = foundation::RecoveryAction::SAFE_MODE;
            decision.recoverable = false;
            decision.reason = "unrecoverable without a known-valid state; escalate";
        }
        return decision;
    }

private:
    // Dependents that an isolation of `capability` disables, excluding the
    // capability itself; these are what a successful recovery restores.
    std::vector<CapabilityId> disabled_dependents(const CapabilityId& capability) const {
        const IsolationDecision isolation = isolation_.isolate(capability);
        std::vector<CapabilityId> restored;
        for (const CapabilityId& id : isolation.disabled) {
            if (id != capability) restored.push_back(id);
        }
        return restored;
    }

    const HealthStateEngine& health_;
    const SubsystemIsolationManager& isolation_;
};

}  // namespace resilience
}  // namespace aura

#endif  // AURA_RESILIENCE_RECOVERYMANAGER_H
