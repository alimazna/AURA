#ifndef AURA_RUNTIME_ADAPTERMANAGER_H
#define AURA_RUNTIME_ADAPTERMANAGER_H

#include "foundation/DataQualityState.h"
#include "foundation/MessageMetadata.h"
#include "foundation/ServiceState.h"
#include "foundation/Timestamp.h"
#include "resilience/CapabilityId.h"
#include "resilience/HealthSnapshot.h"
#include "resilience/HealthStateEngine.h"
#include "resilience/ServiceDescriptor.h"
#include "resilience/SystemSupervisor.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <utility>

namespace aura {
namespace runtime {

// Canonical logical timeframe identity (V3-29).
//
// RT-0001 / Phase 1 deterministic runtime. The nine logical timeframe streams are
// canonical; under the approved MT5 one-EA deployment they are carried by a single
// physical EA over one transport, but each keeps an explicit, stable identity so
// a bar for one timeframe can never be attributed to another.
enum class Timeframe : std::uint8_t {
    UNKNOWN = 0,
    M1,
    M5,
    M15,
    M30,
    H1,
    H4,
    D1,
    W1,
    MN1,
};

constexpr std::string_view to_string(Timeframe timeframe) noexcept {
    switch (timeframe) {
        case Timeframe::UNKNOWN: return "UNKNOWN";
        case Timeframe::M1:      return "M1";
        case Timeframe::M5:      return "M5";
        case Timeframe::M15:     return "M15";
        case Timeframe::M30:     return "M30";
        case Timeframe::H1:      return "H1";
        case Timeframe::H4:      return "H4";
        case Timeframe::D1:      return "D1";
        case Timeframe::W1:      return "W1";
        case Timeframe::MN1:     return "MN1";
    }
    return "UNKNOWN";
}

// Parses a canonical timeframe label. Unknown text yields Timeframe::UNKNOWN
// rather than a silent default.
inline Timeframe timeframe_from_string(std::string_view text) noexcept {
    const Timeframe all[] = {Timeframe::M1,  Timeframe::M5, Timeframe::M15, Timeframe::M30,
                             Timeframe::H1,  Timeframe::H4, Timeframe::D1,  Timeframe::W1,
                             Timeframe::MN1};
    for (const Timeframe tf : all) {
        if (to_string(tf) == text) return tf;
    }
    return Timeframe::UNKNOWN;
}

// The nine canonical timeframes in V3-29 order (M1 .. MN1).
inline constexpr std::array<Timeframe, 9> all_timeframes() noexcept {
    return {Timeframe::M1,  Timeframe::M5, Timeframe::M15, Timeframe::M30,
            Timeframe::H1,  Timeframe::H4, Timeframe::D1,  Timeframe::W1,
            Timeframe::MN1};
}

// Per-stream adapter state. Each of the nine logical streams has its own
// independent status: updating one stream must never alter another.
struct StreamStatus {
    Timeframe timeframe{Timeframe::UNKNOWN};
    foundation::ServiceState state{foundation::ServiceState::STARTING};
    foundation::DataQualityState quality{foundation::DataQualityState::UNKNOWN};
    foundation::Timestamp last_bar_time{};
    std::uint64_t last_sequence{0};
    bool connected{false};
    std::string last_error{};

    bool healthy() const noexcept { return state == foundation::ServiceState::ONLINE; }
};

// MT5/MQL5 adapter boundary manager.
//
// RT-0001 / Phase 1 deterministic runtime. Tracks the nine logical timeframe
// streams carried by one physical EA / one transport. Stream state is strictly
// independent: a failure or recovery of one stream cannot change another stream's
// state, and no data is fabricated on failure. Ingested messages carry their
// timeframe explicitly (no positional multiplexing). This is a boundary and
// bookkeeping contract only: it opens no socket, starts no thread and places no
// order.
class AdapterManager {
public:
    AdapterManager() {
        for (const Timeframe tf : all_timeframes()) {
            StreamStatus status;
            status.timeframe = tf;
            streams_.emplace(tf, std::move(status));
        }
    }

    // Marks a stream connected and starting. Does not affect other streams.
    bool report_connected(Timeframe timeframe) {
        StreamStatus* status = mutable_status(timeframe);
        if (status == nullptr) return false;
        status->connected = true;
        status->state = foundation::ServiceState::STARTING;
        status->last_error.clear();
        return true;
    }

    // Records a completed bar for one stream and marks it ONLINE/VALID.
    bool report_bar(Timeframe timeframe, foundation::Timestamp bar_time, std::uint64_t sequence) {
        StreamStatus* status = mutable_status(timeframe);
        if (status == nullptr) return false;
        status->connected = true;
        status->state = foundation::ServiceState::ONLINE;
        status->quality = foundation::DataQualityState::VALID;
        status->last_bar_time = bar_time;
        status->last_sequence = sequence;
        status->last_error.clear();
        return true;
    }

    // Records a stream-level failure. Only the named stream changes; other streams
    // keep their state (V3-13, MT5 one-EA decision).
    bool report_error(Timeframe timeframe, std::string message) {
        StreamStatus* status = mutable_status(timeframe);
        if (status == nullptr) return false;
        status->state = foundation::ServiceState::DEGRADED;
        status->quality = foundation::DataQualityState::DEGRADED;
        status->last_error = std::move(message);
        return true;
    }

    // Marks one stream offline/disconnected. Other streams are untouched.
    bool report_disconnected(Timeframe timeframe, std::string reason) {
        StreamStatus* status = mutable_status(timeframe);
        if (status == nullptr) return false;
        status->connected = false;
        status->state = foundation::ServiceState::OFFLINE;
        status->quality = foundation::DataQualityState::UNKNOWN;
        status->last_error = std::move(reason);
        return true;
    }

    // Ingest hint: a message plus its explicit timeframe. The timeframe is taken
    // from the argument, never inferred from message position.
    bool ingest(const foundation::MessageMetadata& metadata, Timeframe timeframe) {
        (void)metadata;
        return mutable_status(timeframe) != nullptr;
    }

    const StreamStatus* status(Timeframe timeframe) const {
        const auto it = streams_.find(timeframe);
        return it == streams_.end() ? nullptr : &it->second;
    }

    std::size_t healthy_count() const {
        std::size_t count = 0;
        for (const auto& entry : streams_) {
            if (entry.second.healthy()) ++count;
        }
        return count;
    }

    bool all_healthy() const { return healthy_count() == streams_.size(); }
    std::size_t stream_count() const noexcept { return streams_.size(); }

    const std::map<Timeframe, StreamStatus>& streams() const noexcept { return streams_; }

    // Builds a resilience health snapshot for one stream. A stream that is not
    // valid is reported as not-safe; no healthy state is fabricated.
    resilience::HealthSnapshot health_snapshot(Timeframe timeframe) const {
        const std::string name = std::string("adapter-") + std::string(to_string(timeframe));
        const StreamStatus* status = this->status(timeframe);
        if (status == nullptr) {
            return resilience::HealthSnapshot(
                resilience::ServiceDescriptor(name, foundation::ServiceState::BLOCKED),
                foundation::ServiceState::BLOCKED, foundation::Timestamp{}, "unknown timeframe");
        }
        return resilience::HealthSnapshot(
            resilience::ServiceDescriptor(name, status->state), status->state,
            status->last_bar_time, status->last_error);
    }

    // Delegates aggregate capability health to the resilience supervisor
    // (RS-0019). The adapter boundary surfaces supervised health; it grants no
    // authority and takes no protective action itself.
    foundation::ServiceState supervised_state(
        const resilience::SystemSupervisor& supervisor, const resilience::CapabilityId& capability,
        const resilience::HealthStateEngine::snapshot_map& snapshots) const {
        return supervisor.capability_state(capability, snapshots);
    }

private:
    StreamStatus* mutable_status(Timeframe timeframe) {
        const auto it = streams_.find(timeframe);
        return it == streams_.end() ? nullptr : &it->second;
    }

    std::map<Timeframe, StreamStatus> streams_{};
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_ADAPTERMANAGER_H
