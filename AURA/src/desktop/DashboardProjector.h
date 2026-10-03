#ifndef AURA_DESKTOP_DASHBOARDPROJECTOR_H
#define AURA_DESKTOP_DASHBOARDPROJECTOR_H

#include "desktop/ControlCenterViewModels.h"
#include "evolution/Candidate.h"
#include "evolution/EvolutionGraph.h"
#include "governance/AuditLedger.h"
#include "governance/HumanDecision.h"
#include "learning/KnowledgeObject.h"
#include "operatingwindow/OperatingWindow.h"
#include "research/Experiment.h"

#include <string>
#include <vector>

namespace aura {
namespace desktop {

// Control-center projector (Phase 9).
//
// Pure, deterministic projections from trusted logic layers into read-only view
// models. It performs no I/O and never mutates any input. Unknown inputs remain
// UNKNOWN (never fabricated as OK/healthy/confident). The UI shell owns rendering;
// this layer owns only correct, non-fabricated data.
class ControlCenterProjector {
public:
    static DashboardViewModel dashboard(const std::string& project_name,
                                        const std::string& operating_mode,
                                        operatingwindow::WindowPhase phase,
                                        const std::vector<DashboardTile>& tiles) {
        DashboardViewModel vm;
        vm.project_name = project_name;
        vm.operating_mode = operating_mode;
        vm.window_phase = std::string(operatingwindow::to_string(phase));
        vm.tiles = tiles;
        for (const DashboardTile& t : tiles) {
            if (t.status == TileStatus::UNKNOWN) vm.has_unknown_state = true;
        }
        return vm;
    }

    static KnowledgeViewModel knowledge(const learning::KnowledgeObject& k) {
        KnowledgeViewModel vm;
        vm.knowledge_id = k.knowledge_id;
        vm.observation = k.observation;
        vm.status = std::string(learning::to_string(k.status));
        vm.validity_scope = k.validity_scope;
        vm.confidence_ranking = k.confidence.score();
        vm.revision = k.revision;
        return vm;
    }

    static std::vector<KnowledgeViewModel> knowledge_list(
        const std::vector<learning::KnowledgeObject>& items) {
        std::vector<KnowledgeViewModel> out;
        out.reserve(items.size());
        for (const auto& k : items) out.push_back(knowledge(k));
        return out;
    }

    static ResearchViewModel research(const research::Experiment& e, bool duplicate) {
        ResearchViewModel vm;
        vm.experiment_id = e.experiment_id;
        vm.question = e.question;
        vm.decision = std::string(research::to_string(e.decision));
        vm.evidence_zone = std::string(research::to_string(e.evidence_zone));
        vm.contamination = std::string(research::to_string(e.contamination));
        vm.duplicate = duplicate;
        return vm;
    }

    static std::vector<ResearchViewModel> research_list(
        const std::vector<research::Experiment>& items) {
        std::vector<ResearchViewModel> out;
        out.reserve(items.size());
        for (const auto& e : items) out.push_back(research(e, false));
        return out;
    }

    static CandidateViewModel candidate(const evolution::Candidate& c) {
        CandidateViewModel vm;
        vm.candidate_id = c.candidate_id;
        vm.parent_version = c.parent_version;
        vm.change_type = std::string(evolution::to_string(c.change_type));
        vm.state = std::string(evolution::to_string(c.state));
        vm.revision = c.revision;
        return vm;
    }

    static ApprovalViewModel approval(const governance::DecisionRecord& d) {
        ApprovalViewModel vm;
        vm.decision_id = d.decision_id;
        vm.question = d.question;
        vm.status = std::string(governance::to_string(d.status));
        vm.actor = d.actor;
        return vm;
    }

    // Builds the graph view model from an explicit, caller-supplied node order,
    // so the projection does not depend on internal container ordering.
    static EvolutionGraphViewModel evolution_graph(const std::vector<evolution::EvolutionNode>& nodes) {
        EvolutionGraphViewModel vm;
        for (const auto& n : nodes) {
            if (!n.valid) continue;
            vm.versions.push_back(n.version);
            if (!n.parent_version.empty()) {
                vm.edges.push_back({n.parent_version, n.version});
            }
        }
        return vm;
    }

    static AuditViewModel audit(const governance::AuditRecord& r) {
        AuditViewModel vm;
        vm.audit_id = r.audit_id;
        vm.category = std::string(category_name(r.category));
        vm.actor = r.actor;
        vm.action = r.action;
        vm.recorded_at = r.recorded_at;
        return vm;
    }

    static std::vector<AuditViewModel> audit_list(const std::vector<governance::AuditRecord>& items) {
        std::vector<AuditViewModel> out;
        out.reserve(items.size());
        for (const auto& r : items) out.push_back(audit(r));
        return out;
    }

private:
    static std::string_view category_name(governance::AuditCategory c) noexcept {
        switch (c) {
            case governance::AuditCategory::DECISION:          return "DECISION";
            case governance::AuditCategory::PROMOTION:         return "PROMOTION";
            case governance::AuditCategory::ROLLBACK:          return "ROLLBACK";
            case governance::AuditCategory::RESEARCH_ACTION:   return "RESEARCH_ACTION";
            case governance::AuditCategory::GOVERNANCE_SCREEN: return "GOVERNANCE_SCREEN";
            case governance::AuditCategory::INCIDENT:          return "INCIDENT";
            case governance::AuditCategory::CONFIGURATION:     return "CONFIGURATION";
            case governance::AuditCategory::OBSERVATION:       return "OBSERVATION";
        }
        return "UNKNOWN";
    }
};

}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_DASHBOARDPROJECTOR_H
