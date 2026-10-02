#ifndef AURA_RUNTIME_POSITIONSIMULATOR_H
#define AURA_RUNTIME_POSITIONSIMULATOR_H

#include "foundation/EntityId.h"
#include "foundation/Timestamp.h"
#include "runtime/SignalEngine.h"
#include "runtime/ShadowExecutionEngine.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace aura {
namespace runtime {

// Simulated position lifecycle state. A position is a shadow construct; it never
// represents a live broker position.
enum class PositionState : std::uint8_t {
    UNKNOWN = 0,
    OPEN,
    PARTIALLY_CLOSED,
    CLOSED,
};

constexpr std::string_view to_string(PositionState state) noexcept {
    switch (state) {
        case PositionState::UNKNOWN:          return "UNKNOWN";
        case PositionState::OPEN:             return "OPEN";
        case PositionState::PARTIALLY_CLOSED: return "PARTIALLY_CLOSED";
        case PositionState::CLOSED:           return "CLOSED";
    }
    return "UNKNOWN";
}

// Immutable simulated position snapshot.
struct SimulatedPosition {
    foundation::EntityId position_id{};
    foundation::HashDigest decision_id{};
    SignalDirection direction{SignalDirection::NONE};
    double size{0.0};
    double open_price{0.0};
    double closed_size{0.0};
    double realised_pnl{0.0};
    double costs{0.0};
    PositionState state{PositionState::UNKNOWN};
    foundation::Timestamp opened_at{};
    foundation::Timestamp closed_at{};
    bool is_live{false};  // always false: shadow position, not a broker position
    bool valid{false};
};

// A deterministic lifecycle event (open or close) for auditing.
struct LifecycleEvent {
    foundation::EntityId position_id{};
    PositionState state{PositionState::UNKNOWN};
    double size{0.0};
    double price{0.0};
    foundation::Timestamp at{};
};

// Simulates a position lifecycle deterministically.
//
// RT-0017 / Phase 1 deterministic runtime. Opens from a simulated fill and
// supports partial and full closes. All transitions are deterministic pure
// functions of the supplied prices and timestamps; no clock, no randomness, no
// I/O, no live execution. Partial-fill behaviour from the shadow engine (RT-0016)
// is preserved: a partial fill opens a proportionally smaller position. Every
// transition is recorded as an auditable lifecycle event.
class PositionSimulator {
public:
    PositionSimulator() = default;

    SimulatedPosition open(const SimulatedFill& fill, foundation::Timestamp opened_at) {
        SimulatedPosition position;
        position.decision_id = fill.decision_id;
        position.direction = fill.direction;
        position.opened_at = opened_at;
        if (!fill.valid || !(fill.filled_size > 0.0)) return position;

        position.position_id = foundation::EntityId(std::string("pos|") + std::string(fill.fill_id.view()));
        position.size = fill.filled_size;
        position.open_price = fill.fill_price;
        position.costs = fill.commission + std::abs(fill.slippage);
        position.state = PositionState::OPEN;
        position.valid = true;

        events_.push_back(LifecycleEvent{position.position_id, PositionState::OPEN, position.size,
                                         position.open_price, opened_at});
        return position;
    }

    // Closes `close_size` units of a position at `price`. Returns the updated
    // position; the original snapshot is unchanged. A close of the full remaining
    // size transitions to CLOSED; a smaller close to PARTIALLY_CLOSED.
    SimulatedPosition close(const SimulatedPosition& position, double close_size, double price,
                            foundation::Timestamp closed_at, double extra_costs = 0.0) {
        SimulatedPosition updated = position;
        if (!position.valid || position.state == PositionState::CLOSED) return updated;
        if (!(close_size > 0.0) || close_size > position.size || !(price > 0.0)) return updated;

        const double sign = position.direction == SignalDirection::SHORT ? -1.0 : 1.0;
        const double gross = (price - position.open_price) * close_size * sign;
        updated.realised_pnl = position.realised_pnl + gross;
        updated.costs = position.costs + extra_costs;
        updated.closed_size = position.closed_size + close_size;
        updated.size = position.size - close_size;
        updated.closed_at = closed_at;
        updated.state = updated.size > 0.0 ? PositionState::PARTIALLY_CLOSED : PositionState::CLOSED;

        events_.push_back(LifecycleEvent{position.position_id, updated.state, close_size, price,
                                         closed_at});
        return updated;
    }

    // Net realised P&L after modelled costs.
    static double net_pnl(const SimulatedPosition& position) noexcept {
        return position.realised_pnl - position.costs;
    }

    const std::vector<LifecycleEvent>& events() const noexcept { return events_; }
    void clear_events() { events_.clear(); }

private:
    std::vector<LifecycleEvent> events_{};
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_POSITIONSIMULATOR_H
