#ifndef AURA_FOUNDATION_RECOVERYACTION_H
#define AURA_FOUNDATION_RECOVERYACTION_H

#include <cstdint>
#include <string_view>

namespace aura {
namespace foundation {

// Recovery-action descriptor (V3-17, V3-27, section 113).
//
// FND-0017 / Phase 0 immutable foundation. The Master names `recovery_action` as
// a field of every structured error but does not publish a closed value list, so
// this descriptor is deliberately minimal and explicitly extensible. UNKNOWN is
// representable and must not be treated as safe or as a no-op that hides a
// failure. This type carries no Guardian runtime dependency and performs no
// recovery itself.
enum class RecoveryAction : std::uint8_t {
    UNKNOWN = 0,
    NONE,
    RETRY,
    ROLLBACK,
    SAFE_MODE,
    HALT,
};

constexpr std::string_view to_string(RecoveryAction action) noexcept {
    switch (action) {
        case RecoveryAction::UNKNOWN:   return "UNKNOWN";
        case RecoveryAction::NONE:      return "NONE";
        case RecoveryAction::RETRY:     return "RETRY";
        case RecoveryAction::ROLLBACK:  return "ROLLBACK";
        case RecoveryAction::SAFE_MODE: return "SAFE_MODE";
        case RecoveryAction::HALT:      return "HALT";
    }
    return "UNKNOWN";
}

// UNKNOWN is not a safe default; an unresolved recovery action must be escalated
// rather than silently ignored.
constexpr bool is_known(RecoveryAction action) noexcept {
    return action != RecoveryAction::UNKNOWN;
}

// Actions that suspend or remove the ability to produce decisions. These are
// protective and must not be issued silently by lower-authority components; the
// Guardian must not grant itself new authority (V3-17).
constexpr bool is_protective(RecoveryAction action) noexcept {
    return action == RecoveryAction::ROLLBACK || action == RecoveryAction::SAFE_MODE ||
           action == RecoveryAction::HALT;
}

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_RECOVERYACTION_H
