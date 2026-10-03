#ifndef AURA_MT5_PROTOCOLCODEC_H
#define AURA_MT5_PROTOCOLCODEC_H

#include "foundation/EntityId.h"
#include "foundation/EventId.h"
#include "foundation/EventMetadata.h"
#include "foundation/EventType.h"
#include "foundation/HashAlgorithm.h"
#include "foundation/HashDigest.h"
#include "foundation/IHasher.h"
#include "foundation/MessageMetadata.h"
#include "foundation/ProtocolVersion.h"
#include "foundation/SchemaVersion.h"
#include "foundation/Timestamp.h"
#include "runtime/AdapterManager.h"
#include "runtime/DataBus.h"

#include <charconv>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace aura {
namespace mt5 {

// Message classes carried by the single transport (master data-bus classes mapped
// onto the MT5 boundary). For stream messages the timeframe is explicit in the
// frame; identity is never positional.
enum class MessageType : std::uint8_t {
    UNKNOWN = 0,
    BAR_CLOSED,
    QUOTE,
    SYMBOL_SPEC,
    HEARTBEAT,
    HEALTH,
    ERROR,
};

constexpr std::string_view to_string(MessageType type) noexcept {
    switch (type) {
        case MessageType::UNKNOWN:     return "UNKNOWN";
        case MessageType::BAR_CLOSED:  return "BAR_CLOSED";
        case MessageType::QUOTE:       return "QUOTE";
        case MessageType::SYMBOL_SPEC: return "SYMBOL_SPEC";
        case MessageType::HEARTBEAT:   return "HEARTBEAT";
        case MessageType::HEALTH:      return "HEALTH";
        case MessageType::ERROR:       return "ERROR";
    }
    return "UNKNOWN";
}

inline MessageType message_type_from_string(std::string_view text) noexcept {
    const MessageType all[] = {MessageType::BAR_CLOSED,  MessageType::QUOTE,
                               MessageType::SYMBOL_SPEC, MessageType::HEARTBEAT,
                               MessageType::HEALTH,     MessageType::ERROR};
    for (const MessageType t : all) {
        if (to_string(t) == text) return t;
    }
    return MessageType::UNKNOWN;
}

// Decode outcome. Every failure mode is explicit; a malformed, unknown or
// future-dated frame is never silently accepted (V3-23).
enum class DecodeStatus : std::uint8_t {
    OK = 0,
    MALFORMED,
    UNSUPPORTED_PROTOCOL,
    UNSUPPORTED_SCHEMA,
    UNKNOWN_MESSAGE_TYPE,
    UNKNOWN_TIMEFRAME,
    CHECKSUM_MISMATCH,
    FUTURE_DATED,
};

constexpr std::string_view to_string(DecodeStatus status) noexcept {
    switch (status) {
        case DecodeStatus::OK:                   return "OK";
        case DecodeStatus::MALFORMED:            return "MALFORMED";
        case DecodeStatus::UNSUPPORTED_PROTOCOL: return "UNSUPPORTED_PROTOCOL";
        case DecodeStatus::UNSUPPORTED_SCHEMA:   return "UNSUPPORTED_SCHEMA";
        case DecodeStatus::UNKNOWN_MESSAGE_TYPE: return "UNKNOWN_MESSAGE_TYPE";
        case DecodeStatus::UNKNOWN_TIMEFRAME:    return "UNKNOWN_TIMEFRAME";
        case DecodeStatus::CHECKSUM_MISMATCH:    return "CHECKSUM_MISMATCH";
        case DecodeStatus::FUTURE_DATED:         return "FUTURE_DATED";
    }
    return "MALFORMED";
}

// A decoded stream frame ready to feed the C++ receiver.
struct DecodedFrame {
    DecodeStatus status{DecodeStatus::MALFORMED};
    MessageType type{MessageType::UNKNOWN};
    runtime::Timeframe timeframe{runtime::Timeframe::UNKNOWN};
    std::string symbol{};
    std::uint64_t sequence{0};
    foundation::Timestamp event_time{};
    foundation::Timestamp receive_time{};
    std::string payload{};
    runtime::MarketBar bar{};
    foundation::MessageMetadata message{};
    foundation::EventMetadata event{};

    bool ok() const noexcept { return status == DecodeStatus::OK; }
};

// Deterministic wire codec for the MT5 one-EA transport.
//
// MT5-0008. Canonical frame layout (fields joined by '|'):
//
//   protocol|schema|message_id|type|symbol|timeframe|sequence|event_ns|recv_ns|payload|checksum
//
// `checksum` is the lowercase SHA-256 hex of every byte preceding the final '|'.
// The codec is pure and deterministic: identical inputs produce byte-identical
// frames; no clock, no I/O, no locale dependence (numbers use fixed 8-decimal text
// parsed with std::from_chars). A timeframe is always explicit, so a bar can never
// be attributed to another stream by position.
class ProtocolCodec {
public:
    static constexpr std::string_view kProtocolVersion = "1.0.0";
    static constexpr std::string_view kSchemaVersion = "1.0.0";

    static std::string encode(MessageType type, const std::string& symbol,
                              runtime::Timeframe timeframe, std::uint64_t sequence,
                              foundation::Timestamp event_time, foundation::Timestamp receive_time,
                              const std::string& payload) {
        const std::string message_id =
            make_message_id(type, symbol, timeframe, sequence, event_time);
        const std::string canon = std::string(kProtocolVersion) + "|" + std::string(kSchemaVersion) +
                                  "|" + message_id + "|" + std::string(to_string(type)) + "|" +
                                  symbol + "|" + std::string(runtime::to_string(timeframe)) + "|" +
                                  std::to_string(sequence) + "|" +
                                  std::to_string(event_time.nanoseconds()) + "|" +
                                  std::to_string(receive_time.nanoseconds()) + "|" + payload;
        return canon + "|" + checksum_of(canon);
    }

    static std::string encode_closed_bar(const runtime::MarketBar& bar, const std::string& symbol,
                                         std::uint64_t sequence, foundation::Timestamp receive_time) {
        return encode(MessageType::BAR_CLOSED, symbol, bar.timeframe, sequence, bar.close_time,
                      receive_time, encode_bar_payload(bar));
    }

    // Deterministic OHLCV payload: O,H,L,C,V,closed.
    static std::string encode_bar_payload(const runtime::MarketBar& bar) {
        return format_fixed(bar.open) + "," + format_fixed(bar.high) + "," + format_fixed(bar.low) +
               "," + format_fixed(bar.close) + "," + format_fixed(bar.volume) + "," +
               (bar.closed ? "1" : "0");
    }

    static DecodedFrame decode(std::string_view frame) {
        DecodedFrame out;

        std::string_view fields[11];
        const std::size_t n = split(frame, fields, 11);
        if (n != 11) {
            out.status = DecodeStatus::MALFORMED;
            return out;
        }
        const std::string_view proto = fields[0];
        const std::string_view schema = fields[1];
        const std::string_view message_id = fields[2];
        const std::string_view type_text = fields[3];
        const std::string_view symbol = fields[4];
        const std::string_view tf_text = fields[5];
        const std::string_view seq_text = fields[6];
        const std::string_view event_text = fields[7];
        const std::string_view recv_text = fields[8];
        const std::string_view payload = fields[9];
        const std::string_view checksum = fields[10];

        if (proto != kProtocolVersion) {
            out.status = DecodeStatus::UNSUPPORTED_PROTOCOL;
            return out;
        }
        if (schema != kSchemaVersion) {
            out.status = DecodeStatus::UNSUPPORTED_SCHEMA;
            return out;
        }
        const MessageType type = message_type_from_string(type_text);
        if (type == MessageType::UNKNOWN) {
            out.status = DecodeStatus::UNKNOWN_MESSAGE_TYPE;
            return out;
        }
        const runtime::Timeframe tf = runtime::timeframe_from_string(tf_text);
        if (tf == runtime::Timeframe::UNKNOWN) {
            out.status = DecodeStatus::UNKNOWN_TIMEFRAME;
            return out;
        }

        std::uint64_t sequence = 0;
        if (!parse_u64(seq_text, sequence)) {
            out.status = DecodeStatus::MALFORMED;
            return out;
        }
        foundation::Timestamp::rep event_ns = 0;
        foundation::Timestamp::rep recv_ns = 0;
        if (!parse_i64(event_text, event_ns) || !parse_i64(recv_text, recv_ns)) {
            out.status = DecodeStatus::MALFORMED;
            return out;
        }
        const foundation::Timestamp event_time = foundation::Timestamp::from_nanoseconds(event_ns);
        const foundation::Timestamp receive_time = foundation::Timestamp::from_nanoseconds(recv_ns);

        runtime::MarketBar bar;
        if (type == MessageType::BAR_CLOSED) {
            if (!parse_bar_payload(payload, bar)) {
                out.status = DecodeStatus::MALFORMED;
                return out;
            }
            bar.timeframe = tf;
            bar.close_time = event_time;
        }

        if (message_id.empty() || symbol.empty()) {
            out.status = DecodeStatus::MALFORMED;
            return out;
        }
        const std::size_t last_sep = frame.rfind('|');
        if (last_sep == std::string_view::npos) {
            out.status = DecodeStatus::MALFORMED;
            return out;
        }
        const std::string_view canon = frame.substr(0, last_sep);
        if (checksum != checksum_of(canon)) {
            out.status = DecodeStatus::CHECKSUM_MISMATCH;
            return out;
        }
        // No lookahead: an event whose event_time is after its receive_time is
        // quarantined rather than delivered.
        if (event_time > receive_time) {
            out.status = DecodeStatus::FUTURE_DATED;
            return out;
        }

        out.type = type;
        out.timeframe = tf;
        out.symbol = std::string(symbol);
        out.sequence = sequence;
        out.event_time = event_time;
        out.receive_time = receive_time;
        out.payload = std::string(payload);
        out.bar = bar;
        out.message = foundation::MessageMetadata(
            foundation::EntityId::from_string(message_id),
            foundation::ProtocolVersion::from_string(std::string(proto)),
            foundation::SchemaVersion::from_string(std::string(schema)), receive_time, "mt5-ea",
            "aura-core", foundation::HashDigest::from_hex(checksum_of(canon)));
        out.event = foundation::EventMetadata(
            foundation::EventId::from_string(message_id), event_type_for(type), std::string(symbol),
            "mt5", "ea-1", event_time, receive_time, sequence,
            foundation::SchemaVersion::from_string(std::string(schema)));
        out.status = DecodeStatus::OK;
        return out;
    }

    // Deterministic message identity (stable across processes). Uses ':' as an
    // internal separator so the identity never contains the frame's '|' delimiter.
    static std::string make_message_id(MessageType type, const std::string& symbol,
                                       runtime::Timeframe timeframe, std::uint64_t sequence,
                                       foundation::Timestamp event_time) {
        return "mt5:" + std::string(runtime::to_string(timeframe)) + ":" + std::to_string(sequence) +
               ":" + std::to_string(event_time.nanoseconds()) + ":" + std::string(to_string(type)) +
               ":" + symbol;
    }

    // Lowercase SHA-256 hex of a byte string. Empty on hasher failure.
    static std::string checksum_of(std::string_view text) {
        const std::unique_ptr<foundation::IHasher> hasher =
            foundation::create_hasher(foundation::HashAlgorithm::SHA256);
        if (hasher == nullptr) return {};
        return hasher->hash(text).to_hex();
    }

    // Locale-independent fixed 8-decimal formatting.
    static std::string format_fixed(double value) {
        constexpr int kDecimals = 8;
        constexpr std::int64_t kScale = 100000000;  // 10^8
        const bool negative = value < 0.0;
        const double magnitude = negative ? -value : value;
        const std::int64_t scaled =
            static_cast<std::int64_t>(magnitude * static_cast<double>(kScale) + 0.5);
        const std::int64_t integer = scaled / kScale;
        const std::int64_t fraction = scaled % kScale;
        std::string frac = std::to_string(fraction);
        frac.insert(frac.begin(), static_cast<std::size_t>(kDecimals) - frac.size(), '0');
        return (negative ? "-" : "") + std::to_string(integer) + "." + frac;
    }

private:
    static foundation::EventType event_type_for(MessageType type) noexcept {
        switch (type) {
            case MessageType::BAR_CLOSED:  return foundation::EventType::MARKET_DATA;
            case MessageType::QUOTE:       return foundation::EventType::MARKET_DATA;
            case MessageType::SYMBOL_SPEC: return foundation::EventType::CONFIGURATION;
            case MessageType::HEARTBEAT:   return foundation::EventType::HEALTH;
            case MessageType::HEALTH:      return foundation::EventType::HEALTH;
            case MessageType::ERROR:       return foundation::EventType::AUDIT;
            case MessageType::UNKNOWN:     return foundation::EventType::UNKNOWN;
        }
        return foundation::EventType::UNKNOWN;
    }

    static std::size_t split(std::string_view text, std::string_view* out, std::size_t max) {
        std::size_t count = 0;
        std::size_t start = 0;
        while (count < max) {
            const std::size_t sep = text.find('|', start);
            if (count == max - 1) {
                out[count++] = text.substr(start);
                break;
            }
            if (sep == std::string_view::npos) {
                out[count++] = text.substr(start);
                break;
            }
            out[count++] = text.substr(start, sep - start);
            start = sep + 1;
        }
        return count;
    }

    static bool parse_u64(std::string_view text, std::uint64_t& out) {
        if (text.empty()) return false;
        const auto result = std::from_chars(text.data(), text.data() + text.size(), out);
        return result.ec == std::errc() && result.ptr == text.data() + text.size();
    }

    static bool parse_i64(std::string_view text, foundation::Timestamp::rep& out) {
        if (text.empty()) return false;
        const auto result = std::from_chars(text.data(), text.data() + text.size(), out);
        return result.ec == std::errc() && result.ptr == text.data() + text.size();
    }

    static bool parse_double(std::string_view text, double& out) {
        if (text.empty()) return false;
        const auto result = std::from_chars(text.data(), text.data() + text.size(), out);
        return result.ec == std::errc() && result.ptr == text.data() + text.size();
    }

    static bool parse_bar_payload(std::string_view payload, runtime::MarketBar& bar) {
        std::string_view parts[6];
        std::size_t count = 0;
        std::size_t start = 0;
        while (count < 6) {
            const std::size_t sep = payload.find(',', start);
            if (sep == std::string_view::npos) {
                parts[count++] = payload.substr(start);
                break;
            }
            parts[count++] = payload.substr(start, sep - start);
            start = sep + 1;
        }
        if (count != 6) return false;
        if (!parse_double(parts[0], bar.open) || !parse_double(parts[1], bar.high) ||
            !parse_double(parts[2], bar.low) || !parse_double(parts[3], bar.close) ||
            !parse_double(parts[4], bar.volume)) {
            return false;
        }
        if (parts[5] == "1") bar.closed = true;
        else if (parts[5] == "0") bar.closed = false;
        else return false;
        return true;
    }
};

}  // namespace mt5
}  // namespace aura

#endif  // AURA_MT5_PROTOCOLCODEC_H
