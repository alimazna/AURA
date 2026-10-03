// AURA host executable (APP-0003 / PERSIST-0001).
//
// Real application entrypoint. It constructs one ApplicationShell (the
// one-EA/nine-stream transport + the full runtime pipeline + the file-backed
// persistence/recovery coordinator) and runs one of:
//
//   aura --replay <file> [--store <path>]   run the pipeline deterministically
//                                           over recorded canonical frames, persist
//                                           derived state, print a summary, exit.
//   aura --serve  <port> [--store <path>]   serve the single MT5 transport until
//                                           interrupted, then persist and exit.
//   aura --self-test [--store <path>]       bounded offline smoke: feed a built-in
//                                           deterministic frame set, persist, verify
//                                           recovery, print, exit. Used by CI.
//   aura --recover <path>                   load + verify a persisted store and
//                                           report the V2-36 boot decision only.
//
// It places no live order, reads no clock in the pipeline, and its only execution
// path is shadow simulation. The rich desktop GUI is a Windows deliverable still
// recorded as blocked (see project-control/BLOCKED.md). This never claims
// profitability or production safety.

#include "mt5/ProtocolCodec.h"
#include "runtime/ApplicationShell.h"

#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

namespace {

volatile std::sig_atomic_t g_stop = 0;

extern "C" void on_signal(int) { g_stop = 1; }

int usage() {
    std::fprintf(stderr,
                 "usage:\n"
                 "  aura --replay <frames-file> [--store <path>]\n"
                 "  aura --serve  <port>        [--store <path>]\n"
                 "  aura --self-test            [--store <path>]\n"
                 "  aura --recover <store-path>\n");
    return 2;
}

// Extracts "--store <path>" (or the empty string when absent) from argv.
std::string store_arg(int argc, char** argv) {
    for (int i = 2; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--store") return argv[i + 1];
    }
    return {};
}

void print_summary(aura::runtime::ApplicationShell& shell) {
    const aura::runtime::EngineStatus st = shell.pipeline().status();
    std::printf("AURA shadow run summary\n");
    std::printf("  transport connected : %s\n", st.transport_connected ? "yes" : "no");
    std::printf("  streams healthy     : %zu / %zu\n", st.streams_healthy, st.streams_total);
    std::printf("  bars accepted       : %llu\n", static_cast<unsigned long long>(st.accepted));
    std::printf("  frames rejected     : %llu\n", static_cast<unsigned long long>(st.rejected));
    std::printf("  malformed           : %llu\n", static_cast<unsigned long long>(st.malformed));
    std::printf("  signals             : %llu\n", static_cast<unsigned long long>(st.signals));
    std::printf("  risk proposals      : %llu\n", static_cast<unsigned long long>(st.proposals));
    std::printf("  shadow fills        : %llu\n", static_cast<unsigned long long>(st.fills));
    std::printf("  ledger entries      : %zu\n", shell.pipeline().ledger().size());
    std::printf("  health              : %s (%s)\n", st.healthy ? "HEALTHY" : "NOT HEALTHY",
                std::string(aura::foundation::to_string(st.aggregate)).c_str());
    std::printf("  execution mode      : SHADOW ONLY (no live order path)\n");

    const aura::runtime::Signal& signal = shell.pipeline().last_signal();
    if (signal.valid) {
        std::printf("  last decision id    : %s\n", signal.decision_id.to_hex().c_str());
    }
    std::printf("  last proposal is order: %s\n",
                shell.pipeline().last_proposal().is_order ? "yes (unexpected)" : "no");
    if (!shell.store().path().empty()) {
        std::printf("  persisted store     : %s (%zu records)\n", shell.store().path().c_str(),
                    shell.store().size());
    }
}

// A deterministic built-in frame set: one closed bar on each of the nine streams
// per step. Used by --self-test so CI exercises the full path without a socket.
std::vector<std::string> builtin_frames(int steps) {
    std::vector<std::string> out;
    const auto tfs = aura::runtime::all_timeframes();
    for (int i = 0; i < steps; ++i) {
        for (std::size_t k = 0; k < tfs.size(); ++k) {
            const std::int64_t step = 60 * static_cast<std::int64_t>(k + 1);
            const std::int64_t close = 1'000'000 + static_cast<std::int64_t>(i) * step;
            aura::runtime::MarketBar b;
            b.timeframe = tfs[k];
            b.open_time = aura::foundation::Timestamp::from_seconds(close - 60);
            b.close_time = aura::foundation::Timestamp::from_seconds(close);
            b.open = 100.0 + i + static_cast<double>(k);
            b.high = b.open + 1.0;
            b.low = b.open - 0.5;
            b.close = b.open + 0.8;
            b.volume = 100.0;
            b.closed = true;
            out.push_back(aura::mt5::ProtocolCodec::encode_closed_bar(
                b, "XAUUSD", static_cast<std::uint64_t>(i + 1),
                aura::foundation::Timestamp::from_seconds(close + 1)));
        }
    }
    return out;
}

int replay(const std::string& path, const std::string& store_path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        std::fprintf(stderr, "aura: cannot open %s\n", path.c_str());
        return 1;
    }
    aura::runtime::SocketRuntime sockets;
    aura::runtime::ApplicationShell shell(aura::runtime::ApplicationPipeline::Config{},
                                          store_path);
    std::string line;
    std::size_t processed = 0;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        shell.feed(line + "\n");
        ++processed;
    }
    std::printf("aura: replayed %zu frames\n", processed);
    if (!store_path.empty()) {
        const aura::foundation::PersistenceStatus status = shell.persist_state();
        std::printf("aura: persist state -> %s\n",
                    std::string(aura::foundation::to_string(status)).c_str());
    }
    print_summary(shell);
    return 0;
}

int run_self_test(const std::string& store_path) {
    const std::string path = store_path.empty() ? std::string("aura_selftest.aura") : store_path;
    std::printf("aura: self-test starting (store=%s)\n", path.c_str());

    // Run 1: feed a deterministic frame set and persist.
    aura::runtime::SocketRuntime sockets;
    {
        aura::runtime::ApplicationShell shell(aura::runtime::ApplicationPipeline::Config{}, path);
        for (const std::string& f : builtin_frames(30)) shell.feed(f + "\n");
        const aura::foundation::PersistenceStatus status = shell.persist_state();
        std::printf("aura: self-test persist -> %s\n",
                    std::string(aura::foundation::to_string(status)).c_str());
        print_summary(shell);
        if (status != aura::foundation::PersistenceStatus::OK) {
            std::printf("aura: SELF-TEST FAIL (persist)\n");
            return 1;
        }
    }

    // Run 2: fresh process object restores the persisted state and recovers.
    {
        aura::runtime::ApplicationShell shell(aura::runtime::ApplicationPipeline::Config{}, path);
        const aura::runtime::RecoveryOutcome outcome = shell.recover(false);
        std::printf("aura: self-test recovery  -> %s (resumable=%s) : %s\n",
                    std::string(aura::runtime::to_string(outcome.detected)).c_str(),
                    outcome.resumable ? "yes" : "no", outcome.reason.c_str());
        if (!outcome.resumable) {
            std::printf("aura: SELF-TEST FAIL (recovery not resumable)\n");
            return 1;
        }
        if (!shell.apply_resume()) {
            std::printf("aura: SELF-TEST FAIL (resume restore)\n");
            return 1;
        }
        const std::size_t restored = shell.pipeline().state_store().size();
        const std::size_t ledger = shell.pipeline().ledger().size();
        std::printf("aura: self-test restored  -> %zu timeframes, %zu ledger entries\n", restored,
                    ledger);
        if (restored != 9 || ledger == 0) {
            std::printf("aura: SELF-TEST FAIL (restored counts)\n");
            return 1;
        }
    }

    std::printf("aura: SELF-TEST PASS\n");
    std::remove(path.c_str());
    return 0;
}

int run_recover(const std::string& store_path) {
    aura::runtime::ApplicationShell shell(aura::runtime::ApplicationPipeline::Config{}, store_path);
    const aura::runtime::RecoveryOutcome outcome = shell.recover(false);
    std::printf("aura: recover %s\n", store_path.c_str());
    std::printf("  lifecycle : %s\n",
                std::string(aura::runtime::to_string(outcome.detected)).c_str());
    std::printf("  resumable : %s\n", outcome.resumable ? "yes" : "no");
    std::printf("  fresh     : %s\n", outcome.fresh_start ? "yes" : "no");
    std::printf("  known-good: %s\n", outcome.used_known_good ? "yes" : "no");
    std::printf("  reason    : %s\n", outcome.reason.c_str());
    return outcome.resumable || outcome.fresh_start ? 0 : 1;
}

int serve(std::uint16_t port, const std::string& store_path) {
    aura::runtime::SocketRuntime sockets;
    if (!sockets.ok()) {
        std::fprintf(stderr, "aura: socket runtime failed to initialise\n");
        return 1;
    }
    aura::runtime::ApplicationShell shell(aura::runtime::ApplicationPipeline::Config{}, store_path);
    if (!shell.start(port)) {
        std::fprintf(stderr, "aura: failed to bind port %u\n", static_cast<unsigned>(port));
        return 1;
    }
    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);
    std::printf("aura: listening on 127.0.0.1:%u (shadow mode). Ctrl-C to stop.\n",
                static_cast<unsigned>(port));
    while (!g_stop) {
        shell.serve_once();
    }
    if (!store_path.empty()) {
        const aura::foundation::PersistenceStatus status = shell.persist_state();
        std::printf("aura: persist state -> %s\n",
                    std::string(aura::foundation::to_string(status)).c_str());
    }
    print_summary(shell);
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) return usage();
    const std::string mode = argv[1];
    if (mode == "--replay") {
        if (argc < 3) return usage();
        return replay(argv[2], store_arg(argc, argv));
    }
    if (mode == "--serve") {
        if (argc < 3) return usage();
        return serve(static_cast<std::uint16_t>(std::atoi(argv[2])), store_arg(argc, argv));
    }
    if (mode == "--self-test") return run_self_test(store_arg(argc, argv));
    if (mode == "--recover") {
        if (argc < 3) return usage();
        return run_recover(argv[2]);
    }
    return usage();
}
