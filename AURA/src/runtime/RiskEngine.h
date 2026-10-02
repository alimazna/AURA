#ifndef AURA_RUNTIME_RISKENGINE_H
#define AURA_RUNTIME_RISKENGINE_H

#include "foundation/EntityId.h"
#include "runtime/ConfidenceEngine.h"
#include "runtime/MarketQualityEngine.h"
#include "runtime/SignalEngine.h"

namespace aura {
namespace runtime {

// Deterministic risk proposal (V3-05: risk before execution).
//
// RT-0015 / Phase 1 deterministic runtime. A proposal is explicitly NOT an order:
// `is_order` is always false and this type carries no broker/order fields. It
// records the deterministic sizing inputs (equity, risk fraction, ATR, stop
// distance) and the derived position size so the proposal is auditable and
// reproducible. No execution, no live order, no I/O.
struct RiskProposal {
    foundation::EntityId proposal_id{};
    foundation::HashDigest decision_id{};
    SignalDirection direction{SignalDirection::NONE};
    double account_equity{0.0};
    double risk_fraction{0.0};
    double atr{0.0};
    double stop_atr_multiple{0.0};
    double stop_distance{0.0};
    double position_size{0.0};
    bool is_order{false};  // always false: a proposal is not an order
    bool valid{false};
};

// Produces deterministic risk proposals.
//
// RT-0015 / Phase 1 deterministic runtime. Runs only after score/confidence
// (RT-0012) and market quality (RT-0014): a proposal is produced only when the
// signal is valid, confidence is valid, market quality is usable, and the sizing
// inputs are positive and coherent. Otherwise no proposal is produced (valid
// false) rather than a fabricated or zero-risk one. Sizing is a pure function of
// the supplied inputs; same inputs always yield the same proposal. The engine
// places no order and performs no I/O.
class RiskEngine {
public:
    RiskEngine(double account_equity, double risk_fraction)
        : account_equity_(account_equity), risk_fraction_(risk_fraction) {}

    RiskProposal propose(const Signal& signal, const ConfidenceValue& confidence,
                         const MarketQualityVerdict& market_quality, double atr,
                         double stop_atr_multiple = 2.0) const {
        RiskProposal proposal;
        proposal.decision_id = signal.decision_id;
        proposal.direction = signal.direction;
        proposal.account_equity = account_equity_;
        proposal.risk_fraction = risk_fraction_;
        proposal.atr = atr;
        proposal.stop_atr_multiple = stop_atr_multiple;

        if (!signal.valid || !confidence.valid) return proposal;
        if (!market_quality.usable) return proposal;  // quality gates risk (V3-25)
        if (!(account_equity_ > 0.0) || !(risk_fraction_ > 0.0) || risk_fraction_ > 1.0) return proposal;
        if (!(atr > 0.0) || !(stop_atr_multiple > 0.0)) return proposal;

        proposal.stop_distance = atr * stop_atr_multiple;
        if (!(proposal.stop_distance > 0.0)) return proposal;
        proposal.position_size = (account_equity_ * risk_fraction_) / proposal.stop_distance;
        if (!(proposal.position_size > 0.0)) return proposal;

        proposal.proposal_id = foundation::EntityId(
            std::string("risk|") + signal.decision_id.to_hex() + "|" +
            std::to_string(proposal.position_size));
        proposal.valid = true;
        return proposal;
    }

    double account_equity() const noexcept { return account_equity_; }
    double risk_fraction() const noexcept { return risk_fraction_; }

private:
    double account_equity_;
    double risk_fraction_;
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_RISKENGINE_H
