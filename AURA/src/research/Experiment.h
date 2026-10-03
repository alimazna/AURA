#ifndef AURA_RESEARCH_EXPERIMENT_H
#define AURA_RESEARCH_EXPERIMENT_H

#include "foundation/EntityId.h"
#include "foundation/Timestamp.h"
#include "research/Hypothesis.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace aura {
namespace research {

// Experiment outcome (Master 11.3).
enum class ExperimentOutcome : std::uint8_t {
    UNRESOLVED = 0,
    PROMISING,
    INCONCLUSIVE,
    FALSIFIED,
    REGRESSED,
    CONTAMINATED,
    DUPLICATE,
    BUDGET_REJECTED,
    POLICY_REJECTED,
    NO_CHANGE,
};

constexpr std::string_view to_string(ExperimentOutcome o) noexcept {
    switch (o) {
        case ExperimentOutcome::UNRESOLVED:      return "UNRESOLVED";
        case ExperimentOutcome::PROMISING:       return "PROMISING";
        case ExperimentOutcome::INCONCLUSIVE:    return "INCONCLUSIVE";
        case ExperimentOutcome::FALSIFIED:       return "FALSIFIED";
        case ExperimentOutcome::REGRESSED:       return "REGRESSED";
        case ExperimentOutcome::CONTAMINATED:    return "CONTAMINATED";
        case ExperimentOutcome::DUPLICATE:       return "DUPLICATE";
        case ExperimentOutcome::BUDGET_REJECTED: return "BUDGET_REJECTED";
        case ExperimentOutcome::POLICY_REJECTED: return "POLICY_REJECTED";
        case ExperimentOutcome::NO_CHANGE:       return "NO_CHANGE";
    }
    return "UNRESOLVED";
}

// Evidence zone (Master 15.1) — where the experiment was allowed to look.
enum class EvidenceZone : std::uint8_t {
    E0_EXPLORATION = 0,
    E1_DEVELOPMENT,
    E2_OUT_OF_SAMPLE,
    E3_LOCKED_HOLDOUT,
};

constexpr std::string_view to_string(EvidenceZone z) noexcept {
    switch (z) {
        case EvidenceZone::E0_EXPLORATION:  return "E0";
        case EvidenceZone::E1_DEVELOPMENT:  return "E1";
        case EvidenceZone::E2_OUT_OF_SAMPLE: return "E2";
        case EvidenceZone::E3_LOCKED_HOLDOUT: return "E3";
    }
    return "E0";
}

// Contamination state machine (Master 15.3).
enum class ContaminationState : std::uint8_t {
    CLEAN = 0,
    VALIDATED,
    OOS_EXPOSED,
    HOLDOUT_EXPOSED,
    CONTAMINATED,
    RETIRED,
};

constexpr std::string_view to_string(ContaminationState s) noexcept {
    switch (s) {
        case ContaminationState::CLEAN:           return "CLEAN";
        case ContaminationState::VALIDATED:       return "VALIDATED";
        case ContaminationState::OOS_EXPOSED:     return "OOS_EXPOSED";
        case ContaminationState::HOLDOUT_EXPOSED: return "HOLDOUT_EXPOSED";
        case ContaminationState::CONTAMINATED:    return "CONTAMINATED";
        case ContaminationState::RETIRED:         return "RETIRED";
    }
    return "CLEAN";
}

// Experiment object (Master 11.1). Immutable research record. Reproducibility is
// carried by explicit dataset/feature/evaluator/environment/control version
// strings, not by hidden state.
struct Experiment {
    foundation::EntityId experiment_id{};
    std::string campaign_id{};
    std::string parent_version{};
    foundation::EntityId hypothesis_id{};
    foundation::EntityId failure_id{};
    std::string question{};
    std::string dataset_version{};
    std::string feature_version{};
    std::string parameter_space{};
    std::string candidate_generator_version{};
    std::string evaluator_version{};
    std::string metric_registry_version{};
    std::string environment_version{};
    std::string control_version{};
    std::string candidate_definition{};
    std::int64_t trial_count{0};
    std::string random_seed_policy{};
    std::int64_t budget{0};
    std::string validation_protocol{};
    std::string result{};
    ExperimentOutcome decision{ExperimentOutcome::UNRESOLVED};
    std::string failure_reason{};
    std::int64_t holdout_exposure{0};
    EvidenceZone evidence_zone{EvidenceZone::E0_EXPLORATION};
    ContaminationState contamination{ContaminationState::CLEAN};
    foundation::Timestamp created_at{};

    bool valid() const noexcept { return experiment_id.valid() && !question.empty(); }
};

inline foundation::EntityId make_experiment_id(std::string_view campaign,
                                               std::string_view question,
                                               std::string_view candidate_definition) {
    return foundation::EntityId("E|" + std::string(campaign) + "|" + std::string(question) + "|" +
                                std::string(candidate_definition));
}

}  // namespace research
}  // namespace aura

#endif  // AURA_RESEARCH_EXPERIMENT_H
