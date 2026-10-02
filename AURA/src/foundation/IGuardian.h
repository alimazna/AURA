#ifndef AURA_FOUNDATION_IGUARDIAN_H
#define AURA_FOUNDATION_IGUARDIAN_H

#include "foundation/ErrorSeverity.h"
#include "foundation/GuardianPolicy.h"
#include "foundation/GuardianStatus.h"
#include "foundation/RecoveryAction.h"

namespace aura {
namespace foundation {

// Guardian interface contract (V3-17).
//
// GDN-0003 / Phase 0 Guardian foundation. The interface exposes read-only policy
// access, current status, and a deterministic policy evaluation that yields the
// prescribed recovery action (which may be SAFE_MODE or HALT). It deliberately
// exposes no setter, no self-evolution hook and no authority-escalation surface:
// the Guardian may preserve the system but may not grant itself new authority.
class IGuardian {
public:
    virtual ~IGuardian() = default;

    virtual const GuardianPolicy& policy() const noexcept = 0;
    virtual GuardianStatus status() const noexcept = 0;

    // Deterministic mapping from an error severity to the recovery action the
    // policy prescribes. Must be pure: same input, same output, no side effects.
    virtual RecoveryAction evaluate(ErrorSeverity severity) const noexcept = 0;
};

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_IGUARDIAN_H
