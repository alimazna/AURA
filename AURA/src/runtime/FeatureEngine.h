#ifndef AURA_RUNTIME_FEATUREENGINE_H
#define AURA_RUNTIME_FEATUREENGINE_H

#include "runtime/AdapterManager.h"
#include "runtime/BarFinalizer.h"
#include "runtime/DataBus.h"
#include "runtime/TimeframeStateStore.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace aura {
namespace runtime {

// Deterministic feature set derived from closed bars only.
//
// RT-0006 / Phase 1 deterministic runtime. `based_on` records the exact closed
// bar the features derive from, so a feature vector is always attributable and
// reproducible. Every field is a pure function of the supplied closed bars.
struct FeatureSet {
    Timeframe timeframe{Timeframe::UNKNOWN};
    BarIdentity based_on{};
    bool valid{false};
    std::size_t bar_count{0};
    double last_close{0.0};
    double return_1{0.0};   // (close-open)/open of the last closed bar
    double return_n{0.0};   // (close_last/close_first - 1) over the window
    double true_range{0.0}; // true range of the last closed bar
    double atr{0.0};        // mean true range over the window
    double highest_high{0.0};
    double lowest_low{0.0};
};

// Computes deterministic features from closed data only.
//
// RT-0006 / Phase 1 deterministic runtime. Input bars must all be closed; if any
// supplied bar is still forming the computation is refused (valid=false) rather
// than using a partial bar, so no future information enters a feature (V3-24, no
// lookahead, no repaint). The window is bounded and the computation is a pure
// function of the supplied bars. No clock, no I/O.
class FeatureEngine {
public:
    static constexpr std::size_t kDefaultWindow = 14;

    explicit FeatureEngine(std::size_t window = kDefaultWindow)
        : window_(window == 0 ? kDefaultWindow : window) {}

    FeatureSet compute(const std::vector<MarketBar>& closed_bars, Timeframe timeframe) const {
        FeatureSet features;
        features.timeframe = timeframe;
        if (timeframe == Timeframe::UNKNOWN || closed_bars.empty()) return features;

        // Reject any still-forming bar: only closed data may be used.
        for (const MarketBar& bar : closed_bars) {
            if (!bar.closed) return features;
        }

        const std::size_t start = closed_bars.size() > window_ ? closed_bars.size() - window_ : 0;
        const MarketBar& last = closed_bars.back();

        double highest = closed_bars[start].high;
        double lowest = closed_bars[start].low;
        double true_range_sum = 0.0;
        double prev_close = closed_bars[start].close;
        for (std::size_t i = start; i < closed_bars.size(); ++i) {
            const MarketBar& bar = closed_bars[i];
            if (bar.high > highest) highest = bar.high;
            if (bar.low < lowest) lowest = bar.low;
            const double tr = std::max(bar.high - bar.low,
                                       std::max(bar.high - prev_close, prev_close - bar.low));
            true_range_sum += tr;
            prev_close = bar.close;
        }
        const std::size_t count = closed_bars.size() - start;

        features.valid = true;
        features.bar_count = closed_bars.size();
        features.last_close = last.close;
        features.return_1 = last.open != 0.0 ? (last.close - last.open) / last.open : 0.0;
        const double first_close = closed_bars[start].close;
        features.return_n = first_close != 0.0 ? (last.close / first_close) - 1.0 : 0.0;
        features.true_range =
            std::max(last.high - last.low, std::max(last.high - prev_close, prev_close - last.low));
        features.atr = count > 0 ? true_range_sum / static_cast<double>(count) : 0.0;
        features.highest_high = highest;
        features.lowest_low = lowest;
        features.based_on = BarIdentity(std::string{}, timeframe, last.close_time);
        return features;
    }

    // Convenience overload that attaches provenance from the timeframe state
    // store (RT-0005): the features are based on the store's last processed bar.
    FeatureSet compute(const std::vector<MarketBar>& closed_bars, Timeframe timeframe,
                       const TimeframeStateStore& store) const {
        FeatureSet features = compute(closed_bars, timeframe);
        const TimeframeProgress* progress = store.last_processed(timeframe);
        if (progress != nullptr) features.based_on = progress->last_processed;
        return features;
    }

    std::size_t window() const noexcept { return window_; }

private:
    std::size_t window_;
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_FEATUREENGINE_H
