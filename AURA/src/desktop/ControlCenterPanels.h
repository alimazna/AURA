#ifndef AURA_DESKTOP_CONTROLCENTERPANELS_H
#define AURA_DESKTOP_CONTROLCENTERPANELS_H

#include "desktop/DashboardProjector.h"
#include "evolution/Candidate.h"
#include "evolution/EvolutionGraph.h"
#include "foundation/ErrorRecord.h"
#include "foundation/EntityId.h"
#include "foundation/Timestamp.h"
#include "governance/AuditLedger.h"
#include "governance/HumanDecision.h"
#include "learning/KnowledgeObject.h"
#include "observation/PredictionLedger.h"
#include "operatingwindow/Checkpoint.h"
#include "research/Experiment.h"
#include "runtime/ApplicationPipeline.h"
#include "runtime/ApplicationRecovery.h"
#include "runtime/ShadowLedger.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace aura {
namespace desktop {

// Additional read-only control-center views (V3-37 sections beyond the core
// runtime panels).
//
// These are presentation projections, built from a pre-computed set of trusted
// source records that the runtime owner hands in. They contain no GUI toolkit, no
// network, no filesystem I/O, no clock and no runtime mutation. A section is
// `available` only when a real source exists for it; otherwise the UI renders
// NOT AVAILABLE (and the panel carries a note explaining why) rather than
// fabricating values. Nothing here is a probability, a health guarantee or an
// order.

struct PredictionRow {
    std::string prediction_id{"NOT AVAILABLE"};
    std::string decision_id{"NOT AVAILABLE"};
    std::string symbol{"NOT AVAILABLE"};
    std::string timeframe{"NOT AVAILABLE"};
    std::string direction{"NOT AVAILABLE"};
    double score{0.0};
    std::string reference_price{"NOT AVAILABLE"};
    std::string predicted_at{"NOT AVAILABLE"};
};

struct PredictionPanel {
    // available == true only when the application actually maintains a populated
    // prediction ledger. ApplicationPipeline doesn't today, so this is false and
    // the panel is NOT AVAILABLE rather than faked.
    bool available{false};
    std::size_t total{0};
    std::string note{};
    std::vector<PredictionRow> rows{};  // most recent first, bounded
};

struct LedgerEntryRow {
    std::string type{"UNKNOWN"};
    std::string decision_id{"NOT AVAILABLE"};
    std::string recorded_at{"NOT AVAILABLE"};
    std::string payload{};
};

struct ShadowLedgerPanel {
    bool available{false};
    std::size_t total{0};
    // The shadow ledger is append-only by construction; this mirrors that contract
    // so the UI can state it truthfully.
    bool append_only{true};
    std::vector<LedgerEntryRow> recent{};  // most recent first, bounded
};

struct IncidentRow {
    std::string severity{"UNKNOWN"};
    std::string component{};
    std::string message{};
    std::string state{"UNKNOWN"};
    std::string recovery_action{"UNKNOWN"};
    std::string timestamp{"NOT AVAILABLE"};
};

struct IncidentPanel {
    // available is true whenever the detector ran (source_wired). An empty list is
    // then a genuine "no incidents", not "no data source".
    bool available{false};
    bool source_wired{false};
    std::size_t count{0};
    std::vector<IncidentRow> rows{};
};

struct KnowledgeRow {
    std::string knowledge_id{"NOT AVAILABLE"};
    std::string observation{};
    std::string status{"UNKNOWN"};
    std::string validity_scope{};
    std::uint32_t revision{0};
};

struct KnowledgePanel {
    bool available{false};
    std::size_t identities{0};
    std::size_t revisions{0};
    std::vector<KnowledgeRow> rows{};
};

struct ResearchRow {
    std::string experiment_id{"NOT AVAILABLE"};
    std::string question{};
    std::string decision{"UNRESOLVED"};
    std::string evidence_zone{""};
    std::string contamination{""};
    bool duplicate{false};
};

struct ResearchPanel {
    bool available{false};
    std::size_t total{0};
    std::vector<ResearchRow> rows{};
};

struct CandidateRow {
    std::string candidate_id{"NOT AVAILABLE"};
    std::string parent_version{};
    std::string change_type{"NONE"};
    std::string state{"UNKNOWN"};
    std::uint32_t revision{0};
};

struct CandidatePanel {
    bool available{false};
    std::size_t population{0};
    std::vector<CandidateRow> rows{};
};

struct ValidationPanel {
    // available == false until a validation campaign exists. Whether a campaign is
    // ongoing is not derivable from the current build, so this remains NOT
    // AVAILABLE rather than being presented as an empty "all pass".
    bool available{false};
    std::string note{"no validation campaign source is wired"};
    std::size_t campaigns{0};
};

struct ApprovalRow {
    std::string decision_id{"NOT AVAILABLE"};
    std::string question{};
    std::string status{"UNKNOWN"};
    std::string actor{};
};

struct ApprovalPanel {
    bool available{false};
    std::size_t total{0};
    std::vector<ApprovalRow> rows{};
};

struct EvolutionEdgeRow {
    std::string parent_version{"NOT AVAILABLE"};
    std::string child_version{"NOT AVAILABLE"};
};

struct EvolutionPanel {
    bool available{false};
    std::size_t nodes{0};
    std::vector<EvolutionEdgeRow> edges{};
};

struct SchedulePanel {
    // available == false until the operator has accepted a window. Whether a
    // window is active/human-accepted is not derivable from current runtime state,
    // so this is NOT AVAILABLE rather than an invented ACTIVE.
    bool available{false};
    std::string note{"no accepted operating window is wired to this view"};
    std::string window_phase{"NOT AVAILABLE"};
};

struct CheckpointRow {
    std::string checkpoint_id{"NOT AVAILABLE"};
    std::string validity{"UNVERIFIED"};
    std::string schema_version{};
};

struct CheckpointPanel {
    bool available{false};
    std::size_t total{0};
    std::vector<CheckpointRow> recent{};  // most recent first, bounded
};

struct AuditRow {
    std::string category{"UNKNOWN"};
    std::string actor{};
    std::string action{};
    std::string recorded_at{"NOT AVAILABLE"};
};

struct AuditPanel {
    bool available{false};
    std::size_t total{0};
    std::vector<AuditRow> rows{};
};

struct VersionPanel {
    // Always sourced from the live pipeline configuration (version identity is
    // known for every build), so this section is available without fabrication.
    bool available{true};
    std::string schema_version{};
    std::string strategy_version{};
    std::string configuration_version{};
    std::string strategy_family{"UNKNOWN"};
    std::string symbol{};
};

// Bounded source records the runtime owner hands to the projector. Holding them
// here (rather than reaching into engines) keeps the projection pure and lets the
// state owner decide *when* a copy is made.
struct PanelSources {
    const runtime::ApplicationPipeline* pipeline{nullptr};

    std::vector<observation::Prediction> predictions{};
    bool prediction_source_wired{false};

    std::vector<runtime::LedgerEntry> ledger{};

    std::vector<foundation::ErrorRecord> incidents{};
    bool incident_source_wired{false};

    std::vector<learning::KnowledgeObject> knowledge{};
    std::size_t knowledge_identities{0};

    std::vector<research::Experiment> experiments{};

    std::vector<evolution::Candidate> candidates{};
    std::size_t candidate_population{0};

    std::vector<governance::DecisionRecord> approvals{};

    std::vector<evolution::EvolutionNode> evolution_nodes{};

    std::vector<operatingwindow::Checkpoint> checkpoints{};
};

// Pure read-only projections. Deterministic given the supplied sources; no I/O,
// no clock, no mutation of any input.
class ControlCenterPanels {
public:
    static constexpr std::size_t kRecentLimit = 50;

    static PredictionPanel predictions(const PanelSources& src) {
        PredictionPanel p;
        p.total = src.predictions.size();
        if (!src.prediction_source_wired) {
            p.available = false;
            p.note = "the application maintains no live prediction ledger yet";
            return p;
        }
        p.available = true;
        for (auto it = src.predictions.rbegin();
             it != src.predictions.rend() && p.rows.size() < kRecentLimit; ++it) {
            const observation::Prediction& pr = *it;
            PredictionRow r;
            r.prediction_id = pr.prediction_id.valid() ? pr.prediction_id.value() : "NOT AVAILABLE";
            r.decision_id = !pr.decision_id.empty() ? pr.decision_id.to_hex() : "NOT AVAILABLE";
            r.symbol = pr.symbol.empty() ? "NOT AVAILABLE" : pr.symbol;
            r.timeframe = std::string(runtime::to_string(pr.timeframe));
            r.direction = std::string(runtime::to_string(pr.direction));
            r.score = pr.score;
            r.reference_price = num(pr.reference_price);
            r.predicted_at = pr.predicted_at.nanoseconds() == 0
                                 ? "NOT AVAILABLE"
                                 : std::to_string(pr.predicted_at.nanoseconds());
            p.rows.push_back(std::move(r));
        }
        return p;
    }

    static ShadowLedgerPanel shadow_ledger(const PanelSources& src) {
        ShadowLedgerPanel p;
        p.total = src.ledger.size();
        p.available = !src.ledger.empty();
        for (auto it = src.ledger.rbegin(); it != src.ledger.rend() && p.recent.size() < kRecentLimit;
             ++it) {
            const runtime::LedgerEntry& e = *it;
            LedgerEntryRow r;
            r.type = std::string(runtime::to_string(e.type));
            r.decision_id = !e.decision_id.empty() ? e.decision_id.to_hex() : "NOT AVAILABLE";
            r.recorded_at = e.recorded_at.nanoseconds() == 0
                                ? "NOT AVAILABLE"
                                : std::to_string(e.recorded_at.nanoseconds());
            r.payload = e.payload;
            p.recent.push_back(std::move(r));
        }
        return p;
    }

    static IncidentPanel incidents(const PanelSources& src) {
        IncidentPanel p;
        p.source_wired = src.incident_source_wired;
        p.available = src.incident_source_wired;
        p.count = src.incidents.size();
        for (const foundation::ErrorRecord& e : src.incidents) {
            IncidentRow r;
            r.severity = std::string(foundation::to_string(e.severity()));
            r.component = e.component();
            r.message = e.message();
            r.state = std::string(foundation::to_string(e.state()));
            r.recovery_action = std::string(foundation::to_string(e.recovery_action()));
            r.timestamp = e.timestamp().nanoseconds() == 0
                              ? "NOT AVAILABLE"
                              : std::to_string(e.timestamp().nanoseconds());
            p.rows.push_back(std::move(r));
        }
        return p;
    }

    static KnowledgePanel knowledge(const PanelSources& src) {
        KnowledgePanel p;
        p.identities = src.knowledge_identities;
        p.revisions = src.knowledge.size();
        p.available = src.knowledge_identities > 0 && !src.knowledge.empty();
        for (const learning::KnowledgeObject& k : src.knowledge) {
            KnowledgeRow r;
            r.knowledge_id = k.knowledge_id.valid() ? k.knowledge_id.value() : "NOT AVAILABLE";
            r.observation = k.observation;
            r.status = std::string(learning::to_string(k.status));
            r.validity_scope = k.validity_scope;
            r.revision = k.revision;
            p.rows.push_back(std::move(r));
        }
        return p;
    }

    static ResearchPanel research(const PanelSources& src) {
        ResearchPanel p;
        p.total = src.experiments.size();
        p.available = !src.experiments.empty();
        for (const research::Experiment& e : src.experiments) {
            ResearchRow r;
            r.experiment_id = e.experiment_id.valid() ? e.experiment_id.value() : "NOT AVAILABLE";
            r.question = e.question;
            r.decision = std::string(research::to_string(e.decision));
            r.evidence_zone = std::string(research::to_string(e.evidence_zone));
            r.contamination = std::string(research::to_string(e.contamination));
            p.rows.push_back(std::move(r));
        }
        return p;
    }

    static CandidatePanel candidates(const PanelSources& src) {
        CandidatePanel p;
        p.population = src.candidate_population;
        p.available = src.candidate_population > 0 && !src.candidates.empty();
        for (const evolution::Candidate& c : src.candidates) {
            CandidateRow r;
            r.candidate_id = c.candidate_id.valid() ? c.candidate_id.value() : "NOT AVAILABLE";
            r.parent_version = c.parent_version;
            r.change_type = std::string(evolution::to_string(c.change_type));
            r.state = std::string(evolution::to_string(c.state));
            r.revision = c.revision;
            p.rows.push_back(std::move(r));
        }
        return p;
    }

    static ApprovalPanel approvals(const PanelSources& src) {
        ApprovalPanel p;
        p.total = src.approvals.size();
        p.available = !src.approvals.empty();
        for (const governance::DecisionRecord& d : src.approvals) {
            ApprovalRow r;
            r.decision_id = d.decision_id.valid() ? d.decision_id.value() : "NOT AVAILABLE";
            r.question = d.question;
            r.status = std::string(governance::to_string(d.status));
            r.actor = d.actor;
            p.rows.push_back(std::move(r));
        }
        return p;
    }

    static EvolutionPanel evolution(const PanelSources& src) {
        EvolutionPanel p;
        for (const evolution::EvolutionNode& n : src.evolution_nodes) {
            if (!n.valid) continue;
            ++p.nodes;
            if (!n.parent_version.empty()) p.edges.push_back({n.parent_version, n.version});
        }
        p.available = p.nodes > 0;
        return p;
    }

    static CheckpointPanel checkpoints(const PanelSources& src) {
        CheckpointPanel p;
        p.total = src.checkpoints.size();
        p.available = !src.checkpoints.empty();
        for (auto it = src.checkpoints.rbegin();
             it != src.checkpoints.rend() && p.recent.size() < kRecentLimit; ++it) {
            const operatingwindow::Checkpoint& c = *it;
            CheckpointRow r;
            r.checkpoint_id = c.checkpoint_id.valid() ? c.checkpoint_id.value() : "NOT AVAILABLE";
            r.validity = std::string(operatingwindow::to_string(c.validity));
            r.schema_version = c.schema_version;
            p.recent.push_back(std::move(r));
        }
        return p;
    }

    static AuditPanel audit(const PanelSources& src, const std::vector<governance::AuditRecord>& records) {
        AuditPanel p;
        p.total = records.size();
        p.available = !records.empty();
        for (auto it = records.rbegin(); it != records.rend() && p.rows.size() < kRecentLimit; ++it) {
            const AuditViewModel vm = ControlCenterProjector::audit(*it);
            AuditRow r;
            r.category = vm.category;
            r.actor = vm.actor;
            r.action = vm.action;
            r.recorded_at = vm.recorded_at.nanoseconds() == 0
                                ? "NOT AVAILABLE"
                                : std::to_string(vm.recorded_at.nanoseconds());
            p.rows.push_back(std::move(r));
        }
        (void)src;
        return p;
    }

    static VersionPanel version(const runtime::ApplicationPipeline& pipeline) {
        VersionPanel p;
        const runtime::ApplicationPipeline::Config& c = pipeline.config();
        p.schema_version = c.schema_version;
        p.strategy_version = c.strategy_version;
        p.configuration_version = c.configuration_version;
        p.strategy_family = std::string(runtime::to_string(c.family));
        p.symbol = c.symbol;
        return p;
    }

    static ValidationPanel validation() { return ValidationPanel{}; }
    static SchedulePanel schedule() { return SchedulePanel{}; }

private:
    static std::string num(double v) {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "%.4f", v);
        return std::string(buffer);
    }
};

}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_CONTROLCENTERPANELS_H
