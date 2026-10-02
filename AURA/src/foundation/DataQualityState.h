#ifndef AURA_FOUNDATION_DATAQUALITYSTATE_H
#define AURA_FOUNDATION_DATAQUALITYSTATE_H

#include <cstdint>
#include <string_view>

namespace aura {
namespace foundation {

// Canonical data quality state (V3-25).
//
// FND-0007 / Phase 0 immutable foundation. Only VALID is a success state.
// UNKNOWN, STALE, MISSING, DEGRADED, INVALID, OUT_OF_ORDER, DUPLICATE and
// INCOMPLETE must propagate into capability eligibility and decision gating,
// not merely be logged. Values mirror the Master V3 list exactly.
enum class DataQualityState : std::uint8_t {
    VALID = 0,
    DEGRADED,
    INVALID,
    UNKNOWN,
    STALE,
    MISSING,
    OUT_OF_ORDER,
    DUPLICATE,
    INCOMPLETE,
};

constexpr std::string_view to_string(DataQualityState state) noexcept {
    switch (state) {
        case DataQualityState::VALID:        return "VALID";
        case DataQualityState::DEGRADED:     return "DEGRADED";
        case DataQualityState::INVALID:      return "INVALID";
        case DataQualityState::UNKNOWN:      return "UNKNOWN";
        case DataQualityState::STALE:        return "STALE";
        case DataQualityState::MISSING:      return "MISSING";
        case DataQualityState::OUT_OF_ORDER: return "OUT_OF_ORDER";
        case DataQualityState::DUPLICATE:    return "DUPLICATE";
        case DataQualityState::INCOMPLETE:   return "INCOMPLETE";
    }
    return "UNKNOWN";
}

// Data is usable for a decision only when it is fully VALID. Everything else,
// including UNKNOWN, is not a neutral success state.
constexpr bool is_usable(DataQualityState state) noexcept {
    return state == DataQualityState::VALID;
}

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_DATAQUALITYSTATE_H
