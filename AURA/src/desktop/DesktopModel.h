#ifndef AURA_DESKTOP_DESKTOPMODEL_H
#define AURA_DESKTOP_DESKTOPMODEL_H

#include "desktop/CandleChart.h"
#include "desktop/DashboardProjector.h"
#include "foundation/PersistenceStatus.h"
#include "runtime/AdapterManager.h"
#include "runtime/ApplicationRecovery.h"
#include "runtime/ApplicationShell.h"

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace aura {
namespace desktop {

// A displayed value that may be absent. Missing data is never rendered as a
// success state: it carries the explicit label "NOT AVAILABLE" so an operator
// can never mistake an empty field for a green one.
struct FieldValue {
    bool available{false};
    std::string value{"NOT AVAILABLE"};
};

inline FieldValue field(bool present, std::string value) {
    FieldValue f;
    f.available = present;
    f.value = present ? std::move(value) : std::string("NOT AVAILABLE");
    return f;
}

// One row of the nine-timeframe display (V3-29). Identity is the explicit
// timeframe value, never the row position.
struct TimeframeRow {
    runtime::Timeframe timeframe{runtime::Timeframe::UNKNOWN};
    std::string label{"UNKNOWN"};
    bool present{false};            // has this stream reported anything yet
    std::string service_state{"NOT AVAILABLE"};
    std::string quality{"NOT AVAILABLE"};
    std::uint64_t accepted{0};
    std::uint64_t rejected{0};
    FieldValue last_event{};        // last receive time (ns)
    FieldValue last_close{};        // last processed closed-bar close time (ns)
    FieldValue last_bar_ohlc{};     // last retained bar "O/H/L/C"
    FieldValue provenance{};        // symbol identity of the last processed bar
    FieldValue sequence{};          // processed closed-bar sequence for this stream
    std::string last_error{};
    std::string freshness{"NOT AVAILABLE"};  // relative to the newest stream, or UNKNOWN
};

// The health string for a timeframe row: the stream's own service state, or an
// explicit NOT AVAILABLE when the stream has not reported at all. Shared by the
// terminal composition and the section panels so the wording is identical.
inline std::string row_health(const TimeframeRow& r) {
    return r.present ? r.service_state : "NOT AVAILABLE";
}

// The Canonical nine-frame display. Always returns exactly nine rows in V3-29
// order (M1..MN1); streams with no data are present=false and labelled
// NOT AVAILABLE rather than dropped or merged.
struct TimeframePanel {
    std::vector<TimeframeRow> rows{};
    bool any_present{false};
};

struct OverviewPanel {
    std::string project_name{"AURA"};
    std::string operating_mode{"SHADOW"};
    bool transport_connected{false};
    std::size_t streams_total{0};
    std::size_t streams_healthy{0};
    bool healthy{false};
    std::string aggregate{"STARTING"};
    std::uint64_t accepted{0};
    std::uint64_t rejected{0};
    std::uint64_t malformed{0};
};

struct SignalPanel {
    bool available{false};
    std::string direction{"NOT AVAILABLE"};
    std::string family{"NOT AVAILABLE"};
    std::string symbol{"NOT AVAILABLE"};
    std::string trigger_timeframe{"NOT AVAILABLE"};
    std::string decision_id{"NOT AVAILABLE"};
    // Deterministic score (RT-0011) and confidence (RT-0012). These are AURA's
    // real ranking/confidence values. Neither is a probability: AURA explicitly
    // does not emit a calibrated success probability, so none is displayed.
    bool score_available{false};
    double score{0.0};
    bool confidence_available{false};
    double confidence{0.0};
    // Realized shadow outcomes (historical counts, not a prediction). Available
    // once at least one shadow position has closed.
    bool outcomes_available{false};
    std::uint64_t positions_closed{0};
    std::uint64_t wins{0};
    // The finalized closed bar that triggered the signal (identity + close time).
    // Real values from the runtime; UNKNOWN when no signal has been produced.
    std::string closed_bar{"NOT AVAILABLE"};
    FieldValue closed_bar_close_time{};
};

struct RiskPanel {
    bool available{false};
    std::string direction{"NOT AVAILABLE"};
    double position_size{0.0};
    double stop_distance{0.0};
    double atr{0.0};
    // Deterministic sizing inputs from the real proposal (RT-0015), shown for
    // auditability. These are the proposal's own recorded inputs, not fabricated.
    double account_equity{0.0};
    double risk_fraction{0.0};
    double stop_atr_multiple{0.0};
    // A proposal is never an order; this mirrors the shadow-only invariant.
    bool is_order{false};
};

struct PositionPanel {
    bool has_open_position{false};
    std::size_t ledger_entries{0};
    std::uint64_t shadow_fills{0};
    std::uint64_t positions_opened{0};
    std::uint64_t positions_closed{0};
    std::uint64_t wins{0};
};

struct PersistencePanel {
    std::string store_path{"(in-memory)"};
    std::size_t records{0};
    std::string last_status{"UNKNOWN"};
    std::string lifecycle{"UNKNOWN_STATE"};
    bool resumable{false};
    bool fresh_start{false};
    bool used_known_good{false};
    std::string reason{"no recovery decision has been evaluated"};
};

struct HealthPanel {
    bool healthy{false};
    std::string aggregate{"STARTING"};
    bool watchdog_available{false};
};

// The complete read-only snapshot the GUI renders. It is produced by capturing
// the trusted runtime; the GUI never mutates it and never becomes a source of
// truth.
struct ControlCenterSnapshot {
    OverviewPanel overview{};
    TimeframePanel timeframes{};
    SignalPanel signal{};
    RiskPanel risk{};
    PositionPanel positions{};
    PersistencePanel persistence{};
    HealthPanel health{};
    bool shadow_only{true};
};

// Read-only projection of the live AURA application into control-center view.
//
// Presentation-only: it reads ApplicationShell / ApplicationPipeline state and
// copies values out. It performs no I/O of its own, mutates nothing, reads no
// clock, and fabricates nothing — where a value is unknown or absent it is
// labelled NOT AVAILABLE / UNKNOWN rather than defaulted to HEALTHY. There is
// no live-order path and no action surface here.
class DesktopModel {
public:
    static ControlCenterSnapshot capture(runtime::ApplicationShell& shell,
                                         const runtime::RecoveryOutcome& recovery,
                                         const std::string& last_persist_status = "OK") {
        ControlCenterSnapshot s;

        runtime::ApplicationPipeline& pipeline = shell.pipeline();
        const runtime::EngineStatus& st = pipeline.status();

        s.overview.transport_connected = st.transport_connected;
        s.overview.streams_total = st.streams_total;
        s.overview.streams_healthy = st.streams_healthy;
        s.overview.healthy = st.healthy;
        s.overview.aggregate = std::string(foundation::to_string(st.aggregate));
        s.overview.accepted = st.accepted;
        s.overview.rejected = st.rejected;
        s.overview.malformed = st.malformed;

        s.timeframes = timeframes(pipeline);

        const runtime::Signal& sig = pipeline.last_signal();
        if (sig.valid) {
            s.signal.available = true;
            s.signal.direction = std::string(runtime::to_string(sig.direction));
            s.signal.family = std::string(runtime::to_string(sig.family));
            s.signal.symbol = sig.symbol;
            s.signal.trigger_timeframe = std::string(runtime::to_string(sig.trigger_timeframe));
            s.signal.decision_id = sig.decision_id.to_hex();
            s.signal.closed_bar = sig.closed_bar.valid() ? sig.closed_bar.canonical_string()
                                                         : std::string("UNKNOWN");
            s.signal.closed_bar_close_time = field(sig.closed_bar.close_time().nanoseconds() != 0,
                                                   format_utc_minute(sig.closed_bar.close_time()));
        }

        // Real deterministic score/confidence of the latest signal. Shown only
        // when the pipeline actually produced them; never fabricated.
        const runtime::SignalScore& score = pipeline.last_score();
        if (sig.valid && score.valid) {
            s.signal.score_available = true;
            s.signal.score = score.score;
        }
        const runtime::ConfidenceValue& confidence = pipeline.last_confidence();
        if (sig.valid && confidence.valid) {
            s.signal.confidence_available = true;
            s.signal.confidence = confidence.confidence;
        }
        // Realized shadow outcomes (historical). Counts of closed simulated
        // positions, not a prediction or probability.
        s.signal.positions_closed = st.positions_closed;
        s.signal.wins = st.wins;
        s.signal.outcomes_available = st.positions_closed > 0;

        const runtime::RiskProposal& prop = pipeline.last_proposal();
        if (prop.valid) {
            s.risk.available = true;
            s.risk.direction = std::string(runtime::to_string(prop.direction));
            s.risk.position_size = prop.position_size;
            s.risk.stop_distance = prop.stop_distance;
            s.risk.atr = prop.atr;
            s.risk.account_equity = prop.account_equity;
            s.risk.risk_fraction = prop.risk_fraction;
            s.risk.stop_atr_multiple = prop.stop_atr_multiple;
            s.risk.is_order = prop.is_order;
        }

        s.positions.has_open_position = st.has_open_position;
        s.positions.ledger_entries = pipeline.ledger().size();
        s.positions.shadow_fills = st.fills;
        s.positions.positions_opened = st.positions;
        s.positions.positions_closed = st.positions_closed;
        s.positions.wins = st.wins;

        s.persistence.store_path = shell.store().path().empty() ? "(in-memory)" : shell.store().path();
        s.persistence.records = shell.store().size();
        s.persistence.last_status = last_persist_status;
        s.persistence.lifecycle = std::string(runtime::to_string(recovery.detected));
        s.persistence.resumable = recovery.resumable;
        s.persistence.fresh_start = recovery.fresh_start;
        s.persistence.used_known_good = recovery.used_known_good;
        s.persistence.reason = recovery.reason.empty() ? "no recovery decision recorded" : recovery.reason;

        s.health.healthy = st.healthy;
        s.health.aggregate = std::string(foundation::to_string(st.aggregate));
        s.health.watchdog_available = false;  // no watchdog telemetry source is wired yet

        // Shadow-only invariant: the system has no live-order capability, and the
        // most recent proposal (if any) is never an order.
        s.shadow_only = !prop.is_order;

        return s;
    }

    // Builds exactly nine timeframe rows in canonical order, mapping reported
    // stream state by explicit timeframe identity (never by position).
    static TimeframePanel timeframes(const runtime::ApplicationPipeline& pipeline) {
        TimeframePanel panel;
        const std::vector<runtime::StreamView> views = pipeline.streams();
        for (const runtime::Timeframe tf : runtime::all_timeframes()) {
            TimeframeRow row;
            row.timeframe = tf;
            row.label = std::string(runtime::to_string(tf));

            const runtime::StreamView* match = nullptr;
            for (const runtime::StreamView& v : views) {
                if (v.timeframe == tf) { match = &v; break; }
            }

            // A stream row is "present" only when this stream has actually
            // reported activity. A declared-but-silent stream is NOT AVAILABLE,
            // never a fabricated OFFLINE/HEALTHY success.
            bool stream_activity = false;
            if (match != nullptr) {
                stream_activity = match->connected || match->accepted > 0 ||
                                  match->last_event.nanoseconds() != 0 || !match->last_error.empty();
                if (stream_activity) {
                    row.service_state = match->connected ? (match->healthy ? "ONLINE" : "DEGRADED")
                                                         : "OFFLINE";
                    row.quality = std::string(foundation::to_string(match->quality));
                    row.accepted = match->accepted;
                    row.rejected = match->rejected;
                    row.last_event = field(match->last_event.nanoseconds() != 0,
                                           std::to_string(match->last_event.nanoseconds()));
                    row.last_error = match->last_error;
                }
            }

            // Last processed closed bar progress (identity is explicit, per timeframe).
            const runtime::TimeframeProgress* progress = pipeline.state_store().last_processed(tf);
            if (progress != nullptr) {
                row.last_close = field(progress->last_close.nanoseconds() != 0,
                                       std::to_string(progress->last_close.nanoseconds()));
                row.sequence = field(progress->sequence != 0, std::to_string(progress->sequence));
                if (progress->last_processed.valid()) {
                    row.provenance = field(true, progress->last_processed.canonical_string());
                }
            }

            const runtime::MarketBar* bar = pipeline.last_bar(tf);
            if (bar != nullptr) {
                row.last_bar_ohlc = field(true, format_ohlc(*bar));
            }

            row.present = stream_activity || row.last_close.available || row.last_bar_ohlc.available;
            if (row.present && row.service_state == "NOT AVAILABLE") row.service_state = "STARTING";
            panel.any_present = panel.any_present || row.present;
            panel.rows.push_back(std::move(row));
        }
        assign_freshness(panel);
        return panel;
    }

    // The control-center navigation sections required by the Master (V3-37).
    // Sections without a wired data source are shown as NOT AVAILABLE rather
    // than omitted or faked.
    static std::vector<std::string> sections() {
        return {
            "System Overview", "Market / Data Health", "Timeframes", "Signals",
            "Risk", "Shadow Positions", "Prediction / Observation", "Research",
            "Knowledge", "Candidates", "Validation", "Approval Center",
            "Evolution Graph", "Incidents", "Schedule / Operating Window",
            "Checkpoints / Recovery", "Health / Watchdog", "Audit",
            "Configuration / Version",
        };
    }

private:
    // Deterministic relative freshness: each present stream is compared with the
    // newest close time among the present streams (higher timeframe = later close
    // in the canonical feed). This is computed from data already in the snapshot,
    // never from a wall clock, so it is reproducible. A present stream with no
    // close time is UNKNOWN (never silently fresh); an absent stream is NOT
    // AVAILABLE.
    static void assign_freshness(TimeframePanel& panel) {
        std::int64_t newest = 0;
        bool any = false;
        for (const TimeframeRow& r : panel.rows) {
            if (r.present && r.last_close.available) {
                const std::int64_t v = std::stoll(r.last_close.value);
                if (!any || v > newest) { newest = v; any = true; }
            }
        }
        for (TimeframeRow& r : panel.rows) {
            if (!r.present) { r.freshness = "NOT AVAILABLE"; continue; }
            if (!r.last_close.available) { r.freshness = "UNKNOWN"; continue; }
            const std::int64_t v = std::stoll(r.last_close.value);
            r.freshness = (v >= newest) ? "FRESH" : "LAGGING";
        }
    }

    static std::string format_ohlc(const runtime::MarketBar& bar) {
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), "%.3f / %.3f / %.3f / %.3f", bar.open, bar.high,
                      bar.low, bar.close);
        return std::string(buffer);
    }
};

}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_DESKTOPMODEL_H
