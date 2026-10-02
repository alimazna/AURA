#ifndef AURA_FOUNDATION_AUDITACTION_H
#define AURA_FOUNDATION_AUDITACTION_H

#include <cstdint>
#include <string_view>

namespace aura {
namespace foundation {

// Canonical audited action (V3-27).
//
// AUD-0001 / Phase 0 audit foundation. Audit is append-only and historical
// records cannot be rewritten to make the current system look correct (V3-27).
// This type names the audited action kinds; it performs no recording itself.
// UNKNOWN is representable and must not be treated as a valid action.
enum class AuditAction : std::uint8_t {
    UNKNOWN = 0,
    STARTUP,
    SHUTDOWN,
    CONFIG_CHANGE,
    MODE_CHANGE,
    POLICY_CHANGE,
    DECISION,
    EXECUTION,
    PERSISTENCE,
    RECOVERY,
    ROLLBACK,
    PROMOTION,
    ERROR,
};

constexpr std::string_view to_string(AuditAction action) noexcept {
    switch (action) {
        case AuditAction::UNKNOWN:       return "UNKNOWN";
        case AuditAction::STARTUP:       return "STARTUP";
        case AuditAction::SHUTDOWN:      return "SHUTDOWN";
        case AuditAction::CONFIG_CHANGE: return "CONFIG_CHANGE";
        case AuditAction::MODE_CHANGE:   return "MODE_CHANGE";
        case AuditAction::POLICY_CHANGE: return "POLICY_CHANGE";
        case AuditAction::DECISION:      return "DECISION";
        case AuditAction::EXECUTION:     return "EXECUTION";
        case AuditAction::PERSISTENCE:   return "PERSISTENCE";
        case AuditAction::RECOVERY:      return "RECOVERY";
        case AuditAction::ROLLBACK:      return "ROLLBACK";
        case AuditAction::PROMOTION:     return "PROMOTION";
        case AuditAction::ERROR:         return "ERROR";
    }
    return "UNKNOWN";
}

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_AUDITACTION_H
