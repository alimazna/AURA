// Phase 6 validation behavioural tests (VALID-0001..VALID-0005 verification).
//
// Deterministic; no clock, no threads, no I/O. Exits non-zero on the first failure.

#include "research/Experiment.h"
#include "research/ResearchPlanner.h"
#include "validation/EvaluatorFirewall.h"
#include "validation/EvidenceFirewall.h"
#include "validation/RewardHackingDefense.h"
#include "validation/StatisticalControls.h"
#include "validation/ValidationFirewall.h"

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

static void test_validation_firewall_all_pass() {
    auto plan = research::ResearchPlanner::plan_for(research::EvidenceZone::E2_OUT_OF_SAMPLE, false);
    std::vector<validation::CheckOutcome> outcomes;
    for (auto m : plan.methods) outcomes.push_back({m, validation::CheckResult::PASS, "ok"});
    std::vector<validation::CheckOutcome> evaluated;
    auto v = validation::ValidationFirewall::evaluate(plan, outcomes, evaluated);
    CHECK(v == validation::FirewallVerdict::CREDIBLE);
    CHECK(evaluated.size() == plan.methods.size());
    CHECK(validation::ValidationFirewall::allows_human_review(v));
}

static void test_validation_firewall_missing_is_not_pass() {
    auto plan = research::ResearchPlanner::plan_for(research::EvidenceZone::E2_OUT_OF_SAMPLE, false);
    std::vector<validation::CheckOutcome> outcomes;  // none supplied
    std::vector<validation::CheckOutcome> evaluated;
    auto v = validation::ValidationFirewall::evaluate(plan, outcomes, evaluated);
    CHECK(v == validation::FirewallVerdict::INSUFFICIENT);  // never assumed PASS
    for (auto& o : evaluated) CHECK(o.result == validation::CheckResult::NOT_RUN);
}

static void test_validation_firewall_fail_is_not_credible() {
    auto plan = research::ResearchPlanner::plan_for(research::EvidenceZone::E1_DEVELOPMENT, false);
    std::vector<validation::CheckOutcome> outcomes;
    bool first = true;
    for (auto m : plan.methods) {
        outcomes.push_back({m, first ? validation::CheckResult::FAIL : validation::CheckResult::PASS, "x"});
        first = false;
    }
    std::vector<validation::CheckOutcome> evaluated;
    auto v = validation::ValidationFirewall::evaluate(plan, outcomes, evaluated);
    CHECK(v == validation::FirewallVerdict::NOT_CREDIBLE);
    CHECK(!validation::ValidationFirewall::allows_human_review(v));
}

static void test_evidence_firewall_budget_and_contamination() {
    validation::EvidenceFirewall fw(2);
    validation::HoldoutRequest req;
    req.candidate_hash = "abc";
    req.experiment_definition = "E1";
    req.artifact_hash = "def";
    req.evaluator_version = "EV1";
    req.zone = research::EvidenceZone::E3_LOCKED_HOLDOUT;

    research::ContaminationState cont = research::ContaminationState::CLEAN;
    validation::HoldoutAuditEntry audit;
    CHECK(fw.query(req, cont, audit) == validation::HoldoutDecision::GRANTED);
    CHECK(cont == research::ContaminationState::HOLDOUT_EXPOSED);
    CHECK(audit.decision == validation::HoldoutDecision::GRANTED);

    // second query consumes the last budget slot
    research::ContaminationState cont2 = research::ContaminationState::CLEAN;
    CHECK(fw.query(req, cont2, audit) == validation::HoldoutDecision::GRANTED);
    CHECK(fw.remaining_budget() == 0);

    // budget exhausted
    research::ContaminationState cont3 = research::ContaminationState::CLEAN;
    CHECK(fw.query(req, cont3, audit) == validation::HoldoutDecision::BUDGET_EXHAUSTED);
    CHECK(cont3 == research::ContaminationState::CLEAN);  // no contamination on a refused query

    // contaminated candidate cannot re-query holdout
    research::ContaminationState cont4 = research::ContaminationState::CONTAMINATED;
    CHECK(fw.query(req, cont4, audit) == validation::HoldoutDecision::DENIED);

    // malformed request
    validation::HoldoutRequest bad;
    research::ContaminationState cont5 = research::ContaminationState::CLEAN;
    CHECK(fw.query(bad, cont5, audit) == validation::HoldoutDecision::INVALID_REQUEST);
}

static void test_contamination_monotone() {
    using S = research::ContaminationState;
    CHECK(validation::transition_contamination(S::CLEAN, S::VALIDATED));
    CHECK(validation::transition_contamination(S::VALIDATED, S::OOS_EXPOSED));
    CHECK(validation::transition_contamination(S::OOS_EXPOSED, S::HOLDOUT_EXPOSED));
    CHECK(validation::transition_contamination(S::HOLDOUT_EXPOSED, S::CONTAMINATED));
    CHECK(validation::transition_contamination(S::CONTAMINATED, S::RETIRED));
    // no silent reset
    CHECK(!validation::transition_contamination(S::CONTAMINATED, S::CLEAN));
    CHECK(!validation::transition_contamination(S::HOLDOUT_EXPOSED, S::OOS_EXPOSED));
    CHECK(!validation::transition_contamination(S::RETIRED, S::CLEAN));
    CHECK(validation::transition_contamination(S::CLEAN, S::CLEAN));
}

static void test_evaluator_firewall() {
    validation::EvaluatorVersion family;
    family.version = "EV1";
    family.metric_registry_version = "MR1";
    family.evidence_family = "FAM-A";
    validation::EvaluatorVersion cand = family;
    CHECK(validation::EvaluatorFirewall::admits(family, cand));

    // candidate tries to use a different evaluator version -> refused
    cand.version = "EV2";
    CHECK(!validation::EvaluatorFirewall::admits(family, cand));
    // different evidence family -> cross-family, not comparable
    cand = family;
    cand.evidence_family = "FAM-B";
    CHECK(!validation::EvaluatorFirewall::admits(family, cand));
    CHECK(validation::EvaluatorFirewall::crosses_family(family, cand));
    // invalid family
    validation::EvaluatorVersion invalid;
    CHECK(!validation::EvaluatorFirewall::admits(invalid, family));
}

static void test_reward_hacking() {
    validation::DefenseInput in;
    in.hard_constraints.push_back({"max_drawdown_ok", true});
    in.hard_constraints.push_back({"no_live_leak", true});
    in.quality_objectives.push_back({"stability", 0.9, 0.8, true});
    in.composite_metric = 2.0;
    auto v = validation::RewardHackingDefense::assess(in);
    CHECK(v.review_eligible);
    CHECK(!v.vetoed);

    // high composite metric cannot overcome a failed hard constraint (veto)
    in.hard_constraints[0].satisfied = false;
    v = validation::RewardHackingDefense::assess(in);
    CHECK(v.vetoed);
    CHECK(!v.review_eligible);
    CHECK(v.failed_constraints.size() == 1);

    // absent robustness invalidates even with all constraints satisfied
    in.hard_constraints[0].satisfied = true;
    in.robustness = false;
    v = validation::RewardHackingDefense::assess(in);
    CHECK(!v.review_eligible);
    // failed quality objective
    in.robustness = true;
    in.quality_objectives[0].value = 0.5;
    v = validation::RewardHackingDefense::assess(in);
    CHECK(!v.review_eligible);
    CHECK(!v.failed_objectives.empty());
}

static void test_statistical_controls_no_guarantee() {
    std::vector<validation::StatisticalTool> required{validation::StatisticalTool::PBO,
                                                       validation::StatisticalTool::DSR};
    std::vector<validation::StatisticalEvidence> ev;
    CHECK(!validation::StatisticalControls::evidence_complete(required, ev));  // missing != pass
    ev.push_back({validation::StatisticalTool::PBO, 0.1, true, ""});
    ev.push_back({validation::StatisticalTool::DSR, 0.5, true, ""});
    CHECK(validation::StatisticalControls::evidence_complete(required, ev));
    // threshold semantics: PBO lower-is-better, DSR higher-is-better
    CHECK(validation::StatisticalControls::within_threshold(ev[0], 0.2));
    CHECK(!validation::StatisticalControls::within_threshold(ev[0], 0.05));
    CHECK(validation::StatisticalControls::within_threshold(ev[1], 0.3));
    // present but unavailable result does not count
    std::vector<validation::StatisticalEvidence> ev2;
    ev2.push_back({validation::StatisticalTool::PBO, 0.1, false, ""});
    ev2.push_back({validation::StatisticalTool::DSR, 0.5, true, ""});
    CHECK(!validation::StatisticalControls::evidence_complete(required, ev2));
}

int main() {
    test_validation_firewall_all_pass();
    test_validation_firewall_missing_is_not_pass();
    test_validation_firewall_fail_is_not_credible();
    test_evidence_firewall_budget_and_contamination();
    test_contamination_monotone();
    test_evaluator_firewall();
    test_reward_hacking();
    test_statistical_controls_no_guarantee();
    if (g_failures == 0) {
        std::printf("ValidationTests: ALL PASS\n");
        return 0;
    }
    std::printf("ValidationTests: %d FAILURE(S)\n", g_failures);
    return 1;
}
