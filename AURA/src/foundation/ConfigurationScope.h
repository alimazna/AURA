#ifndef AURA_FOUNDATION_CONFIGURATIONSCOPE_H
#define AURA_FOUNDATION_CONFIGURATIONSCOPE_H

#include <cstdint>
#include <string_view>

namespace aura {
namespace foundation {

// Canonical configuration scope (V3-26).
//
// CFG-0004 / Phase 0 configuration foundation. The Master V3 states that runtime
// configuration and research configuration are distinct concepts; the
// distinction must be preserved rather than collapsed. Scope classifies which
// plane a configuration entry belongs to. It carries no loading or precedence
// behaviour.
enum class ConfigurationScope : std::uint8_t {
    RUNTIME = 0,
    RESEARCH,
};

constexpr std::string_view to_string(ConfigurationScope scope) noexcept {
    switch (scope) {
        case ConfigurationScope::RUNTIME:  return "RUNTIME";
        case ConfigurationScope::RESEARCH: return "RESEARCH";
    }
    return "UNKNOWN";
}

// Only runtime configuration may influence live/shadow decision production.
constexpr bool affects_decisions(ConfigurationScope scope) noexcept {
    return scope == ConfigurationScope::RUNTIME;
}

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_CONFIGURATIONSCOPE_H
