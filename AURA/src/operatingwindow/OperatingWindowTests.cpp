// Phase 8 operating-window/recovery behavioural tests (WIN-0001..WIN-0004).
//
// Deterministic; no clock, no threads, no I/O. Exits non-zero on the first failure.

#include "foundation/EntityId.h"
#include "foundation/Timestamp.h"
#include "operatingwindow/Checkpoint.h"
#include "operatingwindow/OperatingWindow.h"
#include "operatingwindow/ResourceBudget.h"
#include "operatingwindow/WindowOrchestrator.h"

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

static void test_window_bounds_and_no_self_extension() {
    operatingwindow::OperatingWindow w({3 * 3600, 8 * 3600, 300});
    CHECK(w.definition().well_formed());
    // human may set within bounds
    CHECK(w.schedule({"human", 4 * 3600}).accepted);
    // beyond max refused for everyone
    auto d = w.schedule({"runtime", 9 * 3600});
    CHECK(!d.accepted);
    CHECK(d.effective_seconds == 0);
    // runtime cannot schedule below min
    CHECK(!w.schedule({"runtime", 60}).accepted);
    // runtime within bounds accepted
    auto r = w.schedule({"research", 3 * 3600});
    CHECK(r.accepted);
    CHECK(r.effective_seconds == 3 * 3600);
    // zero/negative duration refused
    CHECK(!w.schedule({"human", 0}).accepted);
    // system cannot extend its own window
    CHECK(!operatingwindow::OperatingWindow::may_self_extend());
    // malformed window
    operatingwindow::OperatingWindow bad({8 * 3600, 3 * 3600, 10});
    CHECK(!bad.definition().well_formed());
    CHECK(!bad.schedule({"human", 4 * 3600}).accepted);
}

static void test_window_phases() {
    operatingwindow::OperatingWindow w({3 * 3600, 8 * 3600, 300});
    const std::int64_t win = 4 * 3600;
    CHECK(w.phase_at(0, win) == operatingwindow::WindowPhase::ACTIVE);
    CHECK(w.phase_at(win - 301, win) == operatingwindow::WindowPhase::ACTIVE);
    CHECK(w.phase_at(win - 300, win) == operatingwindow::WindowPhase::DRAINING);
    CHECK(w.phase_at(win - 1, win) == operatingwindow::WindowPhase::DRAINING);
    CHECK(w.phase_at(win, win) == operatingwindow::WindowPhase::OFFLINE);
    CHECK(w.phase_at(win + 10, win) == operatingwindow::WindowPhase::OFFLINE);
    // zero window is offline
    CHECK(w.phase_at(0, 0) == operatingwindow::WindowPhase::OFFLINE);
}

static void test_checkpoint_recovery() {
    operatingwindow::CheckpointStore store;
    operatingwindow::Checkpoint c;
    c.checkpoint_id = foundation::EntityId("CKPT|1");
    c.schema_version = "v1";
    c.artifact_digest = "abc";
    c.created_at = ts(10);
    c.validity = operatingwindow::CheckpointValidity::CORRUPT;
    CHECK(store.record(c));
    CHECK(!store.record(c));  // duplicate rejected
    CHECK(store.size() == 1);
    // corrupt checkpoint is not usable; no known-good -> refuse to guess
    auto d = store.recover("v1", false);
    CHECK(!d.resumed);
    // with known-good available -> recover to known-good
    d = store.recover("v1", true);
    CHECK(d.resumed);
    CHECK(d.used_known_good);

    // a valid checkpoint
    operatingwindow::Checkpoint v;
    v.checkpoint_id = foundation::EntityId("CKPT|2");
    v.schema_version = "v1";
    v.artifact_digest = "def";
    v.created_at = ts(20);
    v.validity = operatingwindow::CheckpointValidity::VALID;
    CHECK(store.record(v));
    d = store.recover("v1", false);
    CHECK(d.resumed);
    CHECK(!d.used_known_good);
    CHECK(d.reason.find("CKPT|2") != std::string::npos);

    // incompatible schema version is not usable -> includes incompatible checkpoint
    CHECK(store.latest_usable("v2") == nullptr);
    // invalid checkpoint (no digest) rejected
    operatingwindow::Checkpoint bad;
    bad.checkpoint_id = foundation::EntityId("CKPT|3");
    bad.schema_version = "v1";
    CHECK(!store.record(bad));
}

static void test_resource_budget() {
    operatingwindow::ResourceBudget b;
    b.set_limit(operatingwindow::ResourceKind::CPU_SECONDS, 100);
    CHECK(b.state(operatingwindow::ResourceKind::CPU_SECONDS) == operatingwindow::ResourceState::OK);
    CHECK(b.consume(operatingwindow::ResourceKind::CPU_SECONDS, 80));
    CHECK(b.state(operatingwindow::ResourceKind::CPU_SECONDS) == operatingwindow::ResourceState::WARNING);
    CHECK(!b.consume(operatingwindow::ResourceKind::CPU_SECONDS, 21));  // no partial spend
    CHECK(b.consumed(operatingwindow::ResourceKind::CPU_SECONDS) == 80);
    CHECK(b.consume(operatingwindow::ResourceKind::CPU_SECONDS, 20));
    CHECK(b.state(operatingwindow::ResourceKind::CPU_SECONDS) == operatingwindow::ResourceState::EXHAUSTED);
    CHECK(!b.consume(operatingwindow::ResourceKind::CPU_SECONDS, 1));
    CHECK(!b.consume(operatingwindow::ResourceKind::CPU_SECONDS, -5));
    // unbounded
    CHECK(b.remaining(operatingwindow::ResourceKind::MEMORY_MB) == -1);
    CHECK(b.consume(operatingwindow::ResourceKind::MEMORY_MB, 1000000));
    CHECK(b.state(operatingwindow::ResourceKind::MEMORY_MB) == operatingwindow::ResourceState::OK);
}

static void test_drain_sequence() {
    operatingwindow::WindowOrchestrator orch({3 * 3600, 8 * 3600, 300});
    std::vector<operatingwindow::ActiveWork> work{
        {"w1", operatingwindow::WorkClass::NON_CRITICAL, false},
        {"w2", operatingwindow::WorkClass::RESUMABLE, true},
        {"w3", operatingwindow::WorkClass::RESUMABLE, false},   // not checkpointed -> refused
        {"w4", operatingwindow::WorkClass::CRITICAL, false},
    };
    auto rep = orch.drain(work);
    CHECK(!rep.completed);  // w3 uncheckpointed resumable blocks completion
    CHECK(rep.stopped == 1);
    CHECK(rep.checkpointed == 1);
    CHECK(rep.finalized == 1);
    CHECK(rep.refused_uncheckpointed_resumable.size() == 1);
    CHECK(rep.refused_uncheckpointed_resumable[0] == "w3");

    // all checkpointed -> completes
    work[2].checkpointed = true;
    rep = orch.drain(work);
    CHECK(rep.completed);
    CHECK(rep.persistence_verified == 1);

    // canonical drain sequence order
    auto seq = operatingwindow::OperatingWindow::drain_sequence();
    CHECK(seq.size() == 7);
    CHECK(seq.front() == operatingwindow::DrainStep::STOP_NEW_NON_CRITICAL);
    CHECK(seq.back() == operatingwindow::DrainStep::OFFLINE);
}

int main() {
    test_window_bounds_and_no_self_extension();
    test_window_phases();
    test_checkpoint_recovery();
    test_resource_budget();
    test_drain_sequence();
    if (g_failures == 0) {
        std::printf("OperatingWindowTests: ALL PASS\n");
        return 0;
    }
    std::printf("OperatingWindowTests: %d FAILURE(S)\n", g_failures);
    return 1;
}
