// Phase 9 desktop control-center view-model tests (DESK-0001..DESK-0002).
//
// Deterministic; no GUI, no clock, no I/O. Exits non-zero on the first failure.

#include "desktop/ControlCenterViewModels.h"
#include "desktop/ControlCenterPanels.h"
#include "desktop/ControlCenterState.h"
#include "desktop/DashboardProjector.h"
#include "desktop/DesktopModel.h"
#include "desktop/RendererPolicy.h"
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
    test_renderer_policy_hints();
    test_renderer_selector_modern_success();
    test_renderer_selector_legacy_fallback();
    test_renderer_selector_both_fail();
    test_renderer_selector_pinned();
    if (g_failures == 0) {
        std::printf("DesktopTests: ALL PASS\n");
        return 0;
    }
    std::printf("DesktopTests: %d FAILURE(S)\n", g_failures);
    return 1;
}
