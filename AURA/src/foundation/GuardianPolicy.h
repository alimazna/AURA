#ifndef AURA_FOUNDATION_GUARDIANPOLICY_H
#define AURA_FOUNDATION_GUARDIANPOLICY_H

#include "foundation/GuardianStatus.h"

#include <string>
#include <utility>

namespace aura {
namespace foundation {

// Guardian policy contract (V3-17).
//
// GDN-0002 / Phase 0 Guardian foundation. The policy is explicit and immutable at
// rest: the Guardian may force SAFE mode or HALT on critical integrity failure,
// and automatic rollback is permitted only to a previously approved KNOWN-GOOD
// state under explicit policy. The Guardian must not be self-evolved by the
// evolution engine and may not grant itself new authority. This type describes
// the policy; it does not evaluate it.
class GuardianPolicy {
public:
    GuardianPolicy() = default;

    GuardianPolicy(bool allow_automatic_rollback, bool require_human_approval_for_live,
                   std::string known_good_reference)
        : allow_automatic_rollback_(allow_automatic_rollback),
          require_human_approval_for_live_(require_human_approval_for_live),
          known_good_reference_(std::move(known_good_reference)) {}

    // Automatic rollback is allowed only when a previously approved KNOWN-GOOD
    // reference exists. A policy that permits rollback without one is invalid.
    bool allow_automatic_rollback() const noexcept { return allow_automatic_rollback_; }
    bool require_human_approval_for_live() const noexcept {
        return require_human_approval_for_live_;
    }
    const std::string& known_good_reference() const noexcept { return known_good_reference_; }

    // Rollback is actually permitted only when it is both allowed and anchored to
    // an approved KNOWN-GOOD state (V3-17).
    bool rollback_permitted() const noexcept {
        return allow_automatic_rollback_ && !known_good_reference_.empty();
    }

    // A policy is well-formed when it does not permit unattended rollback without a
    // KNOWN-GOOD anchor.
    bool well_formed() const noexcept { return !allow_automatic_rollback_ || rollback_permitted(); }

private:
    bool allow_automatic_rollback_{false};
    bool require_human_approval_for_live_{true};
    std::string known_good_reference_{};
};

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_GUARDIANPOLICY_H
