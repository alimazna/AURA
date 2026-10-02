#ifndef AURA_FOUNDATION_PERSISTENCESTATUS_H
#define AURA_FOUNDATION_PERSISTENCESTATUS_H

#include <cstdint>
#include <string_view>

namespace aura {
namespace foundation {

// Canonical persistence status (V3-22).
//
// PER-0001 / Phase 0 persistence foundation. Persistence failure is a safety
// event: critical state must survive process restart and the persistent ledger is
// the historical source of truth. Success, failure and unknown must not be
// collapsed into one another. UNKNOWN is not a success state.
enum class PersistenceStatus : std::uint8_t {
    UNKNOWN = 0,
    OK,
    NOT_FOUND,
    CONFLICT,
    CORRUPT,
    UNAVAILABLE,
    FAILED,
};

constexpr std::string_view to_string(PersistenceStatus status) noexcept {
    switch (status) {
        case PersistenceStatus::UNKNOWN:     return "UNKNOWN";
        case PersistenceStatus::OK:          return "OK";
        case PersistenceStatus::NOT_FOUND:   return "NOT_FOUND";
        case PersistenceStatus::CONFLICT:    return "CONFLICT";
        case PersistenceStatus::CORRUPT:     return "CORRUPT";
        case PersistenceStatus::UNAVAILABLE: return "UNAVAILABLE";
        case PersistenceStatus::FAILED:      return "FAILED";
    }
    return "UNKNOWN";
}

constexpr bool is_success(PersistenceStatus status) noexcept {
    return status == PersistenceStatus::OK;
}

// A failure that must be surfaced to the Guardian rather than swallowed.
constexpr bool is_safety_relevant(PersistenceStatus status) noexcept {
    return status == PersistenceStatus::CORRUPT || status == PersistenceStatus::UNAVAILABLE ||
           status == PersistenceStatus::FAILED;
}

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_PERSISTENCESTATUS_H
