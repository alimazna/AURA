// Phase 9 desktop control-center view-model tests (DESK-0001..DESK-0002).
//
// Deterministic; no GUI, no clock, no I/O. Exits non-zero on the first failure.

#include "desktop/ControlCenterViewModels.h"
#include "desktop/ControlCenterState.h"
#include "desktop/DashboardProjector.h"
#include "desktop/DesktopModel.h"
#include "evolution/Candidate.h"
#include "evolution/EvolutionGraph.h"
#include "governance/AuditLedger.h"
#include "governance/HumanDecision.h"
#include "learning/KnowledgeObject.h"
#include "mt5/ProtocolCodec.h"
#include "operatingwindow/OperatingWindow.h"
#include "research/Experiment.h"
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


int main() {
    test_dashboard_no_fabrication();
    test_knowledge_projection_readonly();
    test_research_and_candidate_projection();
    test_approval_audit_and_graph_projection();
    test_desktop_model_empty_state();
    test_desktop_model_nine_timeframes();
    test_control_center_state_smoke();
    if (g_failures == 0) {
        std::printf("DesktopTests: ALL PASS\n");
        return 0;
    }
    std::printf("DesktopTests: %d FAILURE(S)\n", g_failures);
    return 1;
}
