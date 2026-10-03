#ifndef AURA_RESEARCH_HYPOTHESIS_H
#define AURA_RESEARCH_HYPOTHESIS_H

#include "foundation/EntityId.h"
#include "foundation/Timestamp.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace aura {
namespace research {

// Problem classification (Master V2-11 section 65.1) — classifies the observed
// issue so the engine does not treat every problem as a strategy problem.
enum class ProblemClass : std::uint8_t {
    UNKNOWN = 0,
    DATA,
    SCHEMA,
    CLOCK,
    FEATURE,
    REGIME,
    ELIGIBILITY,
    SIGNAL,
    TIMING,
    RISK,
    EXECUTION,
    RECONCILIATION,
    PERSISTENCE,
    RESEARCH_METHOD,
};

constexpr std::string_view to_string(ProblemClass c) noexcept {
    switch (c) {
        case ProblemClass::UNKNOWN:          return "UNKNOWN";
        case ProblemClass::DATA:             return "DATA";
        case ProblemClass::SCHEMA:           return "SCHEMA";
        case ProblemClass::CLOCK:            return "CLOCK";
        case ProblemClass::FEATURE:          return "FEATURE";
        case ProblemClass::REGIME:           return "REGIME";
        case ProblemClass::ELIGIBILITY:      return "ELIGIBILITY";
        case ProblemClass::SIGNAL:           return "SIGNAL";
        case ProblemClass::TIMING:           return "TIMING";
        case ProblemClass::RISK:             return "RISK";
        case ProblemClass::EXECUTION:        return "EXECUTION";
        case ProblemClass::RECONCILIATION:   return "RECONCILIATION";
        case ProblemClass::PERSISTENCE:      return "PERSISTENCE";
        case ProblemClass::RESEARCH_METHOD:  return "RESEARCH_METHOD";
    }
    return "UNKNOWN";
}

// Hypothesis class (Master V2-11 section 65.2).
enum class HypothesisClass : std::uint8_t {
    UNKNOWN = 0,
    FEATURE_HYPOTHESIS,
    PARAMETER_HYPOTHESIS,
    RULE_HYPOTHESIS,
    REGIME_HYPOTHESIS,
    TIMING_HYPOTHESIS,
    RISK_HYPOTHESIS,
    EXECUTION_HYPOTHESIS,
    INTERACTION_HYPOTHESIS,
    STRUCTURAL_HYPOTHESIS,
    DATA_QUALITY_HYPOTHESIS,
    RESEARCH_METHOD_HYPOTHESIS,
};

constexpr std::string_view to_string(HypothesisClass c) noexcept {
    switch (c) {
        case HypothesisClass::UNKNOWN:                    return "UNKNOWN";
        case HypothesisClass::FEATURE_HYPOTHESIS:         return "FEATURE_HYPOTHESIS";
        case HypothesisClass::PARAMETER_HYPOTHESIS:       return "PARAMETER_HYPOTHESIS";
        case HypothesisClass::RULE_HYPOTHESIS:            return "RULE_HYPOTHESIS";
        case HypothesisClass::REGIME_HYPOTHESIS:          return "REGIME_HYPOTHESIS";
        case HypothesisClass::TIMING_HYPOTHESIS:          return "TIMING_HYPOTHESIS";
        case HypothesisClass::RISK_HYPOTHESIS:            return "RISK_HYPOTHESIS";
        case HypothesisClass::EXECUTION_HYPOTHESIS:       return "EXECUTION_HYPOTHESIS";
        case HypothesisClass::INTERACTION_HYPOTHESIS:     return "INTERACTION_HYPOTHESIS";
        case HypothesisClass::STRUCTURAL_HYPOTHESIS:      return "STRUCTURAL_HYPOTHESIS";
        case HypothesisClass::DATA_QUALITY_HYPOTHESIS:    return "DATA_QUALITY_HYPOTHESIS";
        case HypothesisClass::RESEARCH_METHOD_HYPOTHESIS: return "RESEARCH_METHOD_HYPOTHESIS";
    }
    return "UNKNOWN";
}

// Falsifiable hypothesis (Master section 10.1/65.3).
//
// Phase 4 research plane. Every hypothesis must declare what evidence would make
// it fail (`falsification`); a hypothesis without one is not research-ready. Kept
// separate from runtime: a hypothesis has no path to mutate runtime behaviour.
// It references parent knowledge objects (Phase 3) by identity only.
struct Hypothesis {
    foundation::EntityId hypothesis_id{};
    std::vector<foundation::EntityId> parent_knowledge_ids{};
    ProblemClass problem_class{ProblemClass::UNKNOWN};
    HypothesisClass hypothesis_class{HypothesisClass::UNKNOWN};
    std::string claim{};
    std::string affected_component{};
    std::string context_scope{};
    std::string expected_effect{};
    std::string success_criterion{};
    std::string falsification{};
    std::string proposed_change{};
    std::string primary_metric{};
    std::vector<std::string> secondary_metrics{};
    std::string required_data{};
    std::string safety_constraints{};
    std::string experiment_design{};
    std::int64_t budget{0};
    std::string parent_version{};
    foundation::Timestamp created_at{};

    // Falsifiability is mandatory (Master 10.2).
    bool falsifiable() const noexcept { return !falsification.empty(); }
    bool valid() const noexcept {
        return hypothesis_id.valid() && !claim.empty() && falsifiable();
    }
};

inline foundation::EntityId make_hypothesis_id(std::string_view claim,
                                               std::string_view component) {
    return foundation::EntityId("H|" + std::string(claim) + "|" + std::string(component));
}

}  // namespace research
}  // namespace aura

#endif  // AURA_RESEARCH_HYPOTHESIS_H
