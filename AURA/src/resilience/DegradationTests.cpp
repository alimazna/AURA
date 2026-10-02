// RS-0020 / Phase 0.5 resilience foundation: deterministic behavioural tests for
// graceful degradation, subsystem isolation and stale-candle detection.
//
// This is the phase's test artifact. It exercises real code paths of the
// resilience contracts (no mocks) and asserts the V3-13/V3-15 properties that the
// phase milestone requires. Exits non-zero on any failure.

#include "foundation/Guardian.h"
#include "foundation/ServiceState.h"
#include "foundation/SystemMode.h"
#include "foundation/Timestamp.h"
#include "resilience/CapabilityDescriptor.h"
#include "resilience/CapabilityId.h"
#include "resilience/CapabilityRegistry.h"
#include "resilience/CriticalityPolicy.h"
#include "resilience/DataFreshnessMonitor.h"
#include "resilience/DegradationImpact.h"
#include "resilience/DependencyDescriptor.h"
#include "resilience/DependencyGraph.h"
#include "resilience/FreshnessState.h"
#include "resilience/GracefulDegradationManager.h"
#include "resilience/HealthSnapshot.h"
#include "resilience/HealthStateEngine.h"
#include "resilience/ImpactResolver.h"
#include "resilience/PauseResumeManager.h"
#include "resilience/RecoveryManager.h"
#include "resilience/ServiceDescriptor.h"
#include "resilience/StaleCandleDetector.h"
#include "resilience/SubsystemIsolationManager.h"
#include "resilience/SystemSupervisor.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace aura::resilience;
using aura::foundation::DataQualityState;
using aura::foundation::RecoveryAction;
using aura::foundation::ServiceState;
using aura::foundation::SystemMode;
using aura::foundation::Timestamp;

static int g_failures = 0;
static void check(bool condition, const char* what) {
    if (!condition) {
        std::printf("FAIL: %s\n", what);
        ++g_failures;
    }
}

static ServiceDescriptor svc(const char* name, ServiceState state) {
    return ServiceDescriptor(std::string(name), state);
}
static CapabilityId cap(const char* name) {
    return CapabilityId::from_string(name);
}

// Builds a small dependency graph: m15 <- h4 ; m15 <- m5 ; h4 <- d1
struct Fixture {
    ServiceDescriptor s_h4{svc("svc-h4", ServiceState::ONLINE)};
    ServiceDescriptor s_m15{svc("svc-m15", ServiceState::ONLINE)};
    ServiceDescriptor s_m5{svc("svc-m5", ServiceState::ONLINE)};
    ServiceDescriptor s_d1{svc("svc-d1", ServiceState::ONLINE)};

    CapabilityRegistry registry;
    DependencyGraph graph;

    Fixture() {
        registry.register_capability(CapabilityDescriptor(cap("m15"), s_m15));
        registry.register_capability(CapabilityDescriptor(cap("h4"), s_h4));
        registry.register_capability(CapabilityDescriptor(cap("m5"), s_m5));
        registry.register_capability(CapabilityDescriptor(cap("d1"), s_d1));
        // m15 depends on h4 and m5; h4 depends on d1.
        graph.add_edge(DependencyDescriptor(cap("m15"), cap("h4")));
        graph.add_edge(DependencyDescriptor(cap("m15"), cap("m5")));
        graph.add_edge(DependencyDescriptor(cap("h4"), cap("d1")));
        registry.add_dependency(DependencyDescriptor(cap("m15"), cap("h4")));
        registry.add_dependency(DependencyDescriptor(cap("m15"), cap("m5")));
        registry.add_dependency(DependencyDescriptor(cap("h4"), cap("d1")));
    }

    HealthStateEngine::snapshot_map snapshots(ServiceState h4, ServiceState m15,
                                              ServiceState m5, ServiceState d1) const {
        HealthStateEngine::snapshot_map m;
        m[s_h4] = HealthSnapshot(s_h4, h4, Timestamp::from_seconds(100), "h4");
        m[s_m15] = HealthSnapshot(s_m15, m15, Timestamp::from_seconds(100), "m15");
        m[s_m5] = HealthSnapshot(s_m5, m5, Timestamp::from_seconds(100), "m5");
        m[s_d1] = HealthSnapshot(s_d1, d1, Timestamp::from_seconds(100), "d1");
        return m;
    }
};

static void test_registry_and_graph() {
    Fixture f;
    check(f.registry.size() == 4, "registry holds four capabilities");
    check(f.registry.dependency_count() == 3, "registry holds three edges");
    check(!f.registry.register_capability(CapabilityDescriptor(cap("m15"), f.s_m15)),
          "duplicate capability id rejected");
    check(!f.graph.has_cycle(), "acyclic graph reported acyclic");
    check(f.graph.node_count() == 4, "graph has four nodes");
    // Topological order: dependencies before dependents.
    const std::vector<CapabilityId> order = f.graph.topological_order();
    check(order.size() == 4, "topological order complete");
    auto pos = [&](const char* id) {
        for (std::size_t i = 0; i < order.size(); ++i)
            if (order[i] == cap(id)) return static_cast<int>(i);
        return -1;
    };
    check(pos("d1") < pos("h4"), "d1 precedes h4 in topological order");
    check(pos("h4") < pos("m15"), "h4 precedes m15 in topological order");
    check(pos("m5") < pos("m15"), "m5 precedes m15 in topological order");

    // Self-edge is not a valid dependency.
    check(!DependencyDescriptor(cap("h4"), cap("h4")).valid(), "self-edge rejected");
    check(!f.graph.add_edge(DependencyDescriptor(cap("h4"), cap("h4"))), "self-edge not added");
}

static void test_cycle_detection() {
    DependencyGraph g;
    check(g.add_edge(DependencyDescriptor(cap("a"), cap("b"))), "edge a->b");
    check(g.add_edge(DependencyDescriptor(cap("b"), cap("c"))), "edge b->c");
    check(g.add_edge(DependencyDescriptor(cap("c"), cap("a"))), "edge c->a");
    check(g.has_cycle(), "cycle detected");
    check(g.topological_order().empty(), "cyclic graph has empty topological order");
}

static void test_health_propagation() {
    Fixture f;
    HealthStateEngine engine(f.registry, f.graph);

    // All healthy -> m15 ONLINE.
    check(engine.capability_state(cap("m15"), f.snapshots(ServiceState::ONLINE, ServiceState::ONLINE,
                                                           ServiceState::ONLINE, ServiceState::ONLINE)) ==
              ServiceState::ONLINE,
          "all-online capability is ONLINE");

    // H4 offline -> m15 (depends on h4) degrades, does not stay ONLINE.
    const auto s = f.snapshots(ServiceState::OFFLINE, ServiceState::ONLINE, ServiceState::ONLINE,
                               ServiceState::ONLINE);
    check(engine.capability_state(cap("m15"), s) == ServiceState::DEGRADED,
          "dependency failure degrades dependent");
    check(engine.capability_state(cap("h4"), s) == ServiceState::OFFLINE,
          "failed capability reports its own state");
    // m5 is independent of h4 -> still ONLINE.
    check(engine.capability_state(cap("m5"), s) == ServiceState::ONLINE,
          "independent capability stays ONLINE");

    // Missing snapshot -> BLOCKED, never fabricated ONLINE.
    HealthStateEngine::snapshot_map empty;
    check(engine.capability_state(cap("m15"), empty) == ServiceState::BLOCKED,
          "missing health is BLOCKED");
    // Unregistered capability -> BLOCKED.
    check(engine.capability_state(cap("nope"), empty) == ServiceState::BLOCKED,
          "unregistered capability is BLOCKED");
}

static void test_isolation_and_degradation() {
    Fixture f;
    SubsystemIsolationManager isolation(f.registry, f.graph);

    // Isolating m15 disables only m15 (nothing depends on it).
    const IsolationDecision iso_m15 = isolation.isolate(cap("m15"));
    check(iso_m15.valid, "m15 isolation valid");
    check(iso_m15.disabled.size() == 1, "m15 isolation disables only m15");
    check(iso_m15.available.size() == 3, "three capabilities remain available");
    check(isolation.is_disabled_by(cap("m15"), cap("h4")) == false,
          "isolating m15 does not disable h4");

    // Isolating h4 disables h4 and its dependent m15; m5 and d1 remain.
    const IsolationDecision iso_h4 = isolation.isolate(cap("h4"));
    check(iso_h4.disabled.size() == 2, "h4 isolation disables h4 and m15");
    check(isolation.is_disabled_by(cap("h4"), cap("m15")), "m15 disabled by h4 isolation");
    check(isolation.is_disabled_by(cap("h4"), cap("m5")) == false,
          "m5 (independent) not disabled by h4 isolation");
    check(isolation.is_disabled_by(cap("h4"), cap("d1")) == false,
          "d1 (dependency of h4) not disabled by h4 isolation");

    // Criticality: m5 non-critical, h4 critical, m15 unclassified.
    CriticalityPolicy policy(CriticalityPolicy::classification_map{
        {cap("m5"), Criticality::NON_CRITICAL}, {cap("h4"), Criticality::CRITICAL}});
    GracefulDegradationManager degradation(f.registry, policy, isolation);

    const DegradationDecision d_m5 = degradation.apply(cap("m5"));
    check(d_m5.tolerable, "non-critical failure is tolerable");
    check(!d_m5.requires_protection, "non-critical failure needs no protection");

    const DegradationDecision d_h4 = degradation.apply(cap("h4"));
    check(!d_h4.tolerable, "critical failure is not tolerable");
    check(d_h4.requires_protection, "critical failure requires protection");

    const DegradationDecision d_m15 = degradation.apply(cap("m15"));
    check(!d_m15.tolerable, "unclassified failure is not assumed tolerable");
    check(d_m15.requires_protection, "unclassified failure requires protection");
    check(policy.classify(cap("unknown")) == Criticality::UNCLASSIFIED,
          "unclassified capability is UNCLASSIFIED");
}

static void test_impact_resolver() {
    Fixture f;
    ImpactResolver resolver(f.graph);
    const DegradationImpact impact = resolver.resolve(cap("h4"));
    check(impact.failed() == cap("h4"), "impact names exact failed capability");
    check(impact.affects(cap("h4")), "failed capability is affected");
    check(impact.affects(cap("m15")), "dependent m15 is affected");
    check(!impact.affects(cap("m5")), "independent m5 is not affected");
    check(impact.leaves_available(cap("m5")), "m5 remains available");
    check(impact.leaves_available(cap("d1")), "d1 remains available");
}

static void test_recovery() {
    Fixture f;
    HealthStateEngine engine(f.registry, f.graph);
    SubsystemIsolationManager isolation(f.registry, f.graph);
    RecoveryManager recovery(engine, isolation);

    const auto online = f.snapshots(ServiceState::ONLINE, ServiceState::ONLINE, ServiceState::ONLINE,
                                    ServiceState::ONLINE);
    check(recovery.plan(cap("h4"), online, false).action == RecoveryAction::NONE,
          "online capability needs no recovery");

    const auto h4_offline = f.snapshots(ServiceState::OFFLINE, ServiceState::ONLINE,
                                        ServiceState::ONLINE, ServiceState::ONLINE);
    const RecoveryDecision with_known = recovery.plan(cap("h4"), h4_offline, true);
    check(with_known.action == RecoveryAction::ROLLBACK, "offline + known-valid -> rollback");
    check(with_known.uses_known_valid_state, "rollback uses known-valid state");
    check(with_known.recoverable, "rollback is recoverable");
    check(with_known.restored.size() == 1, "rollback restores dependent m15");

    const RecoveryDecision without_known = recovery.plan(cap("h4"), h4_offline, false);
    check(without_known.action == RecoveryAction::SAFE_MODE,
          "offline without known-valid -> safe mode");
    check(!without_known.recoverable, "unrecoverable without known-valid state");

    const auto m5_degraded = f.snapshots(ServiceState::ONLINE, ServiceState::ONLINE,
                                         ServiceState::DEGRADED, ServiceState::ONLINE);
    check(recovery.plan(cap("m5"), m5_degraded, false).action == RecoveryAction::RETRY,
          "degraded capability is retried");

    const auto paused = f.snapshots(ServiceState::ONLINE, ServiceState::ONLINE,
                                    ServiceState::ONLINE, ServiceState::PAUSED);
    check(recovery.plan(cap("d1"), paused, false).action == RecoveryAction::NONE,
          "paused capability is not treated as failed");
}

static void test_freshness_and_stale_candles() {
    // Freshness classification.
    const Timestamp observed = Timestamp::from_seconds(1000);
    const Timestamp now = Timestamp::from_seconds(1000 + 10);
    check(classify_freshness(observed, now, Timestamp::from_seconds(30).nanoseconds(),
                             Timestamp::from_seconds(60).nanoseconds()) == FreshnessState::FRESH,
          "recent observation is FRESH");
    check(classify_freshness(observed, now, Timestamp::from_seconds(5).nanoseconds(),
                             Timestamp::from_seconds(60).nanoseconds()) == FreshnessState::STALE,
          "observation past stale threshold is STALE");
    check(classify_freshness(observed, now, Timestamp::from_seconds(1).nanoseconds(),
                             Timestamp::from_seconds(5).nanoseconds()) == FreshnessState::EXPIRED,
          "observation past expire threshold is EXPIRED");
    // Future observation is not fabricated into FRESH.
    check(classify_freshness(Timestamp::from_seconds(2000), now, Timestamp::from_seconds(30).nanoseconds(),
                             Timestamp::from_seconds(60).nanoseconds()) == FreshnessState::UNKNOWN,
          "future observation is UNKNOWN, not FRESH");
    // Non-positive thresholds yield UNKNOWN.
    check(classify_freshness(observed, now, 0, Timestamp::from_seconds(60).nanoseconds()) ==
              FreshnessState::UNKNOWN,
          "non-positive threshold is UNKNOWN");
    check(!is_fresh(FreshnessState::UNKNOWN), "UNKNOWN is not fresh");

    // Per-timeframe stale detection: nine distinct identities preserved.
    StaleCandleDetector detector(Timestamp::from_seconds(90).nanoseconds(),
                                 Timestamp::from_seconds(300).nanoseconds());
    StaleCandleDetector::bar_time_map bars;
    const char* const tfs[] = {"M1", "M5", "M15", "M30", "H1", "H4", "D1", "W1", "MN1"};
    for (const char* tf : tfs) bars[std::string(tf)] = Timestamp::from_seconds(1000);
    // M15 goes stale; everything else stays fresh.
    bars["M15"] = Timestamp::from_seconds(1000 - 120);
    const Timestamp now2 = Timestamp::from_seconds(1000 + 5);

    check(detector.quality_of("M15", bars, now2) == DataQualityState::STALE, "M15 reported stale");
    check(detector.quality_of("H4", bars, now2) == DataQualityState::VALID, "H4 reported fresh");
    check(detector.quality_of("M5", bars, now2) == DataQualityState::VALID, "M5 reported fresh");
    check(detector.quality_of("MN1", bars, now2) == DataQualityState::VALID, "MN1 reported fresh");

    const std::vector<std::string> stale = detector.stale_timeframes(bars, now2);
    check(stale.size() == 1 && stale[0] == "M15", "exactly M15 is reported stale");
    check(detector.quality_of("UNKNOWN_TF", bars, now2) == DataQualityState::MISSING,
          "untracked timeframe is MISSING");
}

static void test_pause_resume_and_supervisor() {
    Fixture f;
    PauseResumeManager pr(f.registry);
    // Pausing requires a reason and an operating mode.
    check(!pr.pause(cap("m5"), "", SystemMode::SHADOW), "pause with empty reason rejected");
    check(!pr.pause(cap("m5"), "operator", SystemMode::HALTED), "pause in HALTED mode rejected");
    check(pr.pause(cap("m5"), "operator request", SystemMode::SHADOW), "pause accepted in SHADOW");
    check(pr.is_paused(cap("m5")), "m5 is paused");
    check(pr.reason_for(cap("m5")) != nullptr && *pr.reason_for(cap("m5")) == "operator request",
          "pause reason preserved");
    check(pr.what_paused().size() == 1, "what_paused reports one capability");
    check(!pr.resume(cap("m5"), "", SystemMode::SHADOW), "resume with empty reason rejected");
    check(pr.resume(cap("m5"), "resume", SystemMode::SHADOW), "resume accepted");
    check(!pr.is_paused(cap("m5")), "m5 no longer paused");
    check(!pr.resume(cap("m5"), "resume", SystemMode::SHADOW), "resume when not paused rejected");

    // Supervisor composes and consults the Guardian without escalating itself.
    HealthStateEngine engine(f.registry, f.graph);
    SubsystemIsolationManager isolation(f.registry, f.graph);
    CriticalityPolicy policy(CriticalityPolicy::classification_map{{cap("h4"), Criticality::CRITICAL}});
    GracefulDegradationManager degradation(f.registry, policy, isolation);
    RecoveryManager recovery(engine, isolation);
    aura::foundation::Guardian guardian;

    SystemSupervisor supervisor(engine, degradation, recovery, pr, guardian);
    check(supervisor.guardian_status() == aura::foundation::GuardianStatus::NORMAL,
          "guardian status reported");
    check(supervisor.prescribed_action(aura::foundation::ErrorSeverity::FATAL) == RecoveryAction::HALT,
          "guardian prescribes HALT on FATAL");
    check(supervisor.prescribed_action(aura::foundation::ErrorSeverity::CRITICAL) ==
              RecoveryAction::SAFE_MODE,
          "guardian prescribes SAFE_MODE on CRITICAL");

    const auto online = f.snapshots(ServiceState::ONLINE, ServiceState::ONLINE, ServiceState::ONLINE,
                                    ServiceState::ONLINE);
    const SupervisionView v = supervisor.view(cap("h4"), online, false);
    check(v.capability == cap("h4"), "supervision view names capability");
    check(v.state == ServiceState::ONLINE, "supervision view carries state");
    check(v.degradation.valid, "supervision view carries degradation decision");
    check(v.recovery.valid, "supervision view carries recovery decision");
}

int main() {
    test_registry_and_graph();
    test_cycle_detection();
    test_health_propagation();
    test_isolation_and_degradation();
    test_impact_resolver();
    test_recovery();
    test_freshness_and_stale_candles();
    test_pause_resume_and_supervisor();

    if (g_failures == 0) {
        std::printf("RS-0020 resilience behavioural tests: PASS\n");
        return 0;
    }
    std::printf("RS-0020 resilience behavioural tests: %d FAILURE(S)\n", g_failures);
    return 1;
}
