#ifndef AURA_DESKTOP_GUIPANELS_H
#define AURA_DESKTOP_GUIPANELS_H

#include "desktop/DesktopModel.h"

#include <imgui.h>

#include <cstddef>
#include <string>

namespace aura {
namespace desktop {

// ImGui rendering for the AURA control center. This translation unit contains
// presentation only: it reads a pre-built ControlCenterSnapshot (produced by
// DesktopModel) and draws it. It queries no runtime directly and mutates no
// state. Unknown/absent values are shown explicitly (NOT AVAILABLE / UNKNOWN)
// so a blank field is never mistaken for a healthy one.

inline ImVec4 status_color(const std::string& state) {
    if (state == "ONLINE" || state == "VALID" || state == "HEALTHY" || state == "OK")
        return ImVec4(0.35f, 0.78f, 0.42f, 1.0f);
    if (state == "DEGRADED" || state == "WARNING" || state == "STALE" || state == "RECOVERING")
        return ImVec4(0.92f, 0.75f, 0.30f, 1.0f);
    if (state == "OFFLINE" || state == "ERROR" || state == "CRITICAL" || state == "BLOCKED" ||
        state == "CORRUPTED_STATE" || state == "INVALID")
        return ImVec4(0.90f, 0.38f, 0.38f, 1.0f);
    // UNKNOWN / NOT AVAILABLE and anything unrecognised stay neutral grey.
    return ImVec4(0.62f, 0.62f, 0.66f, 1.0f);
}

inline void labelled_status(const char* label, const std::string& value) {
    ImGui::TextUnformatted(label);
    ImGui::SameLine();
    ImGui::TextColored(status_color(value), "%s", value.c_str());
}

inline void kv(const char* key, const std::string& value) {
    ImGui::TextUnformatted(key);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.85f, 0.87f, 0.92f, 1.0f), "%s", value.c_str());
}

inline void not_available_note(const char* what) {
    ImGui::TextColored(ImVec4(0.62f, 0.62f, 0.66f, 1.0f), "%s: NOT AVAILABLE (no data source wired yet)",
                       what);
}

inline void draw_overview(const ControlCenterSnapshot& s) {
    ImGui::TextColored(ImVec4(0.85f, 0.87f, 0.95f, 1.0f), "System Overview");
    ImGui::Separator();
    kv("Project", s.overview.project_name);
    kv("Operating mode", s.overview.operating_mode + "  (no live order path)");
    labelled_status("Aggregate service state:", s.overview.aggregate);
    labelled_status("Transport:", s.overview.transport_connected ? "CONNECTED" : "UNKNOWN");
    ImGui::Text("Streams healthy: %zu / %zu", s.overview.streams_healthy, s.overview.streams_total);
    ImGui::Text("Bars accepted: %llu   rejected: %llu   malformed: %llu",
                (unsigned long long)s.overview.accepted, (unsigned long long)s.overview.rejected,
                (unsigned long long)s.overview.malformed);
    if (s.overview.streams_total == 0) {
        ImGui::Spacing();
        not_available_note("Runtime streams");
        ImGui::TextWrapped(
            "The runtime has not been started with a transport yet. Start a shadow session "
            "(a replay or the serve transport) to populate live state.");
    }
}

inline void draw_timeframes(const ControlCenterSnapshot& s) {
    ImGui::TextColored(ImVec4(0.85f, 0.87f, 0.95f, 1.0f), "Timeframes (nine logical streams)");
    ImGui::Separator();
    ImGui::TextWrapped(
        "Identity is the explicit timeframe value, never the row position. A silent stream is "
        "shown NOT AVAILABLE and never merged into an aggregate 'healthy' status.");
    ImGui::Spacing();

    const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollX;
    if (ImGui::BeginTable("tf_table", 9, flags)) {
        ImGui::TableSetupColumn("TF");
        ImGui::TableSetupColumn("Present");
        ImGui::TableSetupColumn("Service");
        ImGui::TableSetupColumn("Quality");
        ImGui::TableSetupColumn("Accepted");
        ImGui::TableSetupColumn("Rejected");
        ImGui::TableSetupColumn("Last close (ns)");
        ImGui::TableSetupColumn("Last bar O/H/L/C");
        ImGui::TableSetupColumn("Provenance");
        ImGui::TableHeadersRow();

        for (const TimeframeRow& r : s.timeframes.rows) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(r.label.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(r.present ? "yes" : "NO");
            ImGui::TableNextColumn();
            ImGui::TextColored(status_color(r.service_state), "%s", r.service_state.c_str());
            ImGui::TableNextColumn();
            ImGui::TextColored(status_color(r.quality), "%s", r.quality.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%llu", (unsigned long long)r.accepted);
            ImGui::TableNextColumn();
            ImGui::Text("%llu", (unsigned long long)r.rejected);
            ImGui::TableNextColumn();
            if (r.last_close.available) ImGui::TextUnformatted(r.last_close.value.c_str());
            else ImGui::TextColored(status_color("NA"), "NOT AVAILABLE");
            ImGui::TableNextColumn();
            if (r.last_bar_ohlc.available) ImGui::TextUnformatted(r.last_bar_ohlc.value.c_str());
            else ImGui::TextColored(status_color("NA"), "NOT AVAILABLE");
            ImGui::TableNextColumn();
            if (r.provenance.available) ImGui::TextUnformatted(r.provenance.value.c_str());
            else ImGui::TextColored(status_color("NA"), "NOT AVAILABLE");
        }
        ImGui::EndTable();
    }
}

inline void draw_signals(const ControlCenterSnapshot& s) {
    ImGui::TextColored(ImVec4(0.85f, 0.87f, 0.95f, 1.0f), "Signals");
    ImGui::Separator();
    if (!s.signal.available) {
        not_available_note("Signals");
        ImGui::TextWrapped("No signal has been produced by the runtime yet.");
        return;
    }
    kv("Direction", s.signal.direction);
    kv("Strategy family", s.signal.family);
    kv("Symbol", s.signal.symbol);
    kv("Trigger timeframe", s.signal.trigger_timeframe);
    kv("Decision id", s.signal.decision_id);
}

inline void draw_risk(const ControlCenterSnapshot& s) {
    ImGui::TextColored(ImVec4(0.85f, 0.87f, 0.95f, 1.0f), "Risk");
    ImGui::Separator();
    ImGui::TextWrapped(
        "A risk proposal is a shadow-only sizing decision. It is never an order, and no control "
        "on this surface can place one.");
    ImGui::Spacing();
    if (!s.risk.available) {
        not_available_note("Risk proposal");
        return;
    }
    kv("Direction", s.risk.direction);
    ImGui::Text("Position size: %.4f", s.risk.position_size);
    ImGui::Text("Stop distance: %.5f", s.risk.stop_distance);
    ImGui::Text("ATR: %.5f", s.risk.atr);
    labelled_status("Is live order:", s.risk.is_order ? "YES (UNEXPECTED)" : "NO");
}

inline void draw_positions(const ControlCenterSnapshot& s) {
    ImGui::TextColored(ImVec4(0.85f, 0.87f, 0.95f, 1.0f), "Shadow Positions");
    ImGui::Separator();
    labelled_status("Open shadow position:", s.positions.has_open_position ? "OPEN" : "NONE");
    ImGui::Text("Shadow fills: %llu", (unsigned long long)s.positions.shadow_fills);
    ImGui::Text("Positions opened: %llu", (unsigned long long)s.positions.positions_opened);
    ImGui::Text("Append-only ledger entries: %zu", s.positions.ledger_entries);
}

inline void draw_persistence(const ControlCenterSnapshot& s) {
    ImGui::TextColored(ImVec4(0.85f, 0.87f, 0.95f, 1.0f), "Checkpoints / Recovery");
    ImGui::Separator();
    kv("Store", s.persistence.store_path);
    ImGui::Text("Persisted records: %zu", s.persistence.records);
    labelled_status("Last persist status:", s.persistence.last_status);
    labelled_status("Lifecycle state:", s.persistence.lifecycle);
    labelled_status("Resumable:", s.persistence.resumable ? "YES" : "NO");
    ImGui::Text("Fresh start: %s   Known-good: %s", s.persistence.fresh_start ? "yes" : "no",
                s.persistence.used_known_good ? "yes" : "no");
    ImGui::TextWrapped("Reason: %s", s.persistence.reason.c_str());
    if (s.persistence.lifecycle == "CORRUPTED_STATE") {
        ImGui::TextColored(status_color("CORRUPTED_STATE"),
                           "Recovery is REFUSED for corrupted state; the GUI will not resume it.");
    }
}

inline void draw_health(const ControlCenterSnapshot& s) {
    ImGui::TextColored(ImVec4(0.85f, 0.87f, 0.95f, 1.0f), "Health / Watchdog");
    ImGui::Separator();
    labelled_status("Runtime health:", s.health.healthy ? "HEALTHY" : s.health.aggregate);
    ImGui::Spacing();
    not_available_note("Watchdog");
}

inline void draw_data_health(const ControlCenterSnapshot& s) {
    ImGui::TextColored(ImVec4(0.85f, 0.87f, 0.95f, 1.0f), "Market / Data Health");
    ImGui::Separator();
    labelled_status("Transport:", s.overview.transport_connected ? "CONNECTED" : "UNKNOWN");
    ImGui::Text("Streams healthy: %zu / %zu", s.overview.streams_healthy, s.overview.streams_total);
    ImGui::Spacing();
    ImGui::TextUnformatted("Per-stream data quality:");
    for (const TimeframeRow& r : s.timeframes.rows) {
        ImGui::Bullet();
        ImGui::TextUnformatted(r.label.c_str());
        ImGui::SameLine();
        ImGui::TextColored(status_color(r.quality), "%s", r.quality.c_str());
    }
}

// Sections whose data source is not yet wired: shown as NOT AVAILABLE and
// bounded, rather than fabricated. This preserves the "unknown stays unknown"
// contract while the corresponding adapters are implemented.
inline void draw_placeholder(const char* title, const char* detail) {
    ImGui::TextColored(ImVec4(0.85f, 0.87f, 0.95f, 1.0f), "%s", title);
    ImGui::Separator();
    not_available_note(title);
    ImGui::Spacing();
    ImGui::TextWrapped("%s", detail);
}

// Draws the content area for the selected section index (see DesktopModel::sections()).
inline void draw_section(const ControlCenterSnapshot& s, int section) {
    switch (section) {
        case 0:  draw_overview(s); break;
        case 1:  draw_data_health(s); break;
        case 2:  draw_timeframes(s); break;
        case 3:  draw_signals(s); break;
        case 4:  draw_risk(s); break;
        case 5:  draw_positions(s); break;
        case 15: draw_persistence(s); break;
        case 16: draw_health(s); break;
        case 18:
            draw_placeholder("Configuration / Version",
                             "Version identity is carried by the persisted manifest. The "
                             "configuration/version context panel is not yet wired to a view model.");
            break;
        default:
            draw_placeholder(
                DesktopModel::sections().at(static_cast<std::size_t>(section)).c_str(),
                "This control-center area is defined by the Master (V3-37) but is not yet "
                "backed by a wired read-only adapter. It is shown as NOT AVAILABLE rather than "
                "with fabricated values.");
            break;
    }
}

}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_GUIPANELS_H
