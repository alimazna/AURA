#ifndef AURA_FOUNDATION_GUARDIANSTATUS_H
#define AURA_FOUNDATION_GUARDIANSTATUS_H

#include <cstdint>
#include <string_view>

namespace aura {
namespace foundation {

// Guardian status (V3-13, V3-17).
//
// GDN-0001 / Phase 0 Guardian foundation. The Guardian is the authority that may
// isolate a failing subsystem, force SAFE mode or HALT, and require human
// approval for risky operations. Its status distinguishes normal operation from
// protective states. The Guardian must not grant itself new authority (V3-17), so
// no status here implies a capability grant.
enum class GuardianStatus : std::uint8_t {
    UNKNOWN = 0,
    NORMAL,
    DEGRADED,
    SAFE_MODE,
    HALTED,
};

constexpr std::string_view to_string(GuardianStatus status) noexcept {
    switch (status) {
        case GuardianStatus::UNKNOWN:   return "UNKNOWN";
        case GuardianStatus::NORMAL:    return "NORMAL";
        case GuardianStatus::DEGRADED:  return "DEGRADED";
        case GuardianStatus::SAFE_MODE: return "SAFE_MODE";
        case GuardianStatus::HALTED:    return "HALTED";
    }
    return "UNKNOWN";
}

// A protective status suspends or removes decision/execution capability.
constexpr bool is_protective(GuardianStatus status) noexcept {
    return status == GuardianStatus::SAFE_MODE || status == GuardianStatus::HALTED;
}

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_GUARDIANSTATUS_H
