#ifndef AURA_GOVERNANCE_PROMOTIONGATE_H
#define AURA_GOVERNANCE_PROMOTIONGATE_H

#include "evolution/Candidate.h"
#include "foundation/EntityId.h"
#include "foundation/Timestamp.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace aura {
namespace governance {

// Promotion gate checks (Master 31).
enum class GateCheck : std::uint8_t {
    HUMAN_APPROVAL = 0,
    EVIDENCE_COMPLETE,
    CANDIDATE_HASH_VERIFIED,
    EVALUATOR_VERIFIED,
    METRIC_REGISTRY_VERIFIED,
    NO_FORBIDDEN_STATE,
    ROLLBACK_TARGET_READY,
    MONITORING_READY,
    SHADOW_REQUIREMENT_SATISFIED,
    POLICY_VERSION_COMPATIBLE,
};

constexpr std::string_view to_string(GateCheck c) noexcept {
    switch (c) {
        case GateCheck::HUMAN_APPROVAL:              return "HUMAN_APPROVAL";
        case GateCheck::EVIDENCE_COMPLETE:           return "EVIDENCE_COMPLETE";
        case GateCheck::CANDIDATE_HASH_VERIFIED:     return "CANDIDATE_HASH_VERIFIED";
        case GateCheck::EVALUATOR_VERIFIED:          return "EVALUATOR_VERIFIED";
        case GateCheck::METRIC_REGISTRY_VERIFIED:    return "METRIC_REGISTRY_VERIFIED";
        case GateCheck::NO_FORBIDDEN_STATE:          return "NO_FORBIDDEN_STATE";
        case GateCheck::ROLLBACK_TARGET_READY:       return "ROLLBACK_TARGET_READY";
        case GateCheck::MONITORING_READY:            return "MONITORING_READY";
        case GateCheck::SHADOW_REQUIREMENT_SATISFIED: return "SHADOW_REQUIREMENT_SATISFIED";
        case GateCheck::POLICY_VERSION_COMPATIBLE:   return "POLICY_VERSION_COMPATIBLE";
    }
    return "UNKNOWN";
}

struct CheckState {
    GateCheck check{GateCheck::HUMAN_APPROVAL};
    bool satisfied{false};
    std::string detail{};
};

// Promotion package. Immutable; carries the candidate, its human decision and the
// per-gate state.
struct PromotionPackage {
    foundation::EntityId package_id{};
    foundation::EntityId candidate_id{};
    std::string candidate_hash{};
    std::string human_decision_ref{};
    std::vector<CheckState> checks{};
    foundation::Timestamp assembled_at{};

    bool valid() const noexcept { return package_id.valid() && candidate_id.valid(); }
};

// Promotion gate decision.
struct GateDecision {
    bool promotion_ready{false};
    std::vector<GateCheck> unmet{};
    std::string reason{};
};

// Promotion gate (Master 31).
//
// Phase 7 governance. A candidate is promotion-ready ONLY when every required gate
// is satisfied. Promotion itself is a separate control-plane action: this gate
// never mutates runtime and never promotes (Master 31 "passing research tests does
// not automatically mutate production"). Deterministic; no I/O.
class PromotionGate {
public:
    static const std::vector<GateCheck>& required_checks() {
        static const std::vector<GateCheck> checks{
            GateCheck::HUMAN_APPROVAL,           GateCheck::EVIDENCE_COMPLETE,
            GateCheck::CANDIDATE_HASH_VERIFIED,  GateCheck::EVALUATOR_VERIFIED,
            GateCheck::METRIC_REGISTRY_VERIFIED, GateCheck::NO_FORBIDDEN_STATE,
            GateCheck::ROLLBACK_TARGET_READY,    GateCheck::MONITORING_READY,
            GateCheck::SHADOW_REQUIREMENT_SATISFIED, GateCheck::POLICY_VERSION_COMPATIBLE};
        return checks;
    }

    static GateDecision evaluate(const PromotionPackage& package,
                                 const evolution::Candidate& candidate) {
        GateDecision d;
        if (!package.valid() || !candidate.valid()) {
            d.reason = "invalid package or candidate";
            return d;
        }
        // Shadow requirement (Master 31): a candidate that has not reached the
        // shadow stage cannot be promotion-ready.
        const bool shadow_ok =
            candidate.state == evolution::CandidateState::SHADOW_ACTIVE ||
            candidate.state == evolution::CandidateState::PROMOTION_READY;
        for (GateCheck required : required_checks()) {
            bool satisfied = false;
            for (const CheckState& c : package.checks) {
                if (c.check == required && c.satisfied) {
                    satisfied = true;
                    break;
                }
            }
            if (required == GateCheck::HUMAN_APPROVAL) {
                satisfied = satisfied && !package.human_decision_ref.empty();
            }
            if (required == GateCheck::SHADOW_REQUIREMENT_SATISFIED) {
                satisfied = satisfied && shadow_ok;
            }
            if (!satisfied) d.unmet.push_back(required);
        }
        d.promotion_ready = d.unmet.empty();
        d.reason = d.promotion_ready ? "all gates satisfied" : "unmet gates remain";
        return d;
    }

    // Always false: the gate cannot self-authorize a deployment (Master 39 #19).
    static bool can_self_promote() noexcept { return false; }
};

}  // namespace governance
}  // namespace aura

#endif  // AURA_GOVERNANCE_PROMOTIONGATE_H
