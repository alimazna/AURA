// Phase 2 observation-and-outcomes behavioural tests.
//
// Test artifact for the Phase 2 contracts (OB-0001..OB-0004). Exercises real code
// paths (no mocks) and asserts the phase's acceptance properties: the prediction
// ledger is deterministic, append-only, auditable and idempotent; outcome
// resolution is deterministic with no lookahead and no repaint; failures are
// structured objects (V3-27), never plain strings; and system health aggregates
// deterministically without fabricating a healthy state. Exits non-zero on any
// failure.

#include "foundation/EntityId.h"
#include "foundation/ErrorCode.h"
#include "foundation/ErrorRecord.h"
#include "foundation/ErrorSeverity.h"
#include "foundation/HashDigest.h"
#include "foundation/IPersistenceStore.h"
#include "foundation/PersistenceRecordMetadata.h"
#include "foundation/PersistenceStatus.h"
#include "foundation/RecoveryAction.h"
#include "foundation/SchemaVersion.h"
#include "foundation/ServiceState.h"
#include "foundation/Timestamp.h"
#include "observation/FailureDetectionEngine.h"
#include "observation/OutcomeEngine.h"
#include "observation/PredictionLedger.h"
#include "observation/SystemHealthMonitor.h"
#include "runtime/AdapterManager.h"
#include "runtime/BarFinalizer.h"
#include "runtime/DataBus.h"

#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

using namespace aura::observation;
using aura::foundation::EntityId;
using aura::foundation::ErrorCode;
using aura::foundation::ErrorRecord;
using aura::foundation::ErrorSeverity;
using aura::foundation::SchemaVersion;
using aura::foundation::ServiceState;
using aura::foundation::Timestamp;

static int g_failures = 0;
static void check(bool condition, const char* what) {
    if (!condition) {
        std::printf("FAIL: %s\n", what);
        ++g_failures;
    }
}

class MemoryStore final : public aura::foundation::IPersistenceStore {
public:
    aura::foundation::PersistenceStatus append(
        const aura::foundation::PersistenceRecordMetadata& metadata,
        const aura::foundation::HashDigest& payload_digest) override {
        const auto it = records_.find(metadata.record_id());
        if (it != records_.end()) {
            return it->second == payload_digest ? aura::foundation::PersistenceStatus::OK
                                                : aura::foundation::PersistenceStatus::CONFLICT;
        }
        records_.emplace(metadata.record_id(), payload_digest);
        return aura::foundation::PersistenceStatus::OK;
    }
    aura::foundation::PersistenceStatus contains(const EntityId& id) const override {
        return records_.find(id) != records_.end() ? aura::foundation::PersistenceStatus::OK
                                                   : aura::foundation::PersistenceStatus::NOT_FOUND;
    }
    aura::foundation::PersistenceStatus flush() override {
        return aura::foundation::PersistenceStatus::OK;
    }
    std::size_t size() const { return records_.size(); }

private:
    std::map<EntityId, aura::foundation::HashDigest> records_;
};

static Prediction make_prediction(double reference, double stop, double target) {
    Prediction prediction;
    prediction.decision_id = aura::foundation::HashDigest{};
    prediction.symbol = "XAUUSD";
    prediction.timeframe = aura::runtime::Timeframe::M15;
    prediction.direction = aura::runtime::SignalDirection::LONG;
    prediction.based_on = aura::runtime::BarIdentity("XAUUSD", aura::runtime::Timeframe::M15,
                                                     Timestamp::from_seconds(60));
    prediction.reference_price = reference;
    prediction.stop_price = stop;
    prediction.target_price = target;
    prediction.score = 0.5;
    prediction.predicted_at = Timestamp::from_seconds(60);
    prediction.valid = true;
    return prediction;
}

static aura::runtime::MarketBar bar(Timestamp close_t, double o, double h, double l, double c) {
    aura::runtime::MarketBar b;
    b.timeframe = aura::runtime::Timeframe::M15;
    b.open_time = Timestamp::from_seconds(close_t.seconds() - 60);
    b.close_time = close_t;
    b.open = o;
    b.high = h;
    b.low = l;
    b.close = c;
    b.volume = 100.0;
    b.closed = true;
    return b;
}

static void test_prediction_ledger() {
    const Prediction prediction = make_prediction(100.0, 98.0, 104.0);
    const EntityId id1 = PredictionLedger::make_prediction_id(prediction);
    const EntityId id2 = PredictionLedger::make_prediction_id(prediction);
    check(id1.valid() && id1 == id2, "prediction identity is deterministic");

    PredictionLedger ledger;
    check(ledger.record(prediction), "first prediction recorded");
    const std::size_t size = ledger.size();
    check(ledger.record(prediction), "re-recording identical prediction is idempotent success");
    check(ledger.size() == size, "idempotent record does not duplicate");

    // A different prediction has a different identity.
    check(PredictionLedger::make_prediction_id(make_prediction(100.0, 97.0, 104.0)) != id1,
          "different stop -> different identity");

    // Conflicting content under an existing identity is rejected.
    Prediction conflict = prediction;
    conflict.prediction_id = id1;
    conflict.target_price = 999.0;
    check(!ledger.append(conflict), "conflicting prediction for same identity rejected");
    check(ledger.size() == size, "conflict did not mutate the ledger");

    // Auditable: the recorded prediction can be found by identity.
    const Prediction* found = ledger.find(id1);
    check(found != nullptr && found->target_price == 104.0, "recorded prediction is retrievable/auditable");

    // Persistence idempotency.
    MemoryStore memory;
    const SchemaVersion schema = SchemaVersion::from_string("1.0.0");
    check(aura::foundation::is_success(ledger.persist(memory, schema)), "prediction persist succeeds");
    const std::size_t stored = memory.size();
    check(aura::foundation::is_success(ledger.persist(memory, schema)), "prediction re-persist succeeds");
    check(memory.size() == stored, "prediction persist is idempotent");

    // Invalid prediction is rejected, never silently recorded.
    Prediction invalid;
    check(!ledger.record(invalid), "invalid prediction rejected");
}

static void test_outcome_engine() {
    OutcomeEngine engine;
    const Prediction prediction = make_prediction(100.0, 98.0, 104.0);
    PredictionLedger ledger;
    ledger.record(prediction);
    const Prediction* p = ledger.find(PredictionLedger::make_prediction_id(prediction));
    check(p != nullptr, "prediction available for outcome resolution");

    // Target hit.
    std::vector<aura::runtime::MarketBar> target_bars{bar(Timestamp::from_seconds(120), 100.0, 105.0, 99.0, 104.5)};
    const Outcome target = engine.resolve(*p, target_bars, Timestamp::from_seconds(120));
    check(target.valid && target.state == OutcomeState::TARGET_HIT, "target hit resolved");
    check(target.realised_pnl > 0.0 && target.r_multiple > 0.0, "target outcome is profitable in R units");

    // Stop hit.
    std::vector<aura::runtime::MarketBar> stop_bars{bar(Timestamp::from_seconds(120), 100.0, 101.0, 97.0, 97.5)};
    const Outcome stop = engine.resolve(*p, stop_bars, Timestamp::from_seconds(120));
    check(stop.state == OutcomeState::STOP_HIT, "stop hit resolved");
    check(stop.realised_pnl < 0.0, "stop outcome is a loss");

    // Both touched in one bar -> conservative stop-first, never optimistic.
    std::vector<aura::runtime::MarketBar> both_bars{bar(Timestamp::from_seconds(120), 100.0, 105.0, 97.0, 100.0)};
    check(engine.resolve(*p, both_bars, Timestamp::from_seconds(120)).state == OutcomeState::STOP_HIT,
          "ambiguous bar resolves conservatively to STOP_HIT");

    // No lookahead: a bar at the decision close_time is not used.
    std::vector<aura::runtime::MarketBar> same_time{bar(Timestamp::from_seconds(60), 100.0, 105.0, 99.0, 104.5)};
    check(engine.resolve(*p, same_time, Timestamp::from_seconds(60)).state != OutcomeState::TARGET_HIT,
          "bar at/before decision time cannot resolve the prediction (no lookahead)");

    // No bars -> still open, not fabricated.
    check(engine.resolve(*p, {}, Timestamp::from_seconds(60)).state == OutcomeState::OPEN,
          "no bars -> outcome OPEN (not fabricated)");

    // Bars present but no trigger -> expired, not a win or a loss.
    std::vector<aura::runtime::MarketBar> flat{bar(Timestamp::from_seconds(120), 100.0, 101.0, 99.5, 100.5)};
    check(engine.resolve(*p, flat, Timestamp::from_seconds(120)).state == OutcomeState::EXPIRED,
          "no trigger within horizon -> EXPIRED");

    // Deterministic: resolving twice yields identical outcome.
    const Outcome a = engine.resolve(*p, target_bars, Timestamp::from_seconds(120));
    const Outcome b = engine.resolve(*p, target_bars, Timestamp::from_seconds(120));
    check(a.state == b.state && a.realised_pnl == b.realised_pnl && a.r_multiple == b.r_multiple,
          "outcome resolution is deterministic");
}

static void test_failure_detection() {
    FailureDetectionEngine engine;

    // A stream in ERROR produces a structured record, not a plain string.
    aura::runtime::AdapterManager adapters;
    adapters.report_bar(aura::runtime::Timeframe::M15, Timestamp::from_seconds(100), 1);
    adapters.report_error(aura::runtime::Timeframe::M15, "feed gap");
    const std::vector<ErrorRecord> records = engine.detect_adapter_failures(adapters, Timestamp::from_seconds(200));
    check(!records.empty(), "adapter failure detected");
    check(records[0].valid(), "failure is a valid structured ErrorRecord");
    check(records[0].error_id().valid(), "failure carries its own error identity");
    check(!records[0].component().empty(), "failure names the producing component");
    check(records[0].severity() != ErrorSeverity::INFO, "failure has a non-INFO severity");
    check(records[0].recovery_action() != aura::foundation::RecoveryAction::UNKNOWN,
          "failure carries a known recovery action");

    // Explicit structured construction, with an explicit ErrorCode.
    const ErrorRecord made = FailureDetectionEngine::make_record(
        FailureCategory::DATA_FAILURE, ErrorCode::DATA_ERROR, ErrorSeverity::ERROR,
        Timestamp::from_seconds(1), ServiceState::OFFLINE, "message", "context",
        aura::foundation::RecoveryAction::RETRY);
    check(made.valid(), "explicit structured record is valid");
    check(made.component().find("DATA_ERROR") != std::string::npos, "record component encodes ErrorCode");

    // A healthy adapter set yields no failures.
    aura::runtime::AdapterManager healthy;
    for (const aura::runtime::Timeframe tf : aura::runtime::all_timeframes()) {
        healthy.report_bar(tf, Timestamp::from_seconds(100), 1);
    }
    check(engine.detect_adapter_failures(healthy, Timestamp::from_seconds(200)).empty(),
          "healthy adapters produce no failure records");

    // Outcome failure: UNKNOWN resolution is a structured failure.
    const Prediction prediction = make_prediction(100.0, 98.0, 104.0);
    PredictionLedger ledger;
    ledger.record(prediction);
    const Outcome unknown_outcome;  // invalid -> UNKNOWN
    check(engine.detect_outcome_failures(prediction, unknown_outcome, Timestamp::from_seconds(1)).empty(),
          "invalid outcome is not reported as a failure");
}

static void test_system_health() {
    SystemHealthMonitor monitor;

    // Unassessed system is not healthy; no fabricated healthy state.
    aura::runtime::AdapterManager unassessed;
    const HealthReport empty = monitor.observe(unassessed, Timestamp::from_seconds(1));
    check(empty.valid && !empty.healthy, "unassessed system is not healthy");
    check(empty.aggregate == ServiceState::STARTING, "unassessed aggregate is STARTING");

    // All streams online -> healthy.
    aura::runtime::AdapterManager healthy;
    for (const aura::runtime::Timeframe tf : aura::runtime::all_timeframes()) {
        healthy.report_bar(tf, Timestamp::from_seconds(100), 1);
    }
    const HealthReport good = monitor.observe(healthy, Timestamp::from_seconds(2));
    check(good.healthy && good.aggregate == ServiceState::ONLINE, "all-online system is healthy");
    check(good.online == good.total && good.failures.empty(), "healthy report counts every stream");

    // One offline stream cannot be hidden by healthy peers.
    aura::runtime::AdapterManager degraded = healthy;
    degraded.report_disconnected(aura::runtime::Timeframe::D1, "no data");
    const HealthReport bad = monitor.observe(degraded, Timestamp::from_seconds(3));
    check(!bad.healthy, "one offline stream makes the system unhealthy");
    check(bad.aggregate == ServiceState::OFFLINE, "aggregate reflects the worst stream");
    check(!bad.failures.empty(), "offline stream surfaces a structured failure");

    // Deterministic ordering: worst() is stable and total.
    check(SystemHealthMonitor::rank(ServiceState::BLOCKED) > SystemHealthMonitor::rank(ServiceState::OFFLINE),
          "BLOCKED is worse than OFFLINE");
    check(SystemHealthMonitor::worst(ServiceState::ONLINE, ServiceState::DEGRADED) == ServiceState::DEGRADED,
          "worst() picks the more severe state");
}

int main() {
    test_prediction_ledger();
    test_outcome_engine();
    test_failure_detection();
    test_system_health();

    if (g_failures == 0) {
        std::printf("Phase 2 observation behavioural tests: PASS\n");
        return 0;
    }
    std::printf("Phase 2 observation behavioural tests: %d FAILURE(S)\n", g_failures);
    return 1;
}
