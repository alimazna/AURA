#ifndef AURA_FOUNDATION_ERRORSEVERITY_H
#define AURA_FOUNDATION_ERRORSEVERITY_H

#include <cstdint>
#include <string_view>

namespace aura {
namespace foundation {

// Canonical error severity (V3-27).
//
// FND-0014 / Phase 0 immutable foundation. Severity classifies how serious a
// structured error record is. It is a value type only; it performs no logging,
// no alerting and no recovery. Values mirror the Master V3 severity ordering.
enum class ErrorSeverity : std::uint8_t {
    INFO = 0,
    WARNING,
    ERROR,
    CRITICAL,
    FATAL,
};

constexpr std::string_view to_string(ErrorSeverity severity) noexcept {
    switch (severity) {
        case ErrorSeverity::INFO:     return "INFO";
        case ErrorSeverity::WARNING:  return "WARNING";
        case ErrorSeverity::ERROR:    return "ERROR";
        case ErrorSeverity::CRITICAL: return "CRITICAL";
        case ErrorSeverity::FATAL:    return "FATAL";
    }
    return "UNKNOWN";
}

// CRITICAL and FATAL are integrity-relevant and may force SAFE mode or HALT
// through the Guardian; INFO/WARNING/ERROR are not themselves protective.
constexpr bool is_critical(ErrorSeverity severity) noexcept {
    return severity == ErrorSeverity::CRITICAL || severity == ErrorSeverity::FATAL;
}

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_ERRORSEVERITY_H
