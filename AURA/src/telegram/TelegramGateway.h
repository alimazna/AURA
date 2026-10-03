#ifndef AURA_TELEGRAM_TELEGRAMGATEWAY_H
#define AURA_TELEGRAM_TELEGRAMGATEWAY_H

#include "telegram/TelegramConfig.h"
#include "telegram/TelegramMessage.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace aura {
namespace telegram {

// Outbound message sink. Implemented by the caller's transport (a real bot or a
// test double). The gateway depends only on this interface, never on a concrete
// network library, so it can be tested with a deterministic fake.
class IMessageSink {
public:
    virtual ~IMessageSink() = default;
    // Returns true when the message was accepted by the transport. Must not throw.
    virtual bool send(const TelegramMessage& message) noexcept = 0;
};

// Enqueue result.
enum class SendResult { DISABLED, NO_MESSAGE, ENQUEUED };

// Telegram gateway (V3-38 / section 28.3).
//
// Phase 10 auxiliary Telegram. When the channel is unconfigured/disabled the
// gateway is a safe no-op and the core is unaffected (Telegram outage is
// non-fatal). It only ever emits informational notifications; it never carries a
// command into the system and never becomes source of truth. Message emission is
// deterministic given the sink. No network/file I/O here.
class TelegramGateway {
public:
    // Constructs with an optional sink. A null sink means notifications are dropped
    // safely (never an error for the core).
    explicit TelegramGateway(TelegramConfig config, IMessageSink* sink = nullptr)
        : config_(std::move(config)), sink_(sink) {}

    const TelegramConfig& config() const noexcept { return config_; }

    // True when the gateway can actually deliver (configured and a sink exists).
    bool operational() const noexcept { return config_.configured() && sink_ != nullptr; }

    // Sends an informational message. A disabled/unconfigured gateway is a no-op.
    // Non-informational classes are refused (defense in depth) and never delivered.
    SendResult notify(const TelegramMessage& message) {
        if (!config_.configured()) return SendResult::DISABLED;
        if (!message.valid()) return SendResult::NO_MESSAGE;
        if (!is_informational(message.message_class)) return SendResult::NO_MESSAGE;
        if (sink_ == nullptr) return SendResult::NO_MESSAGE;
        if (!sink_->send(message)) return SendResult::NO_MESSAGE;
        ++enqueued_;
        return SendResult::ENQUEUED;
    }

    std::size_t enqueued_count() const noexcept { return enqueued_; }

    // The gateway must never be required for core correctness.
    static bool is_core_dependency() noexcept { return false; }

private:
    TelegramConfig config_{};
    IMessageSink* sink_{nullptr};
    std::size_t enqueued_{0};
};

}  // namespace telegram
}  // namespace aura

#endif  // AURA_TELEGRAM_TELEGRAMGATEWAY_H
