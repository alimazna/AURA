// Phase 9 desktop control-center view-model tests (DESK-0001..DESK-0002).
//
// Deterministic; no GUI, no clock, no I/O. Exits non-zero on the first failure.

#include "desktop/ControlCenterViewModels.h"
#include "desktop/DashboardProjector.h"
#include "evolution/Candidate.h"
#include "evolution/EvolutionGraph.h"
#include "governance/AuditLedger.h"
#include "governance/HumanDecision.h"
#include "learning/KnowledgeObject.h"
#include "operatingwindow/OperatingWindow.h"
#include "research/Experiment.h"

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

int main() {
    test_dashboard_no_fabrication();
    test_knowledge_projection_readonly();
    test_research_and_candidate_projection();
    test_approval_audit_and_graph_projection();
    if (g_failures == 0) {
        std::printf("DesktopTests: ALL PASS\n");
        return 0;
    }
    std::printf("DesktopTests: %d FAILURE(S)\n", g_failures);
    return 1;
}
