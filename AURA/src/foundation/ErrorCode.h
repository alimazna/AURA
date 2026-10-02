#ifndef AURA_FOUNDATION_ERRORCODE_H
#define AURA_FOUNDATION_ERRORCODE_H

#include <cstdint>
#include <string_view>

namespace aura {
namespace foundation {

// Canonical structured-error code taxonomy (V3-27, section 113).
//
// FND-0015 / Phase 0 immutable foundation. Frozen by the explicit human decision
// recorded in `project-control/DECISIONS.md` (2026-10-02), which resolves the
// V3-45 OPEN DECISION. The taxonomy is the closed set of 13 ERROR categories named
// by Master V3 section 113 and is represented as a strongly typed `enum class`.
//
// The integral identity of each code is explicit and documented so that it never
// depends on compiler enum ordering; the numeric values are stable and are the
// canonical serialized identity. Value 0 is intentionally left unassigned because
// this decision forbids an `UNKNOWN`/`OTHER` category, so 0 is not a valid
// ErrorCode. Severity is NOT embedded here: `ErrorSeverity` is an independent
// record-level field (see `ErrorSeverity.h`). This type is a value type only and
// performs no logging, alerting, recovery, serialization or persistence.
enum class ErrorCode : std::uint8_t {
    DATA_ERROR            = 1,
    SCHEMA_ERROR          = 2,
    CLOCK_ERROR           = 3,
    CONNECTION_ERROR      = 4,
    BROKER_ERROR          = 5,
    RISK_ERROR            = 6,
    EXECUTION_ERROR       = 7,
    RECONCILIATION_ERROR  = 8,
    PERSISTENCE_ERROR     = 9,
    MODEL_ERROR           = 10,
    CONFIG_ERROR          = 11,
    TELEGRAM_ERROR        = 12,
    RECOVERY_ERROR        = 13,
};

// Canonical stable name, matching the section 113 category labels exactly.
constexpr std::string_view to_string(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::DATA_ERROR:           return "DATA_ERROR";
        case ErrorCode::SCHEMA_ERROR:         return "SCHEMA_ERROR";
        case ErrorCode::CLOCK_ERROR:          return "CLOCK_ERROR";
        case ErrorCode::CONNECTION_ERROR:     return "CONNECTION_ERROR";
        case ErrorCode::BROKER_ERROR:         return "BROKER_ERROR";
        case ErrorCode::RISK_ERROR:           return "RISK_ERROR";
        case ErrorCode::EXECUTION_ERROR:      return "EXECUTION_ERROR";
        case ErrorCode::RECONCILIATION_ERROR: return "RECONCILIATION_ERROR";
        case ErrorCode::PERSISTENCE_ERROR:    return "PERSISTENCE_ERROR";
        case ErrorCode::MODEL_ERROR:          return "MODEL_ERROR";
        case ErrorCode::CONFIG_ERROR:         return "CONFIG_ERROR";
        case ErrorCode::TELEGRAM_ERROR:       return "TELEGRAM_ERROR";
        case ErrorCode::RECOVERY_ERROR:       return "RECOVERY_ERROR";
    }
    return "UNKNOWN";
}

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_ERRORCODE_H
