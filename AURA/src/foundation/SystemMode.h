#ifndef AURA_FOUNDATION_SYSTEMMODE_H
#define AURA_FOUNDATION_SYSTEMMODE_H

#include <cstdint>
#include <string_view>

namespace aura {
namespace foundation {

// Canonical system operating mode (V3-14).
//
// FND-0005 / Phase 0 immutable foundation. These values mirror V3-14 exactly and
// must not be merged or renamed. System mode is a distinct concept from
// ServiceState and must not be collapsed into it. SHADOW is representable as the
// primary initial operating mode; this type grants no live-trading authority.
enum class SystemMode : std::uint8_t {
    STARTING = 0,
    RECOVERY,
    NORMAL,
    DEGRADED,
    SHADOW,
    PAUSED,
    MANUAL,
    EMERGENCY,
    HALTED,
};

constexpr std::string_view to_string(SystemMode mode) noexcept {
    switch (mode) {
        case SystemMode::STARTING:  return "STARTING";
        case SystemMode::RECOVERY:  return "RECOVERY";
        case SystemMode::NORMAL:    return "NORMAL";
        case SystemMode::DEGRADED:  return "DEGRADED";
        case SystemMode::SHADOW:    return "SHADOW";
        case SystemMode::PAUSED:    return "PAUSED";
        case SystemMode::MANUAL:    return "MANUAL";
        case SystemMode::EMERGENCY: return "EMERGENCY";
        case SystemMode::HALTED:    return "HALTED";
    }
    return "UNKNOWN";
}

// Modes in which the system may continue producing decisions. NORMAL, DEGRADED
// and SHADOW are the operating modes; the remaining modes are transitional or
// protective. This is a descriptive predicate only and grants no authority.
constexpr bool allows_decision_production(SystemMode mode) noexcept {
    return mode == SystemMode::NORMAL || mode == SystemMode::DEGRADED ||
           mode == SystemMode::SHADOW;
}

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_SYSTEMMODE_H
