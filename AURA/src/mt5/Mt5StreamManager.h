#ifndef AURA_MT5_MT5STREAMMANAGER_H
#define AURA_MT5_MT5STREAMMANAGER_H

#include "foundation/DataQualityState.h"
#include "foundation/ServiceState.h"
#include "foundation/Timestamp.h"
#include "mt5/ProtocolCodec.h"
#include "runtime/AdapterManager.h"
#include "runtime/BarFinalizer.h"
#include "runtime/DataBus.h"
#include "runtime/TimeframeStateStore.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>

namespace aura {
namespace mt5 {

// Why a received frame was not applied to a stream. Accepted is the only success.
enum class ReceiveStatus : std::uint8_t {
    ACCEPTED = 0,
    DECODE_FAILED,
    NOT_A_BAR,
    UNKNOWN_TIMEFRAME,
    DUPLICATE_OR_REGRESSION,
    DUPLICATE_SEQUENCE,
};

constexpr std::string_view to_string(ReceiveStatus status) noexcept {
    switch (status) {
        case ReceiveStatus::ACCEPTED:                return "ACCEPTED";
        case ReceiveStatus::DECODE_FAILED:           return "DECODE_FAILED";
        case ReceiveStatus::NOT_A_BAR:               return "NOT_A_BAR";
        case ReceiveStatus::UNKNOWN_TIMEFRAME:       return "UNKNOWN_TIMEFRAME";
        case ReceiveStatus::DUPLICATE_OR_REGRESSION: return "DUPLICATE_OR_REGRESSION";
        case ReceiveStatus::DUPLICATE_SEQUENCE:      return "DUPLICATE_SEQUENCE";
    }
    return "DECODE_FAILED";
}

struct ReceiveResult {
    ReceiveStatus status{ReceiveStatus::DECODE_FAILED};
    runtime::Timeframe timeframe{runtime::Timeframe::UNKNOWN};
    DecodeStatus decode{DecodeStatus::MALFORMED};

    bool accepted() const noexcept { return status == ReceiveStatus::ACCEPTED; }
};

// Independent per-stream receiver state.
//
// MT5-0009. Each of the nine logical streams has its own state; updating one
// stream never mutates another. Sequence and health are per stream, not global.
struct ReceiverStreamState {
    runtime::Timeframe timeframe{runtime::Timeframe::UNKNOWN};
    foundation::ServiceState state{foundation::ServiceState::STARTING};
    foundation::DataQualityState quality{foundation::DataQualityState::UNKNOWN};
    foundation::Timestamp last_event_time{};
    foundation::Timestamp last_receive_time{};
    std::uint64_t last_sequence{0};
    std::uint64_t accepted_bars{0};
    std::uint64_t rejected{0};
    bool connected{false};
    std::string last_error{};

    bool healthy() const noexcept { return state == foundation::ServiceState::ONLINE; }
};

// C++ receiver stream manager for the MT5 one-EA / nine-stream transport.
//
// MT5-0009. Sits behind the single transport (one EA, one connection). Routes a
// decoded frame to its stream by the explicit timeframe carried in the frame —
// never by arrival position. Applies closed bars through the deterministic
// runtime finalizer (RT-0004) and timeframe state store (RT-0005), so duplicates
// and regressions are rejected idempotently and history is never repainted. A
// stream that fails or recovers does not reset another stream. It opens no socket,
// reads no clock and places no order; the transport itself is MT5-0003.
class Mt5StreamManager {
public:
    Mt5StreamManager() {
        for (const runtime::Timeframe tf : runtime::all_timeframes()) {
            ReceiverStreamState state;
            state.timeframe = tf;
            streams_.emplace(tf, std::move(state));
        }
    }

    // Decodes and applies a raw frame. A malformed/unknown/future-dated frame is
    // rejected and never routed.
    ReceiveResult accept_raw(std::string_view frame) {
        const DecodedFrame decoded = ProtocolCodec::decode(frame);
        if (!decoded.ok()) {
            ReceiveResult result;
            result.status = ReceiveStatus::DECODE_FAILED;
            result.decode = decoded.status;
            result.timeframe = decoded.timeframe;
            mark_rejected(decoded.timeframe, std::string("decode: ") +
                                                 std::string(to_string(decoded.status)));
            return result;
        }
        return accept(decoded);
    }

    // Applies a decoded frame to its explicit stream.
    ReceiveResult accept(const DecodedFrame& frame) {
        ReceiveResult result;
        result.timeframe = frame.timeframe;
        result.decode = frame.status;
        if (!frame.ok()) {
            result.status = ReceiveStatus::DECODE_FAILED;
            mark_rejected(frame.timeframe, "decode failed");
            return result;
        }
        if (frame.timeframe == runtime::Timeframe::UNKNOWN) {
            result.status = ReceiveStatus::UNKNOWN_TIMEFRAME;
            return result;
        }
        ReceiverStreamState* state = mutable_state(frame.timeframe);
        if (state == nullptr) {
            result.status = ReceiveStatus::UNKNOWN_TIMEFRAME;
            return result;
        }

        state->connected = true;
        state->last_receive_time = frame.receive_time;

        if (frame.type != MessageType::BAR_CLOSED) {
            // Non bar traffic keeps the stream alive (health/heartbeat/quote) but
            // is not an authoritative closed bar.
            if (state->state == foundation::ServiceState::STARTING) {
                state->state = foundation::ServiceState::ONLINE;
            }
            result.status = ReceiveStatus::NOT_A_BAR;
            return result;
        }

        // Per-stream sequence discipline: a sequence already consumed for this
        // stream (idempotent duplicate delivery) is rejected.
        if (state->last_sequence != 0 && frame.sequence == state->last_sequence) {
            ++state->rejected;
            result.status = ReceiveStatus::DUPLICATE_SEQUENCE;
            return result;
        }

        runtime::FinalizedBar finalized;
        runtime::MarketEvent event;
        event.metadata = frame.event;
        event.bar = frame.bar;
        if (!finalizer_.finalize(event, finalized)) {
            // Not closed, unknown timeframe, or not strictly after the last
            // finalized bar for this stream (duplicate / regression / repaint).
            ++state->rejected;
            result.status = ReceiveStatus::DUPLICATE_OR_REGRESSION;
            return result;
        }
        if (!store_.advance(finalized)) {
            ++state->rejected;
            result.status = ReceiveStatus::DUPLICATE_OR_REGRESSION;
            return result;
        }

        state->state = foundation::ServiceState::ONLINE;
        state->quality = finalized.quality;
        state->last_event_time = frame.event_time;
        state->last_sequence = frame.sequence;
        ++state->accepted_bars;
        state->last_error.clear();
        result.status = ReceiveStatus::ACCEPTED;
        return result;
    }

    // Records a transport-level failure for one stream only. Other streams keep
    // their state (a failure must not cascade).
    bool report_stream_failure(runtime::Timeframe timeframe, std::string message) {
        ReceiverStreamState* state = mutable_state(timeframe);
        if (state == nullptr) return false;
        state->state = foundation::ServiceState::DEGRADED;
        state->quality = foundation::DataQualityState::DEGRADED;
        state->last_error = std::move(message);
        return true;
    }

    // Marks a stream disconnected without touching the others.
    bool report_disconnected(runtime::Timeframe timeframe, std::string reason) {
        ReceiverStreamState* state = mutable_state(timeframe);
        if (state == nullptr) return false;
        state->connected = false;
        state->state = foundation::ServiceState::OFFLINE;
        state->quality = foundation::DataQualityState::UNKNOWN;
        state->last_error = std::move(reason);
        return true;
    }

    const ReceiverStreamState* stream(runtime::Timeframe timeframe) const {
        const auto it = streams_.find(timeframe);
        return it == streams_.end() ? nullptr : &it->second;
    }

    std::size_t stream_count() const noexcept { return streams_.size(); }

    std::size_t healthy_count() const {
        std::size_t count = 0;
        for (const auto& entry : streams_) {
            if (entry.second.healthy()) ++count;
        }
        return count;
    }

    const runtime::TimeframeStateStore& store() const noexcept { return store_; }
    const std::map<runtime::Timeframe, ReceiverStreamState>& streams() const noexcept {
        return streams_;
    }

    // Restores one timeframe's last-processed progress after a restart, without
    // reprocessing history. Monotonic: a record that would regress the timeframe
    // is refused. The stream is marked ONLINE so the next genuine closed bar that
    // strictly advances it is accepted. This resumes derived state only; it places
    // no order and fabricates no bar.
    bool restore_progress(const runtime::TimeframeProgress& progress) {
        ReceiverStreamState* state = mutable_state(progress.timeframe);
        if (state == nullptr) return false;
        if (!store_.restore(progress)) return false;
        state->state = foundation::ServiceState::ONLINE;
        state->quality = foundation::DataQualityState::VALID;
        state->last_event_time = progress.last_close;
        state->last_receive_time = progress.last_close;
        state->last_sequence = progress.sequence;
        state->accepted_bars = progress.sequence;
        state->connected = true;
        return true;
    }

    // Clears the per-stream sequence guard so a restarted sender may re-use
    // sequence numbers. Does not change last-processed progress.
    void reset_stream_sequences() {
        for (auto& entry : streams_) {
            entry.second.last_sequence = 0;
        }
    }

private:
    ReceiverStreamState* mutable_state(runtime::Timeframe timeframe) {
        const auto it = streams_.find(timeframe);
        return it == streams_.end() ? nullptr : &it->second;
    }

    void mark_rejected(runtime::Timeframe timeframe, std::string message) {
        if (timeframe == runtime::Timeframe::UNKNOWN) return;
        ReceiverStreamState* state = mutable_state(timeframe);
        if (state == nullptr) return;
        ++state->rejected;
        state->last_error = std::move(message);
    }

    runtime::BarFinalizer finalizer_{};
    runtime::TimeframeStateStore store_{};
    std::map<runtime::Timeframe, ReceiverStreamState> streams_{};
};

}  // namespace mt5
}  // namespace aura

#endif  // AURA_MT5_MT5STREAMMANAGER_H
