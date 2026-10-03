#ifndef AURA_GOVERNANCE_FORBIDDENBEHAVIOR_H
#define AURA_GOVERNANCE_FORBIDDENBEHAVIOR_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace aura {
namespace governance {

// Hard-forbidden behaviors (Master 39). Rejected at the architecture/security boundary.
enum class ForbiddenBehavior : std::uint8_t {
    SELF_MODIFY_PRODUCTION = 0,       // 01
    RESEARCH_WRITE_PRODUCTION,        // 02
    CANDIDATE_MODIFY_EVALUATOR,       // 03
    CANDIDATE_MODIFY_METRICS,         // 04
    DELETE_FAILED_EXPERIMENTS,        // 05
    EDIT_HISTORICAL_RESULTS,          // 06
    RAW_HOLDOUT_ACCESS,               // 07
    REOPTIMIZE_ON_HOLDOUT,            // 08
    UNLIMITED_EXPERIMENTATION,        // 09
    FUTURE_INFORMATION_ACCESS,        // 10
    TIMESTAMP_MANIPULATION,           // 11
    HIDDEN_EXTERNAL_DATA,             // 12
    UNAUTHORIZED_DATA_SOURCE_CHANGE,  // 13
    MODIFY_RISK_CONSTRAINTS,          // 14
    BYPASS_APPROVAL_GATES,            // 15
    HIDDEN_EVALUATOR_SIDE_CHANNEL,    // 16
    OPTIMIZE_EVALUATOR_NOT_OBJECTIVE, // 17
    SUPPRESS_FAILURE_SIGNALS,         // 18
    SELF_PROMOTION,                   // 19
    SELF_CREATE_CREDENTIALS,          // 20
    MODIFY_AUDIT_LOGGER,              // 21
    ALTER_ROLLBACK_TARGET,            // 22
    CANDIDATE_SELF_EVALUATION,        // 23
    CIRCULAR_METRIC_CHANGES,          // 24
};

constexpr std::string_view to_string(ForbiddenBehavior b) noexcept {
    switch (b) {
        case ForbiddenBehavior::SELF_MODIFY_PRODUCTION:       return "01_SELF_MODIFY_PRODUCTION";
        case ForbiddenBehavior::RESEARCH_WRITE_PRODUCTION:    return "02_RESEARCH_WRITE_PRODUCTION";
        case ForbiddenBehavior::CANDIDATE_MODIFY_EVALUATOR:   return "03_CANDIDATE_MODIFY_EVALUATOR";
        case ForbiddenBehavior::CANDIDATE_MODIFY_METRICS:     return "04_CANDIDATE_MODIFY_METRICS";
        case ForbiddenBehavior::DELETE_FAILED_EXPERIMENTS:    return "05_DELETE_FAILED_EXPERIMENTS";
        case ForbiddenBehavior::EDIT_HISTORICAL_RESULTS:      return "06_EDIT_HISTORICAL_RESULTS";
        case ForbiddenBehavior::RAW_HOLDOUT_ACCESS:           return "07_RAW_HOLDOUT_ACCESS";
        case ForbiddenBehavior::REOPTIMIZE_ON_HOLDOUT:        return "08_REOPTIMIZE_ON_HOLDOUT";
        case ForbiddenBehavior::UNLIMITED_EXPERIMENTATION:    return "09_UNLIMITED_EXPERIMENTATION";
        case ForbiddenBehavior::FUTURE_INFORMATION_ACCESS:    return "10_FUTURE_INFORMATION_ACCESS";
        case ForbiddenBehavior::TIMESTAMP_MANIPULATION:       return "11_TIMESTAMP_MANIPULATION";
        case ForbiddenBehavior::HIDDEN_EXTERNAL_DATA:         return "12_HIDDEN_EXTERNAL_DATA";
        case ForbiddenBehavior::UNAUTHORIZED_DATA_SOURCE_CHANGE: return "13_UNAUTHORIZED_DATA_SOURCE_CHANGE";
        case ForbiddenBehavior::MODIFY_RISK_CONSTRAINTS:      return "14_MODIFY_RISK_CONSTRAINTS";
        case ForbiddenBehavior::BYPASS_APPROVAL_GATES:        return "15_BYPASS_APPROVAL_GATES";
        case ForbiddenBehavior::HIDDEN_EVALUATOR_SIDE_CHANNEL: return "16_HIDDEN_EVALUATOR_SIDE_CHANNEL";
        case ForbiddenBehavior::OPTIMIZE_EVALUATOR_NOT_OBJECTIVE: return "17_OPTIMIZE_EVALUATOR_NOT_OBJECTIVE";
        case ForbiddenBehavior::SUPPRESS_FAILURE_SIGNALS:     return "18_SUPPRESS_FAILURE_SIGNALS";
        case ForbiddenBehavior::SELF_PROMOTION:               return "19_SELF_PROMOTION";
        case ForbiddenBehavior::SELF_CREATE_CREDENTIALS:      return "20_SELF_CREATE_CREDENTIALS";
        case ForbiddenBehavior::MODIFY_AUDIT_LOGGER:          return "21_MODIFY_AUDIT_LOGGER";
        case ForbiddenBehavior::ALTER_ROLLBACK_TARGET:        return "22_ALTER_ROLLBACK_TARGET";
        case ForbiddenBehavior::CANDIDATE_SELF_EVALUATION:    return "23_CANDIDATE_SELF_EVALUATION";
        case ForbiddenBehavior::CIRCULAR_METRIC_CHANGES:      return "24_CIRCULAR_METRIC_CHANGES";
    }
    return "UNKNOWN";
}

// A proposed action that a governance boundary can screen.
struct ProposedAction {
    std::string actor{};       // "research", "candidate", "runtime", "human"
    std::string description{};
    std::vector<ForbiddenBehavior> requested_behaviors{};
};

// Governance boundary (Master 39).
//
// Phase 7 governance. Screens proposed actions against the hard-forbidden list.
// An action touching any forbidden behavior is rejected (not downgraded, not
// warned-and-allowed). It never executes the action; it only decides admissibility
// and produces an audit trail. Deterministic; no I/O, no runtime mutation.
class ForbiddenBehaviorGuard {
public:
    struct Decision {
        bool allowed{true};
        std::vector<ForbiddenBehavior> violations{};
        std::string reason{};
    };

    static Decision screen(const ProposedAction& action) {
        Decision d;
        if (action.actor.empty()) {
            d.allowed = false;
            d.reason = "unattributed action";
            return d;
        }
        for (ForbiddenBehavior b : action.requested_behaviors) {
            if (is_forbidden_for(action.actor, b)) d.violations.push_back(b);
        }
        if (!d.violations.empty()) {
            d.allowed = false;
            d.reason = "requested forbidden behavior(s)";
        } else {
            d.reason = "allowed";
        }
        return d;
    }

    // Whether an actor is forbidden from a behavior.
    //
    // Two classes of forbidden behavior:
    //  - Unconditionally forbidden (system integrity anchors): no actor may ever
    //    request them (deleting failures, editing history, raw holdout, future
    //    info, timestamp manipulation, hidden channels, self-credentials,
    //    suppressing failures, circular metrics, candidate self-evaluation).
    //  - Governed actions: forbidden to automated actors (research/candidate/
    //    runtime/meta) but requestable by an explicit trusted human maintainer,
    //    which is how a legitimate governed deployment/rollback/evaluator change
    //    is carried out (Master 31/32/16). Even then the request is only screened
    //    here; it still requires the approval gate.
    static bool is_forbidden_for(const std::string& actor, ForbiddenBehavior b) noexcept {
        const bool human = (actor == "trusted_maintainer");
        switch (b) {
            // Unconditionally forbidden integrity anchors.
            case ForbiddenBehavior::DELETE_FAILED_EXPERIMENTS:
            case ForbiddenBehavior::EDIT_HISTORICAL_RESULTS:
            case ForbiddenBehavior::RAW_HOLDOUT_ACCESS:
            case ForbiddenBehavior::FUTURE_INFORMATION_ACCESS:
            case ForbiddenBehavior::TIMESTAMP_MANIPULATION:
            case ForbiddenBehavior::HIDDEN_EXTERNAL_DATA:
            case ForbiddenBehavior::HIDDEN_EVALUATOR_SIDE_CHANNEL:
            case ForbiddenBehavior::SUPPRESS_FAILURE_SIGNALS:
            case ForbiddenBehavior::SELF_CREATE_CREDENTIALS:
            case ForbiddenBehavior::CANDIDATE_SELF_EVALUATION:
            case ForbiddenBehavior::CIRCULAR_METRIC_CHANGES:
            case ForbiddenBehavior::CANDIDATE_MODIFY_EVALUATOR:
            case ForbiddenBehavior::CANDIDATE_MODIFY_METRICS:
            case ForbiddenBehavior::BYPASS_APPROVAL_GATES:
            case ForbiddenBehavior::SELF_PROMOTION:
            case ForbiddenBehavior::OPTIMIZE_EVALUATOR_NOT_OBJECTIVE:
            case ForbiddenBehavior::UNLIMITED_EXPERIMENTATION:
            case ForbiddenBehavior::REOPTIMIZE_ON_HOLDOUT:
                return true;
            // Governed actions: requestable by a trusted human maintainer only.
            case ForbiddenBehavior::SELF_MODIFY_PRODUCTION:
            case ForbiddenBehavior::RESEARCH_WRITE_PRODUCTION:
            case ForbiddenBehavior::UNAUTHORIZED_DATA_SOURCE_CHANGE:
            case ForbiddenBehavior::MODIFY_RISK_CONSTRAINTS:
            case ForbiddenBehavior::MODIFY_AUDIT_LOGGER:
            case ForbiddenBehavior::ALTER_ROLLBACK_TARGET:
                return !human;
        }
        return true;
    }
};

}  // namespace governance
}  // namespace aura

#endif  // AURA_GOVERNANCE_FORBIDDENBEHAVIOR_H
