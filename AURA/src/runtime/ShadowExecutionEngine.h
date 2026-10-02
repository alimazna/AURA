#ifndef AURA_RUNTIME_SHADOWEXECUTIONENGINE_H
#define AURA_RUNTIME_SHADOWEXECUTIONENGINE_H

#include "foundation/EntityId.h"
#include "foundation/Timestamp.h"
#include "runtime/AdapterManager.h"
#include "runtime/RiskEngine.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace aura {
namespace runtime {

// Cost model for simulated shadow execution (V3-32, V3-33).
//
// RT-0016 / Phase 1 deterministic runtime. Models assumed slippage, commission
// and swap. These are explicit assumptions, not measured broker reality; the
// reconciliation engine (RT-0018) is what distinguishes assumed from observed
// behaviour. Values are supplied, never read from a broker.
struct ExecutionCostModel {
    double slippage_per_unit{0.0};
    double commission_per_unit{0.0};
    double swap_per_unit_per_day{0.0};
};

// A simulated fill. This is a shadow fill only: `is_live` is always false and no
// broker order exists.
struct SimulatedFill {
    foundation::EntityId fill_id{};
    foundation::HashDigest decision_id{};
    SignalDirection direction{SignalDirection::NONE};
    double requested_size{0.0};
    double filled_size{0.0};
    double reference_price{0.0};
    double fill_price{0.0};
    double slippage{0.0};
    double commission{0.0};
    bool partial{false};
    bool is_live{false};  // always false: shadow execution never places live orders
    foundation::Timestamp filled_at{};
    bool valid{false};
};

// Models the shadow execution lifecycle.
//
// RT-0016 / Phase 1 deterministic runtime. Given a risk proposal and an explicit
// cost model, produces a deterministic simulated fill including slippage,
// commission and partial-fill behaviour. It never places a live order, never
// contacts a broker and never reads a clock. Same inputs always yield the same
// fill. A shadow fill is research evidence, not proof of live profitability or
// execution equivalence (V3-32).
class ShadowExecutionEngine {
public:
    ShadowExecutionEngine(ExecutionCostModel cost_model, double partial_fill_threshold = 1.0)
        : cost_model_(cost_model), partial_fill_threshold_(partial_fill_threshold) {}

    SimulatedFill simulate(const RiskProposal& proposal, double reference_price,
                           foundation::Timestamp filled_at) const {
        SimulatedFill fill;
        fill.decision_id = proposal.decision_id;
        fill.direction = proposal.direction;
        fill.reference_price = reference_price;
        fill.filled_at = filled_at;

        if (!proposal.valid || !(reference_price > 0.0)) return fill;
        if (!(cost_model_.slippage_per_unit >= 0.0) || !(cost_model_.commission_per_unit >= 0.0)) {
            return fill;
        }

        fill.requested_size = proposal.position_size;

        // Partial-fill behaviour: a deterministic fraction of the requested size
        // fills when the simulated liquidity threshold is below full size.
        double filled = proposal.position_size;
        if (partial_fill_threshold_ > 0.0 && proposal.position_size > partial_fill_threshold_) {
            filled = partial_fill_threshold_;
            fill.partial = true;
        }
        fill.filled_size = filled;

        const double direction_sign =
            proposal.direction == SignalDirection::SHORT ? -1.0 : 1.0;
        fill.slippage = direction_sign * cost_model_.slippage_per_unit * filled;
        fill.fill_price = reference_price + (filled > 0.0 ? fill.slippage / filled : 0.0);
        fill.commission = cost_model_.commission_per_unit * filled;

        fill.fill_id = foundation::EntityId(std::string("fill|") + proposal.decision_id.to_hex() + "|" +
                                            std::to_string(filled) + "|" +
                                            std::to_string(filled_at.nanoseconds()));
        fill.valid = true;
        return fill;
    }

    // Swap accrued over a holding duration, deterministic and non-negative.
    double swap(double size, std::int64_t holding_ns) const {
        if (!(size > 0.0) || holding_ns <= 0) return 0.0;
        const double days = static_cast<double>(holding_ns) / (86400.0 * 1e9);
        return cost_model_.swap_per_unit_per_day * size * days;
    }

    const ExecutionCostModel& cost_model() const noexcept { return cost_model_; }

private:
    ExecutionCostModel cost_model_{};
    double partial_fill_threshold_;
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_SHADOWEXECUTIONENGINE_H
