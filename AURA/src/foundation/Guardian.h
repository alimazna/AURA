#ifndef AURA_FOUNDATION_GUARDIAN_H
#define AURA_FOUNDATION_GUARDIAN_H

#include "foundation/GuardianPolicy.h"
#include "foundation/GuardianStatus.h"
#include "foundation/IGuardian.h"

#include <utility>

namespace aura {
namespace foundation {

// Guardian skeleton (V3-17).
//
// GDN-0004 / Phase 0 Guardian foundation. This is a deterministic skeleton that
// conforms to IGuardian: it holds a policy and a status, and maps error severity
// to a prescribed recovery action. It is deliberately minimal — there is no
// setter and no self-evolution hook, so the Guardian cannot grant itself new
// authority. Status is supplied by an external authority at construction and is
// not mutated here. Behaviour beyond this skeleton (isolation, alerting,
// transitions, persistence) is out of Phase 0 scope.
class Guardian final : public IGuardian {
public:
    Guardian() = default;

    explicit Guardian(GuardianPolicy policy,
                      GuardianStatus initial_status = GuardianStatus::NORMAL)
        : policy_(std::move(policy)), status_(initial_status) {}

    const GuardianPolicy& policy() const noexcept override { return policy_; }
    GuardianStatus status() const noexcept override { return status_; }

    // Deterministic, side-effect-free severity mapping. FATAL forces HALT and
    // CRITICAL forces SAFE mode; both are protective. Lower severities request a
    // retry or no action. This function grants no new authority.
    RecoveryAction evaluate(ErrorSeverity severity) const noexcept override {
        switch (severity) {
            case ErrorSeverity::FATAL:    return RecoveryAction::HALT;
            case ErrorSeverity::CRITICAL: return RecoveryAction::SAFE_MODE;
            case ErrorSeverity::ERROR:    return RecoveryAction::RETRY;
            case ErrorSeverity::WARNING:
            case ErrorSeverity::INFO:     return RecoveryAction::NONE;
        }
        return RecoveryAction::UNKNOWN;
    }

private:
    GuardianPolicy policy_{};
    GuardianStatus status_{GuardianStatus::NORMAL};
};

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_GUARDIAN_H
