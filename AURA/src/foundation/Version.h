#ifndef AURA_FOUNDATION_VERSION_H
#define AURA_FOUNDATION_VERSION_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <tuple>

namespace aura {
namespace foundation {

// Immutable, ordered version identifier.
//
// FND-0003 / Phase 0 immutable foundation. The Master V3 requires versioned
// configuration and provenance of the configuration/strategy/schema versions a
// decision depends on (V3-26). Version is an ordered triple of non-negative
// integers with an optional build tag; it carries no notion of "latest" and
// never mutates.
class Version {
public:
    using component_type = std::uint32_t;

    Version() = default;

    Version(component_type major, component_type minor, component_type patch,
            std::string build_tag = {})
        : major_(major), minor_(minor), patch_(patch), build_tag_(std::move(build_tag)) {}

    // Parses "MAJOR.MINOR.PATCH" with an optional "-BUILDTAG" suffix.
    // Missing numeric components default to zero; non-numeric input yields 0.0.0
    // with an empty build tag rather than throwing.
    static Version from_string(std::string_view text) {
        Version result;
        std::string_view core = text;
        const std::size_t dash = text.find('-');
        if (dash != std::string_view::npos) {
            core = text.substr(0, dash);
            result.build_tag_ = std::string(text.substr(dash + 1));
        }
        component_type* const slots[3] = {&result.major_, &result.minor_, &result.patch_};
        std::size_t slot = 0;
        std::size_t start = 0;
        while (start <= core.size() && slot < 3) {
            const std::size_t dot = core.find('.', start);
            const std::size_t end = (dot == std::string_view::npos) ? core.size() : dot;
            *slots[slot++] = parse_component(core.substr(start, end - start));
            if (dot == std::string_view::npos) break;
            start = dot + 1;
        }
        return result;
    }

    component_type major() const noexcept { return major_; }
    component_type minor() const noexcept { return minor_; }
    component_type patch() const noexcept { return patch_; }
    const std::string& build_tag() const noexcept { return build_tag_; }

    std::string to_string() const {
        std::string out = std::to_string(major_) + "." + std::to_string(minor_) + "." +
                          std::to_string(patch_);
        if (!build_tag_.empty()) out += "-" + build_tag_;
        return out;
    }

    // Deterministic ordering: numeric components first, then the build tag.
    friend bool operator==(const Version& a, const Version& b) noexcept {
        return std::tie(a.major_, a.minor_, a.patch_, a.build_tag_) ==
               std::tie(b.major_, b.minor_, b.patch_, b.build_tag_);
    }
    friend bool operator!=(const Version& a, const Version& b) noexcept { return !(a == b); }
    friend bool operator<(const Version& a, const Version& b) noexcept {
        return std::tie(a.major_, a.minor_, a.patch_, a.build_tag_) <
               std::tie(b.major_, b.minor_, b.patch_, b.build_tag_);
    }

    std::size_t hash() const noexcept {
        std::size_t h = std::hash<component_type>{}(major_);
        h ^= std::hash<component_type>{}(minor_) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        h ^= std::hash<component_type>{}(patch_) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        h ^= std::hash<std::string>{}(build_tag_) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        return h;
    }

private:
    static component_type parse_component(std::string_view digits) noexcept {
        component_type value = 0;
        for (const char c : digits) {
            if (c < '0' || c > '9') return 0;
            value = static_cast<component_type>(value * 10 + static_cast<unsigned>(c - '0'));
        }
        return value;
    }

    component_type major_{0};
    component_type minor_{0};
    component_type patch_{0};
    std::string build_tag_{};
};

}  // namespace foundation
}  // namespace aura

namespace std {
template <>
struct hash<aura::foundation::Version> {
    std::size_t operator()(const aura::foundation::Version& v) const noexcept { return v.hash(); }
};
}  // namespace std

#endif  // AURA_FOUNDATION_VERSION_H
