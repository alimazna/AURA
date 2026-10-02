#ifndef AURA_FOUNDATION_ENTITYID_H
#define AURA_FOUNDATION_ENTITYID_H

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace aura {
namespace foundation {

// Immutable, strongly-typed identifier for a persistent entity.
//
// FND-0001 / Phase 0 immutable foundation. The Master V3 requires deterministic
// identity (V3-01) and deterministic signal/decision identity (V3-23), but does
// not freeze a concrete EntityId representation. This header therefore defines a
// minimal, self-contained, deterministic value type only; it performs no
// serialization, hashing, persistence, networking, logging, or trading.
//
// The value is a non-empty, opaque, byte-exact string token. Two EntityId values
// compare and hash on exactly those bytes, so identity is deterministic and
// reproducible across processes.
class EntityId {
public:
    using value_type = std::string;

    EntityId() = default;

    // Constructs an EntityId from a non-empty token. An empty token yields the
    // invalid (default) identity; no implicit construction from a bare string.
    explicit EntityId(value_type value)
        : value_(std::move(value)) {}

    static EntityId from_string(std::string_view value) {
        return EntityId(value_type(value));
    }

    // An EntityId is valid only when it carries a non-empty token.
    bool valid() const noexcept { return !value_.empty(); }
    explicit operator bool() const noexcept { return valid(); }

    const value_type& value() const noexcept { return value_; }
    std::string_view view() const noexcept { return value_; }

    friend bool operator==(const EntityId& a, const EntityId& b) noexcept {
        return a.value_ == b.value_;
    }
    friend bool operator!=(const EntityId& a, const EntityId& b) noexcept {
        return !(a == b);
    }
    friend bool operator<(const EntityId& a, const EntityId& b) noexcept {
        return a.value_ < b.value_;
    }

    std::size_t hash() const noexcept {
        return std::hash<value_type>{}(value_);
    }

private:
    value_type value_{};
};

}  // namespace foundation
}  // namespace aura

namespace std {
template <>
struct hash<aura::foundation::EntityId> {
    std::size_t operator()(const aura::foundation::EntityId& id) const noexcept {
        return id.hash();
    }
};
}  // namespace std

#endif  // AURA_FOUNDATION_ENTITYID_H
