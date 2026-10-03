// Phase 11 controlled real-world validation tests (RW-0001..RW-0002).
//
// Deterministic; no trading, no network, no clock, no I/O. Exits non-zero on the
// first failure.

#include "foundation/EntityId.h"
#include "foundation/Timestamp.h"
#include "realworld/RealWorldValidation.h"
#include "realworld/ValidationRegistry.h"

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

static void test_readiness_all_required() {
    // no checks supplied -> nothing satisfied, not ready
    auto d = realworld::RealWorldValidation::evaluate({});
    CHECK(!d.ready_for_controlled_validation);
    CHECK(d.unmet.size() == realworld::RealWorldValidation::required_checks().size());

    // satisfy all but REAL_MT5_ENVIRONMENT_AVAILABLE -> still not ready
    std::vector<realworld::CheckState> checks;
    for (auto c : realworld::RealWorldValidation::required_checks()) {
        if (c == realworld::ReadinessCheck::REAL_MT5_ENVIRONMENT_AVAILABLE) continue;
        checks.push_back({c, true, ""});
    }
    d = realworld::RealWorldValidation::evaluate(checks);
    CHECK(!d.ready_for_controlled_validation);
    CHECK(d.unmet.size() == 1);
    CHECK(d.unmet[0] == realworld::ReadinessCheck::REAL_MT5_ENVIRONMENT_AVAILABLE);

    // satisfy the last one -> ready (but execution still separate)
    checks.push_back({realworld::ReadinessCheck::REAL_MT5_ENVIRONMENT_AVAILABLE, true, ""});
    d = realworld::RealWorldValidation::evaluate(checks);
    CHECK(d.ready_for_controlled_validation);

    // never automatic, never claims profitability/safety
    CHECK(!realworld::RealWorldValidation::may_execute_automatically());
    CHECK(!realworld::RealWorldValidation::claims_profitability());
    CHECK(!realworld::RealWorldValidation::claims_production_safety());
}

static void test_registry_append_only() {
    realworld::ValidationRegistry reg;
    realworld::ValidationRun r;
    r.run_id = foundation::EntityId("RUN|1");
    r.campaign_id = "CAMPAIGN-A";
    r.broker_profile = "brokerX-measured";
    r.environment_version = "env-v1";
    r.outcome = realworld::RunOutcome::IN_PROGRESS;
    r.started_at = foundation::Timestamp::from_seconds(100);
    CHECK(reg.record(r));
    CHECK(!reg.record(r));  // duplicate rejected
    CHECK(reg.size() == 1);
    CHECK(reg.contains(r.run_id));

    realworld::ValidationRun r2 = r;
    r2.run_id = foundation::EntityId("RUN|2");
    r2.outcome = realworld::RunOutcome::COMPLETED_SIMULATED;
    r2.notes = "shadow only; not a profitability claim";
    CHECK(reg.record(r2));
    auto all = reg.ordered();
    CHECK(all.size() == 2);
    CHECK(all[0].run_id == r.run_id);
    CHECK(all[1].outcome == realworld::RunOutcome::COMPLETED_SIMULATED);

    // invalid run rejected
    realworld::ValidationRun bad;
    bad.run_id = foundation::EntityId("RUN|3");
    CHECK(!reg.record(bad));
}

int main() {
    test_readiness_all_required();
    test_registry_append_only();
    if (g_failures == 0) {
        std::printf("RealWorldTests: ALL PASS\n");
        return 0;
    }
    std::printf("RealWorldTests: %d FAILURE(S)\n", g_failures);
    return 1;
}
