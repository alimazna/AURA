#include "foundation/IHasher.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

namespace aura {
namespace foundation {
namespace {

constexpr std::array<std::uint32_t, 64> kRoundConstants = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u,
    0xab1c5ed5u, 0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu,
    0x9bdc06a7u, 0xc19bf174u, 0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu,
    0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau, 0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu,
    0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u, 0xa2bfe8a1u, 0xa81a664bu,
    0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u, 0x19a4c116u,
    0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u,
    0xc67178f2u,
};

constexpr std::uint32_t rotr(std::uint32_t x, unsigned n) noexcept {
    return (x >> n) | (x << (32 - n));
}

void process_block(std::uint32_t (&state)[8], const std::uint8_t* block) noexcept {
    std::uint32_t w[64];
    for (unsigned t = 0; t < 16; ++t) {
        w[t] = (static_cast<std::uint32_t>(block[4 * t]) << 24) |
               (static_cast<std::uint32_t>(block[4 * t + 1]) << 16) |
               (static_cast<std::uint32_t>(block[4 * t + 2]) << 8) |
               static_cast<std::uint32_t>(block[4 * t + 3]);
    }
    for (unsigned t = 16; t < 64; ++t) {
        const std::uint32_t s0 = rotr(w[t - 15], 7) ^ rotr(w[t - 15], 18) ^ (w[t - 15] >> 3);
        const std::uint32_t s1 = rotr(w[t - 2], 17) ^ rotr(w[t - 2], 19) ^ (w[t - 2] >> 10);
        w[t] = w[t - 16] + s0 + w[t - 7] + s1;
    }

    std::uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    std::uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

    for (unsigned t = 0; t < 64; ++t) {
        const std::uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        const std::uint32_t ch = (e & f) ^ (~e & g);
        const std::uint32_t t1 = h + s1 + ch + kRoundConstants[t] + w[t];
        const std::uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        const std::uint32_t t2 = s0 + maj;
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

// Deterministic SHA-256 (FIPS 180-4). Stateless per call; no hidden mutable state.
class Sha256Hasher final : public IHasher {
public:
    HashAlgorithm algorithm() const noexcept override { return HashAlgorithm::SHA256; }

    HashDigest hash(const std::uint8_t* data, std::size_t size) const noexcept override {
        if (data == nullptr && size != 0) return HashDigest{};

        std::uint32_t state[8] = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
                                  0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};

        std::size_t offset = 0;
        for (; offset + 64 <= size; offset += 64) process_block(state, data + offset);

        const std::size_t remainder = size - offset;
        std::uint8_t tail[128] = {0};
        if (remainder != 0) std::memcpy(tail, data + offset, remainder);
        tail[remainder] = 0x80u;
        const std::size_t tail_length = (remainder < 56) ? 64 : 128;
        const std::uint64_t bit_length = static_cast<std::uint64_t>(size) * 8u;
        for (unsigned b = 0; b < 8; ++b) {
            tail[tail_length - 1 - b] = static_cast<std::uint8_t>(bit_length >> (8 * b));
        }
        for (std::size_t block = 0; block < tail_length; block += 64) {
            process_block(state, tail + block);
        }

        std::uint8_t digest_bytes[32];
        for (unsigned i = 0; i < 8; ++i) {
            digest_bytes[4 * i] = static_cast<std::uint8_t>(state[i] >> 24);
            digest_bytes[4 * i + 1] = static_cast<std::uint8_t>(state[i] >> 16);
            digest_bytes[4 * i + 2] = static_cast<std::uint8_t>(state[i] >> 8);
            digest_bytes[4 * i + 3] = static_cast<std::uint8_t>(state[i]);
        }
        return HashDigest::from_bytes(digest_bytes, sizeof(digest_bytes));
    }
};

}  // namespace

std::unique_ptr<IHasher> create_hasher(HashAlgorithm algorithm) {
    switch (algorithm) {
        case HashAlgorithm::SHA256:
            return std::unique_ptr<IHasher>(new Sha256Hasher());
        case HashAlgorithm::UNKNOWN:
            break;
    }
    return nullptr;
}

}  // namespace foundation
}  // namespace aura
