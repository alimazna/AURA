#ifndef AURA_DESKTOP_CONTROLCENTERSTATE_H
#define AURA_DESKTOP_CONTROLCENTERSTATE_H

#include "desktop/DesktopModel.h"
#include "foundation/PersistenceStatus.h"
#include "mt5/ProtocolCodec.h"
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

    const ControlCenterSnapshot& snapshot() const noexcept { return snapshot_; }
    bool recovery_evaluated() const noexcept { return recovery_evaluated_; }
    const ControlCenterOptions& options() const noexcept { return options_; }

private:
    ControlCenterOptions options_{};
    runtime::ApplicationShell shell_;
    runtime::RecoveryOutcome recovery_{};
    ControlCenterSnapshot snapshot_{};
    foundation::PersistenceStatus last_persist_{foundation::PersistenceStatus::UNKNOWN};
    bool paused_{false};
    bool recovery_evaluated_{false};
};

}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_CONTROLCENTERSTATE_H
