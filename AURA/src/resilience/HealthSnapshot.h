#ifndef AURA_RESILIENCE_HEALTHSNAPSHOT_H
#define AURA_RESILIENCE_HEALTHSNAPSHOT_H

#include "foundation/ServiceState.h"
#include "foundation/Timestamp.h"
#include "resilience/ServiceDescriptor.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>

namespace aura {
namespace resilience {

// Immutable, timestamped health snapshot for a supervised service.
//
// RS-0005 / Phase 0.5 resilience foundation. Records the service's state and a
// human-readable reason at a specific observation time. It is a value contract
// only: it observes nothing, transitions nothing and performs no I/O. UNKNOWN is
// not a neutral success state: a snapshot whose service is not valid, or whose
// state is not ONLINE, must not be treated as safe (V3-13). The reason is kept
// so a degraded/offline/error condition is attributable rather than silent.
class HealthSnapshot {
public:
    HealthSnapshot() = default;

    HealthSnapshot(ServiceDescriptor service, foundation::ServiceState state,
                   foundation::Timestamp observed_at, std::string reason)
        : service_(std::move(service)),
          state_(state),
          observed_at_(observed_at),
          reason_(std::move(reason)) {}

    // A snapshot is attributable only when it names a valid service.
    bool valid() const noexcept { return service_.valid(); }
    explicit operator bool() const noexcept { return valid(); }

    const ServiceDescriptor& service() const noexcept { return service_; }
    foundation::ServiceState state() const noexcept { return state_; }
    foundation::Timestamp observed_at() const noexcept { return observed_at_; }
    const std::string& reason() const noexcept { return reason_; }

    // A service is operational only when it is actually ONLINE. A default or
    // non-ONLINE snapshot is not safe and must not be treated as neutral.
    bool operational() const noexcept { return foundation::is_operational(state_); }

    friend bool operator==(const HealthSnapshot& a, const HealthSnapshot& b) noexcept {
        return a.service_ == b.service_ && a.state_ == b.state_ &&
               a.observed_at_ == b.observed_at_ && a.reason_ == b.reason_;
    }
    friend bool operator!=(const HealthSnapshot& a, const HealthSnapshot& b) noexcept {
        return !(a == b);
    }
    friend bool operator<(const HealthSnapshot& a, const HealthSnapshot& b) noexcept {
        if (a.service_ != b.service_) return a.service_ < b.service_;
        if (a.observed_at_ != b.observed_at_) return a.observed_at_ < b.observed_at_;
        if (a.state_ != b.state_) return a.state_ < b.state_;
        return a.reason_ < b.reason_;
    }

    std::size_t hash() const noexcept {
        std::size_t h = service_.hash();
        h ^= std::hash<std::uint8_t>{}(static_cast<std::uint8_t>(state_)) +
             0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        h ^= observed_at_.hash() + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        h ^= std::hash<std::string>{}(reason_) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        return h;
    }

private:
    ServiceDescriptor service_{};
    foundation::ServiceState state_{foundation::ServiceState::STARTING};
    foundation::Timestamp observed_at_{};
    std::string reason_{};
};

}  // namespace resilience
}  // namespace aura

#endif  // AURA_RESILIENCE_HEALTHSNAPSHOT_H
