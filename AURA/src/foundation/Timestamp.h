#ifndef AURA_FOUNDATION_TIMESTAMP_H
#define AURA_FOUNDATION_TIMESTAMP_H

#include <cstdint>
#include <functional>
#include <string>

namespace aura {
namespace foundation {

// Immutable point in time with an explicit unit.
//
// FND-0002 / Phase 0 immutable foundation. The Master V3 distinguishes several
// temporal concepts (event_time, receive_time, closed-bar time, processing time,
// research time; V3-24) and requires that no future information enters a decision.
// Timestamp carries no clock, no timezone and no wall-clock interpretation; it is
// a raw count of nanoseconds since the Unix epoch (1970-01-01T00:00:00Z). It never
// reads the system clock, so it introduces no lookahead or repaint behaviour.
class Timestamp {
public:
    using rep = std::int64_t;

    static constexpr std::int64_t kNsPerMicrosecond = 1000;
    static constexpr std::int64_t kNsPerMillisecond = 1000000;
    static constexpr std::int64_t kNsPerSecond = 1000000000;

    Timestamp() = default;

    explicit constexpr Timestamp(rep nanoseconds_since_epoch) noexcept
        : nanos_(nanoseconds_since_epoch) {}

    static constexpr Timestamp from_nanoseconds(rep v) noexcept { return Timestamp(v); }
    static constexpr Timestamp from_microseconds(rep v) noexcept { return Timestamp(v * kNsPerMicrosecond); }
    static constexpr Timestamp from_milliseconds(rep v) noexcept { return Timestamp(v * kNsPerMillisecond); }
    static constexpr Timestamp from_seconds(rep v) noexcept { return Timestamp(v * kNsPerSecond); }

    // Raw count of nanoseconds since the Unix epoch.
    constexpr rep nanoseconds() const noexcept { return nanos_; }
    constexpr rep microseconds() const noexcept { return nanos_ / kNsPerMicrosecond; }
    constexpr rep milliseconds() const noexcept { return nanos_ / kNsPerMillisecond; }
    constexpr rep seconds() const noexcept { return nanos_ / kNsPerSecond; }

    // Deterministic, exact comparison on the stored value.
    friend constexpr bool operator==(Timestamp a, Timestamp b) noexcept { return a.nanos_ == b.nanos_; }
    friend constexpr bool operator!=(Timestamp a, Timestamp b) noexcept { return a.nanos_ != b.nanos_; }
    friend constexpr bool operator<(Timestamp a, Timestamp b) noexcept { return a.nanos_ < b.nanos_; }
    friend constexpr bool operator<=(Timestamp a, Timestamp b) noexcept { return a.nanos_ <= b.nanos_; }
    friend constexpr bool operator>(Timestamp a, Timestamp b) noexcept { return a.nanos_ > b.nanos_; }
    friend constexpr bool operator>=(Timestamp a, Timestamp b) noexcept { return a.nanos_ >= b.nanos_; }

    constexpr Timestamp operator+(Timestamp d) const noexcept { return Timestamp(nanos_ + d.nanos_); }
    constexpr Timestamp operator-(Timestamp d) const noexcept { return Timestamp(nanos_ - d.nanos_); }

    std::size_t hash() const noexcept { return std::hash<rep>{}(nanos_); }

private:
    rep nanos_{0};
};

}  // namespace foundation
}  // namespace aura

namespace std {
template <>
struct hash<aura::foundation::Timestamp> {
    std::size_t operator()(const aura::foundation::Timestamp& t) const noexcept { return t.hash(); }
};
}  // namespace std

#endif  // AURA_FOUNDATION_TIMESTAMP_H
