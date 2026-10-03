// Phase 10 auxiliary Telegram tests (TG-0001..TG-0005).
//
// Deterministic; the transport is a test double, no network, no real token.
// Exits non-zero on the first failure.

#include "telegram/ApprovalRequest.h"
#include "telegram/NotificationPolicy.h"
#include "telegram/TelegramConfig.h"
#include "telegram/TelegramGateway.h"
#include "telegram/TelegramMessage.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace aura;

static int g_failures = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
            ++g_failures;                                                        \
        }                                                                        \
    } while (0)

// Deterministic in-memory sink (test double). Real code path; no mocks of the
// gateway logic itself.
class RecordingSink : public telegram::IMessageSink {
public:
    bool send(const telegram::TelegramMessage& message) noexcept override {
        if (fail_) return false;
        sent.push_back(message);
        return true;
    }
    std::vector<telegram::TelegramMessage> sent{};
    bool fail_{false};
};

static telegram::TelegramConfig configured(bool with_token = true) {
    telegram::TelegramConfig c;
    c.enabled = true;
    if (with_token) c.bot_token = "test-token-not-real";
    c.chat_id = "12345";
    c.bot_role = "operations";
    c.allowed_user_ids = {"human-1"};
    return c;
}

static telegram::TelegramMessage msg(telegram::MessageClass cls = telegram::MessageClass::STATUS) {
    telegram::TelegramMessage m;
    m.message_id = "m1";
    m.message_class = cls;
    m.body = "hello";
    m.created_at = foundation::Timestamp::from_seconds(1);
    return m;
}

static void test_config_secret_free_by_default() {
    telegram::TelegramConfig c;  // default
    CHECK(c.bot_token.empty());  // no embedded/secret default
    CHECK(!c.configured());      // disabled until human configures
    c = configured();
    CHECK(c.configured());
    c.bot_token.clear();
    CHECK(!c.configured());  // token removed -> not configured
    c = configured();
    c.allowed_user_ids.clear();
    CHECK(!c.configured());  // no authorized users -> not configured
}

static void test_gateway_disabled_is_safe_noop() {
    RecordingSink sink;
    telegram::TelegramGateway gw(telegram::TelegramConfig{}, &sink);  // disabled
    CHECK(!gw.operational());
    CHECK(gw.notify(msg()) == telegram::SendResult::DISABLED);
    CHECK(sink.sent.empty());
    CHECK(!telegram::TelegramGateway::is_core_dependency());
}

static void test_gateway_sends_informational_only() {
    RecordingSink sink;
    telegram::TelegramGateway gw(configured(), &sink);
    CHECK(gw.operational());
    CHECK(gw.notify(msg()) == telegram::SendResult::ENQUEUED);
    CHECK(sink.sent.size() == 1);
    CHECK(gw.enqueued_count() == 1);
    // invalid message refused
    CHECK(gw.notify(telegram::TelegramMessage{}) == telegram::SendResult::NO_MESSAGE);
    // transport failure -> not enqueued, no throw
    sink.fail_ = true;
    CHECK(gw.notify(msg()) == telegram::SendResult::NO_MESSAGE);
    CHECK(gw.enqueued_count() == 1);
}

static void test_approval_pipeline_full_and_stops() {
    telegram::ApprovalContext ctx;
    ctx.known_proposal_ids = {"P1"};
    ctx.known_candidate_hashes = {"HASH1"};
    ctx.current_state = "SHADOW_ACTIVE";
    ctx.required_state = "SHADOW_ACTIVE";
    ctx.promotion_manager_available = true;
    std::vector<std::string> users{"human-1"};

    telegram::ApprovalRequest req{"P1", "human-1", "HASH1", "V1.1"};
    auto out = telegram::TelegramApprovalGateway::process(req, ctx, users);
    CHECK(out.forwarded_to_promotion_manager);
    CHECK(out.completed_steps.size() == 6);

    // unknown proposal -> stop at step 1
    telegram::ApprovalRequest bad = req;
    bad.proposal_id = "P9";
    out = telegram::TelegramApprovalGateway::process(bad, ctx, users);
    CHECK(!out.forwarded_to_promotion_manager);
    CHECK(out.stopped_at == telegram::ApprovalStep::VERIFY_PROPOSAL_ID);

    // unauthorized user -> stop at step 2
    bad = req;
    bad.user_id = "stranger";
    out = telegram::TelegramApprovalGateway::process(bad, ctx, users);
    CHECK(out.stopped_at == telegram::ApprovalStep::VERIFY_USER_AUTHORIZATION);

    // hash mismatch -> stop at step 3
    bad = req;
    bad.candidate_hash = "WRONG";
    out = telegram::TelegramApprovalGateway::process(bad, ctx, users);
    CHECK(out.stopped_at == telegram::ApprovalStep::VERIFY_CANDIDATE_HASH);

    // wrong state -> stop at step 4
    telegram::ApprovalContext ctx2 = ctx;
    ctx2.current_state = "EXPERIMENTAL";
    out = telegram::TelegramApprovalGateway::process(req, ctx2, users);
    CHECK(out.stopped_at == telegram::ApprovalStep::VERIFY_CURRENT_STATE);

    // promotion manager down -> stop at final step (Telegram failure is not a mutation)
    telegram::ApprovalContext ctx3 = ctx;
    ctx3.promotion_manager_available = false;
    out = telegram::TelegramApprovalGateway::process(req, ctx3, users);
    CHECK(!out.forwarded_to_promotion_manager);
    CHECK(out.stopped_at == telegram::ApprovalStep::PROMOTION_MANAGER);
}

static void test_notification_policy_never_suppresses_failures() {
    telegram::NotificationPolicy policy;
    // disable STATUS to demonstrate preference handling
    policy.set_preference({telegram::MessageClass::STATUS, false, 0});
    CHECK(!policy.should_emit(telegram::MessageClass::STATUS, 0));
    // failure signals are always eligible even without preference
    CHECK(policy.should_emit(telegram::MessageClass::ERROR, 0));
    CHECK(policy.should_emit(telegram::MessageClass::INCIDENT, 0));
    CHECK(policy.should_emit(telegram::MessageClass::ROLLBACK_NOTIFICATION, 0));
    CHECK(policy.should_emit(telegram::MessageClass::APPROVAL_NEEDED, 0));

    // rate-limit a non-critical class
    telegram::NotificationPolicy p2;
    p2.set_preference({telegram::MessageClass::REPORT, true, 100});
    CHECK(p2.should_emit(telegram::MessageClass::REPORT, 0));
    p2.record_sent(telegram::MessageClass::REPORT, 0);
    CHECK(!p2.should_emit(telegram::MessageClass::REPORT, 50));
    CHECK(p2.should_emit(telegram::MessageClass::REPORT, 100));
    // but an ERROR is not rate-limited away
    p2.record_sent(telegram::MessageClass::ERROR, 0);
    CHECK(p2.should_emit(telegram::MessageClass::ERROR, 1));
}

int main() {
    test_config_secret_free_by_default();
    test_gateway_disabled_is_safe_noop();
    test_gateway_sends_informational_only();
    test_approval_pipeline_full_and_stops();
    test_notification_policy_never_suppresses_failures();
    if (g_failures == 0) {
        std::printf("TelegramTests: ALL PASS\n");
        return 0;
    }
    std::printf("TelegramTests: %d FAILURE(S)\n", g_failures);
    return 1;
}
