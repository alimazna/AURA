#ifndef AURA_FOUNDATION_CONFIGURATIONKEY_H
#define AURA_FOUNDATION_CONFIGURATIONKEY_H

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace aura {
namespace foundation {

// Immutable configuration key.
//
// CFG-0001 / Phase 0 configuration foundation. A key is a non-empty, byte-exact
// opaque token naming a configuration entry. It carries no parsing, lookup,
// defaulting or mutation behaviour; versioning and change tracking belong to the
// snapshot/change types (V3-26).
class ConfigurationKey {
public:
    using value_type = std::string;

    ConfigurationKey() = default;

    explicit ConfigurationKey(value_type key) : key_(std::move(key)) {}

    static ConfigurationKey from_string(std::string_view key) {
        return ConfigurationKey(value_type(key));
    }

    // A key is valid only when it is non-empty.
    bool valid() const noexcept { return !key_.empty(); }
    explicit operator bool() const noexcept { return valid(); }

    const value_type& value() const noexcept { return key_; }
    std::string_view view() const noexcept { return key_; }

    friend bool operator==(const ConfigurationKey& a, const ConfigurationKey& b) noexcept {
        return a.key_ == b.key_;
    }
    friend bool operator!=(const ConfigurationKey& a, const ConfigurationKey& b) noexcept {
        return !(a == b);
    }
    friend bool operator<(const ConfigurationKey& a, const ConfigurationKey& b) noexcept {
        return a.key_ < b.key_;
    }

    std::size_t hash() const noexcept { return std::hash<value_type>{}(key_); }

private:
    value_type key_{};
};

}  // namespace foundation
}  // namespace aura

namespace std {
template <>
struct hash<aura::foundation::ConfigurationKey> {
    std::size_t operator()(const aura::foundation::ConfigurationKey& k) const noexcept {
        return k.hash();
    }
};
}  // namespace std

#endif  // AURA_FOUNDATION_CONFIGURATIONKEY_H
