#ifndef AURA_DESKTOP_STATEVISUALS_H
#define AURA_DESKTOP_STATEVISUALS_H

// Pure state-classification for the AURA control center.
//
// This header deliberately has NO ImGui/GL dependency so the state map can be
// unit-tested in the default (GUI-off) build. It is the single source of truth
// for "what category does this contract state look like". The colour mapping
// lives in AuraTheme.h, which includes this header.
//
// The invariant that matters: UNKNOWN and NOT AVAILABLE are their own
// categories and never collapse into Healthy. A state that is not recognised is
// Unknown, never silently green.

#include <cstdint>
#include <string_view>

namespace aura {
namespace desktop {
namespace theme {

enum class StateCategory : std::uint8_t {
    Healthy,
    Degraded,
    Critical,
    Paused,
    Informational,
    NotAvailable,
    Unknown,
};

inline StateCategory state_category(std::string_view state) {
    if (state == "ONLINE" || state == "VALID" || state == "HEALTHY" || state == "OK" ||
        state == "CONNECTED" || state == "SUPPORTED" || state == "ACCEPTED" || state == "APPROVED" ||
        state == "FRESH")
        return StateCategory::Healthy;
    if (state == "DEGRADED" || state == "WARNING" || state == "STALE" || state == "RECOVERING" ||
        state == "PROMISING" || state == "PROPOSED" || state == "PENDING" ||
        state == "UNRESOLVED" || state == "INCOMPLETE" || state == "DUPLICATE" ||
        state == "OUT_OF_ORDER" || state == "LAGGING")
        return StateCategory::Degraded;
    if (state == "OFFLINE" || state == "ERROR" || state == "CRITICAL" || state == "BLOCKED" ||
        state == "CORRUPTED_STATE" || state == "INVALID" || state == "REFUTED" ||
        state == "REJECTED" || state == "FAILED")
        return StateCategory::Critical;
    if (state == "PAUSED") return StateCategory::Paused;
    if (state == "STARTING" || state == "INFO" || state == "NONE" || state == "NEUTRAL")
        return StateCategory::Informational;
    if (state == "NOT AVAILABLE" || state == "NA" || state == "MISSING")
        return StateCategory::NotAvailable;
    // Empty or unrecognised stays Unknown; never silently healthy.
    return StateCategory::Unknown;
}

inline const char* category_label(StateCategory c) {
    switch (c) {
        case StateCategory::Healthy:       return "HEALTHY";
        case StateCategory::Degraded:      return "DEGRADED";
        case StateCategory::Critical:      return "CRITICAL";
        case StateCategory::Paused:        return "PAUSED";
        case StateCategory::Informational: return "INFO";
        case StateCategory::NotAvailable:  return "NOT AVAILABLE";
        case StateCategory::Unknown:       return "UNKNOWN";
    }
    return "UNKNOWN";
}

// True when a value carries no information (unknown / not available / empty).
// Used to keep such values visually quiet instead of implying a healthy zero.
inline bool is_unknown_like(std::string_view state) {
    const StateCategory c = state_category(state);
    return c == StateCategory::Unknown || c == StateCategory::NotAvailable;
}

}  // namespace theme
}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_STATEVISUALS_H
