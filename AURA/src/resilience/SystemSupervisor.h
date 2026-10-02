#ifndef AURA_RESILIENCE_SYSTEMSUPERVISOR_H
#define AURA_RESILIENCE_SYSTEMSUPERVISOR_H

#include "foundation/IGuardian.h"
#include "foundation/ServiceState.h"
#include "resilience/CapabilityId.h"
#include "resilience/GracefulDegradationManager.h"
#include "resilience/HealthStateEngine.h"
#include "resilience/PauseResumeManager.h"
#include "resilience/RecoveryManager.h"

namespace aura {
namespace resilience {

// Coordinated, read-only view of a capability's resilience state.
//
// RS-0019 / Phase 0.5 resilience foundation. Aggregates the derived health,
// pause intent, degradation decision and recovery decision for one capability.
// It is a snapshot value only; the supervisor produces it without changing any
// runtime state.
struct SupervisionView {
    CapabilityId capability{};
    foundation::ServiceState state{foundation::ServiceState::STARTING};
    bool paused{false};
    DegradationDecision degradation{};
    RecoveryDecision recovery{};
};

// Coordinates health, degradation, recovery and pause/resume.
//
// RS-0019 / Phase 0.5 resilience foundation. Composes the resilience components
// by reference and delegates to them; it holds no independent mutable state and
// performs no I/O. It coordinates without granting new authority: it never
// escalates, never issues a protective action itself, and never enables live
// trading. Where a critical condition requires authority, it consults the
// Guardian (GDN-0003) and reports the prescribed action; issuing that action
// remains the Guardian's authority, not the supervisor's. Deterministic: every
// method is a pure function of the supplied inputs.
class SystemSupervisor {
public:
    SystemSupervisor(const HealthStateEngine& health,
                     const GracefulDegradationManager& degradation,
                     const RecoveryManager& recovery,
                     const PauseResumeManager& pause_resume,
                     const foundation::IGuardian& guardian)
        : health_(health),
          degradation_(degradation),
          recovery_(recovery),
          pause_resume_(pause_resume),
          guardian_(guardian) {}

    foundation::ServiceState capability_state(const CapabilityId& capability,
                                              const HealthStateEngine::snapshot_map& snapshots) const {
        return health_.capability_state(capability, snapshots);
    }

    DegradationDecision degradation(const CapabilityId& failed) const {
        return degradation_.apply(failed);
    }

    RecoveryDecision recovery(const CapabilityId& capability,
                              const HealthStateEngine::snapshot_map& snapshots,
                              bool known_valid_state_available) const {
        return recovery_.plan(capability, snapshots, known_valid_state_available);
    }

    bool is_paused(const CapabilityId& capability) const {
        return pause_resume_.is_paused(capability);
    }

    // Consulted authority. The supervisor reports the Guardian's status and
    // prescribed action; it does not itself escalate or apply them.
    foundation::GuardianStatus guardian_status() const noexcept { return guardian_.status(); }

    foundation::RecoveryAction prescribed_action(foundation::ErrorSeverity severity) const noexcept {
        return guardian_.evaluate(severity);
    }

    SupervisionView view(const CapabilityId& capability,
                         const HealthStateEngine::snapshot_map& snapshots,
                         bool known_valid_state_available) const {
        SupervisionView result;
        result.capability = capability;
        result.state = health_.capability_state(capability, snapshots);
        result.paused = pause_resume_.is_paused(capability);
        result.degradation = degradation_.apply(capability);
        result.recovery = recovery_.plan(capability, snapshots, known_valid_state_available);
        return result;
    }

private:
    const HealthStateEngine& health_;
    const GracefulDegradationManager& degradation_;
    const RecoveryManager& recovery_;
    const PauseResumeManager& pause_resume_;
    const foundation::IGuardian& guardian_;
};

}  // namespace resilience
}  // namespace aura

#endif  // AURA_RESILIENCE_SYSTEMSUPERVISOR_H
