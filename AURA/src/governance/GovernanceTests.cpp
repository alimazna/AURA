// Phase 7 governance behavioural tests (GOV-0001..GOV-0005 verification).
//
// Deterministic; no clock, no threads, no I/O. Exits non-zero on the first failure.

#include "evolution/Candidate.h"
#include "foundation/EntityId.h"
#include "foundation/Timestamp.h"
#include "governance/AuditLedger.h"
#include "governance/ForbiddenBehavior.h"
#include "governance/HumanDecision.h"
#include "governance/PolicyEngine.h"
#include "governance/PromotionGate.h"

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

static foundation::Timestamp ts(std::int64_t s) { return foundation::Timestamp::from_seconds(s); }

static bool has_control(const std::vector<governance::Control>& v, governance::Control c) {
    for (auto x : v) if (x == c) return true;
    return false;
}

static void test_policy_by_change_type() {
    auto param = governance::PolicyEngine::requirement_for(evolution::ChangeType::PARAMETER);
    CHECK(has_control(param.minimum_controls, governance::Control::TIME_AWARE_VALIDATION));
    CHECK(has_control(param.minimum_controls, governance::Control::OUT_OF_SAMPLE));
    CHECK(has_control(param.additional_controls, governance::Control::MULTIPLE_TESTING_CORRECTION));
    CHECK(!param.requires_human_approval);

    auto risk = governance::PolicyEngine::requirement_for(evolution::ChangeType::RISK);
    CHECK(risk.requires_human_approval);  // risk rules require human approval
    auto arch = governance::PolicyEngine::requirement_for(evolution::ChangeType::ARCHITECTURE);
    CHECK(arch.sandbox_only);
    CHECK(has_control(arch.minimum_controls, governance::Control::SANDBOX_ONLY));
    auto eval = governance::PolicyEngine::requirement_for(evolution::ChangeType::EVALUATOR);
    CHECK(eval.requires_human_approval);
    CHECK(has_control(eval.additional_controls,
                      governance::Control::NEW_EVALUATOR_AND_EVIDENCE_FAMILY));
    auto meta = governance::PolicyEngine::requirement_for(evolution::ChangeType::META);
    CHECK(meta.sandbox_only && meta.requires_human_approval);
}

static void test_forbidden_behavior_screen() {
    // automated research actor cannot write production
    governance::ProposedAction a;
    a.actor = "research";
    a.requested_behaviors = {governance::ForbiddenBehavior::RESEARCH_WRITE_PRODUCTION};
    auto d = governance::ForbiddenBehaviorGuard::screen(a);
    CHECK(!d.allowed);
    CHECK(d.violations.size() == 1);

    // trusted human maintainer may request a governed deployment action
    a.actor = "trusted_maintainer";
    d = governance::ForbiddenBehaviorGuard::screen(a);
    CHECK(d.allowed);

    // but even a human may not delete failed experiments (integrity anchor)
    a.requested_behaviors = {governance::ForbiddenBehavior::DELETE_FAILED_EXPERIMENTS};
    d = governance::ForbiddenBehaviorGuard::screen(a);
    CHECK(!d.allowed);

    // candidate self-promotion forbidden for everyone
    a.actor = "candidate";
    a.requested_behaviors = {governance::ForbiddenBehavior::SELF_PROMOTION};
    CHECK(!governance::ForbiddenBehaviorGuard::screen(a).allowed);

    // unattributed action refused
    governance::ProposedAction empty;
    CHECK(!governance::ForbiddenBehaviorGuard::screen(empty).allowed);

    // clean action allowed
    governance::ProposedAction clean;
    clean.actor = "research";
    CHECK(governance::ForbiddenBehaviorGuard::screen(clean).allowed);
}

static evolution::Candidate make_shadow_candidate() {
    evolution::Candidate c;
    c.candidate_id = foundation::EntityId("CAND|V1.0|p|d");
    c.parent_version = "V1.0";
    c.change_type = evolution::ChangeType::PARAMETER;
    c.state = evolution::CandidateState::SHADOW_ACTIVE;
    return c;
}

static void test_promotion_gate() {
    auto candidate = make_shadow_candidate();
    governance::PromotionPackage pkg;
    pkg.package_id = foundation::EntityId("PKG|1");
    pkg.candidate_id = candidate.candidate_id;
    pkg.human_decision_ref = "D12";
    // no checks yet -> not ready
    auto d = governance::PromotionGate::evaluate(pkg, candidate);
    CHECK(!d.promotion_ready);
    CHECK(d.unmet.size() == governance::PromotionGate::required_checks().size());

    // satisfy all except human approval
    for (auto c : governance::PromotionGate::required_checks()) {
        pkg.checks.push_back({c, true, ""});
    }
    pkg.human_decision_ref.clear();
    d = governance::PromotionGate::evaluate(pkg, candidate);
    CHECK(!d.promotion_ready);
    bool human_unmet = false;
    for (auto c : d.unmet) human_unmet = human_unmet || c == governance::GateCheck::HUMAN_APPROVAL;
    CHECK(human_unmet);

    // restore human decision -> ready
    pkg.human_decision_ref = "D12";
    d = governance::PromotionGate::evaluate(pkg, candidate);
    CHECK(d.promotion_ready);

    // shadow requirement: a non-shadow candidate cannot be promotion-ready even
    // if every package check is marked satisfied
    evolution::Candidate not_shadow = candidate;
    not_shadow.state = evolution::CandidateState::REVIEW_READY;
    d = governance::PromotionGate::evaluate(pkg, not_shadow);
    CHECK(!d.promotion_ready);

    // gate cannot self-promote
    CHECK(!governance::PromotionGate::can_self_promote());
}

static void test_human_decision_ledger() {
    governance::HumanDecisionLedger ledger;
    governance::DecisionRecord auto_rec;
    auto_rec.decision_id = foundation::EntityId("DEC|1");
    auto_rec.question = "should we promote?";
    auto_rec.actor = "research";
    auto_rec.status = governance::DecisionStatus::ACCEPTED;
    CHECK(!ledger.record(auto_rec));  // automated actor cannot record ACCEPTED

    auto_rec.status = governance::DecisionStatus::PROPOSED;
    CHECK(ledger.record(auto_rec));
    CHECK(!ledger.record(auto_rec));  // duplicate identity rejected

    governance::DecisionRecord human_rec;
    human_rec.decision_id = foundation::EntityId("DEC|2");
    human_rec.question = "approve candidate?";
    human_rec.actor = "human";
    human_rec.status = governance::DecisionStatus::ACCEPTED;
    human_rec.selected_design = "promote to shadow";
    CHECK(ledger.record(human_rec));
    CHECK(ledger.size() == 2);
    CHECK(ledger.find(human_rec.decision_id)->status == governance::DecisionStatus::ACCEPTED);
    CHECK(ledger.ordered().size() == 2);

    // unattributed decision refused
    governance::DecisionRecord no_actor;
    no_actor.decision_id = foundation::EntityId("DEC|3");
    no_actor.question = "x";
    CHECK(!ledger.record(no_actor));
}

static void test_audit_ledger_append_only() {
    governance::AuditLedger ledger;
    governance::AuditRecord r;
    r.audit_id = foundation::EntityId("AUD|1");
    r.category = governance::AuditCategory::GOVERNANCE_SCREEN;
    r.actor = "research";
    r.action = "screen";
    r.recorded_at = ts(10);
    CHECK(ledger.append(r));
    CHECK(!ledger.append(r));  // duplicate rejected, no overwrite
    CHECK(ledger.size() == 1);

    governance::AuditRecord r2 = r;
    r2.audit_id = foundation::EntityId("AUD|2");
    r2.category = governance::AuditCategory::PROMOTION;
    r2.action = "promote";
    CHECK(ledger.append(r2));
    CHECK(ledger.size() == 2);
    CHECK(ledger.count_category(governance::AuditCategory::PROMOTION) == 1);
    CHECK(!governance::AuditLedger::supports_mutation());  // no edit/delete API

    // invalid record (no actor) refused
    governance::AuditRecord bad;
    bad.audit_id = foundation::EntityId("AUD|3");
    bad.action = "x";
    CHECK(!ledger.append(bad));
}

int main() {
    test_policy_by_change_type();
    test_forbidden_behavior_screen();
    test_promotion_gate();
    test_human_decision_ledger();
    test_audit_ledger_append_only();
    if (g_failures == 0) {
        std::printf("GovernanceTests: ALL PASS\n");
        return 0;
    }
    std::printf("GovernanceTests: %d FAILURE(S)\n", g_failures);
    return 1;
}
