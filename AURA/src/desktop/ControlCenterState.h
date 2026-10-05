#ifndef AURA_DESKTOP_CONTROLCENTERSTATE_H
#define AURA_DESKTOP_CONTROLCENTERSTATE_H

#include "desktop/BiquoteCandles.h"
#include "desktop/CandleChart.h"
#include "desktop/ControlCenterPanels.h"
#include "desktop/DesktopModel.h"
#include "foundation/PersistenceStatus.h"
#include "mt5/ProtocolCodec.h"
#include "observation/FailureDetectionEngine.h"
#include "runtime/ApplicationRecovery.h"
#include "runtime/ApplicationShell.h"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace aura {
namespace desktop {

// Deterministic canonical frame set (one closed bar on each of the nine streams
// per step), identical to the console host's built-in set. Lets the desktop path
// be exercised end-to-end (nine streams, signals, ledger) without a socket.
inline std::vector<std::string> builtin_frames(int steps) {
    std::vector<std::string> out;
    const auto tfs = runtime::all_timeframes();
    for (int i = 0; i < steps; ++i) {
        for (std::size_t k = 0; k < tfs.size(); ++k) {
            const std::int64_t step = 60 * static_cast<std::int64_t>(k + 1);
            const std::int64_t close = 1'000'000 + static_cast<std::int64_t>(i) * step;
            runtime::MarketBar b;
            b.timeframe = tfs[k];
            b.open_time = foundation::Timestamp::from_seconds(close - 60);
            b.close_time = foundation::Timestamp::from_seconds(close);
            b.open = 100.0 + i + static_cast<double>(k);
            b.high = b.open + 1.0;
            b.low = b.open - 0.5;
            b.close = b.open + 0.8;
            b.volume = 100.0;
            b.closed = true;
            out.push_back(mt5::ProtocolCodec::encode_closed_bar(
                b, "XAUUSD", static_cast<std::uint64_t>(i + 1),
                foundation::Timestamp::from_seconds(close + 1)));
        }
    }
    return out;
}

// The complete read-only report the GUI renders: the core runtime snapshot plus
// the additional V3-37 section projections. Built by the single runtime owner
// (ControlCenterState); the GUI never reaches into an engine itself.
struct ControlCenterReport {
    ControlCenterSnapshot snapshot{};
    PredictionPanel predictions{};
    ShadowLedgerPanel shadow_ledger{};
    IncidentPanel incidents{};
    KnowledgePanel knowledge{};
    ResearchPanel research{};
    CandidatePanel candidates{};
    ValidationPanel validation{};
    ApprovalPanel approvals{};
    EvolutionPanel evolution{};
    SchedulePanel schedule{};
    CheckpointPanel checkpoints{};
    AuditPanel audit{};
    VersionPanel version{};
    // The selected timeframe's closed-bar series for the XAUUSD candlestick
    // chart, projected from the runtime's real retained closed bars. Empty (and
    // unavailable) when the selected stream has reported nothing.
    CandleSeries chart{};

    // Which source produced `chart`. `true` only when real Biquote candles were
    // loaded from disk; otherwise the existing synthetic/runtime path supplied
    // them. The UI shows this so an operator is never misled about provenance.
    bool using_real_data{false};
    std::string data_source_note{};
};

// Runtime options for the desktop control center. Kept dependency-free so the
// state object can be constructed and exercised without a windowing system.
struct ControlCenterOptions {
    std::string store_path{};   // empty -> in-memory only
    std::string replay_path{};  // canonical frames fed once at start (optional)
    std::uint16_t serve_port{0};  // 0 -> no transport
    bool keep_store{false};
};

// The desktop application's single runtime owner.
//
// This is the human control surface's bridge to the trusted runtime: it owns one
// ApplicationShell (no second runtime, no duplicate process), captures read-only
// snapshots for rendering, and exposes only safe control-plane operations
// (refresh, pause/resume of the local transport loop, checkpoint, staged stop and
// a read-only recovery report). It never places an order, never fabricates state,
// and never auto-resumes corrupted or incompatible state.
class ControlCenterState {
public:
    explicit ControlCenterState(ControlCenterOptions options)
        : options_(std::move(options)),
          shell_(runtime::ApplicationPipeline::Config{}, options_.store_path) {}

    runtime::ApplicationShell& shell() noexcept { return shell_; }
    const runtime::ApplicationShell& shell() const noexcept { return shell_; }

    // Starts the transport if a serve port was requested. Returns false on bind
    // failure. A replay file is fed separately via feed_replay().
    bool start_transport() {
        if (options_.serve_port == 0) return true;
        return shell_.start(options_.serve_port);
    }

    // Feeds a canonical newline-delimited frames file through the pipeline once.
    // Returns the number of frames processed (0 on open failure).
    std::size_t feed_replay(const std::string& path) {
        std::ifstream input(path, std::ios::binary);
        if (!input) return 0;
        std::string line;
        std::size_t processed = 0;
        while (std::getline(input, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;
            shell_.feed(line + "\n");
            ++processed;
        }
        return processed;
    }

    // Feeds the built-in deterministic frame set. Used by the headless self-test
    // so the desktop projection is exercised with real nine-stream state.
    std::size_t feed_builtin(int steps) {
        std::size_t processed = 0;
        for (const std::string& f : builtin_frames(steps)) {
            shell_.feed(f + "\n");
            ++processed;
        }
        return processed;
    }

    std::size_t feed_replay() { return feed_replay(options_.replay_path); }

    // Advances the transport by one bounded accept/read cycle (safe to call from
    // a render loop; never blocks indefinitely). No-op while paused or when no
    // transport was requested.
    std::size_t poll_transport(int timeout_ms = 0) {
        if (paused_ || options_.serve_port == 0) return 0;
        return shell_.serve_once(timeout_ms);
    }

    // Safe control-plane operations. There is deliberately no order/buy/sell
    // action here and none is reachable from the UI.
    void toggle_pause() { paused_ = !paused_; }
    void pause() noexcept { paused_ = true; }
    void resume() noexcept { paused_ = false; }
    bool paused() const noexcept { return paused_; }

    void request_stop() noexcept { shell_.request_stop(); }
    bool stop_requested() const noexcept { return shell_.stop_requested(); }

    // Persists a checkpoint (clean intent). Records the resulting status so the
    // persistence panel shows the real outcome, never a fabricated OK.
    foundation::PersistenceStatus persist_checkpoint() {
        last_persist_ = shell_.persist_state();
        return last_persist_;
    }

    // Evaluates (but never applies) the boot recovery decision. The GUI reports
    // the decision; corrupted/incompatible state is refused and never resumed
    // merely because the window is open.
    const runtime::RecoveryOutcome& evaluate_recovery(bool known_good_available = false) {
        recovery_ = shell_.recover(known_good_available);
        recovery_evaluated_ = true;
        return recovery_;
    }

    // Captures a fresh read-only snapshot for rendering.
    const ControlCenterSnapshot& refresh() {
        snapshot_ = DesktopModel::capture(
            shell_, recovery_, std::string(foundation::to_string(last_persist_)));
        return snapshot_;
    }

    // ---- Additional V3-37 section sources (read-only copies) ----------------
    //
    // These records are produced by the trusted logic layers (Phase 2-8). They are
    // held as copies so the projector is pure and the GUI never reaches into an
    // engine. The control center does not yet run those planes in its own loop, so
    // unless a source is supplied the corresponding section stays NOT AVAILABLE.

    void set_predictions(std::vector<observation::Prediction> predictions) {
        predictions_ = std::move(predictions);
        prediction_source_wired_ = true;
    }
    void set_knowledge(std::vector<learning::KnowledgeObject> knowledge,
                       std::size_t identities) {
        knowledge_ = std::move(knowledge);
        knowledge_identities_ = identities;
    }
    void set_experiments(std::vector<research::Experiment> experiments) {
        experiments_ = std::move(experiments);
    }
    void set_candidates(std::vector<evolution::Candidate> candidates, std::size_t population) {
        candidates_ = std::move(candidates);
        candidate_population_ = population;
    }
    void set_approvals(std::vector<governance::DecisionRecord> approvals) {
        approvals_ = std::move(approvals);
    }
    void set_evolution_nodes(std::vector<evolution::EvolutionNode> nodes) {
        evolution_nodes_ = std::move(nodes);
    }
    void set_checkpoints(std::vector<operatingwindow::Checkpoint> checkpoints) {
        checkpoints_ = std::move(checkpoints);
    }
    void set_audit(std::vector<governance::AuditRecord> records) { audit_ = std::move(records); }

    // Snapshot of the append-only shadow ledger (bounded by the projector).
    std::vector<runtime::LedgerEntry> ledger_entries() {
        return shell_.pipeline().ledger().ordered();
    }

    // Detects incidents (structured failure records) from the live adapter streams
    // using the existing Phase 2 detector. Marks the incident source as wired, so
    // an empty result is a real "no incidents", not "no data source".
    std::vector<foundation::ErrorRecord> detect_incidents() {
        return incidents_.detect_adapter_failures(shell_.pipeline().adapters(),
                                                  shell_.pipeline().last_observation());
    }

    // Builds the full read-only report for the GUI. Pure projection over copies.
    const ControlCenterReport& refresh_report() {
        refresh();
        PanelSources src;
        src.pipeline = &shell_.pipeline();
        src.predictions = predictions_;
        src.prediction_source_wired = prediction_source_wired_;
        // Copying the whole ledger every frame is avoided: only re-snapshot when
        // the ledger has actually grown (the append-only ledger only ever grows).
        const std::size_t ledger_size = shell_.pipeline().ledger().size();
        if (ledger_size != cached_ledger_size_) {
            cached_ledger_ = shell_.pipeline().ledger().ordered();
            cached_ledger_size_ = ledger_size;
        }
        src.ledger = cached_ledger_;
        src.incidents = incidents_.detect_adapter_failures(shell_.pipeline().adapters(),
                                                           shell_.pipeline().last_observation());
        src.incident_source_wired = true;
        src.knowledge = knowledge_;
        src.knowledge_identities = knowledge_identities_;
        src.experiments = experiments_;
        src.candidates = candidates_;
        src.candidate_population = candidate_population_;
        src.approvals = approvals_;
        src.evolution_nodes = evolution_nodes_;
        src.checkpoints = checkpoints_;

        report_.snapshot = snapshot_;
        report_.predictions = ControlCenterPanels::predictions(src);
        report_.shadow_ledger = ControlCenterPanels::shadow_ledger(src);
        report_.incidents = ControlCenterPanels::incidents(src);
        report_.knowledge = ControlCenterPanels::knowledge(src);
        report_.research = ControlCenterPanels::research(src);
        report_.candidates = ControlCenterPanels::candidates(src);
        report_.validation = ControlCenterPanels::validation();
        report_.approvals = ControlCenterPanels::approvals(src);
        report_.evolution = ControlCenterPanels::evolution(src);
        report_.schedule = ControlCenterPanels::schedule();
        report_.checkpoints = ControlCenterPanels::checkpoints(src);
        report_.audit = ControlCenterPanels::audit(src, audit_);
        report_.version = ControlCenterPanels::version(shell_.pipeline());
        // Optional real candles: prefer the bridge when it has them, else the
        // unchanged synthetic/runtime series. Provenance is recorded honestly.
        const biquote::LoadResult attempt = try_real_candles(selection_.selected());
        if (attempt.loaded) {
            report_.chart = attempt.series;
            report_.using_real_data = true;
            report_.data_source_note = "Biquote real candles";
            last_real_data_note_ = attempt.path_tried;
        } else {
            report_.chart = make_candle_series(selection_.selected(),
                                               shell_.pipeline().bar_series(selection_.selected()));
            report_.using_real_data = false;
            report_.data_source_note = "SYNTHETIC (no Biquote)";
            last_real_data_note_ = attempt.reason;
        }
        return report_;
    }

    // The XAUUSD chart timeframe selector (presentation-only). M15 by default.
    TimeframeSelection& timeframe_selection() noexcept { return selection_; }
    const TimeframeSelection& timeframe_selection() const noexcept { return selection_; }

    // Reads the runtime's real retained closed bars for one timeframe (explicit
    // identity, never a row position) and projects them into chart candles. No
    // candle is invented; an empty series means NO CANDLE DATA.
    //
    // Optional real data: when AURA/bridge/biquote/out/candles_<TF>.json exists
    // (or AURA_BRIDGE_DIR points at it) those real closed bars are preferred.
    // The synthetic path below is completely unchanged and still used whenever
    // the bridge is absent, unreadable or empty, so no existing behaviour can
    // regress. Provenance is recorded in `last_load_` for the UI.
    CandleSeries chart_series(runtime::Timeframe tf) const {
        const biquote::LoadResult attempt = try_real_candles(tf);
        if (attempt.loaded) return attempt.series;
        return make_candle_series(tf, shell_.pipeline().bar_series(tf));
    }

    // Attempts the optional real-candle path without falling back. Returns an
    // unavailable result unless the operator has explicitly enabled the bridge,
    // so no existing caller can change behaviour by accident.
    biquote::LoadResult try_real_candles(runtime::Timeframe tf) const {
        if (!bridge_enabled_) return biquote::LoadResult{};
        return biquote::load_candles(tf, exe_dir_);
    }

    // Enables the optional Biquote bridge and records where the executable lives
    // so bridge/biquote/out/*.json can be found. Until this is called, every path
    // behaves exactly as it did before the integration existed.
    void enable_biquote_bridge(std::string exe_dir) {
        exe_dir_ = std::move(exe_dir);
        bridge_enabled_ = true;
    }

    bool biquote_bridge_enabled() const noexcept { return bridge_enabled_; }

    // True when real Biquote candles are available for the operational
    // timeframe. Used for the honest provenance label.
    bool real_data_available() const {
        return biquote::real_data_available(selection_.selected(), exe_dir_);
    }

    const std::string& last_real_data_note() const noexcept { return last_real_data_note_; }

    const ControlCenterReport& report() const noexcept { return report_; }

    const ControlCenterSnapshot& snapshot() const noexcept { return snapshot_; }
    bool recovery_evaluated() const noexcept { return recovery_evaluated_; }
    const ControlCenterOptions& options() const noexcept { return options_; }

private:
    ControlCenterOptions options_{};
    runtime::ApplicationShell shell_;
    // Optional Biquote bridge support. Disabled by default, so with no bridge
    // enabled every code path behaves exactly as it did before this integration.
    std::string exe_dir_{};
    bool bridge_enabled_{false};
    mutable std::string last_real_data_note_{};
    runtime::RecoveryOutcome recovery_{};
    ControlCenterSnapshot snapshot_{};
    ControlCenterReport report_{};
    foundation::PersistenceStatus last_persist_{foundation::PersistenceStatus::UNKNOWN};
    observation::FailureDetectionEngine incidents_{};
    std::vector<runtime::LedgerEntry> cached_ledger_{};
    std::size_t cached_ledger_size_{0};
    std::vector<observation::Prediction> predictions_{};
    bool prediction_source_wired_{false};
    std::vector<learning::KnowledgeObject> knowledge_{};
    std::size_t knowledge_identities_{0};
    std::vector<research::Experiment> experiments_{};
    std::vector<evolution::Candidate> candidates_{};
    std::size_t candidate_population_{0};
    std::vector<governance::DecisionRecord> approvals_{};
    std::vector<evolution::EvolutionNode> evolution_nodes_{};
    std::vector<operatingwindow::Checkpoint> checkpoints_{};
    std::vector<governance::AuditRecord> audit_{};
    // Presentation-only chart timeframe selection (default M15). Holds an
    // explicit Timeframe, never an index.
    TimeframeSelection selection_{};
    bool paused_{false};
    bool recovery_evaluated_{false};
};

}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_CONTROLCENTERSTATE_H
