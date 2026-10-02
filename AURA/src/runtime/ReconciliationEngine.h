#ifndef AURA_RUNTIME_RECONCILIATIONENGINE_H
#define AURA_RUNTIME_RECONCILIATIONENGINE_H

#include "foundation/EntityId.h"
#include "runtime/ShadowExecutionEngine.h"

#include <cmath>
#include <cstdint>
#include <string_view>

namespace aura {
namespace runtime {

// Reconciliation outcome (V3-33: reconciliation before trust).
//
// RT-0018 / Phase 1 deterministic runtime. Distinguishes assumed shadow
// behaviour from observed broker behaviour. A mismatch is never silently
// accepted; it is surfaced so the model can be corrected before it is trusted.
enum class ReconciliationOutcome : std::uint8_t {
    UNKNOWN = 0,
    MATCHED,
    DIVERGED,
};

constexpr std::string_view to_string(ReconciliationOutcome outcome) noexcept {
    switch (outcome) {
        case ReconciliationOutcome::UNKNOWN:  return "UNKNOWN";
        case ReconciliationOutcome::MATCHED:  return "MATCHED";
        case ReconciliationOutcome::DIVERGED: return "DIVERGED";
    }
    return "UNKNOWN";
}

// Reconciliation verdict for one shadow fill against observed behaviour.
struct ReconciliationResult {
    foundation::EntityId fill_id{};
    ReconciliationOutcome outcome{ReconciliationOutcome::UNKNOWN};
    double assumed_price{0.0};
    double observed_price{0.0};
    double price_divergence{0.0};
    bool trustworthy{false};
    bool valid{false};
};

// Reconciles assumed shadow execution against observed broker reality.
//
// RT-0018 / Phase 1 deterministic runtime. Compares the assumed fill price and
// costs (RT-0016) against observed values and classifies the result. Only an
// in-tolerance match is trustworthy; a divergence is surfaced rather than
// assumed away, so reconciliation precedes trust (V3-05, V3-33). Deterministic
// and pure; it performs no I/O and contacts no broker.
class ReconciliationEngine {
public:
    ReconciliationEngine(double price_tolerance) : price_tolerance_(price_tolerance) {}

    ReconciliationResult reconcile(const SimulatedFill& fill, double observed_price) const {
        ReconciliationResult result;
        result.fill_id = fill.fill_id;
        result.assumed_price = fill.fill_price;
        result.observed_price = observed_price;
        if (!fill.valid) return result;

        result.valid = true;
        result.price_divergence = std::abs(observed_price - fill.fill_price);
        if (price_tolerance_ >= 0.0 && result.price_divergence <= price_tolerance_) {
            result.outcome = ReconciliationOutcome::MATCHED;
            result.trustworthy = true;
        } else {
            result.outcome = ReconciliationOutcome::DIVERGED;
            result.trustworthy = false;
        }
        return result;
    }

    double price_tolerance() const noexcept { return price_tolerance_; }

private:
    double price_tolerance_;
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_RECONCILIATIONENGINE_H
