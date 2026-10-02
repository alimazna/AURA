#ifndef AURA_FOUNDATION_AUDITOUTCOME_H
#define AURA_FOUNDATION_AUDITOUTCOME_H

#include <cstdint>
#include <string_view>

namespace aura {
namespace foundation {

// Canonical audited outcome (V3-27).
//
// AUD-0002 / Phase 0 audit foundation. Failed, rejected and rolled-back outcomes
// are first-class values: the Master requires that failed/rejected/rolled-back
// candidates remain research data rather than being erased (V3-27). This type
// enumerates outcomes only; it does not decide or record them.
enum class AuditOutcome : std::uint8_t {
    UNKNOWN = 0,
    SUCCESS,
    FAILURE,
    REJECTED,
    ROLLED_BACK,
    PARTIAL,
};

constexpr std::string_view to_string(AuditOutcome outcome) noexcept {
    switch (outcome) {
        case AuditOutcome::UNKNOWN:     return "UNKNOWN";
        case AuditOutcome::SUCCESS:     return "SUCCESS";
        case AuditOutcome::FAILURE:     return "FAILURE";
        case AuditOutcome::REJECTED:    return "REJECTED";
        case AuditOutcome::ROLLED_BACK: return "ROLLED_BACK";
        case AuditOutcome::PARTIAL:     return "PARTIAL";
    }
    return "UNKNOWN";
}

// Only a full SUCCESS is a positive outcome; everything else must be preserved as
// a non-success record rather than dropped.
constexpr bool is_success(AuditOutcome outcome) noexcept {
    return outcome == AuditOutcome::SUCCESS;
}

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_AUDITOUTCOME_H
