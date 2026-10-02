#ifndef AURA_OBSERVATION_OUTCOMEENGINE_H
#define AURA_OBSERVATION_OUTCOMEENGINE_H

#include "foundation/Timestamp.h"
#include "observation/PredictionLedger.h"
#include "runtime/BarFinalizer.h"
#include "runtime/DataBus.h"
#include "runtime/PositionSimulator.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace aura {
namespace observation {

// Resolution state of a prediction.
//
// OB-0002 / Phase 2 observation foundation. UNKNOWN means the outcome could not
// be resolved and must not be treated as a success or a loss.
enum class OutcomeState : std::uint8_t {
    UNKNOWN = 0,
    OPEN,
    TARGET_HIT,
    STOP_HIT,
    EXPIRED,
};

constexpr std::string_view to_string(OutcomeState state) noexcept {
    switch (state) {
        case OutcomeState::UNKNOWN:    return "UNKNOWN";
        case OutcomeState::OPEN:       return "OPEN";
        case OutcomeState::TARGET_HIT: return "TARGET_HIT";
        case OutcomeState::STOP_HIT:   return "STOP_HIT";
        case OutcomeState::EXPIRED:    return "EXPIRED";
    }
    return "UNKNOWN";
}

// Resolved outcome of a prediction.
struct Outcome {
    foundation::EntityId prediction_id{};
    foundation::HashDigest decision_id{};
    OutcomeState state{OutcomeState::UNKNOWN};
    double entry_price{0.0};
    double exit_price{0.0};
    double realised_pnl{0.0};
    double r_multiple{0.0};  // realised P&L expressed in units of initial risk
    foundation::Timestamp resolved_at{};
    bool valid{false};
};

// Resolves predictions into outcomes deterministically.
//
// OB-0002 / Phase 2 observation foundation. Only bars strictly later than the
// prediction's based-on closed bar are considered, so no future information
// before the decision enters the outcome (no lookahead). Resolution never
// repaints: once an outcome is resolved it is final. When a single bar contains
// both the stop and the target, the stop is assumed to trigger first, so the
// outcome is never optimistically biased. The simulated position lifecycle
// (RT-0017) is used to compute realised P&L; this is shadow analysis only and
// performs no live execution. Deterministic and pure.
class OutcomeEngine {
public:
    OutcomeEngine() = default;

    Outcome resolve(const Prediction& prediction, const std::vector<runtime::MarketBar>& subsequent_bars,
                    foundation::Timestamp resolved_at) const {
        Outcome outcome;
        outcome.prediction_id = prediction.prediction_id;
        outcome.decision_id = prediction.decision_id;
        outcome.entry_price = prediction.reference_price;
        if (!prediction.valid) return outcome;

        const double risk = std::abs(prediction.reference_price - prediction.stop_price);
        const bool long_side = prediction.direction == runtime::SignalDirection::LONG;
        if (prediction.direction == runtime::SignalDirection::NONE) return outcome;

        outcome.valid = true;
        outcome.state = OutcomeState::OPEN;

        for (const runtime::MarketBar& bar : subsequent_bars) {
            // Only closed bars strictly after the prediction bar may resolve it.
            if (!bar.closed) continue;
            if (bar.timeframe != prediction.timeframe) continue;
            if (bar.close_time <= prediction.based_on.close_time()) continue;

            const bool stop_hit = long_side ? bar.low <= prediction.stop_price
                                            : bar.high >= prediction.stop_price;
            const bool target_hit = long_side ? bar.high >= prediction.target_price
                                              : bar.low <= prediction.target_price;

            // Conservative: the stop is assumed to trigger before the target.
            if (stop_hit) {
                outcome.state = OutcomeState::STOP_HIT;
                outcome.exit_price = prediction.stop_price;
                outcome.resolved_at = bar.close_time;
                return finalise(outcome, prediction, risk);
            }
            if (target_hit) {
                outcome.state = OutcomeState::TARGET_HIT;
                outcome.exit_price = prediction.target_price;
                outcome.resolved_at = bar.close_time;
                return finalise(outcome, prediction, risk);
            }
        }

        // No trigger within the supplied horizon: the prediction expired
        // unresolved rather than being marked a win or a loss.
        if (!subsequent_bars.empty()) {
            outcome.state = OutcomeState::EXPIRED;
            outcome.exit_price = subsequent_bars.back().close;
            outcome.resolved_at = resolved_at;
            return finalise(outcome, prediction, risk);
        }
        return outcome;
    }

private:
    static Outcome finalise(Outcome outcome, const Prediction& prediction, double risk) {
        runtime::PositionSimulator positions;
        runtime::SimulatedFill fill;
        fill.decision_id = prediction.decision_id;
        fill.direction = prediction.direction;
        fill.requested_size = 1.0;
        fill.filled_size = 1.0;
        fill.reference_price = prediction.reference_price;
        fill.fill_price = prediction.reference_price;
        fill.filled_at = prediction.predicted_at;
        fill.fill_id = foundation::EntityId(std::string("outcome|") + prediction.prediction_id.value());
        fill.valid = true;

        const runtime::SimulatedPosition position = positions.open(fill, prediction.predicted_at);
        const runtime::SimulatedPosition closed =
            positions.close(position, position.size, outcome.exit_price, outcome.resolved_at);
        outcome.realised_pnl = runtime::PositionSimulator::net_pnl(closed);
        outcome.r_multiple = risk > 0.0 ? outcome.realised_pnl / risk : 0.0;
        return outcome;
    }
};

}  // namespace observation
}  // namespace aura

#endif  // AURA_OBSERVATION_OUTCOMEENGINE_H
