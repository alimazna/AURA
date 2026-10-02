#ifndef AURA_RUNTIME_DATAVALIDATOR_H
#define AURA_RUNTIME_DATAVALIDATOR_H

#include "foundation/DataQualityState.h"
#include "runtime/DataBus.h"

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace aura {
namespace runtime {

// Validation verdict for one bar. `quality` is the canonical data-quality state;
// `reasons` names every detected defect so the verdict is attributable rather
// than a bare boolean.
struct ValidationResult {
    Timeframe timeframe{Timeframe::UNKNOWN};
    foundation::DataQualityState quality{foundation::DataQualityState::UNKNOWN};
    std::vector<std::string> reasons{};

    bool usable() const noexcept { return foundation::is_usable(quality); }
};

// Assigns canonical data-quality state to incoming bars.
//
// RT-0003 / Phase 1 deterministic runtime. Deterministic, side-effect-free
// structural validation: a bar is VALID only when every check passes. Any defect
// degrades the state (e.g. OHLC inconsistency -> INVALID, non-finite/negative
// volume -> DEGRADED, open==close with zero range -> DEGRADED, negative duration
// -> OUT_OF_ORDER), so quality propagates into eligibility/gating rather than
// being logged and ignored (V3-25). A bar that cannot be assessed is UNKNOWN, and
// UNKNOWN is never usable. The validator fabricates no data.
class DataValidator {
public:
    DataValidator() = default;

    ValidationResult validate(const MarketBar& bar) const {
        ValidationResult result;
        result.timeframe = bar.timeframe;

        if (bar.timeframe == Timeframe::UNKNOWN) {
            result.quality = foundation::DataQualityState::UNKNOWN;
            result.reasons.push_back("unknown timeframe");
            return result;
        }
        if (!bar.closed) {
            result.quality = foundation::DataQualityState::INCOMPLETE;
            result.reasons.push_back("bar not closed");
            return result;
        }
        if (!(std::isfinite(bar.open) && std::isfinite(bar.high) && std::isfinite(bar.low) &&
              std::isfinite(bar.close) && std::isfinite(bar.volume))) {
            result.quality = foundation::DataQualityState::INVALID;
            result.reasons.push_back("non-finite OHLCV");
            return result;
        }
        if (bar.close_time < bar.open_time) {
            result.quality = foundation::DataQualityState::OUT_OF_ORDER;
            result.reasons.push_back("close_time precedes open_time");
            return result;
        }
        if (bar.volume < 0.0) {
            result.quality = foundation::DataQualityState::DEGRADED;
            result.reasons.push_back("negative volume");
        }

        bool ohlc_ok = true;
        if (bar.high < bar.low) {
            ohlc_ok = false;
            result.reasons.push_back("high below low");
        }
        if (bar.high < bar.open || bar.high < bar.close) {
            ohlc_ok = false;
            result.reasons.push_back("high below open/close");
        }
        if (bar.low > bar.open || bar.low > bar.close) {
            ohlc_ok = false;
            result.reasons.push_back("low above open/close");
        }
        if (!ohlc_ok) {
            result.quality = foundation::DataQualityState::INVALID;
            return result;
        }

        if (bar.high == bar.low) {
            // Zero-range bar: structurally valid but low-information.
            result.quality = foundation::DataQualityState::DEGRADED;
            result.reasons.push_back("zero price range");
            return result;
        }

        if (result.reasons.empty()) {
            result.quality = foundation::DataQualityState::VALID;
        }
        return result;
    }

    ValidationResult validate(const MarketEvent& event) const { return validate(event.bar); }

    // Convenience predicate; only VALID is usable.
    bool usable(const MarketBar& bar) const { return validate(bar).usable(); }
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_DATAVALIDATOR_H
