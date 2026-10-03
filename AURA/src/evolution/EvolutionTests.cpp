// Phase 5 evolution behavioural tests (EVOL-0001..EVOL-0004 verification).
//
// Deterministic; no clock, no threads, no I/O. Exits non-zero on the first failure.

#include "foundation/EntityId.h"
#include "foundation/Timestamp.h"
#include "evolution/Candidate.h"
#include "evolution/CandidateComparison.h"
#include "evolution/CandidateRegistry.h"
#include "evolution/EvolutionGraph.h"

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

static evolution::Candidate make_candidate(const std::string& parent, const std::string& params,
                                           const std::string& digest) {
    evolution::Candidate c;
    c.candidate_id = evolution::make_candidate_id(parent, params, digest);
    c.parent_version = parent;
    c.parameter_set = params;
    c.provenance_digest = digest;
    c.change_type = evolution::ChangeType::PARAMETER;
    c.reason = "reduce X in regimeR";
    return c;
}

static void test_candidate_lifecycle_rules() {
    using evolution::CandidateState;
    CHECK(evolution::CandidateLifecycle::can_transition(CandidateState::PROPOSED, CandidateState::SCHEMA_VALIDATED));
    CHECK(evolution::CandidateLifecycle::can_transition(CandidateState::PASSED, CandidateState::REVIEW_READY));
    CHECK(evolution::CandidateLifecycle::can_transition(CandidateState::REVIEW_READY, CandidateState::HUMAN_APPROVED));
    CHECK(evolution::CandidateLifecycle::can_transition(CandidateState::PROMOTION_READY, CandidateState::PROMOTED));
    CHECK(evolution::CandidateLifecycle::can_transition(CandidateState::MONITORED, CandidateState::ROLLED_BACK));
    CHECK(evolution::CandidateLifecycle::can_transition(CandidateState::ROLLED_BACK, CandidateState::RETIRED));
    // no hidden skips
    CHECK(!evolution::CandidateLifecycle::can_transition(CandidateState::PROPOSED, CandidateState::HUMAN_APPROVED));
    CHECK(!evolution::CandidateLifecycle::can_transition(CandidateState::VALIDATING, CandidateState::PROMOTED));
    // cannot reach PROMOTED without human approval path
    CHECK(!evolution::CandidateLifecycle::can_transition(CandidateState::SHADOW_ACTIVE, CandidateState::PROMOTED));
    // terminal
    CHECK(!evolution::CandidateLifecycle::can_transition(CandidateState::RETIRED, CandidateState::PROPOSED));
    CHECK(evolution::CandidateLifecycle::is_terminal(CandidateState::RETIRED));
}

static void test_registry_history_and_no_auto_promotion() {
    using evolution::CandidateState;
    evolution::CandidateRegistry reg;
    auto c = make_candidate("V1.0", "threshold=0.65", "digestA");
    CHECK(reg.propose(c));
    CHECK(!reg.propose(c));  // duplicate identity rejected
    CHECK(reg.population() == 1);

    // advance through a legitimate path
    CHECK(reg.advance(c.candidate_id, CandidateState::SCHEMA_VALIDATED));
    CHECK(reg.advance(c.candidate_id, CandidateState::SANDBOX_READY));
    CHECK(reg.advance(c.candidate_id, CandidateState::EXPERIMENTAL));
    CHECK(reg.advance(c.candidate_id, CandidateState::VALIDATING));
    CHECK(reg.advance(c.candidate_id, CandidateState::PASSED));
    // illegal jump to promotion must be refused
    CHECK(!reg.advance(c.candidate_id, CandidateState::PROMOTED));
    CHECK(!reg.advance(c.candidate_id, CandidateState::HUMAN_APPROVED));

    // full governed path to promotion
    CHECK(reg.advance(c.candidate_id, CandidateState::REVIEW_READY));
    CHECK(reg.advance(c.candidate_id, CandidateState::HUMAN_APPROVED));
    CHECK(reg.advance(c.candidate_id, CandidateState::SHADOW_READY));
    CHECK(reg.advance(c.candidate_id, CandidateState::SHADOW_ACTIVE));
    CHECK(reg.advance(c.candidate_id, CandidateState::PROMOTION_READY));
    CHECK(reg.advance(c.candidate_id, CandidateState::PROMOTED));
    CHECK(reg.latest(c.candidate_id)->state == CandidateState::PROMOTED);

    auto hist = reg.history(c.candidate_id);
    CHECK(hist.size() == 12);
    CHECK(hist.front().state == CandidateState::PROPOSED);  // history retained
    CHECK(hist.back().state == CandidateState::PROMOTED);
    CHECK(hist.back().revision == 11);
    // unknown candidate
    CHECK(!reg.advance(foundation::EntityId("nope"), CandidateState::SCHEMA_VALIDATED));
}

static void test_comparison_veto_and_matching() {
    using evolution::ComparisonVerdict;
    evolution::ComparisonInput in;
    in.control_primary = 1.0;
    in.candidate_primary = 1.5;
    in.improvement_tolerance = 0.05;
    CHECK(evolution::CandidateComparison::compare(in) == ComparisonVerdict::CANDIDATE_BETTER);
    CHECK(evolution::CandidateComparison::permits_review(ComparisonVerdict::CANDIDATE_BETTER));

    // metric gain but safety constraint failed -> VETO (constraints are vetoes)
    in.safety_constraints_satisfied = false;
    CHECK(evolution::CandidateComparison::compare(in) == ComparisonVerdict::VETOED);
    CHECK(!evolution::CandidateComparison::permits_review(ComparisonVerdict::VETOED));
    in.safety_constraints_satisfied = true;

    // unmatched environment -> insufficient evidence, never favorable
    in.environment_matched = false;
    CHECK(evolution::CandidateComparison::compare(in) == ComparisonVerdict::INSUFFICIENT_EVIDENCE);
    in.environment_matched = true;

    // too few trials -> insufficient evidence
    in.enough_trials = false;
    CHECK(evolution::CandidateComparison::compare(in) == ComparisonVerdict::INSUFFICIENT_EVIDENCE);
    in.enough_trials = true;

    // regression
    in.candidate_primary = 0.5;
    CHECK(evolution::CandidateComparison::compare(in) == ComparisonVerdict::CONTROL_BETTER);
    // equivalent within tolerance
    in.candidate_primary = 1.02;
    CHECK(evolution::CandidateComparison::compare(in) == ComparisonVerdict::EQUIVALENT);
}

static void test_evolution_graph() {
    evolution::EvolutionGraph g;
    CHECK(g.add_root("V1.0"));
    CHECK(!g.add_root("V1.0"));  // duplicate version rejected
    CHECK(g.add_child("V1.0", "V1.1A"));
    CHECK(g.add_child("V1.0", "V1.1B"));
    CHECK(g.add_child("V1.0", "V1.1C"));
    CHECK(g.add_child("V1.1A", "D"));
    CHECK(g.add_child("V1.1B", "E"));
    CHECK(g.add_child("V1.1C", "F"));
    // unknown parent rejected (prevents dangling/cyclic structure)
    CHECK(!g.add_child("V9.9", "orphan"));
    CHECK(g.size() == 7);
    auto lin = g.lineage("D");
    CHECK(lin.size() == 3);
    CHECK(lin[0] == "V1.0");
    CHECK(lin[1] == "V1.1A");
    CHECK(lin[2] == "D");
    auto rootlin = g.lineage("V1.0");
    CHECK(rootlin.size() == 1);
    CHECK(g.lineage("missing").empty());
}

int main() {
    test_candidate_lifecycle_rules();
    test_registry_history_and_no_auto_promotion();
    test_comparison_veto_and_matching();
    test_evolution_graph();
    if (g_failures == 0) {
        std::printf("EvolutionTests: ALL PASS\n");
        return 0;
    }
    std::printf("EvolutionTests: %d FAILURE(S)\n", g_failures);
    return 1;
}
