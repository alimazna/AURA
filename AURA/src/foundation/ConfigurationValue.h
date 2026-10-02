#ifndef AURA_FOUNDATION_CONFIGURATIONVALUE_H
#define AURA_FOUNDATION_CONFIGURATIONVALUE_H

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace aura {
namespace foundation {

// Immutable, typed configuration value.
//
// CFG-0002 / Phase 0 configuration foundation. The Master V3 lists configuration
// concerns (thresholds, weights, risk limits, enablement flags, policies,
// simulation parameters; V3-26) that reduce to a small set of scalar kinds. This
// type is a tagged value only: it performs no parsing, no type coercion, no
// defaulting and no mutation. Type mismatches must be surfaced by callers, not
// silently converted.
class ConfigurationValue {
public:
    enum class Type : std::uint8_t {
        NONE = 0,
        BOOLEAN,
        INTEGER,
        DECIMAL,
        TEXT,
    };

    ConfigurationValue() = default;

    static ConfigurationValue boolean(bool v) { return ConfigurationValue(Type::BOOLEAN, v, 0, 0.0, {}); }
    static ConfigurationValue integer(std::int64_t v) { return ConfigurationValue(Type::INTEGER, false, v, 0.0, {}); }
    static ConfigurationValue decimal(double v) { return ConfigurationValue(Type::DECIMAL, false, 0, v, {}); }
    static ConfigurationValue text(std::string v) {
        return ConfigurationValue(Type::TEXT, false, 0, 0.0, std::move(v));
    }

    Type type() const noexcept { return type_; }

    bool is_boolean() const noexcept { return type_ == Type::BOOLEAN; }
    bool is_integer() const noexcept { return type_ == Type::INTEGER; }
    bool is_decimal() const noexcept { return type_ == Type::DECIMAL; }
    bool is_text() const noexcept { return type_ == Type::TEXT; }

    bool as_boolean(bool fallback = false) const noexcept {
        return type_ == Type::BOOLEAN ? bool_value_ : fallback;
    }
    std::int64_t as_integer(std::int64_t fallback = 0) const noexcept {
        return type_ == Type::INTEGER ? int_value_ : fallback;
    }
    double as_decimal(double fallback = 0.0) const noexcept {
        return type_ == Type::DECIMAL ? decimal_value_ : fallback;
    }
    const std::string& as_text() const noexcept { return text_value_; }

    // Two values are equal only when they have the same type and value. A
    // decimal and an integer are never implicitly equal.
    friend bool operator==(const ConfigurationValue& a, const ConfigurationValue& b) noexcept {
        if (a.type_ != b.type_) return false;
        switch (a.type_) {
            case Type::NONE:    return true;
            case Type::BOOLEAN: return a.bool_value_ == b.bool_value_;
            case Type::INTEGER: return a.int_value_ == b.int_value_;
            case Type::DECIMAL: return a.decimal_value_ == b.decimal_value_;
            case Type::TEXT:    return a.text_value_ == b.text_value_;
        }
        return false;
    }
    friend bool operator!=(const ConfigurationValue& a, const ConfigurationValue& b) noexcept {
        return !(a == b);
    }

private:
    ConfigurationValue(Type type, bool b, std::int64_t i, double d, std::string t)
        : type_(type), bool_value_(b), int_value_(i), decimal_value_(d), text_value_(std::move(t)) {}

    Type type_{Type::NONE};
    bool bool_value_{false};
    std::int64_t int_value_{0};
    double decimal_value_{0.0};
    std::string text_value_{};
};

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_CONFIGURATIONVALUE_H
