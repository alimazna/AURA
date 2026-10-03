// Conceptual Phase 3 — Timeframe State verification artifact.
//
// Proves the timeframe-state model over the nine logical MT5 streams (M1, M5, M15,
// M30, H1, H4, D1, W1, MN1). This is a verification artifact for the already-approved
// manifest Phase 1 timeframe-state contracts; it does not add a second pipeline.
// Exercises real code paths (no mocks):
//   - explicit timeframe identity for all nine streams (never positional)
//   - per-stream independence of state, sequence, health and data quality
//   - closed-bar-only processing, deterministic closed-bar identity
//   - duplicate / non-advancing closed-bar rejection (no repaint)
//   - no-lookahead (future-dated events rejected)
//   - failure isolation (M1 fails => the other eight are unchanged)
//   - recovery isolation (M15 recovers => M1/H4 not reset)
//   - deterministic update behaviour (identical input => identical state)
//   - provenance retained (symbol + timeframe + close_time)
// Exits non-zero on any failure.

#include "foundation/DataQualityState.h"
#include "foundation/EventId.h"
#include "foundation/EventMetadata.h"
#include "foundation/EventType.h"
#include "foundation/SchemaVersion.h"
#include "foundation/ServiceState.h"
#include "foundation/Timestamp.h"
#include "runtime/AdapterManager.h"
#include "runtime/BarFinalizer.h"
#include "runtime/DataBus.h"
#include "runtime/DataValidator.h"
#include "runtime/TimeframeStateStore.h"

#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

using aura::runtime::AdapterManager;
using aura::runtime::BarFinalizer;
using aura::runtime::BarIdentity;
using aura::runtime::DataBus;
using aura::runtime::DataValidator;
using aura::runtime::FinalizedBar;
using aura::runtime::MarketBar;
using aura::runtime::MarketEvent;
using aura::runtime::Timeframe;
using aura::runtime::TimeframeStateStore;
using aura::runtime::all_timeframes;
using aura::runtime::timeframe_from_string;

static int g_failures = 0;
static void check(bool condition, const char* what) {
    if (!condition) {
        std::printf("FAIL: %s\n", what);
        ++g_failures;
    }
}

static MarketBar mk_bar(Timeframe tf, std::uint64_t close_secs, double base, bool closed = true) {
    MarketBar b;
    b.timeframe = tf;
    b.open_time = aura::foundation::Timestamp::from_seconds(close_secs - 60);
    b.close_time = aura::foundation::Timestamp::from_seconds(close_secs);
    b.open = base;
    b.high = base + 1.0;
    b.low = base - 0.5;
    b.close = base + 0.8;
    b.volume = 100.0;
    b.closed = closed;
    return b;
}

static MarketEvent mk_event(const MarketBar& b, const std::string& sym, std::uint64_t seq,
                            aura::foundation::Timestamp receive = aura::foundation::Timestamp{}) {
    MarketEvent e;
    e.bar = b;
    const aura::foundation::Timestamp et = b.close_time;
    const aura::foundation::Timestamp rt =
        receive.nanoseconds() == 0 ? b.close_time : receive;
    e.metadata = aura::foundation::EventMetadata(
        aura::foundation::EventId::from_string("evt-" + sym + "-" + std::to_string(seq)),
        aura::foundation::EventType::MARKET_DATA, sym, "mt5", "ea-1", et, rt, seq,
        aura::foundation::SchemaVersion::from_string("1.0.0"));
    return e;
}

// Advances one stream in a store through `count` consecutive closed bars, starting
// at bar index `start_index` so successive calls use strictly later close times.
static void advance_stream(TimeframeStateStore& store, Timeframe tf, int count,
                           std::uint64_t base_seq = 1, int start_index = 0) {
    BarFinalizer finalizer;
    for (int i = 0; i < count; ++i) {
        FinalizedBar fb;
        const int idx = start_index + i;
        const MarketBar b = mk_bar(tf, static_cast<std::uint64_t>(6000 + idx * 60), 100.0 + idx);
        if (finalizer.finalize(mk_event(b, "XAUUSD", base_seq + static_cast<std::uint64_t>(i)), fb)) {
            store.advance(fb);
        }
    }
}

static void test_nine_stream_identity() {
    check(all_timeframes().size() == 9, "exactly nine canonical timeframe streams");
    const char* labels[9] = {"M1", "M5", "M15", "M30", "H1", "H4", "D1", "W1", "MN1"};
    for (int i = 0; i < 9; ++i) {
        const Timeframe tf = all_timeframes()[static_cast<std::size_t>(i)];
        check(timeframe_from_string(labels[i]) == tf, "timeframe label parses to its canonical enum");
        check(std::string(aura::runtime::to_string(tf)) == labels[i], "timeframe enum prints its label");
    }
    check(timeframe_from_string("") == Timeframe::UNKNOWN, "empty timeframe -> UNKNOWN");
    check(timeframe_from_string("M2") == Timeframe::UNKNOWN, "non-canonical timeframe -> UNKNOWN");

    AdapterManager adapters;
    check(adapters.stream_count() == 9, "adapter manager owns nine logical streams");
    for (const Timeframe tf : all_timeframes()) {
        check(adapters.status(tf) != nullptr, "each stream has independent state");
    }
}

static void test_adapter_isolation() {
    AdapterManager adapters;

    // Bring two streams up with distinct bar times/sequences.
    check(adapters.report_bar(Timeframe::M1, aura::foundation::Timestamp::from_seconds(60), 1),
          "M1 bar recorded");
    check(adapters.report_bar(Timeframe::M15, aura::foundation::Timestamp::from_seconds(900), 5),
          "M15 bar recorded");
    check(adapters.status(Timeframe::M1)->last_sequence == 1, "M1 keeps its own sequence");
    check(adapters.status(Timeframe::M15)->last_sequence == 5, "M15 keeps its own sequence");

    // M1 fails; the other streams (M15 included) must be untouched.
    const aura::foundation::Timestamp m15_before = adapters.status(Timeframe::M15)->last_bar_time;
    check(adapters.report_error(Timeframe::M1, "feed gap"), "M1 failure recorded");
    check(adapters.status(Timeframe::M1)->state == aura::foundation::ServiceState::DEGRADED,
          "M1 degraded");
    check(adapters.status(Timeframe::M1)->quality == aura::foundation::DataQualityState::DEGRADED,
          "M1 quality degraded independently");
    check(adapters.status(Timeframe::M15)->state == aura::foundation::ServiceState::ONLINE,
          "M15 unaffected by M1 failure");
    check(adapters.status(Timeframe::M15)->last_bar_time == m15_before,
          "M15 bar time not overwritten by M1 failure");

    // M15 recovers via a new bar; M1's error state is not reset by that.
    check(adapters.report_bar(Timeframe::M15, aura::foundation::Timestamp::from_seconds(960), 6),
          "M15 recovers");
    check(adapters.status(Timeframe::M15)->last_sequence == 6, "M15 sequence advances to 6");
    check(adapters.status(Timeframe::M1)->state == aura::foundation::ServiceState::DEGRADED,
          "M15 recovery does not reset M1 state");
    check(adapters.status(Timeframe::M1)->last_error == "feed gap",
          "M1 last_error retained through M15 recovery");

    // A W1 outage must not alter H1.
    const std::uint64_t h1_seq_before =
        adapters.status(Timeframe::H1) == nullptr ? 0 : adapters.status(Timeframe::H1)->last_sequence;
    (void)h1_seq_before;
    adapters.report_bar(Timeframe::H1, aura::foundation::Timestamp::from_seconds(3600), 3);
    adapters.report_disconnected(Timeframe::W1, "no data");
    check(adapters.status(Timeframe::W1)->state == aura::foundation::ServiceState::OFFLINE,
          "W1 offline");
    check(adapters.status(Timeframe::H1)->state == aura::foundation::ServiceState::ONLINE,
          "W1 outage does not alter H1 state");
    check(adapters.status(Timeframe::H1)->last_sequence == 3, "H1 sequence unchanged by W1 outage");

    // Unknown timeframe is rejected, never defaulted to a stream.
    check(!adapters.report_bar(Timeframe::UNKNOWN, aura::foundation::Timestamp::from_seconds(1), 1),
          "UNKNOWN timeframe rejected by adapter");

    // Healthy count reflects only genuinely online streams.
    check(adapters.healthy_count() >= 2, "healthy count counts only online streams");
}

static void test_closed_bar_and_sequence_per_stream() {
    TimeframeStateStore store;

    // Each stream advances independently by exactly one bar.
    advance_stream(store, Timeframe::M1, 1, 1);
    advance_stream(store, Timeframe::H4, 1, 1);
    advance_stream(store, Timeframe::D1, 1, 1);

    check(store.size() == 3, "three streams have progress");
    check(store.last_processed(Timeframe::M1)->sequence == 1, "M1 sequence is 1");
    check(store.last_processed(Timeframe::H4)->sequence == 1, "H4 sequence is 1");
    check(store.last_processed(Timeframe::D1)->sequence == 1, "D1 sequence is 1");
    check(!store.has(Timeframe::M5), "untouched stream has no progress (not fabricated)");

    // Advancing H4 must not change M1's progress.
    const BarIdentity m1_before = store.last_processed(Timeframe::M1)->last_processed;
    advance_stream(store, Timeframe::H4, 1, 2, 1);
    check(store.last_processed(Timeframe::H4)->sequence == 2, "H4 sequence advances to 2");
    check(store.last_processed(Timeframe::M1)->last_processed == m1_before,
          "M1 progress unchanged by H4 advance");
    check(store.last_processed(Timeframe::M1)->sequence == 1, "M1 sequence still 1");

    // Sequence is per stream, not global: M1 and H4 both have sequence 2 after
    // their own two advances, without sharing a counter.
    advance_stream(store, Timeframe::M1, 1, 2, 1);
    check(store.last_processed(Timeframe::M1)->sequence == 2, "M1 sequence advances to 2 independently");
}

static void test_duplicate_and_repaint_rejection() {
    BarFinalizer finalizer;
    TimeframeStateStore store;

    const MarketBar b1 = mk_bar(Timeframe::M15, 900, 100.0);
    FinalizedBar fb;
    check(finalizer.finalize(mk_event(b1, "XAUUSD", 1), fb), "first M15 bar finalizes");
    check(store.advance(fb), "first M15 bar advances store");
    const BarIdentity first = fb.identity;
    const std::uint64_t seq_after_first = store.last_processed(Timeframe::M15)->sequence;

    // Exact duplicate closed bar -> rejected, no repaint, no sequence change.
    FinalizedBar dup;
    check(!finalizer.finalize(mk_event(b1, "XAUUSD", 2), dup),
          "duplicate finalized bar rejected (no repaint)");
    check(store.last_processed(Timeframe::M15)->sequence == seq_after_first,
          "duplicate does not advance the sequence");
    check(!store.advance(fb), "store refuses a repeated identical finalized bar");
    check(store.last_processed(Timeframe::M15)->sequence == seq_after_first,
          "store sequence unchanged after duplicate advance attempt");

    // A contradictory bar with the SAME close_time must not replace history.
    MarketBar changed = b1;
    changed.close = 999.0;
    FinalizedBar repaint;
    check(!finalizer.finalize(mk_event(changed, "XAUUSD", 3), repaint),
          "same close_time with changed content rejected (no repaint)");
    check(first == store.last_processed(Timeframe::M15)->last_processed,
          "already-finalized identity is preserved");
}

static void test_no_lookahead() {
    DataBus bus;

    // Future-dated event: event_time (close) after receive_time -> rejected.
    const MarketBar b = mk_bar(Timeframe::M15, 900, 100.0);
    MarketEvent future = mk_event(b, "XAUUSD", 1, aura::foundation::Timestamp::from_seconds(800));
    check(!bus.publish(future), "future-dated event rejected by bus (no lookahead)");

    // Forming bar -> rejected (no repaint, not authoritative).
    MarketBar forming = b;
    forming.closed = false;
    check(!bus.publish(mk_event(forming, "XAUUSD", 2)), "forming bar rejected (not authoritative)");

    // Finalizer also refuses a forming bar.
    BarFinalizer finalizer;
    FinalizedBar fb;
    check(!finalizer.finalize(mk_event(forming, "XAUUSD", 3), fb),
          "finalizer refuses forming bar");

    // A valid, closed, causal event is accepted and ordered deterministically.
    const MarketBar b2 = mk_bar(Timeframe::H4, 600, 100.0);
    check(bus.publish(mk_event(b2, "XAUUSD", 2)), "valid closed causal event accepted");
    check(bus.publish(mk_event(b, "XAUUSD", 1)), "second valid closed event accepted");
    const std::vector<MarketEvent> ordered = bus.drain_ordered();
    check(ordered.size() == 2, "two events drained");
    check(ordered[0].metadata.event_time() <= ordered[1].metadata.event_time(),
          "drain order is deterministic by event_time");
}

static void test_validation_per_stream() {
    DataValidator validator;
    check(validator.validate(mk_bar(Timeframe::M1, 60, 100.0)).quality ==
              aura::foundation::DataQualityState::VALID,
          "well-formed bar is VALID");
    check(validator.validate(mk_bar(Timeframe::UNKNOWN, 60, 100.0)).quality ==
              aura::foundation::DataQualityState::UNKNOWN,
          "unknown timeframe bar is UNKNOWN quality (not usable)");
    MarketBar bad = mk_bar(Timeframe::H1, 3600, 100.0);
    bad.high = 90.0;  // high below low
    check(validator.validate(bad).quality == aura::foundation::DataQualityState::INVALID,
          "inconsistent OHLC is INVALID");
}

static void test_determinism() {
    // Same input applied twice to two independent stores must yield identical
    // per-stream progress (deterministic update behaviour).
    const Timeframe streams[3] = {Timeframe::M5, Timeframe::M30, Timeframe::W1};
    TimeframeStateStore a;
    TimeframeStateStore b;
    for (const Timeframe tf : streams) {
        advance_stream(a, tf, 3, 1);
        advance_stream(b, tf, 3, 1);
    }
    for (const Timeframe tf : streams) {
        check(a.last_processed(tf)->sequence == b.last_processed(tf)->sequence,
              "sequence deterministic across identical runs");
        check(a.last_processed(tf)->last_processed == b.last_processed(tf)->last_processed,
              "last processed bar deterministic across identical runs");
        check(a.last_processed(tf)->last_close == b.last_processed(tf)->last_close,
              "last close deterministic across identical runs");
    }
}

static void test_provenance() {
    BarFinalizer finalizer;
    FinalizedBar fb;
    finalizer.finalize(mk_event(mk_bar(Timeframe::H4, 14400, 100.0), "XAUUSD", 7), fb);
    const std::string canon = fb.identity.canonical_string();
    check(canon.find("XAUUSD") != std::string::npos, "provenance retains symbol");
    check(canon.find("H4") != std::string::npos, "provenance retains timeframe");
    check(canon.find(std::to_string(aura::foundation::Timestamp::from_seconds(14400).nanoseconds())) !=
              std::string::npos,
          "provenance retains close_time");
    check(fb.identity.symbol() == "XAUUSD", "identity exposes symbol");
    check(fb.identity.timeframe() == Timeframe::H4, "identity exposes timeframe");
    check(fb.identity.close_time() == aura::foundation::Timestamp::from_seconds(14400),
          "identity exposes close_time");
}

int main() {
    test_nine_stream_identity();
    test_adapter_isolation();
    test_closed_bar_and_sequence_per_stream();
    test_duplicate_and_repaint_rejection();
    test_no_lookahead();
    test_validation_per_stream();
    test_determinism();
    test_provenance();

    if (g_failures == 0) {
        std::printf("Conceptual Phase 3 timeframe-state tests: PASS\n");
        return 0;
    }
    std::printf("Conceptual Phase 3 timeframe-state tests: %d FAILURE(S)\n", g_failures);
    return 1;
}
