#ifndef AURA_VALIDATION_REWARDHACKING_H
#define AURA_VALIDATION_REWARDHACKING_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace aura {
namespace validation {

// Reward-hacking defenses (Master 17).
//
// Phase 6 validation. Candidate success is never reduced to a single mutable
// scalar. A candidate is only review-eligible when hard safety constraints hold
// AND statistical validity holds AND robustness holds AND quality objectives hold.
// Any failed hard constraint is a veto regardless of metric gain. Deterministic;
// no I/O.

struct Constraint {
    std::string name{};
    bool satisfied{true};
};

struct Objective {
    std::string name{};
    double value{0.0};
    double minimum{0.0};
    bool higher_is_better{true};
};

struct DefenseInput {
    std::vector<Constraint> hard_constraints{};
    bool statistical_validity{true};
    bool robustness{true};
    std::vector<Objective> quality_objectives{};
    double composite_metric{0.0};  // the single scalar that must NOT dominate
};

struct DefenseVerdict {
    bool review_eligible{false};
    bool vetoed{false};
    std::vector<std::string> failed_constraints{};
    std::vector<std::string> failed_objectives{};
};

class RewardHackingDefense {
public:
    // Hard constraints are vetoes; metrics are optimizers. Even a high composite
    // metric cannot overcome a failed hard constraint or absent validity/robustness.
    static DefenseVerdict assess(const DefenseInput& in) {
        DefenseVerdict v;
        for (const Constraint& c : in.hard_constraints) {
            if (!c.satisfied) {
                v.vetoed = true;
                v.failed_constraints.push_back(c.name);
            }
        }
        if (!in.statistical_validity) v.failed_objectives.push_back("statistical_validity");
        if (!in.robustness) v.failed_objectives.push_back("robustness");
        for (const Objective& o : in.quality_objectives) {
            const bool ok = o.higher_is_better ? (o.value >= o.minimum) : (o.value <= o.minimum);
            if (!ok) v.failed_objectives.push_back(o.name);
        }
        v.review_eligible = !v.vetoed && in.statistical_validity && in.robustness &&
                            v.failed_objectives.empty();
        return v;
    }
};

}  // namespace validation
}  // namespace aura

#endif  // AURA_VALIDATION_REWARDHACKING_H
