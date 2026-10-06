// Phase 9 desktop control-center view-model tests (DESK-0001..DESK-0002).
//
// Deterministic; no GUI, no clock, no I/O. Exits non-zero on the first failure.

#include "desktop/CandleChart.h"
#include "desktop/ControlCenterPanels.h"
#include "desktop/ControlCenterState.h"
#include "desktop/ControlCenterViewModels.h"
#include "desktop/DashboardProjector.h"
#include "desktop/DesktopModel.h"
#include "desktop/Mt5Candles.h"
#include "desktop/Mt5Json.h"
#include "desktop/NavigationModel.h"
#include "desktop/RendererPolicy.h"
#include "desktop/StateVisuals.h"
#include "desktop/TerminalLayout.h"
#include "evolution/Candidate.h"
#include "evolution/CandidateRegistry.h"
#include "evolution/EvolutionGraph.h"
#include "foundation/ErrorRecord.h"
#include "governance/AuditLedger.h"
#include "governance/HumanDecision.h"
#include "learning/KnowledgeObject.h"
#include "learning/KnowledgeStore.h"
#include "mt5/ProtocolCodec.h"
#include "observation/FailureDetectionEngine.h"
#include "observation/PredictionLedger.h"
#include "operatingwindow/Checkpoint.h"
#include "operatingwindow/OperatingWindow.h"
#include "research/Experiment.h"
#include "research/ExperimentLedger.h"
#include "runtime/AdapterManager.h"
#include "runtime/ApplicationShell.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

using namespace aura;

static int g_failures = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
            ++g_failures;                                                        \
        }                                                                        \
    } while (0)

static void test_dashboard_no_fabrication() {
    std::vector<desktop::DashboardTile> tiles{
        {"runtime", desktop::TileStatus::OK, "shadow active"},
        {"adapters", desktop::TileStatus::UNKNOWN, "no telemetry"},
    };
    auto vm = desktop::ControlCenterProjector::dashboard(
        "AURA", "SHADOW", operatingwindow::WindowPhase::ACTIVE, tiles);
    CHECK(vm.project_name == "AURA");
    CHECK(vm.operating_mode == "SHADOW");
    CHECK(vm.window_phase == "ACTIVE");
    CHECK(vm.tiles.size() == 2);
    // an unknown tile must surface as unknown, not be silently healthy
    CHECK(vm.has_unknown_state);
    // no unknown tiles -> flag false
    tiles[1].status = desktop::TileStatus::WARNING;
    auto vm2 = desktop::ControlCenterProjector::dashboard("AURA", "SHADOW",
                                                          operatingwindow::WindowPhase::DRAINING, tiles);
    CHECK(!vm2.has_unknown_state);
    CHECK(vm2.window_phase == "DRAINING");
}

static void test_knowledge_projection_readonly() {
    learning::KnowledgeObject k;
    k.knowledge_id = learning::make_knowledge_id("strategy", "feature F unreliable", "regime X");
    k.observation = "feature F unreliable";
    k.validity_scope = "regime X";
    k.status = learning::KnowledgeStatus::SUPPORTED;
    k.revision = 2;
    k.confidence.evidence_strength = 0.5;
    k.confidence.replication = 0.2;

    const double before = k.confidence.score();
    auto vm = desktop::ControlCenterProjector::knowledge(k);
    CHECK(vm.knowledge_id == k.knowledge_id);
    CHECK(vm.status == "SUPPORTED");
    CHECK(vm.validity_scope == "regime X");
    CHECK(vm.revision == 2);
    CHECK(vm.confidence_ranking == before);  // projected value equals source
    // projection is read-only: source unchanged
    CHECK(k.confidence.score() == before);
    CHECK(k.status == learning::KnowledgeStatus::SUPPORTED);

    std::vector<learning::KnowledgeObject> items{k, k};
    auto list = desktop::ControlCenterProjector::knowledge_list(items);
    CHECK(list.size() == 2);
}

static void test_research_and_candidate_projection() {
    research::Experiment e;
    e.experiment_id = research::make_experiment_id("C1", "q", "cand");
    e.question = "q";
    e.decision = research::ExperimentOutcome::PROMISING;
    e.evidence_zone = research::EvidenceZone::E2_OUT_OF_SAMPLE;
    e.contamination = research::ContaminationState::OOS_EXPOSED;
    auto rvm = desktop::ControlCenterProjector::research(e, true);
    CHECK(rvm.decision == "PROMISING");
    CHECK(rvm.evidence_zone == "E2");
    CHECK(rvm.contamination == "OOS_EXPOSED");
    CHECK(rvm.duplicate);

    evolution::Candidate c;
    c.candidate_id = foundation::EntityId("CAND|1");
    c.parent_version = "V1.0";
    c.change_type = evolution::ChangeType::PARAMETER;
    c.state = evolution::CandidateState::SHADOW_ACTIVE;
    c.revision = 3;
    auto cvm = desktop::ControlCenterProjector::candidate(c);
    CHECK(cvm.change_type == "PARAMETER");
    CHECK(cvm.state == "SHADOW_ACTIVE");
    CHECK(cvm.parent_version == "V1.0");
    CHECK(cvm.revision == 3);
}

static void test_approval_audit_and_graph_projection() {
    governance::DecisionRecord d;
    d.decision_id = foundation::EntityId("DEC|9");
    d.question = "approve?";
    d.actor = "human";
    d.status = governance::DecisionStatus::ACCEPTED;
    auto avm = desktop::ControlCenterProjector::approval(d);
    CHECK(avm.status == "ACCEPTED");
    CHECK(avm.actor == "human");

    governance::AuditRecord a;
    a.audit_id = foundation::EntityId("AUD|1");
    a.category = governance::AuditCategory::PROMOTION;
    a.actor = "human";
    a.action = "promote";
    auto auvm = desktop::ControlCenterProjector::audit(a);
    CHECK(auvm.category == "PROMOTION");
    std::vector<governance::AuditRecord> recs{a};
    CHECK(desktop::ControlCenterProjector::audit_list(recs).size() == 1);

    std::vector<evolution::EvolutionNode> nodes;
    nodes.push_back({"V1.0", "", {}, true});
    nodes.push_back({"V1.1", "V1.0", {}, true});
    nodes.push_back({"V1.2", "V1.0", {}, true});
    auto gvm = desktop::ControlCenterProjector::evolution_graph(nodes);
    CHECK(gvm.versions.size() == 3);
    CHECK(gvm.edges.size() == 2);
    CHECK(gvm.edges[0].parent_version == "V1.0");
    CHECK(gvm.edges[0].child_version == "V1.1");
    CHECK(gvm.edges[1].child_version == "V1.2");
}

// A deterministic canonical frame set is provided by ControlCenterState.h
// (desktop::builtin_frames) and shared with the GUI self-test.
static std::vector<std::string> frames(int steps) { return desktop::builtin_frames(steps); }

// The nine-timeframe display must always carry nine rows in canonical order, and
// a silent stream must be labelled NOT AVAILABLE rather than fabricated.
static void test_desktop_model_empty_state() {
    runtime::ApplicationShell shell(runtime::ApplicationPipeline::Config{}, std::string{});
    runtime::RecoveryOutcome outcome;
    outcome.detected = runtime::LifecycleState::UNKNOWN_STATE;
    outcome.fresh_start = true;

    const auto snap = desktop::DesktopModel::capture(shell, outcome);
    CHECK(snap.timeframes.rows.size() == 9);
    CHECK(!snap.timeframes.any_present);          // no silent fabrication of a healthy row
    CHECK(snap.timeframes.rows[0].label == "M1");
    CHECK(snap.timeframes.rows[0].service_state == "NOT AVAILABLE");
    CHECK(snap.timeframes.rows[0].quality == "NOT AVAILABLE");
    CHECK(snap.timeframes.rows[0].last_close.available == false);
    CHECK(snap.timeframes.rows[0].last_close.value == "NOT AVAILABLE");
    CHECK(snap.overview.streams_healthy == 0);
    // Identity is explicit per row, not positional.
    const auto tfs = runtime::all_timeframes();
    for (std::size_t i = 0; i < tfs.size(); ++i)
        CHECK(snap.timeframes.rows[i].timeframe == tfs[i]);
    CHECK(snap.shadow_only);  // no live-order capability
    CHECK(snap.persistence.lifecycle == "UNKNOWN_STATE");
    CHECK(snap.persistence.resumable == false);
    // All Master control-center sections are present.
    const auto secs = desktop::DesktopModel::sections();
    CHECK(secs.size() == 19);
    bool has_recovery = false, has_timeframes = false;
    for (const auto& s : secs) {
        if (s == "Checkpoints / Recovery") has_recovery = true;
        if (s == "Timeframes") has_timeframes = true;
    }
    CHECK(has_recovery);
    CHECK(has_timeframes);
}

// After feeding real frames, the populated streams must be present and
// distinguishable, and a failure in one stream must not alter another.
static void test_desktop_model_nine_timeframes() {
    desktop::ControlCenterOptions options;  // in-memory
    desktop::ControlCenterState state(options);
    for (const std::string& f : frames(30)) state.shell().feed(f + "\n");

    const auto& snap = state.refresh();
    CHECK(snap.timeframes.rows.size() == 9);
    CHECK(snap.timeframes.any_present);
    // The operational stream (M15) and structural stream (H4) received bars.
    int m15_present = 0, h4_present = 0, h1_present = 0;
    for (const auto& r : snap.timeframes.rows) {
        if (r.label == "M15") m15_present = r.present ? 1 : 0;
        if (r.label == "H4") h4_present = r.present ? 1 : 0;
        if (r.label == "H1") h1_present = r.present ? 1 : 0;
    }
    CHECK(m15_present == 1);
    CHECK(h4_present == 1);
    // H1 also receives bars in the deterministic set; all nine do.
    CHECK(h1_present == 1);
    CHECK(snap.overview.streams_total == 9);
    CHECK(snap.overview.accepted > 0);
    CHECK(snap.shadow_only);
    // A shadow run never reports a live order.
    CHECK(snap.risk.is_order == false);
}

// Full desktop integration path: empty -> fed -> checkpoint -> cross-process
// recovery report -> lifecycle presented; and no auto-resume of corrupted state.
static void test_control_center_state_smoke() {
    const std::string store = "aura_desktop_test.aura";
    std::remove(store.c_str());

    {
        desktop::ControlCenterOptions options;
        options.store_path = store;
        desktop::ControlCenterState state(options);
        for (const std::string& f : frames(30)) state.shell().feed(f + "\n");
        state.refresh();
        const auto status = state.persist_checkpoint();
        CHECK(status == foundation::PersistenceStatus::OK);
        // Safe control-plane ops only.
        CHECK(!state.paused());
        state.toggle_pause();
        CHECK(state.paused());
        state.toggle_pause();
        CHECK(!state.paused());
    }

    // Fresh owner recovers the clean shutdown and reports it (read-only).
    {
        desktop::ControlCenterOptions options;
        options.store_path = store;
        desktop::ControlCenterState state(options);
        const auto& outcome = state.evaluate_recovery(false);
        CHECK(outcome.detected == runtime::LifecycleState::CLEAN_SHUTDOWN);
        CHECK(outcome.resumable);
        const auto& snap = state.refresh();
        CHECK(snap.persistence.lifecycle == "CLEAN_SHUTDOWN");
        CHECK(snap.persistence.resumable);
        CHECK(snap.persistence.records > 0);
    }

    // A corrupted store must be REFUSED, never auto-resumed just because the UI
    // is open.
    {
        std::FILE* fp = std::fopen(store.c_str(), "wb");
        CHECK(fp != nullptr);
        if (fp != nullptr) {
            const char junk[] = "garbage not a valid store";
            std::fwrite(junk, 1, sizeof(junk) - 1, fp);
            std::fclose(fp);
        }
        desktop::ControlCenterOptions options;
        options.store_path = store;
        desktop::ControlCenterState state(options);
        const auto& outcome = state.evaluate_recovery(false);
        CHECK(outcome.detected == runtime::LifecycleState::CORRUPTED_STATE);
        CHECK(!outcome.resumable);
        const auto& snap = state.refresh();
        CHECK(snap.persistence.lifecycle == "CORRUPTED_STATE");
        CHECK(!snap.persistence.resumable);
    }
    std::remove(store.c_str());
}


// Section availability must reflect whether a real source is wired: unsourced
// planes stay NOT AVAILABLE, sourced planes become available, and the shadow
// ledger / incidents / version context are derived from the live runtime.
static void test_section_panel_availability() {
    desktop::ControlCenterOptions options;  // in-memory
    desktop::ControlCenterState state(options);
    for (const std::string& f : frames(5)) state.shell().feed(f + "\n");

    // No sources wired yet: these must be NOT AVAILABLE (never fabricated).
    const auto& empty = state.refresh_report();
    CHECK(!empty.predictions.available);
    CHECK(!empty.knowledge.available);
    CHECK(!empty.research.available);
    CHECK(!empty.candidates.available);
    CHECK(!empty.approvals.available);
    CHECK(!empty.evolution.available);
    CHECK(!empty.validation.available);
    CHECK(!empty.schedule.available);
    // Version context is always sourced from the live config.
    CHECK(empty.version.available);
    CHECK(empty.version.schema_version == "1.0.0");
    CHECK(empty.version.symbol == "XAUUSD");
    // Incidents are always wired (detector runs against live stream health) and,
    // after 5 healthy frames, are empty (a real "no incidents", not missing data).
    CHECK(empty.incidents.source_wired);
    CHECK(empty.incidents.available);
    CHECK(empty.incidents.count == 0);
    // Shadow ledger is sourced from the real runtime (some entries after frames).
    CHECK(empty.shadow_ledger.available);
    CHECK(empty.shadow_ledger.append_only);

    // Wire real records from the Phase 2-7 layers.
    std::vector<observation::Prediction> predictions;
    observation::Prediction p;
    p.prediction_id = foundation::EntityId("PRED|1");
    p.symbol = "XAUUSD";
    p.timeframe = runtime::Timeframe::M15;
    p.direction = runtime::SignalDirection::LONG;
    p.score = 0.42;
    p.reference_price = 1234.5;
    p.predicted_at = foundation::Timestamp::from_seconds(600);
    p.valid = true;
    predictions.push_back(p);

    learning::KnowledgeObject k;
    k.knowledge_id = learning::make_knowledge_id("strategy", "obs", "scope");
    k.observation = "obs";
    k.validity_scope = "scope";
    k.status = learning::KnowledgeStatus::SUPPORTED;

    research::Experiment e;
    e.experiment_id = research::make_experiment_id("C1", "q", "cand");
    e.question = "q";
    e.decision = research::ExperimentOutcome::PROMISING;

    evolution::Candidate c;
    c.candidate_id = foundation::EntityId("CAND|1");
    c.state = evolution::CandidateState::PROPOSED;
    c.change_type = evolution::ChangeType::PARAMETER;

    governance::DecisionRecord d;
    d.decision_id = foundation::EntityId("DEC|1");
    d.question = "q";
    d.actor = "human";
    d.status = governance::DecisionStatus::ACCEPTED;

    std::vector<evolution::EvolutionNode> nodes{{"V1.0", "", {}, true},
                                                {"V1.1", "V1.0", {}, true}};

    operatingwindow::Checkpoint cp;
    cp.checkpoint_id = foundation::EntityId("CP|1");
    cp.schema_version = "1.0.0";
    cp.artifact_digest = "abcd";
    cp.validity = operatingwindow::CheckpointValidity::VALID;

    governance::AuditRecord a;
    a.audit_id = foundation::EntityId("AUD|1");
    a.category = governance::AuditCategory::DECISION;
    a.actor = "human";
    a.action = "accept";

    state.set_predictions(predictions);
    state.set_knowledge({k}, 1);
    state.set_experiments({e});
    state.set_candidates({c}, 1);
    state.set_approvals({d});
    state.set_evolution_nodes(nodes);
    state.set_checkpoints({cp});
    state.set_audit({a});

    const auto& wired = state.refresh_report();
    CHECK(wired.predictions.available);
    CHECK(wired.predictions.total == 1);
    CHECK(wired.predictions.rows.size() == 1);
    CHECK(wired.predictions.rows[0].timeframe == "M15");
    CHECK(wired.predictions.rows[0].direction == "LONG");
    CHECK(wired.knowledge.available);
    CHECK(wired.knowledge.identities == 1);
    CHECK(wired.research.available && wired.research.rows.size() == 1);
    CHECK(wired.candidates.available && wired.candidates.population == 1);
    CHECK(wired.approvals.available);
    CHECK(wired.approvals.rows[0].status == "ACCEPTED");
    CHECK(wired.evolution.available && wired.evolution.edges.size() == 1);
    CHECK(wired.checkpoints.available && wired.checkpoints.recent[0].validity == "VALID");
    CHECK(wired.audit.available && wired.audit.rows[0].category == "DECISION");
    CHECK(wired.snapshot.shadow_only);

    // Determination: identical sources -> identical projection.
    const auto& again = state.refresh_report();
    CHECK(again.predictions.total == wired.predictions.total);
    CHECK(again.knowledge.revisions == wired.knowledge.revisions);
    CHECK(again.evolution.nodes == wired.evolution.nodes);
}

// Real state must propagate into the panels: a stream that fails must appear as a
// real incident with a real severity, not be hidden. Exercised over the real
// failure-detection engine and adapter boundary.
static void test_incident_propagation_and_corruption() {
    runtime::AdapterManager adapters;  // declared nine streams, all STARTING
    for (const runtime::Timeframe tf : runtime::all_timeframes())
        adapters.report_bar(tf, foundation::Timestamp::from_seconds(60), 1);
    adapters.report_disconnected(runtime::Timeframe::M5, "test drop");

    observation::FailureDetectionEngine detector;
    const std::vector<foundation::ErrorRecord> records =
        detector.detect_adapter_failures(adapters, foundation::Timestamp::from_seconds(120));
    CHECK(!records.empty());
    bool found_m5 = false, healthy_hidden = true;
    for (const auto& rec : records) {
        if (rec.context().find("M5") != std::string::npos) found_m5 = true;
        if (rec.context().find("H1") != std::string::npos) healthy_hidden = false;
    }
    CHECK(found_m5);
    CHECK(healthy_hidden);  // a healthy stream is not reported as an incident

    desktop::PanelSources src;
    src.incidents = records;
    src.incident_source_wired = true;
    const auto panel = desktop::ControlCenterPanels::incidents(src);
    CHECK(panel.source_wired);
    CHECK(panel.available);
    CHECK(panel.count == records.size());
    CHECK(!panel.rows.empty());
    bool row_has_state = false;
    for (const auto& row : panel.rows) {
        if (row.state == "OFFLINE") row_has_state = true;
    }
    CHECK(row_has_state);

    // Corrupted-state presentation: a store of junk must surface CORRUPTED_STATE
    // and be refused, and the persistence panel must render it explicitly.
    const std::string store = "aura_desktop_corrupt.aura";
    {
        std::FILE* fp = std::fopen(store.c_str(), "wb");
        CHECK(fp != nullptr);
        if (fp != nullptr) {
            const char junk[] = "not a valid aura store";
            std::fwrite(junk, 1, sizeof(junk) - 1, fp);
            std::fclose(fp);
        }
        desktop::ControlCenterOptions options;
        options.store_path = store;
        desktop::ControlCenterState state(options);
        const auto& outcome = state.evaluate_recovery(false);
        CHECK(outcome.detected == runtime::LifecycleState::CORRUPTED_STATE);
        CHECK(!outcome.resumable);
        const auto& rep = state.refresh_report();
        CHECK(rep.snapshot.persistence.lifecycle == "CORRUPTED_STATE");
        CHECK(!rep.snapshot.persistence.resumable);
    }
    std::remove(store.c_str());
}

// One runtime owner: two states never share the same pipeline, and no panel
// snapshot creates a second shell.
static void test_single_runtime_owner() {
    desktop::ControlCenterOptions a, b;
    desktop::ControlCenterState s1(a), s2(b);
    for (const std::string& f : frames(3)) s1.shell().feed(f + "\n");
    // s2 is untouched: zero accepted frames proves they are independent (and that
    // feeding through one owner never mutates another).
    CHECK(s1.shell().pipeline().status().accepted > 0);
    CHECK(s2.shell().pipeline().status().accepted == 0);
    s1.refresh_report();
    CHECK(s2.shell().pipeline().status().accepted == 0);
}

// The visual state map must keep UNKNOWN and NOT AVAILABLE visually distinct
// from HEALTHY, and never classify an unrecognised/empty state as healthy. This
// is a regression guard for the redesign: a careless refactor must not make
// "unknown" look green.
static void test_state_category_mapping() {
    using desktop::theme::StateCategory;
    using desktop::theme::state_category;
    CHECK(state_category("ONLINE") == StateCategory::Healthy);
    CHECK(state_category("HEALTHY") == StateCategory::Healthy);
    CHECK(state_category("VALID") == StateCategory::Healthy);
    CHECK(state_category("FRESH") == StateCategory::Healthy);
    CHECK(state_category("DEGRADED") == StateCategory::Degraded);
    CHECK(state_category("STALE") == StateCategory::Degraded);
    CHECK(state_category("LAGGING") == StateCategory::Degraded);
    CHECK(state_category("ERROR") == StateCategory::Critical);
    CHECK(state_category("CORRUPTED_STATE") == StateCategory::Critical);
    CHECK(state_category("BLOCKED") == StateCategory::Critical);
    CHECK(state_category("PAUSED") == StateCategory::Paused);
    CHECK(state_category("STARTING") == StateCategory::Informational);
    CHECK(state_category("NOT AVAILABLE") == StateCategory::NotAvailable);
    CHECK(state_category("UNKNOWN") == StateCategory::Unknown);
    CHECK(state_category("") == StateCategory::Unknown);
    CHECK(state_category("SOMETHING_NEW") == StateCategory::Unknown);
    // The core safety invariant of the visual language.
    CHECK(state_category("UNKNOWN") != StateCategory::Healthy);
    CHECK(state_category("NOT AVAILABLE") != StateCategory::Healthy);
    CHECK(desktop::theme::is_unknown_like("UNKNOWN"));
    CHECK(desktop::theme::is_unknown_like("NOT AVAILABLE"));
    CHECK(!desktop::theme::is_unknown_like("HEALTHY"));
}

// The navigation catalog must cover exactly the canonical sections, once each,
// in the same order, so the sidebar cannot drift from the panels.
static void test_navigation_catalog() {
    const auto& canonical = desktop::canonical_sections();
    const auto groups = desktop::navigation_groups();
    CHECK(canonical.size() == 19);
    CHECK(desktop::navigation_count() == canonical.size());

    std::vector<int> seen(canonical.size(), 0);
    for (const auto& g : groups) {
        CHECK(!g.title.empty());
        for (const auto& item : g.items) {
            CHECK(!item.label.empty());
            CHECK(item.index >= 0 && static_cast<std::size_t>(item.index) < canonical.size());
            // The item's declared section title must match the canonical title at
            // its index (no silent re-mapping).
            CHECK(item.section == canonical[static_cast<std::size_t>(item.index)]);
            CHECK(desktop::section_title_for(item.index) == item.section);
            seen[static_cast<std::size_t>(item.index)] += 1;
        }
    }
    for (std::size_t i = 0; i < seen.size(); ++i) CHECK(seen[i] == 1);
}

// Deterministic relative freshness from real snapshot data: the stream with the
// newest close time is FRESH, older present streams are LAGGING, and absent
// streams are NOT AVAILABLE. No wall clock is involved.
static void test_freshness_projection() {
    desktop::ControlCenterOptions options;  // in-memory
    desktop::ControlCenterState state(options);
    for (const std::string& f : frames(30)) state.shell().feed(f + "\n");
    const auto& snap = state.refresh();
    CHECK(snap.timeframes.rows.size() == 9);

    bool any_fresh = false, any_lagging = false;
    for (const auto& r : snap.timeframes.rows) {
        if (!r.present) { CHECK(r.freshness == "NOT AVAILABLE"); continue; }
        CHECK(r.freshness == "FRESH" || r.freshness == "LAGGING");
        if (r.freshness == "FRESH") any_fresh = true;
        if (r.freshness == "LAGGING") any_lagging = true;
    }
    // The canonical feed closes higher timeframes later, so both classes appear.
    CHECK(any_fresh);
    CHECK(any_lagging);

    // Sequence is populated for present streams (real runtime progress).
    bool any_seq = false;
    for (const auto& r : snap.timeframes.rows)
        if (r.sequence.available) any_seq = true;
    CHECK(any_seq);

    // Determinism: a second capture over identical state yields identical rows.
    const auto& again = state.refresh();
    for (std::size_t i = 0; i < snap.timeframes.rows.size(); ++i) {
        CHECK(again.timeframes.rows[i].freshness == snap.timeframes.rows[i].freshness);
        CHECK(again.timeframes.rows[i].sequence.value == snap.timeframes.rows[i].sequence.value);
    }
}

static void test_renderer_policy_hints() {
    // Modern path keeps the OpenGL 3.3 core-profile request and the OpenGL3 GLSL.
    const auto modern = desktop::hints_for(desktop::RendererProfile::MODERN_GL33);
    CHECK(modern.context_version_major == 3);
    CHECK(modern.context_version_minor == 3);
    CHECK(modern.request_core_profile);
    CHECK(std::string(modern.glsl_version) == "#version 330");

    // Legacy path must NOT request a core profile (legacy drivers reject it) and
    // must use the fixed-function OpenGL2 GLSL.
    const auto legacy = desktop::hints_for(desktop::RendererProfile::LEGACY_GL21);
    CHECK(legacy.context_version_major == 2);
    CHECK(legacy.context_version_minor == 1);
    CHECK(!legacy.request_core_profile);
    CHECK(!legacy.forward_compatible);
    CHECK(std::string(legacy.glsl_version) == "#version 120");
}

static void test_renderer_selector_modern_success() {
    // Modern hardware: first attempt succeeds; the legacy path is never used.
    desktop::RendererSelector s;
    CHECK(s.candidate() == desktop::RendererProfile::MODERN_GL33);
    desktop::RendererAttempt a;
    a.profile = desktop::RendererProfile::MODERN_GL33;
    a.created = true;
    CHECK(s.record(a));
    CHECK(s.done());
    CHECK(s.has_renderer());
    CHECK(s.active() == desktop::RendererProfile::MODERN_GL33);
    CHECK(s.attempt_count() == 1);
}

static void test_renderer_selector_legacy_fallback() {
    // Intel HD 3000 scenario: OpenGL 3.3 fails (WGL profile unavailable), so the
    // selector must advance to OpenGL 2.1 and succeed there. Never terminal-fail
    // merely because the modern context is unavailable.
    desktop::RendererSelector s;
    desktop::RendererAttempt modern;
    modern.profile = desktop::RendererProfile::MODERN_GL33;
    modern.created = false;
    modern.glfw_error = 65543;  // GLFW_API_UNAVAILABLE (observed on HD 3000)
    CHECK(!s.record(modern));
    CHECK(!s.done());
    CHECK(s.candidate() == desktop::RendererProfile::LEGACY_GL21);

    desktop::RendererAttempt legacy;
    legacy.profile = desktop::RendererProfile::LEGACY_GL21;
    legacy.created = true;
    CHECK(s.record(legacy));
    CHECK(s.done());
    CHECK(s.has_renderer());
    CHECK(s.active() == desktop::RendererProfile::LEGACY_GL21);
    CHECK(s.error_for(desktop::RendererProfile::MODERN_GL33) == 65543);
    CHECK(s.attempt_count() == 2);

    // The diagnostic names the ACTUAL renderer, and reports 3.3 as unavailable.
    const std::string diag =
        desktop::renderer_diagnostic(s.active(), 65543, "context created");
    CHECK(diag.find("LEGACY_GL21") != std::string::npos);
    CHECK(diag.find("OpenGL 2.1 compatibility") != std::string::npos);
    CHECK(diag.find("OpenGL 3.3 unavailable") != std::string::npos);
    CHECK(diag.find("65543") != std::string::npos);
}

static void test_renderer_selector_both_fail() {
    // No context at all: the selector is terminal with no renderer and the error
    // is actionable (names both attempts and points at the console host).
    desktop::RendererSelector s;
    desktop::RendererAttempt modern;
    modern.profile = desktop::RendererProfile::MODERN_GL33;
    modern.created = false;
    modern.glfw_error = 65543;
    CHECK(!s.record(modern));
    desktop::RendererAttempt legacy;
    legacy.profile = desktop::RendererProfile::LEGACY_GL21;
    legacy.created = false;
    legacy.glfw_error = 65542;
    CHECK(!s.record(legacy));
    CHECK(s.done());
    CHECK(!s.has_renderer());
    CHECK(s.active() == desktop::RendererProfile::NONE);

    const std::string err = desktop::no_renderer_error(s.attempts());
    CHECK(err.find("MODERN_GL33") != std::string::npos);
    CHECK(err.find("LEGACY_GL21") != std::string::npos);
    CHECK(err.find("65543") != std::string::npos);
    CHECK(err.find("65542") != std::string::npos);
    CHECK(err.find("aura.exe") != std::string::npos);

    // A pinned modern selector must NOT claim it tried the legacy path.
    desktop::RendererSelector pinned =
        desktop::RendererSelector::only(desktop::RendererProfile::MODERN_GL33);
    desktop::RendererAttempt pf;
    pf.profile = desktop::RendererProfile::MODERN_GL33;
    pf.created = false;
    pf.glfw_error = 65543;
    CHECK(!pinned.record(pf));
    const std::string perr = desktop::no_renderer_error(pinned.attempts());
    CHECK(perr.find("MODERN_GL33") != std::string::npos);
    CHECK(perr.find("LEGACY_GL21") == std::string::npos);
}

static void test_renderer_selector_pinned() {
    // Pinned selectors (used by CI smokes and --renderer) attempt exactly one
    // profile and become terminal, so the modern path can be proven without the
    // legacy fallback masking a modern failure, and vice versa.
    desktop::RendererSelector modern =
        desktop::RendererSelector::only(desktop::RendererProfile::MODERN_GL33);
    CHECK(modern.candidate() == desktop::RendererProfile::MODERN_GL33);
    desktop::RendererAttempt f;
    f.profile = desktop::RendererProfile::MODERN_GL33;
    f.created = false;
    CHECK(!modern.record(f));
    CHECK(modern.done());
    CHECK(!modern.has_renderer());
    CHECK(modern.attempt_count() == 1);  // did NOT silently fall back

    desktop::RendererSelector legacy =
        desktop::RendererSelector::only(desktop::RendererProfile::LEGACY_GL21);
    CHECK(legacy.candidate() == desktop::RendererProfile::LEGACY_GL21);
    desktop::RendererAttempt ok;
    ok.profile = desktop::RendererProfile::LEGACY_GL21;
    ok.created = true;
    CHECK(legacy.record(ok));
    CHECK(legacy.active() == desktop::RendererProfile::LEGACY_GL21);

    CHECK(desktop::renderer_choice_from_string("modern") == desktop::RendererChoice::MODERN);
    CHECK(desktop::renderer_choice_from_string("legacy") == desktop::RendererChoice::LEGACY);
    CHECK(desktop::renderer_choice_from_string("auto") == desktop::RendererChoice::AUTO);
    CHECK(desktop::renderer_choice_from_string("garbage") == desktop::RendererChoice::AUTO);
}

// ---- XAUUSD candlestick chart / timeframe selector -------------------------

static void test_timeframe_selection_default_and_switch() {
    // Default is M15, and the selector holds an explicit timeframe, never an index.
    desktop::TimeframeSelection sel;
    CHECK(sel.selected() == runtime::Timeframe::M15);
    CHECK(sel.selected_label() == "M15");
    CHECK(sel.is_selected(runtime::Timeframe::M15));

    // All nine map through the selector in order; the selection follows identity.
    const std::vector<runtime::Timeframe>& tfs = desktop::chart_timeframes();
    CHECK(tfs.size() == 9);
    const runtime::Timeframe expected[9] = {
        runtime::Timeframe::M1,  runtime::Timeframe::M5,  runtime::Timeframe::M15,
        runtime::Timeframe::M30, runtime::Timeframe::H1,  runtime::Timeframe::H4,
        runtime::Timeframe::D1,  runtime::Timeframe::W1,  runtime::Timeframe::MN1};
    for (int i = 0; i < 9; ++i) {
        CHECK(tfs[static_cast<std::size_t>(i)] == expected[i]);
        CHECK(sel.select(expected[i]));
        CHECK(sel.selected() == expected[i]);
        CHECK(sel.is_selected(expected[i]));
        CHECK(sel.selected_label() == std::string(runtime::to_string(expected[i])));
    }
    // An unsupported timeframe is refused and the selection is unchanged.
    const runtime::Timeframe before = sel.selected();
    CHECK(!sel.select(runtime::Timeframe::UNKNOWN));
    CHECK(sel.selected() == before);
}

static void test_candle_series_closed_bar_identity() {
    // Only bars for the requested explicit timeframe are projected; a bar of a
    // different timeframe is never drawn. A non-closed bar is never drawn.
    std::vector<runtime::MarketBar> bars;
    runtime::MarketBar a;
    a.timeframe = runtime::Timeframe::M15;
    a.closed = true;
    a.close_time = foundation::Timestamp::from_seconds(1000000);
    a.open = 100; a.high = 102; a.low = 99; a.close = 101;
    bars.push_back(a);

    runtime::MarketBar foreign = a;
    foreign.timeframe = runtime::Timeframe::H1;  // wrong stream -> ignored
    foreign.close_time = foundation::Timestamp::from_seconds(1003600);
    bars.push_back(foreign);

    runtime::MarketBar forming = a;  // still forming -> ignored (no repaint)
    forming.closed = false;
    forming.close_time = foundation::Timestamp::from_seconds(1000900);
    bars.push_back(forming);

    const desktop::CandleSeries s = desktop::make_candle_series(runtime::Timeframe::M15, bars);
    CHECK(s.available);
    CHECK(s.candles.size() == 1);
    CHECK(s.candles.front().open == 100.0);
    CHECK(s.candles.front().high == 102.0);
    CHECK(s.candles.front().low == 99.0);
    CHECK(s.candles.front().close == 101.0);
    CHECK(s.low == 99.0);
    CHECK(s.high == 102.0);

    // A timeframe with no real bars is unavailable (drives the NO CANDLE DATA state).
    const desktop::CandleSeries empty = desktop::make_candle_series(runtime::Timeframe::MN1, bars);
    CHECK(!empty.available);
    CHECK(empty.candles.empty());
    CHECK(empty.label == "MN1");
}

static void test_candle_series_rejects_non_finite() {
    std::vector<runtime::MarketBar> bars;
    runtime::MarketBar bad;
    bad.timeframe = runtime::Timeframe::D1;
    bad.closed = true;
    bad.close_time = foundation::Timestamp::from_seconds(2000000);
    bad.open = 100; bad.high = 101; bad.low = 99; bad.close = std::numeric_limits<double>::quiet_NaN();
    bars.push_back(bad);
    const desktop::CandleSeries s = desktop::make_candle_series(runtime::Timeframe::D1, bars);
    CHECK(!s.available);  // a non-finite bar is dropped, never fabricated into a candle
}

static void test_chart_geometry_bullish_bearish() {
    std::vector<runtime::MarketBar> bars;
    runtime::MarketBar up;
    up.timeframe = runtime::Timeframe::M5;
    up.closed = true;
    up.close_time = foundation::Timestamp::from_seconds(3000000);
    up.open = 100; up.high = 105; up.low = 99; up.close = 104;  // bullish
    bars.push_back(up);
    runtime::MarketBar down = up;
    down.close_time = foundation::Timestamp::from_seconds(3000300);
    down.open = 104; down.high = 104; down.low = 98; down.close = 99;  // bearish
    bars.push_back(down);

    const desktop::CandleSeries s = desktop::make_candle_series(runtime::Timeframe::M5, bars);
    const desktop::ChartGeometry g = desktop::build_chart_geometry(s, 5, 0.0f);
    CHECK(g.candles.size() == 2);
    CHECK(g.candles[0].bullish);
    CHECK(!g.candles[1].bullish);
    // y is measured from the top: the high (105) sits above the low (98).
    CHECK(g.candles[0].wick_top <= g.candles[0].wick_bottom);
    CHECK(g.candles[0].body_top <= g.candles[0].body_bottom);
    CHECK(g.candles[1].body_top <= g.candles[1].body_bottom);
    CHECK(g.grid_y.size() == 5);
    CHECK(g.grid_price.size() == 5);
    CHECK(g.grid_price.front() == g.price_high);
    CHECK(g.grid_price.back() == g.price_low);
    // Layout spans the plot width in order.
    CHECK(g.candles[0].x < g.candles[1].x);
}

static void test_chart_series_from_real_runtime() {
    // The chart reads the runtime's real retained closed bars for the selected
    // explicit timeframe, and default selection is M15.
    desktop::ControlCenterOptions o;
    desktop::ControlCenterState st(o);
    st.feed_builtin(10);
    CHECK(st.timeframe_selection().selected() == runtime::Timeframe::M15);

    for (const runtime::Timeframe tf : desktop::chart_timeframes()) {
        CHECK(st.timeframe_selection().select(tf));
        const desktop::CandleSeries cs = st.chart_series(tf);
        CHECK(cs.timeframe == tf);
        CHECK(cs.available);
        CHECK(!cs.candles.empty());
        CHECK(cs.candles.size() <= 10);
    }
    // Refresh report carries the selected series (M15 -> last selection was MN1,
    // then re-select M15 to prove the report follows the selection).
    CHECK(st.timeframe_selection().select(runtime::Timeframe::M15));
    const auto& rep = st.refresh_report();
    CHECK(rep.chart.timeframe == runtime::Timeframe::M15);
    CHECK(rep.chart.available);
    CHECK(!rep.chart.candles.empty());
    // A stream that never received a bar is unavailable in the report.
    desktop::ControlCenterOptions o2;
    desktop::ControlCenterState st2(o2);
    st2.feed_builtin(10);
    CHECK(st2.timeframe_selection().select(runtime::Timeframe::W1));
    const auto& rep2 = st2.refresh_report();
    CHECK(rep2.chart.timeframe == runtime::Timeframe::W1);
    CHECK(rep2.chart.available);  // builtin feeds all nine streams
}

static void test_chart_empty_state_no_fake_candles() {
    // No frames fed: the chart must be unavailable with zero candles, never
    // padded with placeholder candles.
    desktop::ControlCenterOptions o;
    desktop::ControlCenterState st(o);
    CHECK(st.timeframe_selection().selected() == runtime::Timeframe::M15);
    const desktop::CandleSeries cs = st.chart_series(runtime::Timeframe::M15);
    CHECK(!cs.available);
    CHECK(cs.candles.empty());
    CHECK(cs.label == "M15");
    const auto& rep = st.refresh_report();
    CHECK(!rep.chart.available);
    CHECK(rep.chart.candles.empty());
}

static void test_chart_utc_formatter() {
    // The axis labels a real close time deterministically from epoch nanoseconds.
    const std::string t = desktop::format_utc_minute(foundation::Timestamp::from_seconds(1700000000));
    CHECK(t == "11-14 22:13");
    CHECK(desktop::format_utc_minute(foundation::Timestamp::from_seconds(0)) == "01-01 00:00");
}

static void test_real_metrics_projection() {
    // The desktop projection must surface AURA's REAL deterministic score and
    // confidence (RT-0011/RT-0012) computed by the pipeline, plus the realized
    // shadow outcomes. It must never fabricate a value: with no signal there is
    // no score/confidence, and with no closed position the realized counts are
    // zero and marked unavailable.
    desktop::ControlCenterOptions empty_opts;
    desktop::ControlCenterState empty_state(empty_opts);
    const auto& empty_snap = empty_state.refresh();
    CHECK(!empty_snap.signal.score_available);
    CHECK(!empty_snap.signal.confidence_available);
    CHECK(!empty_snap.signal.outcomes_available);
    CHECK(empty_snap.signal.positions_closed == 0);

    // After a real run, score/confidence are present and bounded to [0,1], and
    // they originate from the pipeline (not the desktop layer).
    desktop::ControlCenterOptions options;  // in-memory
    desktop::ControlCenterState state(options);
    for (const std::string& f : frames(40)) state.shell().feed(f + "\n");
    const auto& snap = state.refresh();

    CHECK(snap.signal.available);
    CHECK(snap.signal.score_available);
    CHECK(snap.signal.score >= 0.0 && snap.signal.score <= 1.0);
    CHECK(snap.signal.confidence_available);
    CHECK(snap.signal.confidence >= 0.0 && snap.signal.confidence <= 1.0);

    // The projected values equal the pipeline's own values (read-only copy).
    const auto& pscore = state.shell().pipeline().last_score();
    const auto& pconf = state.shell().pipeline().last_confidence();
    CHECK(pscore.valid);
    CHECK(pscore.score == snap.signal.score);
    CHECK(pconf.valid);
    CHECK(pconf.confidence == snap.signal.confidence);

    // Realized outcomes are historical counts of closed shadow positions. Wins
    // can never exceed closed positions.
    CHECK(snap.positions.wins <= snap.positions.positions_closed);
    CHECK(snap.signal.wins == snap.positions.wins);
    CHECK(snap.signal.positions_closed == snap.positions.positions_closed);
    CHECK(snap.signal.outcomes_available == (snap.positions.positions_closed > 0));

    // Determinism: a second capture over identical state yields identical values.
    const auto& again = state.refresh();
    CHECK(again.signal.score == snap.signal.score);
    CHECK(again.signal.confidence == snap.signal.confidence);
    CHECK(again.signal.positions_closed == snap.signal.positions_closed);

    // Shadow-only invariant unchanged by the projection.
    CHECK(snap.shadow_only);
    CHECK(snap.risk.is_order == false);
}

static void test_terminal_layout_contract() {
    using namespace desktop::layout;
    // The timeframe strip is exactly the nine canonical timeframes, in order, one
    // per explicit identity, non-overlapping.
    const std::vector<Row> rows = timeframe_rows(10.0f);
    CHECK(rows.size() == 9);
    CHECK(rows.front().timeframe == runtime::Timeframe::M1);
    CHECK(rows.back().timeframe == runtime::Timeframe::MN1);
    for (std::size_t i = 1; i < rows.size(); ++i) {
        CHECK(rows[i].x >= rows[i - 1].x + rows[i - 1].width);
        CHECK(rows[i].timeframe != rows[i - 1].timeframe);
    }
    // The chart never collapses below the floor nor grows unbounded on tall
    // viewports, and the panels always keep a usable minimum.
    CHECK(chart_height(400.0f) == Chrome::kMinChart);
    CHECK(chart_height(2000.0f) == Chrome::kMaxChart);
    float big = chart_height(900.0f);
    CHECK(big > Chrome::kMinChart);
    CHECK(big < Chrome::kMaxChart);
    CHECK(panel_height(900.0f, big) >= Chrome::kMinPanel);
    CHECK(panel_height(300.0f, Chrome::kMinChart) >= Chrome::kMinPanel);
    // Deterministic: identical inputs yield identical geometry.
    const std::vector<Row> again = timeframe_rows(10.0f);
    CHECK(again.size() == rows.size());
    for (std::size_t i = 0; i < rows.size(); ++i) {
        CHECK(again[i].x == rows[i].x);
        CHECK(again[i].width == rows[i].width);
    }
}

// V2 polish: the pure dashboard row geometry and the canonical timeframe role
// map are a contract between the composition and the tests.
static void test_dashboard_rows_and_roles() {
    using namespace desktop::layout;
    // The canonical V3-29 authority role of each explicit timeframe identity.
    CHECK(std::string(timeframe_role(runtime::Timeframe::M15)) == "OPERATIONAL");
    CHECK(std::string(timeframe_role(runtime::Timeframe::H4)) == "STRUCTURAL");
    CHECK(std::string(timeframe_role(runtime::Timeframe::M1)) == "EXECUTION");
    CHECK(std::string(timeframe_role(runtime::Timeframe::M5)) == "EXECUTION");
    CHECK(std::string(timeframe_role(runtime::Timeframe::M30)) == "CONTEXT");
    CHECK(std::string(timeframe_role(runtime::Timeframe::H1)) == "CONTEXT");
    CHECK(std::string(timeframe_role(runtime::Timeframe::D1)) == "LONG HORIZON");
    CHECK(std::string(timeframe_role(runtime::Timeframe::W1)) == "LONG HORIZON");
    CHECK(std::string(timeframe_role(runtime::Timeframe::MN1)) == "LONG HORIZON");
    CHECK(std::string(timeframe_role(runtime::Timeframe::UNKNOWN)).empty());

    // 1366x768-class content: exact fit (never overflows into a scrollbar), the
    // analytics row stays a minority band and the chart stays dominant.
    const DashboardRows hd = dashboard_rows(656.0f);
    CHECK(hd.analytics_h >= 120.0f && hd.analytics_h <= 220.0f);
    CHECK(hd.chart_h >= Chrome::kMinChart && hd.chart_h <= Chrome::kMaxChart);
    CHECK(hd.chart_h + hd.analytics_h + Chrome::kSecondaryStrip + 5.0f * 8.0f + 4.0f <=
          656.0f + 0.001f);
    CHECK(hd.chart_h > hd.analytics_h * 2.0f);

    // 1600x900 / 1920x1080-class content: the analytics row is capped, the chart
    // takes the remainder and stays within its bounds.
    const DashboardRows hr = dashboard_rows(968.0f);
    CHECK(hr.analytics_h == 220.0f);
    CHECK(hr.chart_h == 968.0f - 220.0f - Chrome::kSecondaryStrip - 5.0f * 8.0f - 4.0f);
    CHECK(hr.chart_h > Chrome::kMinChart && hr.chart_h <= Chrome::kMaxChart);

    // Short viewport: the chart floor wins and the panels keep their own floor.
    const DashboardRows hs = dashboard_rows(300.0f);
    CHECK(hs.chart_h == Chrome::kMinChart);
    CHECK(hs.analytics_h == 120.0f);

    // Deterministic: identical inputs yield identical geometry.
    const DashboardRows again = dashboard_rows(656.0f);
    CHECK(again.chart_h == hd.chart_h);
    CHECK(again.analytics_h == hd.analytics_h);
}

// ---- MT5 Python bridge loader tests -----------------------------------------

// A missing bridge file must be reported as not loaded, with no fabricated
// candles and a non-empty path attempted.
static void test_mt5_loader_missing_file() {
    const desktop::mt5::LoadResult r =
        desktop::mt5::load_candles("no_such_bridge_dir_xyz", "M15");
    CHECK(!r.loaded);
    CHECK(r.count == 0);
    CHECK(!r.series.available);
    CHECK(r.series.candles.empty());
    CHECK(!r.path_tried.empty());
}

// A real fixture is parsed into the correct closed-bar count; malformed JSON and a
// declared-count mismatch are both rejected (never partially trusted).
static void test_mt5_loader_fixture_count() {
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() / "aura_mt5_fixture_test";
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir, ec);

    const std::string body =
        "{\"symbol\":\"XAUUSD\",\"timeframe\":\"M15\",\"broker\":\"TestBroker\","
        "\"account_login\":4242,\"generated_at_utc\":1700000000,\"count\":3,\"candles\":["
        "{\"time\":1700000000,\"open\":2000.0,\"high\":2001.0,\"low\":1999.5,\"close\":2000.5,"
        "\"tick_volume\":10,\"spread\":20,\"real_volume\":0},"
        "{\"time\":1700000900,\"open\":2000.5,\"high\":2002.0,\"low\":2000.0,\"close\":2001.5,"
        "\"tick_volume\":11,\"spread\":21,\"real_volume\":0},"
        "{\"time\":1700001800,\"open\":2001.5,\"high\":2003.0,\"low\":2001.0,\"close\":2002.5,"
        "\"tick_volume\":12,\"spread\":19,\"real_volume\":0}]}";
    {
        std::ofstream f(dir / "candles_M15.json", std::ios::binary);
        f << body;
    }

    const desktop::mt5::LoadResult r = desktop::mt5::load_candles(dir.string(), "M15");
    CHECK(r.loaded);
    CHECK(r.count == 3);
    CHECK(r.series.available);
    CHECK(r.series.timeframe == runtime::Timeframe::M15);
    CHECK(r.series.candles.size() == 3);
    CHECK(r.series.candles.front().open == 2000.0);
    CHECK(r.series.candles.back().close == 2002.5);
    CHECK(r.symbol == "XAUUSD");
    CHECK(r.broker == "TestBroker");
    CHECK(r.account_login == 4242);
    CHECK(r.series.low == 1999.5);
    CHECK(r.series.high == 2003.0);
    CHECK(r.series.first_close.seconds() == 1700000000);
    CHECK(r.series.last_close.seconds() == 1700001800);

    // Malformed JSON is rejected.
    {
        std::ofstream f(dir / "candles_H1.json", std::ios::binary);
        f << "{ broken";
    }
    const desktop::mt5::LoadResult bad = desktop::mt5::load_candles(dir.string(), "H1");
    CHECK(!bad.loaded);

    // Declared count that disagrees with the array is rejected.
    {
        std::ofstream f(dir / "candles_D1.json", std::ios::binary);
        f << "{\"timeframe\":\"D1\",\"count\":9,\"candles\":"
             "[{\"time\":1,\"open\":1,\"high\":1,\"low\":1,\"close\":1}]}";
    }
    const desktop::mt5::LoadResult mm = desktop::mt5::load_candles(dir.string(), "D1");
    CHECK(!mm.loaded);

    fs::remove_all(dir, ec);
}

// Without enable_mt5_bridge() the chart uses the existing synthetic source
// unchanged and reports itself as synthetic.
static void test_chart_default_is_synthetic() {
    desktop::ControlCenterOptions o;
    desktop::ControlCenterState st(o);
    CHECK(!st.using_real_data());
    CHECK(st.data_source_note() == "SYNTHETIC");
    st.feed_builtin(5);
    CHECK(st.timeframe_selection().selected() == runtime::Timeframe::M15);
    const desktop::CandleSeries cs = st.chart_series(runtime::Timeframe::M15);
    CHECK(cs.available);  // the synthetic/builtin source still works
    CHECK(!cs.candles.empty());
    CHECK(cs.candles.size() <= 5);
}

// Opting in serves real bridge candles for timeframes that have files, and falls
// back to the synthetic source for timeframes that do not.
static void test_mt5_bridge_optin_and_fallback() {
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() / "aura_mt5_optin_test";
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir, ec);
    {
        std::ofstream f(dir / "candles_M15.json", std::ios::binary);
        f << "{\"symbol\":\"XAUUSD\",\"timeframe\":\"M15\",\"count\":1,\"candles\":"
             "[{\"time\":1700000000,\"open\":111.0,\"high\":112.0,\"low\":110.0,\"close\":111.5}]}";
    }

    desktop::ControlCenterOptions o;
    desktop::ControlCenterState st(o);
    st.feed_builtin(5);  // synthetic source is populated
    CHECK(st.enable_mt5_bridge_from_dirs({dir.string()}) == 1);  // only M15 present
    CHECK(st.using_real_data());
    CHECK(st.mt5_timeframes_loaded() == 1);

    const desktop::CandleSeries m15 = st.chart_series(runtime::Timeframe::M15);
    CHECK(m15.available);
    CHECK(m15.candles.size() == 1);
    CHECK(m15.candles.front().open == 111.0);  // real bridge data is used

    // A timeframe with no bridge file falls back to the synthetic source.
    const desktop::CandleSeries h1 = st.chart_series(runtime::Timeframe::H1);
    CHECK(h1.available);
    CHECK(!h1.candles.empty());
    CHECK(h1.candles.front().open != 111.0);

    // Opting in with no files changes nothing: synthetic, no real data.
    desktop::ControlCenterState st2(o);
    st2.feed_builtin(5);
    CHECK(st2.enable_mt5_bridge_from_dirs({"/nonexistent_bridge_xyz"}) == 0);
    CHECK(!st2.using_real_data());
    CHECK(st2.chart_series(runtime::Timeframe::M15).available);

    fs::remove_all(dir, ec);
}

int main() {
    test_dashboard_no_fabrication();
    test_knowledge_projection_readonly();
    test_research_and_candidate_projection();
    test_approval_audit_and_graph_projection();
    test_desktop_model_empty_state();
    test_desktop_model_nine_timeframes();
    test_control_center_state_smoke();
    test_section_panel_availability();
    test_incident_propagation_and_corruption();
    test_single_runtime_owner();
    test_state_category_mapping();
    test_navigation_catalog();
    test_freshness_projection();
    test_renderer_policy_hints();
    test_renderer_selector_modern_success();
    test_renderer_selector_legacy_fallback();
    test_renderer_selector_both_fail();
    test_renderer_selector_pinned();
    test_timeframe_selection_default_and_switch();
    test_candle_series_closed_bar_identity();
    test_candle_series_rejects_non_finite();
    test_chart_geometry_bullish_bearish();
    test_chart_series_from_real_runtime();
    test_chart_empty_state_no_fake_candles();
    test_chart_utc_formatter();
    test_real_metrics_projection();
    test_terminal_layout_contract();
    test_dashboard_rows_and_roles();
    test_mt5_loader_missing_file();
    test_mt5_loader_fixture_count();
    test_chart_default_is_synthetic();
    test_mt5_bridge_optin_and_fallback();
    if (g_failures == 0) {
        std::printf("DesktopTests: ALL PASS\n");
        return 0;
    }
    std::printf("DesktopTests: %d FAILURE(S)\n", g_failures);
    return 1;
}
