#ifndef AURA_FOUNDATION_IHASHER_H
#define AURA_FOUNDATION_IHASHER_H

#include "foundation/HashAlgorithm.h"
#include "foundation/HashDigest.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

namespace aura {
namespace foundation {

// Hasher interface contract (V3-01, V3-27).
//
// INT-0002 / Phase 0 integrity foundation. A hasher maps a byte sequence to a
// HashDigest for a declared algorithm. The contract is deterministic and
// stateless per call: identical input and algorithm must always yield an
// identical digest, and no call may depend on hidden mutable state. Implementations
// must not throw across this boundary; an unsupported algorithm returns an empty
// digest, which callers must treat as failure rather than as a valid hash.
class IHasher {
public:
    virtual ~IHasher() = default;

    virtual HashAlgorithm algorithm() const noexcept = 0;

    // Deterministic digest of exactly `size` bytes at `data`.
    // Returns an empty digest when `data` is null with non-zero size, or when the
    // algorithm is unsupported.
    virtual HashDigest hash(const std::uint8_t* data, std::size_t size) const noexcept = 0;

    // Convenience overload over a text view; identical bytes yield identical digests.
    HashDigest hash(std::string_view text) const noexcept {
        return hash(reinterpret_cast<const std::uint8_t*>(text.data()), text.size());
    }
};

// Factory for the canonical hasher implementation (INT-0003 Hasher.cpp).
// Declared here because the canonical decomposition provides no separate Hasher.h.
// Returns nullptr for an unsupported/unknown algorithm rather than substituting a
// different algorithm.
std::unique_ptr<IHasher> create_hasher(HashAlgorithm algorithm);

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_IHASHER_H
