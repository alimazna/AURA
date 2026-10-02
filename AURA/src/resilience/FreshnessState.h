#ifndef AURA_RESILIENCE_FRESHNESSSTATE_H
#define AURA_RESILIENCE_FRESHNESSSTATE_H

#include "foundation/Timestamp.h"

#include <cstdint>
#include <string_view>

namespace aura {
namespace resilience {

// Data-freshness state (V3-13, V3-25).
//
// RS-0006 / Phase 0.5 resilience foundation. Freshness is distinct from service
// state and from data-quality state; it classifies how current an observed value
// is relative to a reference time. UNKNOWN is not a neutral success state: data
// whose freshness cannot be established must not be treated as fresh.
enum class FreshnessState : std::uint8_t {
    UNKNOWN = 0,
    FRESH,
    STALE,
    EXPIRED,
};

constexpr std::string_view to_string(FreshnessState state) noexcept {
    switch (state) {
        case FreshnessState::UNKNOWN: return "UNKNOWN";
        case FreshnessState::FRESH:   return "FRESH";
        case FreshnessState::STALE:   return "STALE";
        case FreshnessState::EXPIRED: return "EXPIRED";
    }
    return "UNKNOWN";
}

// Only FRESH is a success state. UNKNOWN, STALE and EXPIRED must not be treated
// as fresh or as safe.
constexpr bool is_fresh(FreshnessState state) noexcept {
    return state == FreshnessState::FRESH;
}

// Classifies the age of an observation. `observed` is the time the value was
// produced; `now` is the reference time; `stale_after` and `expire_after` are
// positive durations. Deterministic and pure: it reads no clock. An observation
// newer than `now` (negative age) is not fabricated into FRESH; it is UNKNOWN
// because the timeline is inconsistent. Durations that are not positive yield
// UNKNOWN rather than a silent default.
constexpr FreshnessState classify_freshness(foundation::Timestamp observed,
                                            foundation::Timestamp now,
                                            foundation::Timestamp::rep stale_after_ns,
                                            foundation::Timestamp::rep expire_after_ns) noexcept {
    if (stale_after_ns <= 0 || expire_after_ns <= 0 || stale_after_ns > expire_after_ns) {
        return FreshnessState::UNKNOWN;
    }
    const foundation::Timestamp::rep age = now.nanoseconds() - observed.nanoseconds();
    if (age < 0) return FreshnessState::UNKNOWN;
    if (age > expire_after_ns) return FreshnessState::EXPIRED;
    if (age > stale_after_ns) return FreshnessState::STALE;
    return FreshnessState::FRESH;
}

}  // namespace resilience
}  // namespace aura

#endif  // AURA_RESILIENCE_FRESHNESSSTATE_H
