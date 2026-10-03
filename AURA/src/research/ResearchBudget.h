#ifndef AURA_RESEARCH_RESEARCHBUDGET_H
#define AURA_RESEARCH_RESEARCHBUDGET_H

#include <algorithm>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>

namespace aura {
namespace research {

// Budget categories (Master 13.1).
enum class BudgetCategory : std::uint8_t {
    COMPUTE_BUDGET = 0,
    EXPERIMENT_BUDGET,
    TRIAL_BUDGET,
    CANDIDATE_BUDGET,
    STRUCTURAL_MUTATION_BUDGET,
    ADAPTIVE_DECISION_BUDGET,
    HOLDOUT_QUERY_BUDGET,
    VALIDATION_RUN_BUDGET,
    BRANCH_BUDGET,
};

constexpr std::string_view to_string(BudgetCategory c) noexcept {
    switch (c) {
        case BudgetCategory::COMPUTE_BUDGET:            return "COMPUTE_BUDGET";
        case BudgetCategory::EXPERIMENT_BUDGET:         return "EXPERIMENT_BUDGET";
        case BudgetCategory::TRIAL_BUDGET:              return "TRIAL_BUDGET";
        case BudgetCategory::CANDIDATE_BUDGET:          return "CANDIDATE_BUDGET";
        case BudgetCategory::STRUCTURAL_MUTATION_BUDGET: return "STRUCTURAL_MUTATION_BUDGET";
        case BudgetCategory::ADAPTIVE_DECISION_BUDGET:  return "ADAPTIVE_DECISION_BUDGET";
        case BudgetCategory::HOLDOUT_QUERY_BUDGET:      return "HOLDOUT_QUERY_BUDGET";
        case BudgetCategory::VALIDATION_RUN_BUDGET:     return "VALIDATION_RUN_BUDGET";
        case BudgetCategory::BRANCH_BUDGET:             return "BRANCH_BUDGET";
    }
    return "UNKNOWN";
}

// Budget state (Master 13.3).
enum class BudgetState : std::uint8_t {
    GREEN = 0,  // within policy
    YELLOW,     // approaching limit
    RED,        // exhausted
    FROZEN,     // no further adaptive research allowed
};

constexpr std::string_view to_string(BudgetState s) noexcept {
    switch (s) {
        case BudgetState::GREEN:  return "GREEN";
        case BudgetState::YELLOW: return "YELLOW";
        case BudgetState::RED:    return "RED";
        case BudgetState::FROZEN: return "FROZEN";
    }
    return "GREEN";
}

// Research budget ledger (Master 13). Tracks spend per category against a declared
// limit. Budget is evidence, not just compute: the number of trials and adaptive
// decisions is part of the evidential record. Deterministic; refuses to fabricate
// capacity. A category with no declared limit is treated as unbounded but reported
// as GREEN (there is nothing to approach).
class ResearchBudget {
public:
    ResearchBudget() = default;

    void set_limit(BudgetCategory category, std::int64_t limit) { limits_[category] = limit; }

    void set_frozen(bool frozen) noexcept { frozen_ = frozen; }
    bool frozen() const noexcept { return frozen_; }

    std::int64_t spent(BudgetCategory category) const {
        const auto it = spent_.find(category);
        return it == spent_.end() ? 0 : it->second;
    }

    // Charges `amount` to a category. Refused (returns false) when the budget is
    // frozen, when `amount` is negative, or when the charge would exceed the
    // declared limit. On refusal nothing is charged (no partial spend).
    bool charge(BudgetCategory category, std::int64_t amount) {
        if (frozen_ || amount < 0) return false;
        const std::int64_t current = spent(category);
        const auto it = limits_.find(category);
        if (it != limits_.end() && current + amount > it->second) return false;
        spent_[category] = current + amount;
        return true;
    }

    // Remaining capacity, or -1 when unbounded.
    std::int64_t remaining(BudgetCategory category) const {
        const auto it = limits_.find(category);
        if (it == limits_.end()) return -1;
        return it->second - spent(category);
    }

    // GREEN < 70% used, YELLOW 70..99%, RED exhausted, FROZEN if globally frozen.
    BudgetState state(BudgetCategory category) const {
        if (frozen_) return BudgetState::FROZEN;
        const auto it = limits_.find(category);
        if (it == limits_.end()) return BudgetState::GREEN;
        if (it->second <= 0) return BudgetState::RED;
        const std::int64_t used = spent(category);
        if (used >= it->second) return BudgetState::RED;
        // 70% threshold using integer math to stay deterministic.
        if (used * 10 >= it->second * 7) return BudgetState::YELLOW;
        return BudgetState::GREEN;
    }

private:
    std::map<BudgetCategory, std::int64_t> limits_{};
    std::map<BudgetCategory, std::int64_t> spent_{};
    bool frozen_{false};
};

}  // namespace research
}  // namespace aura

#endif  // AURA_RESEARCH_RESEARCHBUDGET_H
