#ifndef AURA_RUNTIME_APPLICATIONPIPELINE_H
#define AURA_RUNTIME_APPLICATIONPIPELINE_H

#include "foundation/DataQualityState.h"
#include "foundation/SchemaVersion.h"
#include "foundation/ServiceState.h"
#include "foundation/Timestamp.h"
#include "foundation/Version.h"
#include "mt5/Mt5StreamManager.h"
#include "mt5/ProtocolCodec.h"
#include "observation/SystemHealthMonitor.h"
#include "runtime/AdapterManager.h"
#include "runtime/ConfidenceEngine.h"
#include "runtime/EligibilityEngine.h"
#include "runtime/FeatureEngine.h"
#include "runtime/MarketQualityEngine.h"
#include "runtime/PositionSimulator.h"
#include "runtime/RegimeEngine.h"
#include "runtime/RiskEngine.h"
#include "runtime/ScoreEngine.h"
#include "runtime/ShadowExecutionEngine.h"
#include "runtime/ShadowLedger.h"
#include "runtime/SignalEngine.h"
#include "runtime/StructureEngine.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace aura {
namespace runtime {

// Read-only per-stream view for operator projection.
struct StreamView {
    Timeframe timeframe{Timeframe::UNKNOWN};
    bool connected{false};
    bool healthy{false};
    foundation::DataQualityState quality{foundation::DataQualityState::UNKNOWN};
    foundation::Timestamp last_event{};
    std::uint64_t accepted{0};
    std::uint64_t rejected{0};
    std::string last_error{};
};

// Deterministic aggregate application status.
struct EngineStatus {
    bool transport_connected{false};
    std::size_t streams_total{0};
    std::size_t streams_healthy{0};
    std::uint64_t accepted{0};
    std::uint64_t rejected{0};
    std::uint64_t malformed{0};
    std::uint64_t signals{0};
    std::uint64_t proposals{0};
    std::uint64_t fills{0};
    std::uint64_t positions{0};
    bool healthy{false};
    foundation::ServiceState aggregate{foundation::ServiceState::STARTING};
    bool has_open_position{false};
};

// The AURA end-to-end application pipeline.
//
// APP-0001. This is the integration point the audit identified as missing: it
// wires one real MT5 wire frame (the same canonical frame the MQL5 EA emits and
// ProtocolCodec encodes) through the whole approved runtime:
//
//   frame -> codec -> one-EA/nine-stream receiver -> adapter boundary
//         -> H4 structure / regime -> M15 features -> eligibility -> signal
//         -> score -> confidence -> market quality -> risk -> shadow fill
//         -> simulated position -> append-only shadow ledger -> health
//
// It preserves every invariant of the underlying engines: closed bars only (no
// lookahead), explicit-timeframe routing (no positional multiplexing), per-stream
// isolation, deterministic identity, append-only/idempotent ledger, and a strictly
// shadow-only execution path (no live order is ever placed). It performs no I/O
// and reads no clock; the transport and the process loop are supplied by the
// caller. It fabricates nothing: a state that is not established stays UNKNOWN.
class ApplicationPipeline {
public:
    struct Config {
        std::string symbol{"XAUUSD"};
        StrategyFamily family{StrategyFamily::S03_SOVEREIGN_BREAKOUT};
        double account_equity{100000.0};
        double risk_fraction{0.01};
        double stop_atr_multiple{2.0};
        ExecutionCostModel cost_model{0.1, 1.0, 0.0};
        double partial_fill_threshold{2.0};
        std::size_t bar_window{32};
        std::string strategy_version{"1.0.0"};
        std::string configuration_version{"1.0.0"};
    };

    ApplicationPipeline() : ApplicationPipeline(Config{}) {}

    explicit ApplicationPipeline(Config config)
        : config_(std::move(config)),
          signal_engine_(config_.family,
                         foundation::Version::from_string(config_.strategy_version),
                         foundation::Version::from_string(config_.configuration_version)),
          risk_engine_(config_.account_equity, config_.risk_fraction),
          shadow_(config_.cost_model, config_.partial_fill_threshold) {}

    // Ingests one raw wire frame. Returns the receiver's per-stream result. A
    // non-accepted frame is counted and leaves derived state untouched; it is
    // never fabricated into a bar.
    mt5::ReceiveResult on_frame(std::string_view frame) {
        const mt5::DecodedFrame decoded = mt5::ProtocolCodec::decode(frame);
        const mt5::ReceiveResult result = stream_manager_.accept(decoded);

        if (decoded.status != mt5::DecodeStatus::OK) {
            ++status_.malformed;
            ++status_.rejected;
            return result;
        }
        if (!result.accepted()) {
            ++status_.rejected;
            return result;
        }

        // Authoritative accepted closed bar: retain it, reflect stream health and
        // record the observation time.
        ++status_.accepted;
        retain_bar(decoded.bar);
        status_.transport_connected = true;
        last_observation_ = decoded.receive_time;

        adapters_.report_bar(decoded.timeframe, decoded.event_time, decoded.sequence);

        if (decoded.timeframe == Timeframe::M15) analyse_operational_bar(decoded.bar);
        refresh_status();
        return result;
    }

    // Deterministic, read-only aggregate status.
    const EngineStatus& status() { refresh_status(); return status_; }

    std::vector<StreamView> streams() const {
        std::vector<StreamView> views;
        for (const auto& entry : stream_manager_.streams()) {
            const mt5::ReceiverStreamState& s = entry.second;
            StreamView v;
            v.timeframe = s.timeframe;
            v.connected = s.connected;
            v.healthy = s.healthy();
            v.quality = s.quality;
            v.last_event = s.last_event_time;
            v.accepted = s.accepted_bars;
            v.rejected = s.rejected;
            v.last_error = s.last_error;
            views.push_back(std::move(v));
        }
        return views;
    }

    const ShadowLedger& ledger() const noexcept { return ledger_; }
    const TimeframeStateStore& state_store() const noexcept { return stream_manager_.store(); }
    const AdapterManager& adapters() const noexcept { return adapters_; }
    const Config& config() const noexcept { return config_; }

    // The most recent accepted closed bar for a timeframe, or nullptr.
    const MarketBar* last_bar(Timeframe timeframe) const {
        const auto it = bars_.find(timeframe);
        if (it == bars_.end() || it->second.empty()) return nullptr;
        return &it->second.back();
    }

    // Most recent generated signal (invalid when none).
    const Signal& last_signal() const noexcept { return last_signal_; }
    // Most recent risk proposal (invalid when none).
    const RiskProposal& last_proposal() const noexcept { return last_proposal_; }

private:
    void retain_bar(const MarketBar& bar) {
        std::vector<MarketBar>& v = bars_[bar.timeframe];
        v.push_back(bar);
        if (v.size() > config_.bar_window) v.erase(v.begin(), v.begin() + (v.size() - config_.bar_window));
    }

    // Runs the full analysis chain after an authoritative M15 closed bar.
    void analyse_operational_bar(const MarketBar& m15_bar) {
        // Close any open shadow position at this bar's close (deterministic).
        if (open_position_.valid && open_position_.state != PositionState::CLOSED) {
            const SimulatedPosition closed =
                positions_.close(open_position_, open_position_.size, m15_bar.close,
                                 m15_bar.close_time, 0.0);
            if (closed.valid) ledger_.record_position(closed, m15_bar.close_time);
            open_position_ = closed;
            status_.has_open_position = open_position_.state != PositionState::CLOSED;
        }

        // H4 structural authority -> regime.
        const auto h4_it = bars_.find(Timeframe::H4);
        if (h4_it == bars_.end() || h4_it->second.size() < 2) return;
        const FeatureSet h4_features = features_.compute(h4_it->second, Timeframe::H4);
        const StructureContext h4_structure = structure_engine_.compute(h4_features);
        const RegimeVerdict regime = regime_engine_.classify(h4_structure, h4_features);

        // M15 operational features.
        const auto m15_it = bars_.find(Timeframe::M15);
        if (m15_it == bars_.end()) return;
        const FeatureSet m15_features = features_.compute(m15_it->second, Timeframe::M15);

        // Data quality of the operational stream as observed by the receiver.
        foundation::DataQualityState quality = foundation::DataQualityState::UNKNOWN;
        const auto* m15_state = stream_manager_.stream(Timeframe::M15);
        if (m15_state != nullptr) quality = m15_state->quality;

        const EligibilityVerdict verdict = eligibility_.evaluate(m15_features, regime, quality);
        const Signal signal = signal_engine_.generate(verdict, regime, stream_manager_.store(),
                                                      config_.symbol);
        last_signal_ = signal;
        if (!signal.valid) return;
        ++status_.signals;
        ledger_.record_signal(signal, m15_bar.close_time);

        const SignalScore score = scorer_.score_with_direction(signal, regime, m15_features);
        const ConfidenceValue confidence = confidence_.evaluate(score, quality);
        const MarketQualityVerdict market_quality = market_quality_.evaluate(adapters_);

        const RiskProposal proposal = risk_engine_.propose(signal, confidence, market_quality,
                                                           m15_features.atr,
                                                           config_.stop_atr_multiple);
        last_proposal_ = proposal;
        if (!proposal.valid) return;
        ++status_.proposals;

        const SimulatedFill fill = shadow_.simulate(proposal, m15_bar.close, m15_bar.close_time);
        if (!fill.valid) return;
        ++status_.fills;
        ledger_.record_fill(fill, m15_bar.close_time);

        const SimulatedPosition opened = positions_.open(fill, m15_bar.close_time);
        if (!opened.valid) return;
        ++status_.positions;
        ledger_.record_position(opened, m15_bar.close_time);
        open_position_ = opened;
        status_.has_open_position = true;
    }

    void refresh_status() {
        status_.streams_total = stream_manager_.stream_count();
        status_.streams_healthy = stream_manager_.healthy_count();
        const observation::HealthReport report = health_monitor_.observe(adapters_, last_observation_);
        status_.healthy = report.healthy;
        status_.aggregate = report.aggregate;
    }

    Config config_{};
    FeatureEngine features_{};
    StructureEngine structure_engine_{};
    RegimeEngine regime_engine_{};
    EligibilityEngine eligibility_{};
    ScoreEngine scorer_{};
    ConfidenceEngine confidence_{};
    MarketQualityEngine market_quality_{};
    SignalEngine signal_engine_;
    RiskEngine risk_engine_;
    ShadowExecutionEngine shadow_;
    AdapterManager adapters_{};
    mt5::Mt5StreamManager stream_manager_{};
    observation::SystemHealthMonitor health_monitor_{};
    PositionSimulator positions_{};
    ShadowLedger ledger_{};

    std::map<Timeframe, std::vector<MarketBar>> bars_{};
    SimulatedPosition open_position_{};
    Signal last_signal_{};
    RiskProposal last_proposal_{};
    foundation::Timestamp last_observation_{};
    EngineStatus status_{};
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_APPLICATIONPIPELINE_H
