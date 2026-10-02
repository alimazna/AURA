#ifndef AURA_FOUNDATION_HASHALGORITHM_H
#define AURA_FOUNDATION_HASHALGORITHM_H

#include <cstdint>
#include <string_view>

namespace aura {
namespace foundation {

// Canonical hash algorithm identifier (V3-01, V3-27).
//
// INT-0001 / Phase 0 integrity foundation. Algorithm identifiers are explicit and
// versioned so a digest is never interpreted under an unknown algorithm. UNKNOWN
// is representable and must not be treated as safe. This type names algorithms; it
// does not compute digests.
enum class HashAlgorithm : std::uint8_t {
    UNKNOWN = 0,
    SHA256,
};

constexpr std::string_view to_string(HashAlgorithm algorithm) noexcept {
    switch (algorithm) {
        case HashAlgorithm::UNKNOWN: return "UNKNOWN";
        case HashAlgorithm::SHA256:  return "SHA-256";
    }
    return "UNKNOWN";
}

constexpr bool is_known(HashAlgorithm algorithm) noexcept {
    return algorithm != HashAlgorithm::UNKNOWN;
}

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_HASHALGORITHM_H
