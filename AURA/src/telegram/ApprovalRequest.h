#ifndef AURA_TELEGRAM_APPROVALREQUEST_H
#define AURA_TELEGRAM_APPROVALREQUEST_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace aura {
namespace telegram {

// Steps of the Telegram approval pipeline (Master 29 "Telegram authority rule").
// Telegram never outranks the control plane: a request must traverse every step.
enum class ApprovalStep : std::uint8_t {
    VERIFY_PROPOSAL_ID = 0,
    VERIFY_USER_AUTHORIZATION,
    VERIFY_CANDIDATE_HASH,
    VERIFY_CURRENT_STATE,
    RECORD_APPROVAL,
    PROMOTION_MANAGER,
};

constexpr std::string_view to_string(ApprovalStep s) noexcept {
    switch (s) {
        case ApprovalStep::VERIFY_PROPOSAL_ID:        return "VERIFY_PROPOSAL_ID";
        case ApprovalStep::VERIFY_USER_AUTHORIZATION: return "VERIFY_USER_AUTHORIZATION";
        case ApprovalStep::VERIFY_CANDIDATE_HASH:     return "VERIFY_CANDIDATE_HASH";
        case ApprovalStep::VERIFY_CURRENT_STATE:      return "VERIFY_CURRENT_STATE";
        case ApprovalStep::RECORD_APPROVAL:           return "RECORD_APPROVAL";
        case ApprovalStep::PROMOTION_MANAGER:         return "PROMOTION_MANAGER";
    }
    return "VERIFY_PROPOSAL_ID";
}

// A bounded approval request received over Telegram.
struct ApprovalRequest {
    std::string proposal_id{};
    std::string user_id{};        // Telegram user id, must be authorized
    std::string candidate_hash{}; // must match the trusted registry
    std::string version{};        // candidate/version under review
};

// Result of walking the approval pipeline.
struct ApprovalOutcome {
    bool forwarded_to_promotion_manager{false};
    std::vector<ApprovalStep> completed_steps{};
    ApprovalStep stopped_at{ApprovalStep::VERIFY_PROPOSAL_ID};
    std::string reason{};
};

// Trusted context the gateway verifies against. Supplied by the control plane;
// the gateway never invents these values.
struct ApprovalContext {
    std::vector<std::string> known_proposal_ids{};
    std::vector<std::string> known_candidate_hashes{};
    std::string current_state{};          // e.g. "SHADOW_ACTIVE"
    std::string required_state{};         // state required to accept an approval
    bool promotion_manager_available{true};
};

// Telegram approval gateway (Master 29).
//
// Phase 10 auxiliary Telegram. A Telegram request is only a request: it must pass
// through the approval service, verifying proposal id, user authorization,
// candidate hash and current state before an approval is recorded and forwarded to
// the promotion manager. Any failure stops the pipeline; a Telegram failure can
// never cause an arbitrary production mutation. Deterministic; no network here the
// transport is injected by the caller.
class TelegramApprovalGateway {
public:
    // Evaluates the pipeline. Only when every verification passes does the request
    // reach the promotion manager (which itself is a separate governance action).
    static ApprovalOutcome process(const ApprovalRequest& request, const ApprovalContext& ctx,
                                   const std::vector<std::string>& authorized_user_ids) {
        ApprovalOutcome out;
        // 1. proposal id known
        if (!contains(ctx.known_proposal_ids, request.proposal_id)) {
            out.stopped_at = ApprovalStep::VERIFY_PROPOSAL_ID;
            out.reason = "unknown proposal id";
            return out;
        }
        out.completed_steps.push_back(ApprovalStep::VERIFY_PROPOSAL_ID);
        // 2. user authorized
        if (!contains(authorized_user_ids, request.user_id)) {
            out.stopped_at = ApprovalStep::VERIFY_USER_AUTHORIZATION;
            out.reason = "user not authorized";
            return out;
        }
        out.completed_steps.push_back(ApprovalStep::VERIFY_USER_AUTHORIZATION);
        // 3. candidate hash matches trusted registry
        if (!contains(ctx.known_candidate_hashes, request.candidate_hash)) {
            out.stopped_at = ApprovalStep::VERIFY_CANDIDATE_HASH;
            out.reason = "candidate hash mismatch";
            return out;
        }
        out.completed_steps.push_back(ApprovalStep::VERIFY_CANDIDATE_HASH);
        // 4. current state matches required state
        if (ctx.current_state != ctx.required_state) {
            out.stopped_at = ApprovalStep::VERIFY_CURRENT_STATE;
            out.reason = "current state does not permit approval";
            return out;
        }
        out.completed_steps.push_back(ApprovalStep::VERIFY_CURRENT_STATE);
        // 5. record approval
        out.completed_steps.push_back(ApprovalStep::RECORD_APPROVAL);
        // 6. forward to promotion manager
        if (!ctx.promotion_manager_available) {
            out.stopped_at = ApprovalStep::PROMOTION_MANAGER;
            out.reason = "promotion manager unavailable";
            return out;
        }
        out.completed_steps.push_back(ApprovalStep::PROMOTION_MANAGER);
        out.forwarded_to_promotion_manager = true;
        out.reason = "all verifications passed; forwarded to promotion manager";
        return out;
    }

private:
    static bool contains(const std::vector<std::string>& v, const std::string& s) {
        for (const auto& x : v) {
            if (x == s) return true;
        }
        return false;
    }
};

}  // namespace telegram
}  // namespace aura

#endif  // AURA_TELEGRAM_APPROVALREQUEST_H
