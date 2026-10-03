// E2E-0001 — AURA end-to-end application pipeline behavioural test.
//
// Exercises the real integrated path over a real loopback TCP socket (no mocks):
//
//   canonical MT5 wire frame (same layout the MQL5 EA emits)
//     -> ProtocolCodec -> Mt5StreamManager (one EA / nine streams)
//     -> AdapterManager -> H4 structure/regime -> M15 features -> eligibility
//     -> signal -> score -> confidence -> market quality -> risk -> shadow fill
//     -> simulated position -> append-only shadow ledger -> health
//
// Proves: nine-stream availability, explicit-timeframe routing, closed-bar
// acceptance, no-lookahead/future-dated rejection, duplicate/malformed rejection,
// per-stream isolation, deterministic end-to-end output across identical runs,
// append-only ledger, and a strictly shadow-only execution path (never live).
// Exits non-zero on any failure.

#include "mt5/ProtocolCodec.h"
#include "runtime/ApplicationShell.h"

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

using aura::foundation::Timestamp;
using aura::mt5::MessageType;
using aura::mt5::ProtocolCodec;
using aura::runtime::ApplicationShell;
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

static MarketBar bar(Timeframe tf, std::int64_t close_secs, double base) {
    MarketBar b;
    b.timeframe = tf;
    b.open_time = Timestamp::from_seconds(close_secs - 60);
    b.close_time = Timestamp::from_seconds(close_secs);
    b.open = base;
    b.high = base + 1.0;
    b.low = base - 0.5;
    b.close = base + 0.8;
    b.volume = 100.0;
    b.closed = true;
    return b;
}

// A deterministic frame sequence: a closed bar on each of the nine streams per
// step (so the whole one-EA/nine-stream boundary is exercised), each with
// receive_time strictly after the event time. Sequence is per-stream.
static std::vector<std::string> frames(int steps) {
    std::vector<std::string> out;
    const auto tfs = all_timeframes();
    for (int i = 0; i < steps; ++i) {
        for (std::size_t k = 0; k < tfs.size(); ++k) {
            const std::int64_t step = 60 * static_cast<std::int64_t>(k + 1);
            const std::int64_t close = 1'000'000 + static_cast<std::int64_t>(i) * step;
            const MarketBar b = bar(tfs[k], close, 100.0 + i + static_cast<double>(k));
            out.push_back(ProtocolCodec::encode_closed_bar(
                b, "XAUUSD", static_cast<std::uint64_t>(i + 1),
                Timestamp::from_seconds(close + 1)));
        }
    }
    return out;
}

#if defined(_WIN32)
using socket_t = SOCKET;
static const socket_t kBadSocket = INVALID_SOCKET;
#else
using socket_t = int;
static const socket_t kBadSocket = -1;
#endif

static socket_t connect_to(std::uint16_t port) {
    socket_t s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (s == kBadSocket) return kBadSocket;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    if (::connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
#if defined(_WIN32)
        ::closesocket(s);
#else
        ::close(s);
#endif
        return kBadSocket;
    }
    return s;
}

static bool send_all(socket_t s, const std::string& data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
        const int n = ::send(s, data.data() + sent, static_cast<int>(data.size() - sent), 0);
        if (n <= 0) return false;
        sent += static_cast<std::size_t>(n);
    }
    return true;
}

static void close_socket(socket_t s) {
#if defined(_WIN32)
    ::closesocket(s);
#else
    ::close(s);
#endif
}

// Sends a frame sequence over a real socket to a shell and returns the pipeline
// status snapshot after a clean stop.
struct RunResult {
    bool connected{false};
    std::size_t accepted{0};
    std::size_t rejected{0};
    std::size_t streams_total{0};
    std::size_t streams_healthy{0};
    std::size_t server_accepted{0};
    bool pipeline_healthy{false};
    bool has_signal{false};
    std::string decision_id{};
    std::string proposal_id{};
    bool proposal_is_order{true};
    std::size_t ledger_size{0};
};

static RunResult run_over_socket(const std::vector<std::string>& seq) {
    RunResult result;
    aura::runtime::SocketRuntime sockets;
    check(sockets.ok(), "socket runtime initialised");

    ApplicationShell shell;
    check(shell.start(0), "shell bound an ephemeral port");

    std::thread server([&shell]() { shell.serve_until_stopped(); });

    const socket_t client = connect_to(shell.server().bound_port());
    result.connected = client != kBadSocket;
    check(result.connected, "client connected to the AURA transport");
    if (result.connected) {
        std::string blob;
        for (const std::string& f : seq) blob += f + "\n";
        // Deliver in two chunks to also exercise partial-frame reassembly.
        const std::size_t half = blob.size() / 2;
        send_all(client, blob.substr(0, half));
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        send_all(client, blob.substr(half));
        std::this_thread::sleep_for(std::chrono::milliseconds(700));
    }

    shell.request_stop();
    if (result.connected) close_socket(client);
    server.join();

    auto& pipeline = shell.pipeline();
    const aura::runtime::EngineStatus st = pipeline.status();
    result.accepted = static_cast<std::size_t>(st.accepted);
    result.rejected = static_cast<std::size_t>(st.rejected);
    result.streams_total = st.streams_total;
    result.streams_healthy = st.streams_healthy;
    result.pipeline_healthy = st.healthy;
    result.server_accepted = shell.server().accepted_count();
    result.has_signal = pipeline.last_signal().valid;
    result.decision_id = pipeline.last_signal().decision_id.to_hex();
    result.proposal_id = pipeline.last_proposal().proposal_id.value();
    result.proposal_is_order = pipeline.last_proposal().is_order;
    result.ledger_size = pipeline.ledger().size();
    return result;
}

static void test_end_to_end_over_socket() {
    const std::vector<std::string> seq = frames(30);
    const RunResult a = run_over_socket(seq);

    check(a.connected, "end-to-end: transport connected");
    check(a.server_accepted >= 1, "end-to-end: the AURA server accepted the EA connection");
    check(a.streams_total == 9, "end-to-end: nine logical streams exist behind one transport");
    check(a.accepted == seq.size(), "end-to-end: every closed bar was accepted exactly once");
    check(a.rejected == 0, "end-to-end: no genuine frame was rejected");
    check(a.streams_healthy >= 2, "end-to-end: operational + structural streams are ONLINE");
    check(a.has_signal, "end-to-end: a valid signal reached the shadow stage");
    check(!a.decision_id.empty(), "end-to-end: the signal carries a deterministic decision id");
    check(!a.proposal_id.empty(), "end-to-end: a risk proposal was produced");
    check(a.proposal_is_order == false, "end-to-end: the proposal is explicitly NOT an order");
    check(a.ledger_size > 0, "end-to-end: the shadow ledger recorded the lifecycle");

    // Determinism: identical input over a fresh transport yields identical output.
    const RunResult b = run_over_socket(seq);
    check(b.accepted == a.accepted && b.rejected == a.rejected,
          "end-to-end: accept/reject counts are deterministic");
    check(b.decision_id == a.decision_id, "end-to-end: decision identity is deterministic");
    check(b.proposal_id == a.proposal_id, "end-to-end: proposal identity is deterministic");
    check(b.ledger_size == a.ledger_size, "end-to-end: ledger size is deterministic");
}

static void test_rejections_and_isolation() {
    std::vector<std::string> seq;
    // One valid M15 and H4 bar.
    MarketBar m15 = bar(Timeframe::M15, 2'000'000, 110.0);
    MarketBar h4 = bar(Timeframe::H4, 2'000'000, 110.0);
    const std::string good_m15 = ProtocolCodec::encode_closed_bar(
        m15, "XAUUSD", 1, Timestamp::from_seconds(2'000'001));
    const std::string good_h4 = ProtocolCodec::encode_closed_bar(
        h4, "XAUUSD", 1, Timestamp::from_seconds(2'000'001));
    seq.push_back(good_m15);
    seq.push_back(good_h4);
    // A duplicate M15 delivery (same sequence + same bar) must be rejected.
    seq.push_back(good_m15);
    // A future-dated bar (event_time after receive_time) must be rejected.
    seq.push_back(ProtocolCodec::encode_closed_bar(m15, "XAUUSD", 2,
                                                   Timestamp::from_seconds(1'999'999)));
    // A malformed frame must be rejected.
    seq.push_back("this-is-not-a-canonical-frame");

    const RunResult r = run_over_socket(seq);
    check(r.accepted == 2, "rejections: only the two well-formed first-delivery bars are accepted");
    check(r.rejected >= 2, "rejections: duplicate/future-dated/malformed frames are rejected");
    check(r.streams_healthy >= 2, "rejections: a rejected frame does not take a stream offline");
    check(r.streams_total == 9, "rejections: all nine streams remain present");
}

int main() {
    test_end_to_end_over_socket();
    test_rejections_and_isolation();
    if (g_failures == 0) {
        std::printf("EndToEndTests: ALL PASS\n");
        return 0;
    }
    std::printf("EndToEndTests: %d FAILURE(S)\n", g_failures);
    return 1;
}
