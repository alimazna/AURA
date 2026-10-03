#ifndef AURA_DESKTOP_GUIPANELS_H
#define AURA_DESKTOP_GUIPANELS_H

#include "desktop/AuraTheme.h"
#include "desktop/AuraWidgets.h"
#include "desktop/CandleChartWidget.h"
#include "desktop/ControlCenterState.h"
#include "desktop/DesktopModel.h"

#include <imgui.h>

#include <cstddef>
#include <string>

namespace aura {
namespace desktop {

// Presentation-only rendering for the AURA control center.
//
// This translation unit reads a pre-built ControlCenterReport (produced by the
// single runtime owner, ControlCenterState) and draws it through the AURA design
// system. It queries no runtime directly and mutates no state. Unknown/absent
// values are always shown explicitly (NOT AVAILABLE / UNKNOWN) so a blank field
// is never mistaken for a healthy value or a zero.

// ---- small local helpers ---------------------------------------------------

// The health string for a timeframe row: the stream's own service state, or an
// explicit NOT AVAILABLE when the stream has not reported at all.
inline std::string row_health(const TimeframeRow& r) {
    return r.present ? r.service_state : "NOT AVAILABLE";
}

// The last recorded signal direction for a specific stream. The runtime keeps a
// single most-recent signal, so only the row whose explicit timeframe matches
// its trigger timeframe can show a direction; other rows show a muted dash
// meaning "no signal recorded for this stream" (never a fabricated value).
inline std::string signal_for_row(const SignalPanel& s, const TimeframeRow& r) {
    if (!s.available) return "\u2014";
    if (s.trigger_timeframe == r.label) return s.direction;
    return "\u2014";
}

// Counts how many of the nine rows have actually reported.
inline int present_count(const TimeframePanel& tf) {
    int n = 0;
    for (const TimeframeRow& r : tf.rows)
        if (r.present) ++n;
    return n;
}

// ---- Dashboard -------------------------------------------------------------

// Finds the nine-timeframe row whose explicit identity matches. Never positional.
inline const TimeframeRow* find_timeframe_row(const TimeframePanel& panel,
                                              runtime::Timeframe tf) {
    for (const TimeframeRow& r : panel.rows)
        if (r.timeframe == tf) return &r;
    return nullptr;
}

// The XAUUSD market area: the primary visual element of the dashboard. It shows a
// real OHLC candlestick chart for the selected timeframe, a timeframe selector,
// and chart metadata sourced from the runtime. It never fabricates candles: a
// timeframe with no real closed bars shows an explicit NO CANDLE DATA state, and
// the metadata fields fall back to NOT AVAILABLE rather than a fake value.
inline void draw_market_chart(const ControlCenterReport& r, ControlCenterState& state) {
    const CandleSeries& series = r.chart;
    const TimeframeRow* row = find_timeframe_row(r.snapshot.timeframes, series.timeframe);

    widgets::card_begin("chart_card", nullptr, ImVec2(0.0f, 0.0f), 566.0f);

    // Title strip with the symbol identity and the selected timeframe.
    ImGui::TextColored(theme::kTextPrimary, "XAUUSD");
    ImGui::SameLine();
    ImGui::TextColored(theme::kTextMuted, "\u00b7");
    ImGui::SameLine();
    ImGui::TextColored(theme::kAccent, "%s", series.label.c_str());
    ImGui::SameLine();
    ImGui::TextColored(theme::kTextMuted, "\u00b7 SHADOW ONLY");

    // Timeframe selector: clicking switches the displayed candle stream.
    ImGui::Spacing();
    chart::timeframe_selector(state.timeframe_selection());
    ImGui::Spacing();

    // Chart metadata / context for the selected timeframe.
    {
        const int bars = static_cast<int>(series.candles.size());
        ImGui::TextColored(theme::kTextMuted, "Closed bars");
        ImGui::SameLine();
        if (series.available) ImGui::TextColored(theme::kTextPrimary, "%d", bars);
        else ImGui::TextColored(theme::kNotAvailable, "NOT AVAILABLE");

        ImGui::SameLine();
        ImGui::TextColored(theme::kTextMuted, "  |  Sequence");
        ImGui::SameLine();
        if (row != nullptr) widgets::value_cell(row->sequence.available, row->sequence.value);
        else ImGui::TextColored(theme::kNotAvailable, "NOT AVAILABLE");

        ImGui::SameLine();
        ImGui::TextColored(theme::kTextMuted, "  |  Freshness");
        ImGui::SameLine();
        if (row != nullptr) widgets::state_cell(row->freshness);
        else ImGui::TextColored(theme::kNotAvailable, "NOT AVAILABLE");

        ImGui::SameLine();
        ImGui::TextColored(theme::kTextMuted, "  |  Stream");
        ImGui::SameLine();
        if (row != nullptr) widgets::state_cell(row_health(*row));
        else ImGui::TextColored(theme::kNotAvailable, "NOT AVAILABLE");
    }
    if (series.available) {
        ImGui::TextColored(theme::kTextMuted, "Range");
        ImGui::SameLine();
        ImGui::TextColored(theme::kTextSecondary, "%.3f \u2013 %.3f", series.low, series.high);
        ImGui::SameLine();
        ImGui::TextColored(theme::kTextMuted, "  |  Last closed");
        ImGui::SameLine();
        ImGui::TextColored(theme::kTextSecondary, "%s",
                           format_utc_minute(series.last_close).c_str());
    }

    ImGui::Spacing();
    chart::candlestick_chart(series, 380.0f);
    widgets::card_end();
}

inline void draw_dashboard(const ControlCenterReport& r, ControlCenterState& state) {
    const ControlCenterSnapshot& s = r.snapshot;
    widgets::section_header("Dashboard", "Read-only overview of the AURA shadow runtime");

    // Primary element: the XAUUSD candlestick market area with timeframe selector.
    draw_market_chart(r, state);
    ImGui::Spacing();
    ImGui::Spacing();

    const float avail = ImGui::GetContentRegionAvail().x;
    const float gap = theme::kSpace3;
    const float card_w = (avail - gap * 3.0f) / 4.0f;
    const float card_h = 138.0f;

    // --- Status row: Health / Execution mode / Persistence / Recovery --------
    widgets::card_begin("dash_health", "SYSTEM HEALTH", ImVec2(card_w, card_h));
    widgets::badge(s.overview.healthy ? "HEALTHY" : s.overview.aggregate.c_str(),
                   theme::status_color(s.overview.healthy ? "HEALTHY" : s.overview.aggregate));
    ImGui::Spacing();
    widgets::kv_row("Streams", std::to_string(s.overview.streams_healthy) + " / " +
                                   std::to_string(s.overview.streams_total) + " healthy");
    widgets::kv_row("Transport", s.overview.transport_connected ? "CONNECTED" : "UNKNOWN");
    widgets::kv_row("Aggregate", s.overview.aggregate);
    widgets::card_end();

    ImGui::SameLine(0.0f, gap);
    widgets::card_begin("dash_mode", "EXECUTION MODE", ImVec2(card_w, card_h));
    widgets::badge("SHADOW ONLY", theme::kShadow);
    ImGui::Spacing();
    widgets::kv_row("Mode", s.overview.operating_mode);
    widgets::kv_row("Live orders", "NONE (no live path)");
    widgets::kv_row("Invariant", s.shadow_only ? "HELD" : "VIOLATED");
    widgets::card_end();

    ImGui::SameLine(0.0f, gap);
    widgets::card_begin("dash_persist", "PERSISTENCE", ImVec2(card_w, card_h));
    widgets::badge(s.persistence.last_status.c_str(), theme::status_color(s.persistence.last_status));
    ImGui::Spacing();
    widgets::kv_row("Store", s.persistence.store_path);
    widgets::kv_row("Records", std::to_string(s.persistence.records));
    widgets::kv_row("Append-only", "YES");
    widgets::card_end();

    ImGui::SameLine(0.0f, gap);
    widgets::card_begin("dash_recovery", "RECOVERY", ImVec2(card_w, card_h));
    widgets::badge(s.persistence.lifecycle.c_str(), theme::status_color(s.persistence.lifecycle));
    ImGui::Spacing();
    widgets::kv_row("Resumable", s.persistence.resumable ? "YES" : "NO");
    widgets::kv_row("Fresh start", s.persistence.fresh_start ? "yes" : "no");
    widgets::kv_row("Known-good", s.persistence.used_known_good ? "yes" : "no");
    widgets::card_end();

    ImGui::Spacing();
    ImGui::Spacing();

    // --- Nine timeframe summary ---------------------------------------------
    widgets::card_begin("dash_tf", "NINE TIMEFRAME SUMMARY", ImVec2(0.0f, 0.0f), 268.0f);
    ImGui::TextColored(theme::kTextMuted,
                       "%d / 9 streams reporting  \u00b7  identity is the explicit timeframe, "
                       "never the row position",
                       present_count(s.timeframes));
    ImGui::Spacing();
    if (ImGui::BeginTable("dash_tf_table", 6, widgets::table_flags(false, false))) {
        ImGui::TableSetupColumn("TF", ImGuiTableColumnFlags_WidthFixed, 52.0f);
        ImGui::TableSetupColumn("Health");
        ImGui::TableSetupColumn("Quality");
        ImGui::TableSetupColumn("Seq");
        ImGui::TableSetupColumn("Freshness");
        ImGui::TableSetupColumn("Signal");
        ImGui::TableHeadersRow();
        for (const TimeframeRow& row : s.timeframes.rows) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(theme::kTextPrimary, "%s", row.label.c_str());
            ImGui::TableNextColumn();
            widgets::state_cell(row_health(row));
            ImGui::TableNextColumn();
            widgets::state_cell(row.quality);
            ImGui::TableNextColumn();
            widgets::value_cell(row.sequence.available, row.sequence.value);
            ImGui::TableNextColumn();
            widgets::state_cell(row.freshness);
            ImGui::TableNextColumn();
            const std::string sig = signal_for_row(s.signal, row);
            if (sig == "\u2014") ImGui::TextColored(theme::kTextMuted, "%s", sig.c_str());
            else ImGui::TextColored(theme::status_color("HEALTHY"), "%s", sig.c_str());
        }
        ImGui::EndTable();
    }
    widgets::card_end();

    ImGui::Spacing();
    ImGui::Spacing();

    // --- Signal + ledger summaries ------------------------------------------
    const float half = (avail - gap) / 2.0f;
    widgets::card_begin("dash_signal", "SIGNAL SUMMARY", ImVec2(half, 0.0f), 168.0f);
    if (!s.signal.available) {
        widgets::empty_state("Signal", "no signal has been produced by the runtime yet");
    } else {
        widgets::badge(s.signal.direction.c_str(), theme::status_color("HEALTHY"));
        ImGui::Spacing();
        widgets::kv_row("Symbol", s.signal.symbol);
        widgets::kv_row("Timeframe", s.signal.trigger_timeframe);
        widgets::kv_row("Family", s.signal.family);
        widgets::kv_row("Decision id", s.signal.decision_id);
    }
    widgets::card_end();

    ImGui::SameLine(0.0f, gap);
    widgets::card_begin("dash_ledger", "SHADOW LEDGER SUMMARY", ImVec2(half, 0.0f), 168.0f);
    widgets::kv_row("Entries", std::to_string(r.shadow_ledger.total));
    widgets::kv_row("Append-only", r.shadow_ledger.append_only ? "yes" : "no");
    widgets::kv_row("Open position", s.positions.has_open_position ? "OPEN" : "NONE");
    widgets::kv_row("Shadow fills", std::to_string(s.positions.shadow_fills));
    if (!r.shadow_ledger.available || r.shadow_ledger.recent.empty()) {
        widgets::empty_note("No ledger entries yet (a shadow session has not produced any).");
    } else {
        const LedgerEntryRow& e = r.shadow_ledger.recent.front();
        widgets::kv_row("Latest", e.type + "  @ " + e.recorded_at);
    }
    widgets::card_end();

    ImGui::Spacing();
    ImGui::Spacing();

    // --- Recent incidents ----------------------------------------------------
    widgets::card_begin("dash_incidents", "RECENT INCIDENTS", ImVec2(0.0f, 0.0f), 168.0f);
    if (!r.incidents.available) {
        widgets::empty_state("Incidents", "no failure-detection source is wired");
    } else if (r.incidents.rows.empty()) {
        widgets::empty_note("No incidents detected from the current adapter stream health.");
    } else {
        if (ImGui::BeginTable("dash_inc_table", 4, widgets::table_flags())) {
            ImGui::TableSetupColumn("Severity", ImGuiTableColumnFlags_WidthFixed, 96.0f);
            ImGui::TableSetupColumn("Component");
            ImGui::TableSetupColumn("Recovery");
            ImGui::TableSetupColumn("Message");
            ImGui::TableHeadersRow();
            std::size_t shown = 0;
            for (const IncidentRow& i : r.incidents.rows) {
                if (shown++ >= 5) break;
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                widgets::state_cell(i.severity);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(i.component.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(i.recovery_action.c_str());
                ImGui::TableNextColumn();
                ImGui::TextWrapped("%s", i.message.c_str());
            }
            ImGui::EndTable();
        }
    }
    widgets::card_end();

    ImGui::Spacing();
    ImGui::Spacing();

    // --- Version / configuration context ------------------------------------
    widgets::card_begin("dash_version", "VERSION / CONFIGURATION", ImVec2(0.0f, 0.0f), 150.0f);
    widgets::kv_row("Schema", r.version.schema_version);
    widgets::kv_row("Strategy", r.version.strategy_version);
    widgets::kv_row("Configuration", r.version.configuration_version);
    widgets::kv_row("Family", r.version.strategy_family);
    widgets::kv_row("Symbol", r.version.symbol);
    widgets::card_end();
}

// ---- Market / Data Health --------------------------------------------------

inline void draw_market(const ControlCenterSnapshot& s) {
    widgets::section_header("Market", "Transport and per-stream data quality");

    const float avail = ImGui::GetContentRegionAvail().x;
    const float gap = theme::kSpace3;
    const float w = (avail - gap * 3.0f) / 4.0f;

    widgets::card_begin("mkt_transport", "TRANSPORT", ImVec2(w, 96.0f));
    widgets::badge(s.overview.transport_connected ? "CONNECTED" : "UNKNOWN",
                   theme::status_color(s.overview.transport_connected ? "CONNECTED" : "UNKNOWN"));
    ImGui::Spacing();
    widgets::kv_row("Streams", std::to_string(s.overview.streams_healthy) + " / " +
                                   std::to_string(s.overview.streams_total));
    widgets::card_end();

    ImGui::SameLine(0.0f, gap);
    widgets::card_begin("mkt_accepted", "ACCEPTED", ImVec2(w, 96.0f));
    widgets::metric_u64("Closed bars", (unsigned long long)s.overview.accepted, true);
    widgets::card_end();

    ImGui::SameLine(0.0f, gap);
    widgets::card_begin("mkt_rejected", "REJECTED", ImVec2(w, 96.0f));
    widgets::metric_u64("Closed bars", (unsigned long long)s.overview.rejected, true);
    widgets::card_end();

    ImGui::SameLine(0.0f, gap);
    widgets::card_begin("mkt_malformed", "MALFORMED", ImVec2(w, 96.0f));
    widgets::metric_u64("Frames", (unsigned long long)s.overview.malformed, true);
    widgets::card_end();

    ImGui::Spacing();
    ImGui::Spacing();

    widgets::card_begin("mkt_quality", "PER-STREAM DATA QUALITY", ImVec2(0.0f, 0.0f), 300.0f);
    if (ImGui::BeginTable("mkt_q_table", 4, widgets::table_flags())) {
        ImGui::TableSetupColumn("Timeframe", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Health");
        ImGui::TableSetupColumn("Quality");
        ImGui::TableSetupColumn("Last event (ns)");
        ImGui::TableHeadersRow();
        for (const TimeframeRow& r : s.timeframes.rows) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(theme::kTextPrimary, "%s", r.label.c_str());
            ImGui::TableNextColumn();
            widgets::state_cell(row_health(r));
            ImGui::TableNextColumn();
            widgets::state_cell(r.quality);
            ImGui::TableNextColumn();
            widgets::value_cell(r.last_event.available, r.last_event.value);
        }
        ImGui::EndTable();
    }
    widgets::card_end();
}

// ---- Timeframes ------------------------------------------------------------

inline void draw_timeframes(const ControlCenterSnapshot& s) {
    widgets::section_header(
        "Timeframes", "Nine logical streams \u00b7 identity is the explicit timeframe, not the row");
    ImGui::TextWrapped(
        "A silent stream is shown NOT AVAILABLE and is never merged into an aggregate "
        "'healthy' status.");

    widgets::card_begin("tf_card", nullptr, ImVec2(0.0f, 0.0f), 420.0f);
    if (ImGui::BeginTable("tf_table", 10, widgets::table_flags(false, true))) {
        ImGui::TableSetupColumn("Timeframe", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableSetupColumn("Health");
        ImGui::TableSetupColumn("Quality");
        ImGui::TableSetupColumn("Sequence");
        ImGui::TableSetupColumn("Freshness");
        ImGui::TableSetupColumn("Last Closed Bar");
        ImGui::TableSetupColumn("Provenance");
        ImGui::TableSetupColumn("Signal");
        ImGui::TableSetupColumn("Accepted");
        ImGui::TableSetupColumn("Rejected");
        ImGui::TableHeadersRow();
        for (const TimeframeRow& row : s.timeframes.rows) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(theme::kTextPrimary, "%s", row.label.c_str());
            ImGui::TableNextColumn();
            widgets::state_cell(row_health(row));
            ImGui::TableNextColumn();
            widgets::state_cell(row.quality);
            ImGui::TableNextColumn();
            widgets::value_cell(row.sequence.available, row.sequence.value);
            ImGui::TableNextColumn();
            widgets::state_cell(row.freshness);
            ImGui::TableNextColumn();
            widgets::value_cell(row.last_close.available, row.last_close.value);
            ImGui::TableNextColumn();
            widgets::value_cell(row.provenance.available, row.provenance.value);
            ImGui::TableNextColumn();
            const std::string sig = signal_for_row(s.signal, row);
            if (sig == "\u2014") ImGui::TextColored(theme::kTextMuted, "%s", sig.c_str());
            else ImGui::TextColored(theme::status_color("HEALTHY"), "%s", sig.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%llu", (unsigned long long)row.accepted);
            ImGui::TableNextColumn();
            ImGui::Text("%llu", (unsigned long long)row.rejected);
        }
        ImGui::EndTable();
    }
    widgets::card_end();

    ImGui::Spacing();
    widgets::card_begin("tf_ohlc", "LAST BAR (O / H / L / C)", ImVec2(0.0f, 0.0f), 220.0f);
    if (ImGui::BeginTable("tf_ohlc_table", 2, widgets::table_flags())) {
        ImGui::TableSetupColumn("Timeframe", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableSetupColumn("O / H / L / C");
        ImGui::TableHeadersRow();
        for (const TimeframeRow& row : s.timeframes.rows) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(theme::kTextPrimary, "%s", row.label.c_str());
            ImGui::TableNextColumn();
            widgets::value_cell(row.last_bar_ohlc.available, row.last_bar_ohlc.value);
        }
        ImGui::EndTable();
    }
    widgets::card_end();
}

// ---- Signals ---------------------------------------------------------------

inline void draw_signals(const ControlCenterSnapshot& s) {
    widgets::section_header("Signals", "Most recent decision produced by the runtime");

    widgets::card_begin("sig_card", "LATEST SIGNAL", ImVec2(0.0f, 0.0f), 220.0f);
    if (!s.signal.available) {
        widgets::empty_state("Signal", "no signal has been produced by the runtime yet");
    } else {
        widgets::badge(s.signal.direction.c_str(), theme::status_color("HEALTHY"));
        ImGui::Spacing();
        widgets::kv_row("Direction", s.signal.direction);
        widgets::kv_row("Strategy family", s.signal.family);
        widgets::kv_row("Symbol", s.signal.symbol);
        widgets::kv_row("Trigger timeframe", s.signal.trigger_timeframe);
        widgets::kv_row("Decision id", s.signal.decision_id);
        widgets::kv_row("Score / confidence", "NOT AVAILABLE (no calibrated probability is claimed)");
    }
    widgets::card_end();
}

// ---- Risk ------------------------------------------------------------------

inline void draw_risk(const ControlCenterSnapshot& s) {
    widgets::section_header("Risk", "Shadow-only sizing decision \u00b7 never an order");

    widgets::card_begin("risk_card", "RISK PROPOSAL", ImVec2(0.0f, 0.0f), 240.0f);
    if (!s.risk.available) {
        widgets::empty_state("Risk proposal", "no risk proposal has been produced yet");
    } else {
        widgets::badge(s.risk.direction.c_str(), theme::status_color("HEALTHY"));
        ImGui::Spacing();
        widgets::kv_row("Direction", s.risk.direction);
        widgets::kv_row("Position size", std::to_string(s.risk.position_size));
        widgets::kv_row("Stop distance", std::to_string(s.risk.stop_distance));
        widgets::kv_row("ATR", std::to_string(s.risk.atr));
        widgets::kv_state_row("Is live order", s.risk.is_order ? "YES (UNEXPECTED)" : "NO");
    }
    widgets::card_end();

    ImGui::Spacing();
    widgets::card_begin("risk_gate", "GATING", ImVec2(0.0f, 0.0f), 110.0f);
    widgets::empty_state("Risk gating state", "no risk-gate telemetry source is wired to this view");
    widgets::card_end();
}

// ---- Shadow Positions ------------------------------------------------------

inline void draw_positions(const ControlCenterSnapshot& s) {
    widgets::section_header("Shadow Positions",
                            "Simulated fills only \u00b7 there are no order controls on this surface");

    const float avail = ImGui::GetContentRegionAvail().x;
    const float gap = theme::kSpace3;
    const float w = (avail - gap * 2.0f) / 3.0f;

    widgets::card_begin("pos_open", "OPEN POSITION", ImVec2(w, 110.0f));
    widgets::badge(s.positions.has_open_position ? "OPEN" : "NONE",
                   theme::status_color(s.positions.has_open_position ? "DEGRADED" : "NONE"));
    ImGui::Spacing();
    widgets::kv_row("Ledger entries", std::to_string(s.positions.ledger_entries));
    widgets::card_end();

    ImGui::SameLine(0.0f, gap);
    widgets::card_begin("pos_fills", "SHADOW FILLS", ImVec2(w, 110.0f));
    widgets::metric_u64("Simulated fills", (unsigned long long)s.positions.shadow_fills, true);
    widgets::card_end();

    ImGui::SameLine(0.0f, gap);
    widgets::card_begin("pos_opened", "POSITIONS OPENED", ImVec2(w, 110.0f));
    widgets::metric_u64("Simulated positions", (unsigned long long)s.positions.positions_opened, true);
    widgets::card_end();

    ImGui::Spacing();
    widgets::card_begin("pos_ledger", "SHADOW LEDGER", ImVec2(0.0f, 0.0f), 90.0f);
    widgets::kv_row("Entries", std::to_string(s.positions.ledger_entries));
    widgets::kv_row("Append-only", "yes (entries are never mutated or erased)");
    widgets::card_end();
}

// ---- Observation / Prediction ---------------------------------------------

inline void draw_prediction(const ControlCenterReport& r) {
    widgets::section_header("Observation", "Prediction ledger (ranking score, not a probability)");
    if (!r.predictions.available) {
        widgets::card_begin("pred_na", nullptr, ImVec2(0.0f, 0.0f), 130.0f);
        widgets::empty_state("Prediction ledger", r.predictions.note.c_str());
        widgets::card_end();
        return;
    }
    widgets::card_begin("pred_card", "PREDICTIONS", ImVec2(0.0f, 0.0f), 320.0f);
    widgets::kv_row("Total", std::to_string(r.predictions.total));
    ImGui::Spacing();
    if (ImGui::BeginTable("pred_table", 6, widgets::table_flags(false, true))) {
        ImGui::TableSetupColumn("Timeframe");
        ImGui::TableSetupColumn("Direction");
        ImGui::TableSetupColumn("Symbol");
        ImGui::TableSetupColumn("Score (ranking)");
        ImGui::TableSetupColumn("Reference");
        ImGui::TableSetupColumn("Predicted at (ns)");
        ImGui::TableHeadersRow();
        for (const PredictionRow& p : r.predictions.rows) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(p.timeframe.c_str());
            ImGui::TableNextColumn(); widgets::state_cell(p.direction);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(p.symbol.c_str());
            ImGui::TableNextColumn(); ImGui::Text("%.4f", p.score);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(p.reference_price.c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(p.predicted_at.c_str());
        }
        ImGui::EndTable();
    }
    widgets::card_end();
}

// ---- Research --------------------------------------------------------------

inline void draw_research(const ControlCenterReport& r) {
    widgets::section_header("Research", "Hypotheses and experiments (sandboxed, no runtime effect)");
    if (!r.research.available) {
        widgets::card_begin("res_na", nullptr, ImVec2(0.0f, 0.0f), 130.0f);
        widgets::empty_state("Experiment ledger", "no experiment has been recorded in this session");
        widgets::card_end();
        return;
    }
    widgets::card_begin("res_card", "EXPERIMENTS", ImVec2(0.0f, 0.0f), 320.0f);
    widgets::kv_row("Total", std::to_string(r.research.total));
    ImGui::Spacing();
    if (ImGui::BeginTable("exp_table", 4, widgets::table_flags())) {
        ImGui::TableSetupColumn("Question");
        ImGui::TableSetupColumn("Decision");
        ImGui::TableSetupColumn("Evidence zone");
        ImGui::TableSetupColumn("Contamination");
        ImGui::TableHeadersRow();
        for (const ResearchRow& e : r.research.rows) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextWrapped("%s", e.question.c_str());
            ImGui::TableNextColumn(); widgets::state_cell(e.decision);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(e.evidence_zone.c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(e.contamination.c_str());
        }
        ImGui::EndTable();
    }
    widgets::card_end();
}

// ---- Knowledge -------------------------------------------------------------

inline void draw_knowledge(const ControlCenterReport& r) {
    widgets::section_header("Knowledge", "Knowledge identities and revisions (append-only history)");
    if (!r.knowledge.available) {
        widgets::card_begin("kn_na", nullptr, ImVec2(0.0f, 0.0f), 130.0f);
        widgets::empty_state("Knowledge store",
                             "no knowledge revision has been recorded in this session");
        widgets::card_end();
        return;
    }
    widgets::card_begin("kn_card", "KNOWLEDGE", ImVec2(0.0f, 0.0f), 340.0f);
    widgets::kv_row("Identities", std::to_string(r.knowledge.identities));
    widgets::kv_row("Revisions", std::to_string(r.knowledge.revisions));
    ImGui::Spacing();
    if (ImGui::BeginTable("kn_table", 4, widgets::table_flags())) {
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("Rev", ImGuiTableColumnFlags_WidthFixed, 48.0f);
        ImGui::TableSetupColumn("Scope");
        ImGui::TableSetupColumn("Observation");
        ImGui::TableHeadersRow();
        for (const KnowledgeRow& k : r.knowledge.rows) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); widgets::state_cell(k.status);
            ImGui::TableNextColumn(); ImGui::Text("%u", (unsigned)k.revision);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(k.validity_scope.c_str());
            ImGui::TableNextColumn(); ImGui::TextWrapped("%s", k.observation.c_str());
        }
        ImGui::EndTable();
    }
    widgets::card_end();
}

// ---- Candidates ------------------------------------------------------------

inline void draw_candidates(const ControlCenterReport& r) {
    widgets::section_header("Candidates", "Candidate registry (proposals, not promoted versions)");
    if (!r.candidates.available) {
        widgets::card_begin("cand_na", nullptr, ImVec2(0.0f, 0.0f), 130.0f);
        widgets::empty_state("Candidate registry", "no candidate has been registered in this session");
        widgets::card_end();
        return;
    }
    widgets::card_begin("cand_card", "CANDIDATES", ImVec2(0.0f, 0.0f), 340.0f);
    widgets::kv_row("Population", std::to_string(r.candidates.population));
    ImGui::Spacing();
    if (ImGui::BeginTable("cand_table", 4, widgets::table_flags())) {
        ImGui::TableSetupColumn("Candidate");
        ImGui::TableSetupColumn("Parent");
        ImGui::TableSetupColumn("Change");
        ImGui::TableSetupColumn("State");
        ImGui::TableHeadersRow();
        for (const CandidateRow& c : r.candidates.rows) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(c.candidate_id.c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(c.parent_version.c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(c.change_type.c_str());
            ImGui::TableNextColumn(); widgets::state_cell(c.state);
        }
        ImGui::EndTable();
    }
    widgets::card_end();
}

// ---- Validation ------------------------------------------------------------

inline void draw_validation(const ControlCenterReport& r) {
    widgets::section_header("Validation", "Out-of-sample evidence firewall (Phase 6)");
    widgets::card_begin("val_card", "VALIDATION EVIDENCE", ImVec2(0.0f, 0.0f), 200.0f);
    if (!r.validation.available) {
        widgets::empty_state("Validation evidence",
                             "no validation campaign source is wired. A missing campaign is NOT a "
                             "pass: absent evidence is never treated as validation.");
    }
    ImGui::Spacing();
    ImGui::TextColored(theme::kTextMuted,
                       "No profitability or calibration claim is made on this surface.");
    widgets::card_end();
}

// ---- Governance / Approvals -----------------------------------------------

inline void draw_approvals(const ControlCenterReport& r) {
    widgets::section_header("Governance", "Human decisions (automated actors cannot self-approve)");
    widgets::card_begin("gov_card", "APPROVAL CENTER", ImVec2(0.0f, 0.0f), 340.0f);
    if (!r.approvals.available) {
        widgets::empty_note("No human decision has been recorded in this session.");
    } else {
        widgets::kv_row("Decisions", std::to_string(r.approvals.total));
        ImGui::Spacing();
        if (ImGui::BeginTable("appr_table", 4, widgets::table_flags())) {
            ImGui::TableSetupColumn("Decision");
            ImGui::TableSetupColumn("Status");
            ImGui::TableSetupColumn("Actor");
            ImGui::TableSetupColumn("Question");
            ImGui::TableHeadersRow();
            for (const ApprovalRow& a : r.approvals.rows) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::TextUnformatted(a.decision_id.c_str());
                ImGui::TableNextColumn(); widgets::state_cell(a.status);
                ImGui::TableNextColumn(); ImGui::TextUnformatted(a.actor.c_str());
                ImGui::TableNextColumn(); ImGui::TextWrapped("%s", a.question.c_str());
            }
            ImGui::EndTable();
        }
    }
    widgets::card_end();
}

// ---- Evolution -------------------------------------------------------------

inline void draw_evolution(const ControlCenterReport& r) {
    widgets::section_header("Evolution", "Version lineage (parent \u2192 child)");
    widgets::card_begin("evo_card", "EVOLUTION GRAPH", ImVec2(0.0f, 0.0f), 340.0f);
    if (!r.evolution.available) {
        widgets::empty_state("Evolution graph", "no version lineage has been recorded in this session");
    } else {
        widgets::kv_row("Nodes", std::to_string(r.evolution.nodes));
        widgets::kv_row("Edges", std::to_string(r.evolution.edges.size()));
        ImGui::Spacing();
        if (ImGui::BeginTable("evo_table", 2, widgets::table_flags())) {
            ImGui::TableSetupColumn("Parent");
            ImGui::TableSetupColumn("Child");
            ImGui::TableHeadersRow();
            for (const EvolutionEdgeRow& e : r.evolution.edges) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::TextUnformatted(e.parent_version.c_str());
                ImGui::TableNextColumn(); ImGui::TextUnformatted(e.child_version.c_str());
            }
            ImGui::EndTable();
        }
    }
    widgets::card_end();
}

// ---- Incidents -------------------------------------------------------------

inline void draw_incidents(const ControlCenterReport& r) {
    widgets::section_header("Incidents", "Failure records detected from live adapter health");
    widgets::card_begin("inc_card", "INCIDENTS", ImVec2(0.0f, 0.0f), 340.0f);
    if (!r.incidents.available) {
        widgets::empty_state("Incident source", "no failure-detection source is wired");
    } else if (r.incidents.rows.empty()) {
        widgets::empty_note("No incidents detected from the current adapter stream health.");
    } else {
        widgets::kv_row("Detected", std::to_string(r.incidents.count));
        ImGui::Spacing();
        if (ImGui::BeginTable("inc_table", 5, widgets::table_flags())) {
            ImGui::TableSetupColumn("Severity", ImGuiTableColumnFlags_WidthFixed, 96.0f);
            ImGui::TableSetupColumn("Component");
            ImGui::TableSetupColumn("State");
            ImGui::TableSetupColumn("Recovery");
            ImGui::TableSetupColumn("Message");
            ImGui::TableHeadersRow();
            for (const IncidentRow& i : r.incidents.rows) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); widgets::state_cell(i.severity);
                ImGui::TableNextColumn(); ImGui::TextUnformatted(i.component.c_str());
                ImGui::TableNextColumn(); widgets::state_cell(i.state);
                ImGui::TableNextColumn(); ImGui::TextUnformatted(i.recovery_action.c_str());
                ImGui::TableNextColumn(); ImGui::TextWrapped("%s", i.message.c_str());
            }
            ImGui::EndTable();
        }
    }
    widgets::card_end();
}

// ---- Schedule --------------------------------------------------------------

inline void draw_schedule(const ControlCenterReport& r) {
    widgets::section_header("Schedule", "Human-bounded operating window (cannot self-extend)");
    widgets::card_begin("sch_card", "OPERATING WINDOW", ImVec2(0.0f, 0.0f), 180.0f);
    if (!r.schedule.available) {
        widgets::empty_state("Operating window", r.schedule.note.c_str());
        ImGui::Spacing();
        ImGui::TextWrapped(
            "This view is not wired to an accepted schedule yet, so it is shown NOT AVAILABLE "
            "rather than as an invented ACTIVE window.");
    }
    widgets::card_end();
}

// ---- Recovery / Checkpoints -----------------------------------------------

inline void draw_checkpoints(const ControlCenterReport& r) {
    widgets::section_header("Recovery", "Persistence and crash-recovery decision (report-only)");
    const PersistencePanel& s = r.snapshot.persistence;

    widgets::card_begin("rec_card", "RECOVERY DECISION", ImVec2(0.0f, 0.0f), 220.0f);
    widgets::badge(s.lifecycle.c_str(), theme::status_color(s.lifecycle));
    ImGui::Spacing();
    widgets::kv_row("Store", s.store_path);
    widgets::kv_row("Persisted records", std::to_string(s.records));
    widgets::kv_state_row("Last persist status", s.last_status);
    widgets::kv_row("Resumable", s.resumable ? "YES" : "NO");
    widgets::kv_row("Fresh start", s.fresh_start ? "yes" : "no");
    widgets::kv_row("Known-good", s.used_known_good ? "yes" : "no");
    ImGui::Spacing();
    ImGui::TextWrapped("Reason: %s", s.reason.c_str());
    if (s.lifecycle == "CORRUPTED_STATE") {
        ImGui::Spacing();
        ImGui::TextColored(theme::kCritical,
                           "Recovery is REFUSED for corrupted state; the GUI will not resume it.");
    }
    widgets::card_end();

    ImGui::Spacing();
    widgets::card_begin("cp_card", "OPERATING-WINDOW CHECKPOINTS", ImVec2(0.0f, 0.0f), 260.0f);
    if (!r.checkpoints.available) {
        widgets::empty_state("Operating-window checkpoints",
                             "no operating-window checkpoint source is wired");
    } else {
        widgets::kv_row("Checkpoints", std::to_string(r.checkpoints.total));
        ImGui::Spacing();
        if (ImGui::BeginTable("cp_table", 3, widgets::table_flags())) {
            ImGui::TableSetupColumn("Checkpoint");
            ImGui::TableSetupColumn("Validity");
            ImGui::TableSetupColumn("Schema");
            ImGui::TableHeadersRow();
            for (const CheckpointRow& c : r.checkpoints.recent) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::TextUnformatted(c.checkpoint_id.c_str());
                ImGui::TableNextColumn(); widgets::state_cell(c.validity);
                ImGui::TableNextColumn(); ImGui::TextUnformatted(c.schema_version.c_str());
            }
            ImGui::EndTable();
        }
    }
    widgets::card_end();
}

// ---- Health / Watchdog -----------------------------------------------------

inline void draw_health(const ControlCenterSnapshot& s) {
    widgets::section_header("Health", "Runtime health and watchdog status");
    widgets::card_begin("hl_card", "RUNTIME HEALTH", ImVec2(0.0f, 0.0f), 180.0f);
    widgets::badge(s.health.healthy ? "HEALTHY" : s.health.aggregate.c_str(),
                   theme::status_color(s.health.healthy ? "HEALTHY" : s.health.aggregate));
    ImGui::Spacing();
    widgets::kv_row("Aggregate", s.health.aggregate);
    widgets::kv_row("Streams", std::to_string(s.overview.streams_healthy) + " / " +
                                   std::to_string(s.overview.streams_total));
    ImGui::Spacing();
    widgets::empty_state("Watchdog", "no watchdog telemetry source is wired to this view");
    widgets::card_end();
}

// ---- Audit -----------------------------------------------------------------

inline void draw_audit(const ControlCenterReport& r) {
    widgets::section_header("Audit", "Append-only history (never edited or deleted)");
    widgets::card_begin("aud_card", "AUDIT LEDGER", ImVec2(0.0f, 0.0f), 340.0f);
    if (!r.audit.available) {
        widgets::empty_note("No audit record has been appended in this session.");
    } else {
        widgets::kv_row("Records", std::to_string(r.audit.total));
        ImGui::Spacing();
        if (ImGui::BeginTable("audit_table", 4, widgets::table_flags())) {
            ImGui::TableSetupColumn("Category");
            ImGui::TableSetupColumn("Actor");
            ImGui::TableSetupColumn("Action");
            ImGui::TableSetupColumn("Recorded at (ns)");
            ImGui::TableHeadersRow();
            for (const AuditRow& a : r.audit.rows) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::TextUnformatted(a.category.c_str());
                ImGui::TableNextColumn(); ImGui::TextUnformatted(a.actor.c_str());
                ImGui::TableNextColumn(); ImGui::TextUnformatted(a.action.c_str());
                ImGui::TableNextColumn(); ImGui::TextUnformatted(a.recorded_at.c_str());
            }
            ImGui::EndTable();
        }
    }
    widgets::card_end();
}

// ---- Configuration / Version ----------------------------------------------

inline void draw_version(const ControlCenterReport& r) {
    widgets::section_header("Configuration", "Version identity verified before any resume");
    widgets::card_begin("ver_card", "VERSION CONTEXT", ImVec2(0.0f, 0.0f), 220.0f);
    widgets::kv_row("Schema version", r.version.schema_version);
    widgets::kv_row("Strategy version", r.version.strategy_version);
    widgets::kv_row("Configuration version", r.version.configuration_version);
    widgets::kv_row("Strategy family", r.version.strategy_family);
    widgets::kv_row("Symbol", r.version.symbol);
    ImGui::Spacing();
    ImGui::TextWrapped("An incompatible persisted state is refused rather than mis-resumed.");
    widgets::card_end();
}

// Draws the content area for the selected section index (see DesktopModel::sections()).
inline void draw_section(ControlCenterState& state, const ControlCenterReport& r, int section) {
    switch (section) {
        case 0:  draw_dashboard(r, state); break;
        case 1:  draw_market(r.snapshot); break;
        case 2:  draw_timeframes(r.snapshot); break;
        case 3:  draw_signals(r.snapshot); break;
        case 4:  draw_risk(r.snapshot); break;
        case 5:  draw_positions(r.snapshot); break;
        case 6:  draw_prediction(r); break;
        case 7:  draw_research(r); break;
        case 8:  draw_knowledge(r); break;
        case 9:  draw_candidates(r); break;
        case 10: draw_validation(r); break;
        case 11: draw_approvals(r); break;
        case 12: draw_evolution(r); break;
        case 13: draw_incidents(r); break;
        case 14: draw_schedule(r); break;
        case 15: draw_checkpoints(r); break;
        case 16: draw_health(r.snapshot); break;
        case 17: draw_audit(r); break;
        case 18: draw_version(r); break;
        default: {
            widgets::section_header("Section", "Master (V3-37) control-center area");
            widgets::card_begin("def_card", nullptr, ImVec2(0.0f, 0.0f), 140.0f);
            widgets::empty_state(
                "This section",
                "is defined by the Master (V3-37) but is not yet backed by a wired read-only "
                "adapter. It is shown as NOT AVAILABLE rather than with fabricated values.");
            widgets::card_end();
            break;
        }
    }
}

}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_GUIPANELS_H
