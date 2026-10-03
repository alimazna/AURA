#ifndef AURA_DESKTOP_GUIPANELS_H
#define AURA_DESKTOP_GUIPANELS_H

#include "desktop/ControlCenterState.h"
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

// A bounded "nothing to show" note with an explanation, used by wired sections
// whose real source is currently empty (a genuine empty state, not missing data).
inline void empty_note(const char* detail) {
    ImGui::TextColored(ImVec4(0.62f, 0.62f, 0.66f, 1.0f), "%s", detail);
}

inline void section_title(const char* title) {
    ImGui::TextColored(ImVec4(0.85f, 0.87f, 0.95f, 1.0f), "%s", title);
    ImGui::Separator();
}

inline void draw_prediction(const ControlCenterReport& r) {
    section_title("Prediction / Observation");
    if (!r.predictions.available) {
        not_available_note("Prediction ledger");
        ImGui::TextWrapped("%s", r.predictions.note.c_str());
        return;
    }
    ImGui::Text("Predictions: %zu", r.predictions.total);
    if (ImGui::BeginTable("pred_table", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Timeframe");
        ImGui::TableSetupColumn("Direction");
        ImGui::TableSetupColumn("Symbol");
        ImGui::TableSetupColumn("Score (ranking, not probability)");
        ImGui::TableSetupColumn("Reference");
        ImGui::TableSetupColumn("Predicted at (ns)");
        ImGui::TableHeadersRow();
        for (const PredictionRow& p : r.predictions.rows) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(p.timeframe.c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(p.direction.c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(p.symbol.c_str());
            ImGui::TableNextColumn(); ImGui::Text("%.4f", p.score);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(p.reference_price.c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(p.predicted_at.c_str());
        }
        ImGui::EndTable();
    }
}

inline void draw_shadow_ledger(const ControlCenterReport& r) {
    section_title("Shadow Ledger (append-only)");
    ImGui::TextWrapped(
        "The persistent shadow ledger is the historical source of truth (V3-22). Entries are "
        "appended and never mutated, reordered or erased.");
    ImGui::Text("Entries: %zu   append-only: %s", r.shadow_ledger.total,
                r.shadow_ledger.append_only ? "yes" : "no");
    if (!r.shadow_ledger.available) {
        empty_note("No ledger entries yet (a shadow session has not produced any).");
        return;
    }
    if (ImGui::BeginTable("ledger_table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                                   ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Decision id");
        ImGui::TableSetupColumn("Recorded at (ns)");
        ImGui::TableSetupColumn("Payload");
        ImGui::TableHeadersRow();
        for (const LedgerEntryRow& e : r.shadow_ledger.recent) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(e.type.c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(e.decision_id.c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(e.recorded_at.c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(e.payload.c_str());
        }
        ImGui::EndTable();
    }
}

inline void draw_research(const ControlCenterReport& r) {
    section_title("Research");
    if (!r.research.available) {
        not_available_note("Experiment ledger");
        ImGui::TextWrapped("No experiment has been recorded in this session.");
        return;
    }
    ImGui::Text("Experiments: %zu", r.research.total);
    if (ImGui::BeginTable("exp_table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Question");
        ImGui::TableSetupColumn("Decision");
        ImGui::TableSetupColumn("Evidence zone");
        ImGui::TableSetupColumn("Contamination");
        ImGui::TableHeadersRow();
        for (const ResearchRow& e : r.research.rows) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextWrapped("%s", e.question.c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(e.decision.c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(e.evidence_zone.c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(e.contamination.c_str());
        }
        ImGui::EndTable();
    }
}

inline void draw_knowledge(const ControlCenterReport& r) {
    section_title("Knowledge");
    if (!r.knowledge.available) {
        not_available_note("Knowledge store");
        ImGui::TextWrapped("No knowledge revision has been recorded in this session.");
        return;
    }
    ImGui::Text("Identities: %zu   revisions: %zu", r.knowledge.identities, r.knowledge.revisions);
    if (ImGui::BeginTable("kn_table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("Rev");
        ImGui::TableSetupColumn("Scope");
        ImGui::TableSetupColumn("Observation");
        ImGui::TableHeadersRow();
        for (const KnowledgeRow& k : r.knowledge.rows) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(status_color(k.status), "%s", k.status.c_str());
            ImGui::TableNextColumn(); ImGui::Text("%u", (unsigned)k.revision);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(k.validity_scope.c_str());
            ImGui::TableNextColumn(); ImGui::TextWrapped("%s", k.observation.c_str());
        }
        ImGui::EndTable();
    }
}

inline void draw_candidates(const ControlCenterReport& r) {
    section_title("Candidates");
    if (!r.candidates.available) {
        not_available_note("Candidate registry");
        ImGui::TextWrapped("No candidate has been registered in this session.");
        return;
    }
    ImGui::Text("Population: %zu", r.candidates.population);
    if (ImGui::BeginTable("cand_table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
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
            ImGui::TableNextColumn();
            ImGui::TextColored(status_color(c.state), "%s", c.state.c_str());
        }
        ImGui::EndTable();
    }
}

inline void draw_validation(const ControlCenterReport& r) {
    section_title("Validation Evidence");
    if (!r.validation.available) {
        not_available_note("Validation evidence");
        ImGui::TextWrapped(
            "No validation campaign source is wired. A missing campaign is NOT a pass "
            "(Phase 6: absent evidence is never treated as validation). No profitability or "
            "calibration claim is made here.");
        return;
    }
}

inline void draw_approvals(const ControlCenterReport& r) {
    section_title("Approval Center");
    ImGui::TextWrapped(
        "Human decisions are recorded immutably; automated actors cannot record an accepted "
        "human decision (Phase 7).");
    if (!r.approvals.available) {
        empty_note("No human decision has been recorded in this session.");
        return;
    }
    ImGui::Text("Decisions: %zu", r.approvals.total);
    if (ImGui::BeginTable("appr_table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Decision");
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("Actor");
        ImGui::TableSetupColumn("Question");
        ImGui::TableHeadersRow();
        for (const ApprovalRow& a : r.approvals.rows) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(a.decision_id.c_str());
            ImGui::TableNextColumn();
            ImGui::TextColored(status_color(a.status), "%s", a.status.c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(a.actor.c_str());
            ImGui::TableNextColumn(); ImGui::TextWrapped("%s", a.question.c_str());
        }
        ImGui::EndTable();
    }
}

inline void draw_evolution(const ControlCenterReport& r) {
    section_title("Evolution Graph");
    if (!r.evolution.available) {
        not_available_note("Evolution graph");
        ImGui::TextWrapped("No version lineage has been recorded in this session.");
        return;
    }
    ImGui::Text("Nodes: %zu   edges: %zu", r.evolution.nodes, r.evolution.edges.size());
    for (const EvolutionEdgeRow& e : r.evolution.edges) {
        ImGui::Bullet();
        ImGui::Text("%s -> %s", e.parent_version.c_str(), e.child_version.c_str());
    }
}

inline void draw_schedule(const ControlCenterReport& r) {
    section_title("Schedule / Operating Window");
    if (!r.schedule.available) {
        not_available_note("Operating window");
        ImGui::TextWrapped("%s", r.schedule.note.c_str());
        ImGui::TextWrapped(
            "The window is human-bounded and cannot self-extend (Phase 8). This view is not "
            "wired to an accepted schedule yet, so it is shown NOT AVAILABLE rather than as an "
            "invented ACTIVE window.");
        return;
    }
}

inline void draw_incidents(const ControlCenterReport& r) {
    section_title("Incidents");
    if (!r.incidents.available) {
        not_available_note("Incident source");
        ImGui::TextWrapped("No failure detection source is wired.");
        return;
    }
    ImGui::Text("Detected incidents: %zu", r.incidents.count);
    if (r.incidents.rows.empty()) {
        empty_note("No incidents detected from the current adapter stream health.");
        return;
    }
    if (ImGui::BeginTable("inc_table", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Severity");
        ImGui::TableSetupColumn("Component");
        ImGui::TableSetupColumn("State");
        ImGui::TableSetupColumn("Recovery");
        ImGui::TableSetupColumn("Message");
        ImGui::TableHeadersRow();
        for (const IncidentRow& i : r.incidents.rows) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(status_color(i.severity), "%s", i.severity.c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(i.component.c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(i.state.c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(i.recovery_action.c_str());
            ImGui::TableNextColumn(); ImGui::TextWrapped("%s", i.message.c_str());
        }
        ImGui::EndTable();
    }
}

inline void draw_checkpoints(const ControlCenterReport& r) {
    section_title("Checkpoints / Recovery");
    // Persistence/recovery decision (V2-36) — always sourced from the live shell.
    const PersistencePanel& s = r.snapshot.persistence;
    kv("Store", s.store_path);
    ImGui::Text("Persisted records: %zu", s.records);
    labelled_status("Last persist status:", s.last_status);
    labelled_status("Lifecycle state:", s.lifecycle);
    labelled_status("Resumable:", s.resumable ? "YES" : "NO");
    ImGui::Text("Fresh start: %s   Known-good: %s", s.fresh_start ? "yes" : "no",
                s.used_known_good ? "yes" : "no");
    ImGui::TextWrapped("Reason: %s", s.reason.c_str());
    if (s.lifecycle == "CORRUPTED_STATE") {
        ImGui::TextColored(status_color("CORRUPTED_STATE"),
                           "Recovery is REFUSED for corrupted state; the GUI will not resume it.");
    }
    ImGui::Spacing();
    ImGui::TextUnformatted("Operating-window checkpoints:");
    if (!r.checkpoints.available) {
        ImGui::TextColored(ImVec4(0.62f, 0.62f, 0.66f, 1.0f),
                           "NOT AVAILABLE (no operating-window checkpoint source wired)");
        return;
    }
    ImGui::Text("Checkpoints: %zu", r.checkpoints.total);
    if (ImGui::BeginTable("cp_table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Checkpoint");
        ImGui::TableSetupColumn("Validity");
        ImGui::TableSetupColumn("Schema");
        ImGui::TableHeadersRow();
        for (const CheckpointRow& c : r.checkpoints.recent) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(c.checkpoint_id.c_str());
            ImGui::TableNextColumn();
            ImGui::TextColored(status_color(c.validity), "%s", c.validity.c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(c.schema_version.c_str());
        }
        ImGui::EndTable();
    }
}

inline void draw_audit(const ControlCenterReport& r) {
    section_title("Audit");
    ImGui::TextWrapped("The audit ledger is append-only; history is never edited or deleted.");
    if (!r.audit.available) {
        empty_note("No audit record has been appended in this session.");
        return;
    }
    ImGui::Text("Records: %zu", r.audit.total);
    if (ImGui::BeginTable("audit_table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Category"); ImGui::TableSetupColumn("Actor");
        ImGui::TableSetupColumn("Action"); ImGui::TableSetupColumn("Recorded at (ns)");
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

inline void draw_version(const ControlCenterReport& r) {
    section_title("Configuration / Version Context");
    kv("Schema version", r.version.schema_version);
    kv("Strategy version", r.version.strategy_version);
    kv("Configuration version", r.version.configuration_version);
    kv("Strategy family", r.version.strategy_family);
    kv("Symbol", r.version.symbol);
    ImGui::TextWrapped(
        "Version identity is verified before any resume; an incompatible persisted state is "
        "refused rather than mis-resumed.");
}

// Draws the content area for the selected section index (see DesktopModel::sections()).
inline void draw_section(const ControlCenterReport& r, int section) {
    switch (section) {
        case 0:  draw_overview(r.snapshot); break;
        case 1:  draw_data_health(r.snapshot); break;
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
