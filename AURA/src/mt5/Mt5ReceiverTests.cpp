// MT5-0010 — MT5 one-EA / nine-stream receiver behavioural tests.
//
// Exercises the real C++ codec and receiver (no mocks): encode/decode round-trip,
// explicit-timeframe routing across all nine streams, malformed / missing-timeframe
// / wrong-timeframe / duplicate / future-dated rejection, per-stream isolation
// (failure and recovery), per-stream sequence and health, no-lookahead, and
// determinism. Exits non-zero on any failure.

#include "foundation/ServiceState.h"
#include "foundation/Timestamp.h"
#include "mt5/Mt5StreamManager.h"
#include "mt5/ProtocolCodec.h"
#include "runtime/AdapterManager.h"
#include "runtime/DataBus.h"

#include <cstdio>
#include <string>
#include <vector>

using aura::foundation::Timestamp;
using aura::mt5::DecodeStatus;
using aura::mt5::MessageType;
using aura::mt5::Mt5StreamManager;
using aura::mt5::ProtocolCodec;
using aura::mt5::ReceiveStatus;
using aura::runtime::MarketBar;
using aura::runtime::Timeframe;
using aura::runtime::all_timeframes;

static int g_failures = 0;
static void check(bool condition, const char* what) {
    if (!condition) {
        std::printf("FAIL: %s\n", what);
        ++g_failures;
    }
}

static MarketBar bar(Timeframe tf, std::uint64_t close_secs, double base) {
    MarketBar b;
    b.timeframe = tf;
    b.open_time = Timestamp::from_seconds(static_cast<std::int64_t>(close_secs) - 60);
    b.close_time = Timestamp::from_seconds(static_cast<std::int64_t>(close_secs));
    b.open = base;
    b.high = base + 1.0;
    b.low = base - 0.5;
    b.close = base + 0.8;
    b.volume = 100.0;
    b.closed = true;
    return b;
}

static std::string frame_for(const MarketBar& b, const std::string& symbol, std::uint64_t seq,
                             Timestamp receive) {
    return ProtocolCodec::encode_closed_bar(b, symbol, seq, receive);
}

static void test_encode_decode_roundtrip() {
    const MarketBar b = bar(Timeframe::M15, 900, 100.0);
    const std::string frame = frame_for(b, "XAUUSD", 7, Timestamp::from_seconds(900));
    const auto decoded = ProtocolCodec::decode(frame);
    check(decoded.ok(), "round-trip decodes OK");
    check(decoded.type == MessageType::BAR_CLOSED, "type preserved");
    check(decoded.timeframe == Timeframe::M15, "timeframe preserved");
    check(decoded.symbol == "XAUUSD", "symbol preserved");
    check(decoded.sequence == 7, "sequence preserved");
    check(decoded.event_time == b.close_time, "event_time preserved");
    check(decoded.receive_time == b.close_time, "receive_time preserved");
    check(decoded.bar.closed, "closed flag preserved");
    check(decoded.bar.close == b.close, "close price preserved");

    // Deterministic encoding: identical inputs -> byte-identical frames.
    const std::string again = frame_for(b, "XAUUSD", 7, Timestamp::from_seconds(900));
    check(frame == again, "encoding is deterministic");
    check(ProtocolCodec::make_message_id(MessageType::BAR_CLOSED, "XAUUSD", Timeframe::M15, 7,
                                         b.close_time) ==
              ProtocolCodec::make_message_id(MessageType::BAR_CLOSED, "XAUUSD", Timeframe::M15, 7,
                                             b.close_time),
          "message identity deterministic");
}

static void test_nine_stream_routing() {
    Mt5StreamManager manager;
    check(manager.stream_count() == 9, "receiver owns nine streams");

    // Send one closed bar per stream; each must land on its own explicit stream.
    std::uint64_t seq = 1;
    for (const Timeframe tf : all_timeframes()) {
        const std::string frame = frame_for(bar(tf, 6000 + seq * 60, 100.0 + seq), "XAUUSD", seq,
                                            Timestamp::from_seconds(6000 + seq * 60));
        const auto result = manager.accept_raw(frame);
        check(result.accepted(), "bar accepted");
        check(result.timeframe == tf, "frame routed to its explicit timeframe");
        const auto* state = manager.stream(tf);
        check(state != nullptr && state->accepted_bars == 1, "stream received exactly one bar");
        check(state->last_sequence == seq, "stream keeps its own sequence");
        check(state->healthy(), "stream online after a closed bar");
        ++seq;
    }
    check(manager.healthy_count() == 9, "all nine streams healthy");
    check(manager.store().size() == 9, "all nine streams have progress");
}

static void test_malformed_and_wrong_timeframe() {
    Mt5StreamManager manager;

    check(!ProtocolCodec::decode("garbage").ok(), "garbage frame rejected");
    check(ProtocolCodec::decode("garbage").status == DecodeStatus::MALFORMED,
          "garbage classified MALFORMED");
    check(!ProtocolCodec::decode("").ok(), "empty frame rejected");

    // Missing / unknown timeframe label is rejected, never defaulted.
    const MarketBar b = bar(Timeframe::M15, 900, 100.0);
    std::string frame = frame_for(b, "XAUUSD", 1, Timestamp::from_seconds(900));
    // Corrupt the timeframe field (field index 5) to an unknown label and fix the
    // checksum so only the timeframe check can fail.
    {
        std::vector<std::string> fields;
        std::string cur;
        for (const char c : frame) {
            if (c == '|') { fields.push_back(cur); cur.clear(); }
            else cur.push_back(c);
        }
        fields.push_back(cur);
        fields[5] = "M2";  // non-canonical
        std::string canon;
        for (std::size_t i = 0; i + 1 < fields.size(); ++i) {
            if (i) canon += "|";
            canon += fields[i];
        }
        frame = canon + "|" + ProtocolCodec::checksum_of(canon);
    }
    const auto unknown_tf = ProtocolCodec::decode(frame);
    check(unknown_tf.status == DecodeStatus::UNKNOWN_TIMEFRAME,
          "non-canonical timeframe rejected as UNKNOWN_TIMEFRAME");
    check(manager.accept_raw(frame).status == ReceiveStatus::DECODE_FAILED,
          "receiver refuses missing-timeframe frame");

    // Wrong symbol is not rejected by the codec (symbol is data), but a valid
    // frame for one symbol must not corrupt another stream's routing.
    const auto wrong_symbol = manager.accept_raw(frame_for(b, "EURUSD", 1, Timestamp::from_seconds(900)));
    check(wrong_symbol.accepted(), "valid frame with another symbol still routes by timeframe");
    check(manager.store().last_processed(Timeframe::M15) != nullptr, "M15 progress recorded");
}

static void test_checksum_and_future_rejection() {
    const MarketBar b = bar(Timeframe::H1, 3600, 100.0);
    std::string frame = frame_for(b, "XAUUSD", 1, Timestamp::from_seconds(3600));

    // Tamper with the payload -> checksum mismatch.
    std::string tampered = frame;
    tampered.replace(tampered.find("100.00000000"), 12, "999.00000000");
    check(ProtocolCodec::decode(tampered).status == DecodeStatus::CHECKSUM_MISMATCH,
          "tampered payload detected by checksum");

    // Future-dated: event_time after receive_time -> rejected (no lookahead).
    const std::string future =
        ProtocolCodec::encode(MessageType::BAR_CLOSED, "XAUUSD", Timeframe::H1, 1, b.close_time,
                              Timestamp::from_seconds(3000), ProtocolCodec::encode_bar_payload(b));
    check(ProtocolCodec::decode(future).status == DecodeStatus::FUTURE_DATED,
          "future-dated frame rejected (no lookahead)");
}

static void test_duplicate_and_regression() {
    Mt5StreamManager manager;
    const MarketBar b1 = bar(Timeframe::M5, 300, 100.0);
    const std::string f1 = frame_for(b1, "XAUUSD", 1, Timestamp::from_seconds(300));
    check(manager.accept_raw(f1).accepted(), "first M5 bar accepted");

    // Exact duplicate (same close_time, same sequence) -> rejected, no repaint.
    check(manager.accept_raw(f1).status == ReceiveStatus::DUPLICATE_SEQUENCE,
          "duplicate sequence rejected");
    check(manager.stream(Timeframe::M5)->accepted_bars == 1, "duplicate not counted");

    // Same close_time with a different sequence -> regression rejected by the
    // finalizer (bar not strictly later).
    const std::string f2 = frame_for(b1, "XAUUSD", 2, Timestamp::from_seconds(300));
    check(manager.accept_raw(f2).status == ReceiveStatus::DUPLICATE_OR_REGRESSION,
          "non-advancing closed bar rejected (no repaint)");
    check(manager.stream(Timeframe::M5)->accepted_bars == 1, "regression not counted");
    check(manager.store().last_processed(Timeframe::M5)->sequence == 1,
          "store sequence unchanged after duplicate/regression");

    // A strictly later bar with a new sequence is accepted.
    const std::string f3 = frame_for(bar(Timeframe::M5, 360, 101.0), "XAUUSD", 3,
                                     Timestamp::from_seconds(360));
    check(manager.accept_raw(f3).accepted(), "next closed bar accepted");
    check(manager.stream(Timeframe::M5)->accepted_bars == 2, "accepted count advances");
}

static void test_stream_isolation() {
    Mt5StreamManager manager;

    // Bring M1 and H4 up.
    check(manager.accept_raw(frame_for(bar(Timeframe::M1, 60, 100.0), "XAUUSD", 1,
                                       Timestamp::from_seconds(60)))
              .accepted(),
          "M1 up");
    check(manager.accept_raw(frame_for(bar(Timeframe::H4, 14400, 100.0), "XAUUSD", 1,
                                       Timestamp::from_seconds(14400)))
              .accepted(),
          "H4 up");
    const auto h4_time_before = manager.stream(Timeframe::H4)->last_event_time;

    // M1 fails; H4 must be unchanged.
    manager.report_stream_failure(Timeframe::M1, "gap");
    check(manager.stream(Timeframe::M1)->state == aura::foundation::ServiceState::DEGRADED,
          "M1 degraded");
    check(manager.stream(Timeframe::H4)->state == aura::foundation::ServiceState::ONLINE,
          "H4 unaffected by M1 failure");
    check(manager.stream(Timeframe::H4)->last_event_time == h4_time_before,
          "H4 event time unchanged");

    // M1 recovers; H4 must not reset.
    check(manager.accept_raw(frame_for(bar(Timeframe::M1, 120, 101.0), "XAUUSD", 2,
                                       Timestamp::from_seconds(120)))
              .accepted(),
          "M1 recovers with a later bar");
    check(manager.stream(Timeframe::M1)->state == aura::foundation::ServiceState::ONLINE,
          "M1 online after recovery");
    check(manager.stream(Timeframe::H4)->state == aura::foundation::ServiceState::ONLINE &&
              manager.stream(Timeframe::H4)->last_sequence == 1,
          "H4 not reset by M1 recovery");

    // A W1 outage must not silently alter H1.
    check(manager.accept_raw(frame_for(bar(Timeframe::H1, 3600, 100.0), "XAUUSD", 1,
                                       Timestamp::from_seconds(3600)))
              .accepted(),
          "H1 up");
    manager.report_disconnected(Timeframe::W1, "no data");
    check(manager.stream(Timeframe::W1)->state == aura::foundation::ServiceState::OFFLINE, "W1 offline");
    check(manager.stream(Timeframe::H1)->state == aura::foundation::ServiceState::ONLINE,
          "H1 unchanged by W1 outage");
}

static void test_determinism() {
    // The same frame stream applied to two managers yields identical per-stream
    // progress (deterministic, transport-arrival-order independent).
    const Timeframe streams[9] = {Timeframe::M1, Timeframe::M5, Timeframe::M15, Timeframe::M30,
                                  Timeframe::H1, Timeframe::H4, Timeframe::D1,  Timeframe::W1,
                                  Timeframe::MN1};
    std::vector<std::string> frames;
    for (const Timeframe tf : streams) {
        for (std::uint64_t i = 1; i <= 3; ++i) {
            const std::uint64_t close = 10000 + i * 60;
            frames.push_back(frame_for(bar(tf, close, 100.0 + static_cast<double>(i)), "XAUUSD", i,
                                       Timestamp::from_seconds(static_cast<std::int64_t>(close))));
        }
    }

    Mt5StreamManager a;
    Mt5StreamManager b;
    for (const std::string& f : frames) a.accept_raw(f);
    for (const std::string& f : frames) b.accept_raw(f);

    for (const Timeframe tf : streams) {
        check(a.stream(tf)->last_sequence == b.stream(tf)->last_sequence,
              "per-stream sequence deterministic");
        check(a.stream(tf)->accepted_bars == b.stream(tf)->accepted_bars,
              "per-stream accepted count deterministic");
        check(a.store().last_processed(tf)->last_processed == b.store().last_processed(tf)->last_processed,
              "per-stream last processed bar deterministic");
    }
}

int main() {
    test_encode_decode_roundtrip();
    test_nine_stream_routing();
    test_malformed_and_wrong_timeframe();
    test_checksum_and_future_rejection();
    test_duplicate_and_regression();
    test_stream_isolation();
    test_determinism();

    if (g_failures == 0) {
        std::printf("MT5 one-EA / nine-stream receiver tests: PASS\n");
        return 0;
    }
    std::printf("MT5 one-EA / nine-stream receiver tests: %d FAILURE(S)\n", g_failures);
    return 1;
}
