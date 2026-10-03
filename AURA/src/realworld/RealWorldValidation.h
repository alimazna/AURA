#ifndef AURA_REALWORLD_REALWORLDVALIDATION_H
#define AURA_REALWORLD_REALWORLDVALIDATION_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace aura {
namespace realworld {

// Preconditions for controlled real-world validation (V3-40 Phase 11: "Only after
// the preceding layers are stable and the applicable evidence/safety gates have
// been satisfied").
enum class ReadinessCheck : std::uint8_t {
    PRECEDING_PHASES_STABLE = 0,
    EVIDENCE_GATES_SATISFIED,
    SAFETY_GATES_SATISFIED,
    SHADOW_EVIDENCE_PRESENT,
    OPERATING_WINDOW_DEFINED,
    ROLLBACK_TARGET_KNOWN_GOOD,
    MONITORING_READY,
    HUMAN_AUTHORIZATION_PRESENT,
    REAL_MT5_ENVIRONMENT_AVAILABLE,
};

constexpr std::string_view to_string(ReadinessCheck c) noexcept {
    switch (c) {
        case ReadinessCheck::PRECEDING_PHASES_STABLE:      return "PRECEDING_PHASES_STABLE";
        case ReadinessCheck::EVIDENCE_GATES_SATISFIED:     return "EVIDENCE_GATES_SATISFIED";
        case ReadinessCheck::SAFETY_GATES_SATISFIED:       return "SAFETY_GATES_SATISFIED";
        case ReadinessCheck::SHADOW_EVIDENCE_PRESENT:      return "SHADOW_EVIDENCE_PRESENT";
        case ReadinessCheck::OPERATING_WINDOW_DEFINED:     return "OPERATING_WINDOW_DEFINED";
        case ReadinessCheck::ROLLBACK_TARGET_KNOWN_GOOD:   return "ROLLBACK_TARGET_KNOWN_GOOD";
        case ReadinessCheck::MONITORING_READY:             return "MONITORING_READY";
        case ReadinessCheck::HUMAN_AUTHORIZATION_PRESENT:  return "HUMAN_AUTHORIZATION_PRESENT";
        case ReadinessCheck::REAL_MT5_ENVIRONMENT_AVAILABLE: return "REAL_MT5_ENVIRONMENT_AVAILABLE";
    }
    return "UNKNOWN";
}

struct CheckState {
    ReadinessCheck check{ReadinessCheck::PRECEDING_PHASES_STABLE};
    bool satisfied{false};
    std::string detail{};
};

struct ReadinessDecision {
    bool ready_for_controlled_validation{false};
    std::vector<ReadinessCheck> unmet{};
    std::string reason{};
};

// Controlled real-world validation gate (Phase 11).
//
// Phase 11 is the FINAL step and is deliberately gate-controlled. This type only
// decides whether the preconditions are met; it does not execute any live trading,
// does not enable unattended execution, and makes no profitability, calibrated
// probability, broker-validation or production-safety claim. A missing or unknown
// check is never assumed satisfied. Execution remains a separate human action.
class RealWorldValidation {
public:
    static const std::vector<ReadinessCheck>& required_checks() {
        static const std::vector<ReadinessCheck> checks{
            ReadinessCheck::PRECEDING_PHASES_STABLE,          ReadinessCheck::EVIDENCE_GATES_SATISFIED,
            ReadinessCheck::SAFETY_GATES_SATISFIED,           ReadinessCheck::SHADOW_EVIDENCE_PRESENT,
            ReadinessCheck::OPERATING_WINDOW_DEFINED,         ReadinessCheck::ROLLBACK_TARGET_KNOWN_GOOD,
            ReadinessCheck::MONITORING_READY,                 ReadinessCheck::HUMAN_AUTHORIZATION_PRESENT,
            ReadinessCheck::REAL_MT5_ENVIRONMENT_AVAILABLE};
        return checks;
    }

    static ReadinessDecision evaluate(const std::vector<CheckState>& checks) {
        ReadinessDecision d;
        for (ReadinessCheck required : required_checks()) {
            bool satisfied = false;
            for (const CheckState& c : checks) {
                if (c.check == required && c.satisfied) {
                    satisfied = true;
                    break;
                }
            }
            if (!satisfied) d.unmet.push_back(required);
        }
        d.ready_for_controlled_validation = d.unmet.empty();
        d.reason = d.ready_for_controlled_validation
                       ? "all readiness gates satisfied (execution remains a separate human action)"
                       : "readiness gates unmet";
        return d;
    }

    // Real-world validation is never automatic and never unattended.
    static bool may_execute_automatically() noexcept { return false; }

    // No claim beyond the gates. Callers must not derive these from this layer.
    static bool claims_profitability() noexcept { return false; }
    static bool claims_production_safety() noexcept { return false; }
};

}  // namespace realworld
}  // namespace aura

#endif  // AURA_REALWORLD_REALWORLDVALIDATION_H
