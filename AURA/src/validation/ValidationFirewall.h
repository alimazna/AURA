#ifndef AURA_VALIDATION_VALIDATIONFIREWALL_H
#define AURA_VALIDATION_VALIDATIONFIREWALL_H

#include "research/ResearchPlanner.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace aura {
namespace validation {

// Result of a single validation check.
enum class CheckResult : std::uint8_t {
    PASS = 0,
    FAIL,
    NOT_RUN,
};

constexpr std::string_view to_string(CheckResult r) noexcept {
    switch (r) {
        case CheckResult::PASS:    return "PASS";
        case CheckResult::FAIL:    return "FAIL";
        case CheckResult::NOT_RUN: return "NOT_RUN";
    }
    return "NOT_RUN";
}

// Overall firewall verdict (Master 14.1 boundary between "interesting" and "credible").
enum class FirewallVerdict : std::uint8_t {
    INSUFFICIENT = 0,  // required checks not all run/available
    CREDIBLE,          // required checks passed
    NOT_CREDIBLE,      // at least one required check failed
};

constexpr std::string_view to_string(FirewallVerdict v) noexcept {
    switch (v) {
        case FirewallVerdict::INSUFFICIENT: return "INSUFFICIENT";
        case FirewallVerdict::CREDIBLE:     return "CREDIBLE";
        case FirewallVerdict::NOT_CREDIBLE: return "NOT_CREDIBLE";
    }
    return "INSUFFICIENT";
}

struct CheckOutcome {
    research::ValidationMethod method{research::ValidationMethod::LEAKAGE_CHECKS};
    CheckResult result{CheckResult::NOT_RUN};
    std::string detail{};
};

// Validation firewall (Master 14).
//
// Phase 6 validation. Evaluates a pre-fixed validation plan against per-method
// outcomes. A missing outcome is NOT_RUN (never assumed PASS). Any FAIL makes the
// candidate NOT_CREDIBLE; all required checks PASS makes it CREDIBLE; otherwise
// INSUFFICIENT. It is a firewall, not a guarantee: CREDIBLE means the planned
// checks passed, not that the candidate is profitable or safe to promote.
// Deterministic; no I/O, no runtime mutation.
class ValidationFirewall {
public:
    static FirewallVerdict evaluate(const research::ValidationPlan& plan,
                                    const std::vector<CheckOutcome>& outcomes,
                                    std::vector<CheckOutcome>& evaluated) {
        evaluated.clear();
        bool any_fail = false;
        bool all_pass = !plan.methods.empty();
        for (research::ValidationMethod m : plan.methods) {
            const CheckOutcome* found = nullptr;
            for (const CheckOutcome& o : outcomes) {
                if (o.method == m) {
                    found = &o;
                    break;
                }
            }
            if (found == nullptr) {
                evaluated.push_back({m, CheckResult::NOT_RUN, "no outcome supplied"});
                all_pass = false;
                continue;
            }
            evaluated.push_back(*found);
            if (found->result == CheckResult::FAIL) {
                any_fail = true;
                all_pass = false;
            } else if (found->result != CheckResult::PASS) {
                all_pass = false;
            }
        }
        if (any_fail) return FirewallVerdict::NOT_CREDIBLE;
        if (all_pass) return FirewallVerdict::CREDIBLE;
        return FirewallVerdict::INSUFFICIENT;
    }

    // Never claims production readiness; only the boundary verdict.
    static bool allows_human_review(FirewallVerdict v) noexcept {
        return v == FirewallVerdict::CREDIBLE;
    }
};

}  // namespace validation
}  // namespace aura

#endif  // AURA_VALIDATION_VALIDATIONFIREWALL_H
