// Phase 4 research-plane behavioural tests (RESEARCH-0001..RESEARCH-0007 verification).
//
// Deterministic; no clock, no threads, no I/O. Exits non-zero on the first failure.

#include "foundation/Timestamp.h"
#include "research/Experiment.h"
#include "research/ExperimentFingerprint.h"
#include "research/ExperimentLedger.h"
#include "research/Hypothesis.h"
#include "research/ResearchBudget.h"
#include "research/ResearchPlanner.h"
#include "research/ResearchSandbox.h"

#include <cstdio>
#include <string>

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

static research::Hypothesis make_hypothesis() {
    research::Hypothesis h;
    h.hypothesis_id = research::make_hypothesis_id("reduce X in regimeR", "FeatureEngine");
    h.problem_class = research::ProblemClass::FEATURE;
    h.hypothesis_class = research::HypothesisClass::FEATURE_HYPOTHESIS;
    h.claim = "reduce X in regimeR";
    h.affected_component = "FeatureEngine";
    h.context_scope = "regimeR";
    h.success_criterion = "improve failure slice";
    h.falsification = "no improvement in predeclared failure slice, or regression outside it";
    h.primary_metric = "failure_concentration";
    h.created_at = ts(1);
    return h;
}

static research::Experiment make_experiment(const std::string& cand) {
    research::Experiment e;
    e.experiment_id = research::make_experiment_id("C1", "does reducing X help?", cand);
    e.campaign_id = "C1";
    e.hypothesis_id = research::make_hypothesis_id("reduce X in regimeR", "FeatureEngine");
    e.question = "does reducing X help?";
    e.dataset_version = "ds1";
    e.feature_version = "fv1";
    e.evaluator_version = "eval1";
    e.environment_version = "env1";
    e.control_version = "ctrl1";
    e.candidate_definition = cand;
    e.random_seed_policy = "fixed-42";
    e.validation_protocol = "wf-v1";
    e.budget = 1000;
    e.created_at = ts(2);
    return e;
}

static void test_falsifiability() {
    research::Hypothesis h = make_hypothesis();
    CHECK(h.valid());
    CHECK(h.falsifiable());
    research::Hypothesis no_falsification = h;
    no_falsification.falsification.clear();
    CHECK(!no_falsification.valid());  // falsifiability is mandatory
    research::Hypothesis no_claim = h;
    no_claim.claim.clear();
    CHECK(!no_claim.valid());
}

static void test_fingerprint_deterministic() {
    research::Experiment a = make_experiment("candA");
    research::Experiment b = make_experiment("candA");
    CHECK(research::ExperimentFingerprint::of_hex(a) == research::ExperimentFingerprint::of_hex(b));
    // transient fields (timestamp, result) must not change identity
    b.created_at = ts(999);
    b.result = "some result";
    CHECK(research::ExperimentFingerprint::of_hex(a) == research::ExperimentFingerprint::of_hex(b));
    // identity-relevant field changes fingerprint
    b.dataset_version = "ds2";
    CHECK(research::ExperimentFingerprint::of_hex(a) != research::ExperimentFingerprint::of_hex(b));
}

static void test_ledger_and_duplicates() {
    research::ExperimentLedger ledger;
    research::Experiment a = make_experiment("candA");
    CHECK(ledger.record(a));
    CHECK(ledger.record(a));  // idempotent re-record of same identity
    CHECK(ledger.size() == 1);

    // a conflicting record under same identity is rejected
    research::Experiment conflict = a;
    conflict.result = "x";
    CHECK(!ledger.record(conflict));
    CHECK(ledger.size() == 1);

    // a different identity but identical fingerprint is a duplicate: rejected, original preserved
    research::Experiment dup = a;
    dup.experiment_id = foundation::EntityId("E|other-id");
    CHECK(ledger.contains_fingerprint(dup));
    CHECK(!ledger.record(dup));
    CHECK(ledger.size() == 1);

    // a genuinely different experiment is accepted
    research::Experiment other = make_experiment("candB");
    CHECK(ledger.record(other));
    CHECK(ledger.size() == 2);
    CHECK(ledger.ordered().size() == 2);
    CHECK(ledger.ordered()[0].candidate_definition == "candA");  // append order preserved
    CHECK(ledger.find(dup.experiment_id) == nullptr);
}

static void test_budget() {
    research::ResearchBudget b;
    b.set_limit(research::BudgetCategory::TRIAL_BUDGET, 100);
    CHECK(b.state(research::BudgetCategory::TRIAL_BUDGET) == research::BudgetState::GREEN);
    CHECK(b.remaining(research::BudgetCategory::TRIAL_BUDGET) == 100);
    CHECK(b.charge(research::BudgetCategory::TRIAL_BUDGET, 70));
    CHECK(b.state(research::BudgetCategory::TRIAL_BUDGET) == research::BudgetState::YELLOW);
    CHECK(!b.charge(research::BudgetCategory::TRIAL_BUDGET, 31));  // would exceed
    CHECK(b.spent(research::BudgetCategory::TRIAL_BUDGET) == 70);  // no partial spend
    CHECK(b.charge(research::BudgetCategory::TRIAL_BUDGET, 30));
    CHECK(b.state(research::BudgetCategory::TRIAL_BUDGET) == research::BudgetState::RED);
    CHECK(!b.charge(research::BudgetCategory::TRIAL_BUDGET, 1));  // exhausted
    CHECK(!b.charge(research::BudgetCategory::TRIAL_BUDGET, -5)); // negative refused
    // unbounded category
    CHECK(b.remaining(research::BudgetCategory::COMPUTE_BUDGET) == -1);
    CHECK(b.charge(research::BudgetCategory::COMPUTE_BUDGET, 1000000));
    CHECK(b.state(research::BudgetCategory::COMPUTE_BUDGET) == research::BudgetState::GREEN);
    // freeze stops adaptive research
    b.set_frozen(true);
    CHECK(b.state(research::BudgetCategory::COMPUTE_BUDGET) == research::BudgetState::FROZEN);
    CHECK(!b.charge(research::BudgetCategory::COMPUTE_BUDGET, 1));
}

static void test_planner_fixed_before_evaluation() {
    research::ValidationPlan p = research::ResearchPlanner::plan_for(research::EvidenceZone::E0_EXPLORATION, false);
    CHECK(!p.empty());
    CHECK(!p.requires_locked_oos);
    CHECK(p.allow_adaptive_selection);
    // deeper zone requires locked OOS/holdout
    auto p2 = research::ResearchPlanner::plan_for(research::EvidenceZone::E3_LOCKED_HOLDOUT, false);
    CHECK(p2.requires_locked_oos);
    CHECK(p2.requires_holdout);
    CHECK(!p2.allow_adaptive_selection);
    // structural change forces deeper stats even in E1
    auto p3 = research::ResearchPlanner::plan_for(research::EvidenceZone::E1_DEVELOPMENT, true);
    bool has_pbo = false;
    for (auto m : p3.methods) has_pbo = has_pbo || m == research::ValidationMethod::PBO_DSR;
    CHECK(has_pbo);
    // fixing stamps time; immutability check detects post-hoc change
    CHECK(research::ResearchPlanner::fix(p, ts(5)));
    CHECK(p.fixed_at == ts(5));
    research::ValidationPlan changed = p;
    changed.methods.push_back(research::ValidationMethod::PBO_DSR);
    CHECK(!research::ResearchPlanner::is_immutable_against(p, changed));
    CHECK(research::ResearchPlanner::is_immutable_against(p, p));
}

static void test_sandbox_admission() {
    CHECK(!research::ResearchSandbox::may_mutate_runtime());
    research::ExperimentLedger ledger;
    research::Experiment e = make_experiment("candA");
    research::ValidationPlan p = research::ResearchPlanner::plan_for(research::EvidenceZone::E1_DEVELOPMENT, false);

    // not recorded yet -> refused
    CHECK(!research::ResearchSandbox::admit(e, ledger, p).admitted);
    // no hypothesis -> refused
    research::Experiment no_h = e;
    no_h.hypothesis_id = foundation::EntityId{};
    no_h.experiment_id = research::make_experiment_id("C1", "does reducing X help?", "candA2");
    no_h.candidate_definition = "candA2";
    CHECK(!research::ResearchSandbox::admit(no_h, ledger, p).admitted);
    // plan not fixed -> refused
    CHECK(!research::ResearchSandbox::admit(e, ledger, p).admitted);
    CHECK(research::ResearchPlanner::fix(p, ts(9)));
    CHECK(ledger.record(e));
    auto d = research::ResearchSandbox::admit(e, ledger, p);
    CHECK(d.admitted);
    // sandbox never mutates runtime
    CHECK(!research::ResearchSandbox::may_mutate_runtime());
}

int main() {
    test_falsifiability();
    test_fingerprint_deterministic();
    test_ledger_and_duplicates();
    test_budget();
    test_planner_fixed_before_evaluation();
    test_sandbox_admission();
    if (g_failures == 0) {
        std::printf("ResearchTests: ALL PASS\n");
        return 0;
    }
    std::printf("ResearchTests: %d FAILURE(S)\n", g_failures);
    return 1;
}
