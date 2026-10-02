#ifndef AURA_FOUNDATION_SERVICESTATE_H
#define AURA_FOUNDATION_SERVICESTATE_H

#include <cstdint>
#include <string_view>

namespace aura {
namespace foundation {

// Canonical service / subsystem state (V3-14).
//
// FND-0004 / Phase 0 immutable foundation. Service state is distinct from system
// operating mode (SystemMode) and must not be merged with it. Values mirror the
// Master V3 list exactly; they are never reordered or renumbered silently.
enum class ServiceState : std::uint8_t {
    STARTING = 0,
    ONLINE,
    DEGRADED,
    OFFLINE,
    RECOVERING,
    PAUSED,
    BLOCKED,
    ERROR,
};

// Stable wire/log name. UNKNOWN is not a state and must not be treated as safe.
constexpr std::string_view to_string(ServiceState state) noexcept {
    switch (state) {
        case ServiceState::STARTING:   return "STARTING";
        case ServiceState::ONLINE:     return "ONLINE";
        case ServiceState::DEGRADED:   return "DEGRADED";
        case ServiceState::OFFLINE:    return "OFFLINE";
        case ServiceState::RECOVERING: return "RECOVERING";
        case ServiceState::PAUSED:     return "PAUSED";
        case ServiceState::BLOCKED:    return "BLOCKED";
        case ServiceState::ERROR:      return "ERROR";
    }
    return "UNKNOWN";
}

// A service is usable only when it is fully ONLINE. Every other state must be
// treated as not-safe rather than as a neutral success.
constexpr bool is_operational(ServiceState state) noexcept {
    return state == ServiceState::ONLINE;
}

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_SERVICESTATE_H
