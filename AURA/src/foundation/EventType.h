#ifndef AURA_FOUNDATION_EVENTTYPE_H
#define AURA_FOUNDATION_EVENTTYPE_H

#include <cstdint>
#include <string_view>

namespace aura {
namespace foundation {

// Canonical event type discriminator (V3-14, V3-23).
//
// FND-0012 / Phase 0 immutable foundation. EventType is a value type only: it
// declares the recognised event kinds and their stable names. It performs no
// dispatch, subscription, queueing or I/O. The list is intentionally limited to
// the event kinds named by the Master V3; unknown kinds must be rejected or
// quarantined by receivers rather than treated as valid.
enum class EventType : std::uint8_t {
    UNKNOWN = 0,
    MARKET_DATA,
    SIGNAL,
    DECISION,
    RISK,
    EXECUTION,
    HEALTH,
    AUDIT,
    CONFIGURATION,
};

constexpr std::string_view to_string(EventType type) noexcept {
    switch (type) {
        case EventType::UNKNOWN:       return "UNKNOWN";
        case EventType::MARKET_DATA:   return "MARKET_DATA";
        case EventType::SIGNAL:        return "SIGNAL";
        case EventType::DECISION:      return "DECISION";
        case EventType::RISK:          return "RISK";
        case EventType::EXECUTION:     return "EXECUTION";
        case EventType::HEALTH:        return "HEALTH";
        case EventType::AUDIT:         return "AUDIT";
        case EventType::CONFIGURATION: return "CONFIGURATION";
    }
    return "UNKNOWN";
}

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_EVENTTYPE_H
