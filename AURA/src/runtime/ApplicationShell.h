#ifndef AURA_RUNTIME_APPLICATIONSHELL_H
#define AURA_RUNTIME_APPLICATIONSHELL_H

#include "runtime/ApplicationPipeline.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace aura {
namespace runtime {

// RAII for the platform socket library (Winsock on Windows, no-op on POSIX).
class SocketRuntime {
public:
    SocketRuntime() {
#if defined(_WIN32)
        WSADATA data;
        ok_ = WSAStartup(MAKEWORD(2, 2), &data) == 0;
#else
        ok_ = true;
#endif
    }
    ~SocketRuntime() {
#if defined(_WIN32)
        if (ok_) WSACleanup();
#endif
    }
    SocketRuntime(const SocketRuntime&) = delete;
    SocketRuntime& operator=(const SocketRuntime&) = delete;
    bool ok() const noexcept { return ok_; }

private:
    bool ok_{false};
};

// A minimal blocking TCP server that accepts the single MT5 transport connection.
//
// APP-0002. Exactly one physical EA connects (the one-EA/nine-stream decision), so
// this server accepts one client at a time and delivers each newline-delimited
// canonical frame to the pipeline. It is deliberately dependency-free (POSIX
// sockets on Linux, Winsock on Windows), opens no listening port behind a
// configured one, and performs no trading action whatsoever. The caller supplies a
// stop flag so the accept/read loop returns on shutdown rather than blocking
// forever.
class TcpFrameServer {
public:
    TcpFrameServer() = default;
    ~TcpFrameServer() { close_all(); }
    TcpFrameServer(const TcpFrameServer&) = delete;
    TcpFrameServer& operator=(const TcpFrameServer&) = delete;

    // Binds and listens on the loopback address. Port 0 requests an ephemeral port
    // (useful for tests); bound_port() reports the actual port.
    bool listen_on(std::uint16_t port, const std::string& bind_address = "127.0.0.1") {
        close_all();
        socket_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (socket_ == kInvalid) return false;

        int yes = 1;
        ::setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR,
                     reinterpret_cast<const char*>(&yes), sizeof(yes));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        if (::inet_pton(AF_INET, bind_address.c_str(), &addr.sin_addr) != 1) {
            close_all();
            return false;
        }
        if (::bind(socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            close_all();
            return false;
        }
        if (::listen(socket_, 1) != 0) {
            close_all();
            return false;
        }
        sockaddr_in bound{};
        socklen_t len = sizeof(bound);
        if (::getsockname(socket_, reinterpret_cast<sockaddr*>(&bound), &len) == 0) {
            bound_port_ = ntohs(bound.sin_port);
        }
        return true;
    }

    std::uint16_t bound_port() const noexcept { return bound_port_; }
    bool listening() const noexcept { return socket_ != kInvalid; }

    // Accepts one client, waiting up to `timeout_ms` for a connection so the
    // caller's loop can observe a shutdown flag instead of blocking forever on a
    // silent transport. Returns true only when a client was accepted.
    bool accept_one(int timeout_ms = 250) {
        if (socket_ == kInvalid) return false;
        if (!wait_readable(socket_, timeout_ms)) return false;
        const int client = ::accept(socket_, nullptr, nullptr);
        if (client == kInvalid) return false;
        active_ = client;
        ++accepted_;
        return true;
    }

    std::size_t accepted_count() const noexcept { return accepted_; }
    bool client_connected() const noexcept { return active_ != kInvalid; }

    // Reads currently available bytes from the active connection.
    //
    // `would_block` is set to true when the read timed out with no data (the
    // non-fatal steady state), so the caller can continue its loop and can react to
    // a stop flag.
    std::string read_available(bool& would_block, int timeout_ms = 250) {
        would_block = false;
        std::string data;
        if (active_ == kInvalid) {
            would_block = true;
            return data;
        }
        set_recv_timeout(active_, timeout_ms);
        char buffer[4096];
        const int n = ::recv(active_, buffer, sizeof(buffer), 0);
        if (n > 0) {
            data.assign(buffer, static_cast<std::size_t>(n));
            return data;
        }
        if (n == 0) {
            disconnect_client();
            return data;
        }
        would_block = true;
        return data;
    }

    void disconnect_client() {
        if (active_ != kInvalid) {
            close_socket(active_);
            active_ = kInvalid;
        }
    }

    void close_all() {
        disconnect_client();
        if (socket_ != kInvalid) {
            close_socket(socket_);
            socket_ = kInvalid;
        }
    }

#if defined(_WIN32)
    static constexpr std::intptr_t kInvalid = static_cast<std::intptr_t>(INVALID_SOCKET);
#else
    static constexpr int kInvalid = -1;
#endif

private:
#if defined(_WIN32)
    using socket_native = SOCKET;
#else
    using socket_native = int;
#endif

    static void close_socket(std::intptr_t s) {
#if defined(_WIN32)
        ::closesocket(static_cast<SOCKET>(s));
#else
        ::close(static_cast<int>(s));
#endif
    }

    static void set_recv_timeout(std::intptr_t s, int timeout_ms) {
#if defined(_WIN32)
        DWORD tv = static_cast<DWORD>(timeout_ms);
        ::setsockopt(static_cast<SOCKET>(s), SOL_SOCKET, SO_RCVTIMEO,
                     reinterpret_cast<const char*>(&tv), sizeof(tv));
#else
        timeval tv{};
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        ::setsockopt(static_cast<int>(s), SOL_SOCKET, SO_RCVTIMEO,
                     reinterpret_cast<const char*>(&tv), sizeof(tv));
#endif
    }

    // Waits until a socket is readable (a pending connection or data) or the
    // timeout expires. Uses select on both platforms so accept() never blocks the
    // host loop indefinitely.
    static bool wait_readable(std::intptr_t s, int timeout_ms) {
        fd_set set;
        FD_ZERO(&set);
#if defined(_WIN32)
        const socket_native handle = static_cast<socket_native>(s);
        FD_SET(handle, &set);
#else
        const socket_native handle = static_cast<socket_native>(s);
        FD_SET(handle, &set);
#endif
        timeval tv{};
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        const int rc = ::select(static_cast<int>(s) + 1, &set, nullptr, nullptr, &tv);
        return rc > 0;
    }

    std::intptr_t socket_{kInvalid};
    std::intptr_t active_{kInvalid};
    std::uint16_t bound_port_{0};
    std::size_t accepted_{0};
};

// Host-driven application shell.
//
// APP-0002. Wires one TcpFrameServer to one ApplicationPipeline. The host process
// owns the loop (so the same code runs under a test harness, a Windows console
// host, or a service), and the shell splits the incoming byte stream into
// newline-delimited frames and feeds each whole frame to the pipeline. It never
// places an order, reads no clock, and never fabricates a bar. `stop` is a
// caller-owned flag set by a signal handler or the GUI to request a clean drain.
class ApplicationShell {
public:
    ApplicationShell() : ApplicationShell(ApplicationPipeline::Config{}) {}

    explicit ApplicationShell(ApplicationPipeline::Config config)
        : pipeline_(std::move(config)) {}

    ApplicationPipeline& pipeline() noexcept { return pipeline_; }
    TcpFrameServer& server() noexcept { return server_; }

    bool start(std::uint16_t port, const std::string& bind_address = "127.0.0.1") {
        return server_.listen_on(port, bind_address);
    }

    // Processes a chunk of bytes. Complete newline-delimited frames are delivered
    // to the pipeline in order; a trailing partial frame is retained until the rest
    // arrives. Returns the number of complete frames processed.
    std::size_t feed(std::string_view chunk) {
        pending_.append(chunk.data(), chunk.size());
        std::size_t processed = 0;
        std::size_t start = 0;
        for (;;) {
            const std::size_t nl = pending_.find('\n', start);
            if (nl == std::string::npos) break;
            std::string_view frame(pending_.data() + start, nl - start);
            if (!frame.empty() && frame.back() == '\r') frame.remove_suffix(1);
            if (!frame.empty()) {
                pipeline_.on_frame(frame);
                ++processed;
            }
            start = nl + 1;
        }
        pending_.erase(0, start);
        return processed;
    }

    std::size_t pending_bytes() const noexcept { return pending_.size(); }
    void request_stop() noexcept { stop_ = true; }
    bool stop_requested() const noexcept { return stop_; }

    // Drains the socket until the stop flag is set. Returns the number of frames
    // processed. The read uses a short timeout so the loop stays responsive to
    // `request_stop()` without an external thread.
    std::size_t serve_until_stopped() {
        std::size_t total = 0;
        while (!stop_) {
            total += serve_once();
        }
        return total;
    }

    // Runs one accept/read cycle with a short read timeout. Returns the number of
    // complete frames processed in this cycle. The host process owns the outer loop
    // so a console/GUI/host can also observe its own shutdown flag between cycles.
    std::size_t serve_once(int timeout_ms = 250) {
        if (!server_.client_connected()) {
            if (!server_.accept_one()) return 0;
        }
        bool would_block = false;
        const std::string data = server_.read_available(would_block, timeout_ms);
        if (data.empty()) return 0;
        return feed(data);
    }

private:
    ApplicationPipeline pipeline_;
    TcpFrameServer server_;
    std::string pending_{};
    bool stop_{false};
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_APPLICATIONSHELL_H
