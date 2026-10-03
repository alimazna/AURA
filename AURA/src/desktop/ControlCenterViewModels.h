#ifndef AURA_DESKTOP_CONTROLCENTERVIEWMODELS_H
#define AURA_DESKTOP_CONTROLCENTERVIEWMODELS_H

#include "evolution/Candidate.h"
#include "foundation/EntityId.h"
#include "foundation/Timestamp.h"
#include "governance/HumanDecision.h"
#include "learning/KnowledgeObject.h"
#include "operatingwindow/OperatingWindow.h"
#include "research/Experiment.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace aura {
namespace desktop {

// Phase 9 is a presentation layer. These are read-only, deterministic data
// projections that a UI shell would render. They contain no GUI toolkit, no
// network, no filesystem I/O, and no logic that mutates runtime. Values are
// copied from the trusted logic layers; nothing is fabricated. Where a value is
// unknown it is shown as UNKNOWN rather than defaulting to a healthy/green state.

// Dashboard tile for a subsystem health/state indicator.
enum class TileStatus : std::uint8_t { UNKNOWN = 0, OK, WARNING, CRITICAL };

constexpr std::string_view to_string(TileStatus s) noexcept {
    switch (s) {
        case TileStatus::UNKNOWN:  return "UNKNOWN";
        case TileStatus::OK:       return "OK";
        case TileStatus::WARNING:  return "WARNING";
        case TileStatus::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}

struct DashboardTile {
    std::string name{};
    TileStatus status{TileStatus::UNKNOWN};
    std::string detail{};
};

struct DashboardViewModel {
    std::string project_name{};
    std::string operating_mode{};
    std::string window_phase{};
    std::vector<DashboardTile> tiles{};
    bool has_unknown_state{false};
};

struct KnowledgeViewModel {
    foundation::EntityId knowledge_id{};
    std::string observation{};
    std::string status{};
    std::string validity_scope{};
    double confidence_ranking{0.0};
    std::uint32_t revision{0};
};

struct ResearchViewModel {
    foundation::EntityId experiment_id{};
    std::string question{};
    std::string decision{};
    std::string evidence_zone{};
    std::string contamination{};
    bool duplicate{false};
};

struct CandidateViewModel {
    foundation::EntityId candidate_id{};
    std::string parent_version{};
    std::string change_type{};
    std::string state{};
    std::uint32_t revision{0};
};

struct ApprovalViewModel {
    foundation::EntityId decision_id{};
    std::string question{};
    std::string status{};
    std::string actor{};
};

struct EvolutionEdgeViewModel {
    std::string parent_version{};
    std::string child_version{};
};

struct EvolutionGraphViewModel {
    std::vector<std::string> versions{};
    std::vector<EvolutionEdgeViewModel> edges{};
};

struct IncidentViewModel {
    foundation::EntityId incident_id{};
    std::string severity{};
    std::string summary{};
};

struct ScheduleViewModel {
    std::string actor{};
    std::int64_t duration_seconds{0};
    bool accepted{false};
    std::string reason{};
};

struct AuditViewModel {
    foundation::EntityId audit_id{};
    std::string category{};
    std::string actor{};
    std::string action{};
    foundation::Timestamp recorded_at{};
};

}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_CONTROLCENTERVIEWMODELS_H
