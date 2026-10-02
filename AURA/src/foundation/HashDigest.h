#ifndef AURA_FOUNDATION_HASHDIGEST_H
#define AURA_FOUNDATION_HASHDIGEST_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace aura {
namespace foundation {

// Immutable fixed-size hash digest.
//
// FND-0006 / Phase 0 immutable foundation. A digest is an opaque byte sequence
// with an explicit length. This type stores and compares digests; it does not
// compute them (hashing belongs to the integrity layer, INT-0001..INT-0003).
// Determinism and byte-exact comparison are the only behaviours provided.
class HashDigest {
public:
    static constexpr std::size_t kMaxSize = 64;  // fits SHA-256/SHA-512.

    HashDigest() = default;

    // Constructs a digest from exactly `size` leading bytes of `bytes`.
    // Bytes beyond kMaxSize are ignored; an oversized request is clamped.
    static HashDigest from_bytes(const std::uint8_t* bytes, std::size_t size) noexcept {
        HashDigest digest;
        if (bytes == nullptr) return digest;
        if (size > kMaxSize) size = kMaxSize;
        digest.size_ = size;
        for (std::size_t i = 0; i < size; ++i) digest.bytes_[i] = bytes[i];
        return digest;
    }

    // Constructs a digest from lowercase/uppercase hex text. Returns an empty
    // digest for invalid input rather than throwing. Odd-length input drops the
    // trailing nibble.
    static HashDigest from_hex(std::string_view hex) noexcept {
        HashDigest digest;
        const std::size_t nibbles = hex.size() / 2;
        const std::size_t size = nibbles > kMaxSize ? kMaxSize : nibbles;
        for (std::size_t i = 0; i < size; ++i) {
            const int hi = hex_value(hex[2 * i]);
            const int lo = hex_value(hex[2 * i + 1]);
            if (hi < 0 || lo < 0) return HashDigest{};
            digest.bytes_[i] = static_cast<std::uint8_t>((hi << 4) | lo);
        }
        digest.size_ = size;
        return digest;
    }

    bool empty() const noexcept { return size_ == 0; }
    std::size_t size() const noexcept { return size_; }
    const std::array<std::uint8_t, kMaxSize>& bytes() const noexcept { return bytes_; }
    const std::uint8_t* data() const noexcept { return bytes_.data(); }

    std::string to_hex() const {
        static const char kHex[] = "0123456789abcdef";
        std::string out;
        out.reserve(size_ * 2);
        for (std::size_t i = 0; i < size_; ++i) {
            out.push_back(kHex[bytes_[i] >> 4]);
            out.push_back(kHex[bytes_[i] & 0x0F]);
        }
        return out;
    }

    friend bool operator==(const HashDigest& a, const HashDigest& b) noexcept {
        if (a.size_ != b.size_) return false;
        for (std::size_t i = 0; i < a.size_; ++i) {
            if (a.bytes_[i] != b.bytes_[i]) return false;
        }
        return true;
    }
    friend bool operator!=(const HashDigest& a, const HashDigest& b) noexcept { return !(a == b); }

    std::size_t hash() const noexcept {
        std::size_t h = std::hash<std::size_t>{}(size_);
        for (std::size_t i = 0; i < size_; ++i) {
            h ^= std::hash<std::uint8_t>{}(bytes_[i]) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        }
        return h;
    }

private:
    static constexpr int hex_value(char c) noexcept {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }

    std::array<std::uint8_t, kMaxSize> bytes_{};
    std::size_t size_{0};
};

}  // namespace foundation
}  // namespace aura

namespace std {
template <>
struct hash<aura::foundation::HashDigest> {
    std::size_t operator()(const aura::foundation::HashDigest& d) const noexcept { return d.hash(); }
};
}  // namespace std

#endif  // AURA_FOUNDATION_HASHDIGEST_H
