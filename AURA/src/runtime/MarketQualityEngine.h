#ifndef AURA_RUNTIME_MARKETQUALITYENGINE_H
#define AURA_RUNTIME_MARKETQUALITYENGINE_H

#include "foundation/DataQualityState.h"
#include "runtime/AdapterManager.h"

#include <cstdint>
#include <string_view>

namespace aura {
namespace runtime {

// Aggregate market-quality classification (V3-25).
//
// RT-0014 / Phase 1 deterministic runtime. Derived from observed per-stream
// data-quality states; UNKNOWN is not neutral and means quality could not be
// established.
enum class MarketQuality : std::uint8_t {
    UNKNOWN = 0,
    GOOD,
    DEGRADED,
    BAD,
};

constexpr std::string_view to_string(MarketQuality quality) noexcept {
    switch (quality) {
        case MarketQuality::UNKNOWN:  return "UNKNOWN";
        case MarketQuality::GOOD:     return "GOOD";
        case MarketQuality::DEGRADED: return "DEGRADED";
        case MarketQuality::BAD:      return "BAD";
    }
    return "UNKNOWN";
}

// Aggregate market-quality verdict.
struct MarketQualityVerdict {
    MarketQuality quality{MarketQuality::UNKNOWN};
    bool usable{false};
    bool valid{false};
};

// Aggregates observed data-quality state into a market-quality verdict.
//
// RT-0014 / Phase 1 deterministic runtime. Uses only the observed per-stream
// quality recorded by the adapter boundary (RT-0001); it fabricates no quality
// and assumes no healthy default. A stream whose quality is not VALID degrades
// the aggregate; an unassessed stream yields UNKNOWN. The verdict is usable only
// when GOOD. Deterministic and pure.
class MarketQualityEngine {
public:
    MarketQualityEngine() = default;

    MarketQualityVerdict evaluate(const AdapterManager& adapters) const {
        MarketQualityVerdict verdict;
        bool any_unknown = false;
        bool any_degraded = false;
        bool any_bad = false;
        std::size_t assessed = 0;

        for (const auto& entry : adapters.streams()) {
            const StreamStatus& stream = entry.second;
            switch (stream.quality) {
                case foundation::DataQualityState::VALID:
                    break;
                case foundation::DataQualityState::UNKNOWN:
                case foundation::DataQualityState::MISSING:
                    any_unknown = true;
                    break;
                case foundation::DataQualityState::INVALID:
                case foundation::DataQualityState::OUT_OF_ORDER:
                case foundation::DataQualityState::DUPLICATE:
                case foundation::DataQualityState::INCOMPLETE:
                    any_bad = true;
                    break;
                case foundation::DataQualityState::DEGRADED:
                case foundation::DataQualityState::STALE:
                    any_degraded = true;
                    break;
            }
            ++assessed;
        }

        if (assessed == 0 || any_unknown) {
            verdict.quality = MarketQuality::UNKNOWN;
        } else if (any_bad) {
            verdict.quality = MarketQuality::BAD;
        } else if (any_degraded) {
            verdict.quality = MarketQuality::DEGRADED;
        } else {
            verdict.quality = MarketQuality::GOOD;
        }
        verdict.usable = verdict.quality == MarketQuality::GOOD;
        verdict.valid = true;
        return verdict;
    }
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_MARKETQUALITYENGINE_H
