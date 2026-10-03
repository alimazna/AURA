// AURA host executable (APP-0003).
//
// This is a real application entrypoint: it constructs one ApplicationShell (the
// one-EA/nine-stream transport + the full runtime pipeline) and either
//
//   aura --replay <file>   read newline-delimited canonical frames from a file and
//                          run the whole pipeline deterministically, then print a
//                          summary and exit (offline / CI / evidence mode), or
//   aura --serve <port>    bind and serve the single MT5 transport connection until
//                          interrupted (operational / shadow mode).
//
// It places no live order and reads no clock in the pipeline; shadow mode is the
// only execution path. This is intentionally a console host: the rich desktop GUI
// is a Windows deliverable that remains BLOCKED pending a Windows toolchain (see
// project-control/BLOCKED.md). It never claims profitability or production safety.

#include "runtime/ApplicationShell.h"

#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

namespace {

volatile std::sig_atomic_t g_stop = 0;

extern "C" void on_signal(int) { g_stop = 1; }

int usage() {
    std::fprintf(stderr,
                 "usage:\n"
                 "  aura --replay <frames-file>   run the pipeline over recorded frames\n"
                 "  aura --serve  <port>          serve the MT5 transport until interrupted\n");
    return 2;
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
}

int replay(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        std::fprintf(stderr, "aura: cannot open %s\n", path.c_str());
        return 1;
    }
    aura::runtime::SocketRuntime sockets;
    aura::runtime::ApplicationShell shell;
    std::string line;
    std::size_t processed = 0;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        shell.feed(line + "\n");
        ++processed;
    }
    std::printf("aura: replayed %zu frames\n", processed);
    print_summary(shell);
    return 0;
}

int serve(std::uint16_t port) {
    aura::runtime::SocketRuntime sockets;
    if (!sockets.ok()) {
        std::fprintf(stderr, "aura: socket runtime failed to initialise\n");
        return 1;
    }
    aura::runtime::ApplicationShell shell;
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
    shell.request_stop();
    print_summary(shell);
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) return usage();
    const std::string mode = argv[1];
    if (mode == "--replay") return replay(argv[2]);
    if (mode == "--serve") return serve(static_cast<std::uint16_t>(std::atoi(argv[2])));
    return usage();
}
