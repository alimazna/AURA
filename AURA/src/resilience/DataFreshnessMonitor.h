#ifndef AURA_RESILIENCE_DATAFRESHNESSMONITOR_H
#define AURA_RESILIENCE_DATAFRESHNESSMONITOR_H

#include "foundation/Timestamp.h"
#include "resilience/FreshnessState.h"

#include <cstdint>

namespace aura {
namespace resilience {

// Evaluates data freshness against explicit thresholds.
//
// RS-0012 / Phase 0.5 resilience foundation. Holds the stale/expire thresholds
// and classifies an observation deterministically. It reads no clock: the caller
// supplies both the observation time and the reference time, so evaluation
// introduces no lookahead and no hidden wall-clock dependency. Distinguishes
// fresh from stale and unknown and never fabricates freshness; a misconfigured
// threshold set yields UNKNOWN rather than a silent FRESH.
class DataFreshnessMonitor {
public:
    using rep = foundation::Timestamp::rep;

    DataFreshnessMonitor() = default;

    DataFreshnessMonitor(rep stale_after_ns, rep expire_after_ns)
        : stale_after_ns_(stale_after_ns), expire_after_ns_(expire_after_ns) {}

    rep stale_after_ns() const noexcept { return stale_after_ns_; }
    rep expire_after_ns() const noexcept { return expire_after_ns_; }

    // True when the thresholds are coherent and positive.
    bool well_configured() const noexcept {
        return stale_after_ns_ > 0 && expire_after_ns_ > 0 && stale_after_ns_ <= expire_after_ns_;
    }

    FreshnessState evaluate(foundation::Timestamp observed, foundation::Timestamp now) const noexcept {
        return classify_freshness(observed, now, stale_after_ns_, expire_after_ns_);
    }

    // Convenience predicate; only FRESH is fresh. UNKNOWN is not fresh.
    bool is_fresh(foundation::Timestamp observed, foundation::Timestamp now) const noexcept {
        return resilience::is_fresh(evaluate(observed, now));
    }

private:
    rep stale_after_ns_{0};
    rep expire_after_ns_{0};
};

}  // namespace resilience
}  // namespace aura

#endif  // AURA_RESILIENCE_DATAFRESHNESSMONITOR_H
