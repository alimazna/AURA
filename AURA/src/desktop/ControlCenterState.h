#ifndef AURA_DESKTOP_CONTROLCENTERSTATE_H
#define AURA_DESKTOP_CONTROLCENTERSTATE_H

#include "desktop/CandleChart.h"
#include "desktop/ControlCenterPanels.h"
#include "desktop/DesktopModel.h"
#include "desktop/Mt5Candles.h"
#include "foundation/PersistenceStatus.h"
#include "mt5/ProtocolCodec.h"
#include "observation/FailureDetectionEngine.h"
#include "runtime/ApplicationRecovery.h"
#include "runtime/ApplicationShell.h"

#include <cstdint>
#include <fstream>
#include <map>
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
            out.push_back(aura::mt5::ProtocolCodec::encode_closed_bar(
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
        report_.chart = chart_series(selection_.selected());
        return report_;
    }

    // The XAUUSD chart timeframe selector (presentation-only). M15 by default.
    TimeframeSelection& timeframe_selection() noexcept { return selection_; }
    const TimeframeSelection& timeframe_selection() const noexcept { return selection_; }

    // Reads a timeframe's closed-bar series for the chart. When the opt-in MT5
    // bridge is enabled and a real candle file is present for this timeframe it is
    // served; otherwise this falls back to the runtime's existing retained series
    // (the synthetic/builtin source) unchanged. No candle is invented; an empty
    // series means NO CANDLE DATA.
    CandleSeries chart_series(runtime::Timeframe tf) const {
        if (mt5_enabled_) {
            const auto it = mt5_cache_.find(tf);
            if (it != mt5_cache_.end() && it->second.loaded) return it->second.series;
        }
        return make_candle_series(tf, shell_.pipeline().bar_series(tf));
    }

    // ---- Opt-in MT5 bridge (real candle files) --------------------------------
    //
    // Enabling the bridge is purely additive: it looks for real XAUUSD candle
    // files exported by the MT5 Python bridge and, where present, serves them to
    // the chart. Absent files change nothing. The bridge never enables live
    // trading; the system remains shadow-only.

    // Loads any available bridge candle files. Returns the number of timeframes
    // loaded. `exe_dir` is used to resolve the packaged bridge location.
    std::size_t enable_mt5_bridge(const std::string& exe_dir) {
        return enable_mt5_bridge_from_dirs(mt5::candidate_dirs(exe_dir));
    }

    // As above, but with explicit candidate directories (used by tests and by any
    // caller that already knows where the bridge output lives). Order is honoured.
    std::size_t enable_mt5_bridge_from_dirs(const std::vector<std::string>& dirs) {
        mt5_enabled_ = true;
        mt5_dirs_ = dirs;
        mt5_cache_.clear();
        std::size_t loaded = 0;
        for (const runtime::Timeframe tf : chart_timeframes()) {
            const std::string label(runtime::to_string(tf));
            mt5::LoadResult r = mt5::load_candles_any(mt5_dirs_, label);
            if (r.loaded) ++loaded;
            mt5_cache_.emplace(tf, std::move(r));
        }
        mt5_loaded_ = loaded;
        return loaded;
    }

    // True when the chart is currently being served real MT5 bridge data.
    bool using_real_data() const noexcept { return mt5_enabled_ && mt5_loaded_ > 0; }

    // A short, honest description of the chart data source, for logging.
    std::string data_source_note() const {
        if (using_real_data())
            return "LIVE (MT5) - " + std::to_string(mt5_loaded_) +
                   "/9 timeframes from the MT5 bridge";
        if (mt5_enabled_) return "SYNTHETIC (MT5 bridge enabled, no candle files found)";
        return "SYNTHETIC";
    }

    std::size_t mt5_timeframes_loaded() const noexcept { return mt5_loaded_; }

    const ControlCenterReport& report() const noexcept { return report_; }

    const ControlCenterSnapshot& snapshot() const noexcept { return snapshot_; }
    bool recovery_evaluated() const noexcept { return recovery_evaluated_; }
    const ControlCenterOptions& options() const noexcept { return options_; }

private:
    ControlCenterOptions options_{};
    runtime::ApplicationShell shell_;
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
    // Opt-in MT5 bridge state. Empty/disabled by default, so behavior is
    // unchanged unless enable_mt5_bridge() is called.
    bool mt5_enabled_{false};
    std::size_t mt5_loaded_{0};
    std::vector<std::string> mt5_dirs_{};
    std::map<runtime::Timeframe, mt5::LoadResult> mt5_cache_{};
};

}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_CONTROLCENTERSTATE_H
