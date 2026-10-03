// Phase 3 self-learning behavioural tests (LEARN-0001..LEARN-0008 verification).
//
// Deterministic; no clock, no threads, no I/O. Exits non-zero on the first failure.

#include "foundation/EntityId.h"
#include "foundation/ErrorCode.h"
#include "foundation/ErrorRecord.h"
#include "foundation/ErrorSeverity.h"
#include "foundation/RecoveryAction.h"
#include "foundation/ServiceState.h"
#include "foundation/Timestamp.h"
#include "learning/ContextLearning.h"
#include "learning/ContradictionEngine.h"
#include "learning/FailureMemory.h"
#include "learning/KnowledgeDecay.h"
#include "learning/KnowledgeLifecycle.h"
#include "learning/KnowledgeObject.h"
#include "learning/KnowledgeStore.h"
#include "observation/FailureDetectionEngine.h"

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

static foundation::Timestamp ts(std::int64_t seconds) {
    return foundation::Timestamp::from_seconds(seconds);
}

static learning::KnowledgeObject make_object(const std::string& obs, const std::string& scope,
                                             learning::KnowledgeStatus status,
                                             std::int64_t first, std::int64_t assessed,
                                             std::uint32_t revision = 0) {
    learning::KnowledgeObject o;
    o.knowledge_id = learning::make_knowledge_id("strategy", obs, scope);
    o.observation = obs;
    o.validity_scope = scope;
    o.status = status;
    o.first_observed_at = ts(first);
    o.assessed_at = ts(assessed);
    o.revision = revision;
    return o;
}

static void test_point_in_time() {
    learning::KnowledgeObject o = make_object("feature F unreliable", "regime X",
                                              learning::KnowledgeStatus::OBSERVED, 100, 200);
    CHECK(o.valid());
    CHECK(o.is_point_in_time_consistent(ts(150)));   // evidence before assessment: OK
    CHECK(o.is_point_in_time_consistent(ts(200)));   // evidence at assessment: OK
    CHECK(!o.is_point_in_time_consistent(ts(201)));  // future evidence: rejected
    // assessment before first observation is impossible
    learning::KnowledgeObject bad = make_object("x", "s", learning::KnowledgeStatus::OBSERVED, 300, 200);
    CHECK(!bad.is_point_in_time_consistent(ts(300)));
}

static void test_store_versioning() {
    learning::KnowledgeStore store;
    learning::KnowledgeObject r0 = make_object("feature F unreliable", "regime X",
                                               learning::KnowledgeStatus::SUSPECTED, 100, 200, 0);
    CHECK(store.record(r0));
    CHECK(store.record(r0));  // idempotent no-op
    CHECK(store.revision_count() == 1);
    CHECK(store.identity_count() == 1);

    // conflicting payload for same (identity, revision) rejected
    learning::KnowledgeObject conflict = r0;
    conflict.conclusion = "different";
    CHECK(!store.record(conflict));
    CHECK(store.revision_count() == 1);

    // superseding revision keeps lineage and does not erase r0
    learning::KnowledgeObject r1 = make_object("feature F unreliable", "regime X",
                                               learning::KnowledgeStatus::SUPPORTED, 100, 300, 1);
    r1.lineage_refs.push_back(r0.knowledge_id.value());
    CHECK(store.record(r1));
    CHECK(store.revision_count() == 2);
    auto chain = store.revisions(r0.knowledge_id);
    CHECK(chain.size() == 2);
    CHECK(chain.front().status == learning::KnowledgeStatus::SUSPECTED);  // history retained
    CHECK(chain.back().status == learning::KnowledgeStatus::SUPPORTED);
    CHECK(chain.back().lineage_refs.size() == 1);

    // rewind rejected (no silent overwrite with an older revision)
    learning::KnowledgeObject r0b = r0;
    CHECK(!store.record(r0b));

    // first revision of a new identity must be 0
    learning::KnowledgeObject fresh = make_object("other", "scope", learning::KnowledgeStatus::OBSERVED, 1, 2, 5);
    CHECK(!store.record(fresh));
}

static void test_lifecycle() {
    using learning::KnowledgeStatus;
    CHECK(learning::KnowledgeLifecycle::can_transition(KnowledgeStatus::OBSERVED, KnowledgeStatus::SUSPECTED));
    CHECK(learning::KnowledgeLifecycle::can_transition(KnowledgeStatus::SUSPECTED, KnowledgeStatus::REFUTED));
    CHECK(learning::KnowledgeLifecycle::can_transition(KnowledgeStatus::SUPPORTED, KnowledgeStatus::CONTRADICTED));
    CHECK(learning::KnowledgeLifecycle::can_transition(KnowledgeStatus::VALIDATED, KnowledgeStatus::AGING));
    CHECK(learning::KnowledgeLifecycle::can_transition(KnowledgeStatus::AGING, KnowledgeStatus::REVALIDATION));
    CHECK(learning::KnowledgeLifecycle::can_transition(KnowledgeStatus::CONTRADICTED, KnowledgeStatus::UNDER_INVESTIGATION));
    // same-state idempotent
    CHECK(learning::KnowledgeLifecycle::can_transition(KnowledgeStatus::OBSERVED, KnowledgeStatus::OBSERVED));
    // illegal skips
    CHECK(!learning::KnowledgeLifecycle::can_transition(KnowledgeStatus::OBSERVED, KnowledgeStatus::VALIDATED));
    CHECK(!learning::KnowledgeLifecycle::can_transition(KnowledgeStatus::OBSERVED, KnowledgeStatus::OPERATIONAL_KNOWLEDGE));
    // terminal
    CHECK(!learning::KnowledgeLifecycle::can_transition(KnowledgeStatus::REFUTED, KnowledgeStatus::SUPPORTED));
    CHECK(learning::KnowledgeLifecycle::is_terminal(KnowledgeStatus::REFUTED));

    learning::KnowledgeObject o = make_object("obs", "scope", KnowledgeStatus::SUPPORTED, 1, 2);
    learning::KnowledgeObject out;
    CHECK(learning::KnowledgeLifecycle::advance(o, KnowledgeStatus::VALIDATED, out) != nullptr);
    CHECK(out.status == KnowledgeStatus::VALIDATED);
    CHECK(learning::KnowledgeLifecycle::advance(o, KnowledgeStatus::OPERATIONAL_KNOWLEDGE, out) == nullptr);
}

static void test_context_learning_no_leakage() {
    std::vector<learning::ContextObservation> obs;
    obs.push_back({"regimeX", true, ts(10), ts(20)});   // known before as_of
    obs.push_back({"regimeX", false, ts(11), ts(21)});  // known before as_of
    obs.push_back({"regimeX", true, ts(12), ts(100)});  // outcome known AFTER as_of: must be excluded
    obs.push_back({"regimeY", true, ts(13), ts(22)});
    auto summary = learning::ContextLearning::summarize(obs, ts(50));
    CHECK(summary.size() == 2);
    CHECK(summary[0].context == "regimeX");
    CHECK(summary[0].supporting == 1);
    CHECK(summary[0].contradicting == 1);
    CHECK(summary[0].used == 2);
    CHECK(summary[0].excluded_future == 1);
    CHECK(!summary[0].point_in_time_clean);
    CHECK(summary[0].support_ratio == 0.5);
    CHECK(summary[1].context == "regimeY");
    CHECK(summary[1].supporting == 1);
    CHECK(summary[1].point_in_time_clean);
    CHECK(summary[1].support_ratio == 1.0);
}

static void test_contradiction() {
    using learning::KnowledgeStatus;
    auto a = make_object("feature X unreliable", "regimeR", KnowledgeStatus::SUPPORTED, 1, 2);
    a.context = "high vol";
    auto b = make_object("feature X unreliable", "regimeR", KnowledgeStatus::REFUTED, 1, 3);
    b.context = "low vol";
    auto c = make_object("feature X unreliable", "regimeR", KnowledgeStatus::SUPPORTED, 1, 4);
    c.context = "high vol";  // same polarity as a: not a contradiction
    std::vector<learning::KnowledgeObject> objs{a, b, c};
    auto cs = learning::ContradictionEngine::detect(objs);
    CHECK(cs.size() == 2);  // (a,b) and (b,c); but b is REFUTED and a,c SUPPORTED with different contexts
    // Both sides preserved; no mutation.
    CHECK(cs[0].knowledge_a.valid());
    CHECK(cs[0].knowledge_b.valid());
    CHECK(cs[0].status == learning::ContradictionStatus::RESEARCH_REQUIRED);
    // Order-independent identity
    auto id1 = learning::ContradictionEngine::make_contradiction_id(a.knowledge_id, b.knowledge_id);
    auto id2 = learning::ContradictionEngine::make_contradiction_id(b.knowledge_id, a.knowledge_id);
    CHECK(id1 == id2);

    // Same context + opposite claims is NOT flagged by this rule (requires context split)
    auto d = make_object("feature X unreliable", "regimeR", KnowledgeStatus::SUPPORTED, 1, 2);
    d.context = "same";
    auto e = make_object("feature X unreliable", "regimeR", KnowledgeStatus::REFUTED, 1, 2);
    e.context = "same";
    CHECK(learning::ContradictionEngine::detect({d, e}).empty());
}

static void test_decay() {
    using learning::KnowledgeStatus;
    learning::KnowledgeObject o =
        make_object("obs", "scope", KnowledgeStatus::VALIDATED, 1, 2);
    o.revalidation_due_at = ts(100);
    CHECK(!learning::KnowledgeDecay::is_due(o, ts(99)));
    CHECK(learning::KnowledgeDecay::is_due(o, ts(100)));
    CHECK(learning::KnowledgeDecay::proposed_status(o, ts(99)) == KnowledgeStatus::VALIDATED);
    CHECK(learning::KnowledgeDecay::proposed_status(o, ts(100)) == KnowledgeStatus::AGING);
    // explicit trigger forces due
    CHECK(learning::KnowledgeDecay::is_due(o, ts(1),
                                           learning::RevalidationTrigger::BROKER_PROFILE_CHANGE));
    // aging object moves to revalidation, never deleted
    o.status = KnowledgeStatus::AGING;
    CHECK(learning::KnowledgeDecay::proposed_status(o, ts(100)) == KnowledgeStatus::REVALIDATION);
    // non-ageable status is left unchanged (no illegal transition proposed)
    o.status = KnowledgeStatus::OBSERVED;
    CHECK(learning::KnowledgeDecay::proposed_status(o, ts(100)) == KnowledgeStatus::OBSERVED);
}

static void test_failure_memory() {
    learning::FailureMemory mem;
    foundation::ErrorRecord r1(foundation::EntityId("err|1"), "failure.DATA_FAILURE.DATA_ERROR",
                               foundation::ErrorSeverity::WARNING, ts(10),
                               foundation::ServiceState::DEGRADED, "m1", "ctx",
                               foundation::RecoveryAction::RETRY);
    foundation::ErrorRecord r2(foundation::EntityId("err|2"), "failure.DATA_FAILURE.DATA_ERROR",
                               foundation::ErrorSeverity::ERROR, ts(20),
                               foundation::ServiceState::OFFLINE, "m2", "ctx",
                               foundation::RecoveryAction::RETRY);
    CHECK(mem.remember(r1, observation::FailureCategory::DATA_FAILURE, foundation::ErrorCode::DATA_ERROR,
                       "m1", ts(10), ts(15)));
    CHECK(mem.remember(r1, observation::FailureCategory::DATA_FAILURE, foundation::ErrorCode::DATA_ERROR,
                       "m1", ts(10), ts(15)));  // idempotent
    CHECK(mem.size() == 1);
    CHECK(mem.remember(r2, observation::FailureCategory::DATA_FAILURE, foundation::ErrorCode::DATA_ERROR,
                       "m2", ts(20), ts(200)));  // outcome known late
    CHECK(mem.size() == 2);

    std::size_t excluded = 0;
    auto patterns = mem.patterns(ts(50), excluded);
    CHECK(excluded == 1);          // r2 excluded (outcome known at 200 > 50)
    CHECK(patterns.size() == 1);
    CHECK(patterns[0].occurrences == 1);
    CHECK(patterns[0].first_seen == ts(10));

    auto all = mem.patterns(ts(500), excluded);
    CHECK(excluded == 0);
    CHECK(all.size() == 1);
    CHECK(all[0].occurrences == 2);
    CHECK(all[0].first_seen == ts(10));
    CHECK(all[0].last_seen == ts(20));

    // no identity -> rejected
    foundation::ErrorRecord empty;
    CHECK(!mem.remember(empty, observation::FailureCategory::UNKNOWN, foundation::ErrorCode::DATA_ERROR,
                        "", ts(1), ts(1)));
}

int main() {
    test_point_in_time();
    test_store_versioning();
    test_lifecycle();
    test_context_learning_no_leakage();
    test_contradiction();
    test_decay();
    test_failure_memory();
    if (g_failures == 0) {
        std::printf("LearningTests: ALL PASS\n");
        return 0;
    }
    std::printf("LearningTests: %d FAILURE(S)\n", g_failures);
    return 1;
}
