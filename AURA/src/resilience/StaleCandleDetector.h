#ifndef AURA_RESILIENCE_STALECANDLEDETECTOR_H
#define AURA_RESILIENCE_STALECANDLEDETECTOR_H

#include "foundation/DataQualityState.h"
#include "foundation/Timestamp.h"
#include "resilience/FreshnessState.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace aura {
namespace resilience {

// Detects stale closed candles per timeframe.
//
// RS-0013 / Phase 0.5 resilience foundation. The nine canonical timeframe streams
// are identified by their stable V3-29 labels ("M1", "M5", "M15", "M30", "H1",
// "H4", "D1", "W1", "MN1"); the label is the identity and is preserved exactly so
// a stale M15 candle can never be reported as, or overwrite, H4. The detector
// classifies the most recent closed-bar time of each timeframe against explicit
// thresholds and never treats stale data as fresh: only a fresh candle maps to
// DataQualityState::VALID. Deterministic and pure; it reads no clock and performs
// no I/O. Timeframes held in an ordered map so iteration is deterministic.
class StaleCandleDetector {
public:
    using rep = foundation::Timestamp::rep;
    using bar_time_map = std::map<std::string, foundation::Timestamp>;

    StaleCandleDetector() = default;

    StaleCandleDetector(rep stale_after_ns, rep expire_after_ns)
        : stale_after_ns_(stale_after_ns), expire_after_ns_(expire_after_ns) {}

    rep stale_after_ns() const noexcept { return stale_after_ns_; }
    rep expire_after_ns() const noexcept { return expire_after_ns_; }

    bool well_configured() const noexcept {
        return stale_after_ns_ > 0 && expire_after_ns_ > 0 && stale_after_ns_ <= expire_after_ns_;
    }

    // Quality of the latest closed candle for one timeframe. An untracked
    // timeframe is MISSING (not fresh); a non-positive/incoherent threshold set
    // yields UNKNOWN rather than a fabricated VALID.
    foundation::DataQualityState quality_of(const std::string& timeframe,
                                            const bar_time_map& last_closed,
                                            foundation::Timestamp now) const {
        if (!well_configured()) return foundation::DataQualityState::UNKNOWN;
        const auto it = last_closed.find(timeframe);
        if (it == last_closed.end()) return foundation::DataQualityState::MISSING;
        switch (classify_freshness(it->second, now, stale_after_ns_, expire_after_ns_)) {
            case FreshnessState::FRESH:   return foundation::DataQualityState::VALID;
            case FreshnessState::STALE:   return foundation::DataQualityState::STALE;
            case FreshnessState::EXPIRED: return foundation::DataQualityState::STALE;
            case FreshnessState::UNKNOWN: return foundation::DataQualityState::UNKNOWN;
        }
        return foundation::DataQualityState::UNKNOWN;
    }

    // True only when the timeframe's latest closed candle is fresh.
    bool is_fresh(const std::string& timeframe, const bar_time_map& last_closed,
                  foundation::Timestamp now) const {
        return quality_of(timeframe, last_closed, now) == foundation::DataQualityState::VALID;
    }

    // Every timeframe that is not fresh (stale, expired, missing or unknown), in
    // deterministic label order. This reports the exact stale timeframe(s) rather
    // than a single aggregate flag.
    std::vector<std::string> stale_timeframes(const bar_time_map& last_closed,
                                              foundation::Timestamp now) const {
        std::vector<std::string> result;
        for (const auto& entry : last_closed) {
            if (!is_fresh(entry.first, last_closed, now)) result.push_back(entry.first);
        }
        return result;
    }

private:
    rep stale_after_ns_{0};
    rep expire_after_ns_{0};
};

}  // namespace resilience
}  // namespace aura

#endif  // AURA_RESILIENCE_STALECANDLEDETECTOR_H
