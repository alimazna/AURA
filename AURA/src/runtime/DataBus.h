#ifndef AURA_RUNTIME_DATABUS_H
#define AURA_RUNTIME_DATABUS_H

#include "foundation/EventMetadata.h"
#include "foundation/Timestamp.h"
#include "runtime/AdapterManager.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace aura {
namespace runtime {

// Immutable OHLCV bar for one timeframe. `closed` distinguishes a bar that has
// finished forming from a bar still forming; only closed bars may flow into
// decision-making (V3-24).
struct MarketBar {
    Timeframe timeframe{Timeframe::UNKNOWN};
    foundation::Timestamp open_time{};
    foundation::Timestamp close_time{};
    double open{0.0};
    double high{0.0};
    double low{0.0};
    double close{0.0};
    double volume{0.0};
    bool closed{false};
};

// A market-data event: envelope metadata plus the bar payload. The timeframe is
// carried explicitly inside the bar, never inferred from position.
struct MarketEvent {
    foundation::EventMetadata metadata{};
    MarketBar bar{};

    // Deterministic ordering key. Earlier event_time first; ties broken by
    // sequence_id then event id so ordering never depends on arrival order.
    bool precedes(const MarketEvent& other) const {
        if (metadata.event_time() != other.metadata.event_time()) {
            return metadata.event_time() < other.metadata.event_time();
        }
        if (metadata.sequence_id() != other.metadata.sequence_id()) {
            return metadata.sequence_id() < other.metadata.sequence_id();
        }
        if (metadata.event_id() != other.metadata.event_id()) {
            return metadata.event_id() < other.metadata.event_id();
        }
        return false;
    }
};

// Deterministic C++ data bus for market events.
//
// RT-0002 / Phase 1 deterministic runtime. Accepts market events and delivers
// them in a deterministic order (event_time, then sequence_id, then event id),
// independent of arrival order. It enforces causality and repaint discipline at
// the boundary: an event whose event_time is in the future relative to its
// receive_time is quarantined rather than delivered (no lookahead), and a bar
// that is not closed is never delivered downstream (no repaint). Duplicate
// deliveries of the same bar identity are rejected. This is an in-process
// ordered buffer only: no threads, no I/O, no clock.
class DataBus {
public:
    DataBus() = default;

    // Publishes a market event. Returns false, with no change, when the event is
    // structurally invalid, future-dated, or not a closed bar.
    bool publish(MarketEvent event) {
        if (!event.metadata.valid()) return false;
        if (event.metadata.event_time() > event.metadata.receive_time()) return false;  // no lookahead
        if (!event.bar.closed) return false;                                            // no repaint
        if (event.bar.timeframe == Timeframe::UNKNOWN) return false;
        if (event.metadata.event_time() != event.bar.close_time) return false;
        pending_.push_back(std::move(event));
        return true;
    }

    std::size_t pending_count() const noexcept { return pending_.size(); }
    bool empty() const noexcept { return pending_.empty(); }

    // Drains all pending events in deterministic order. The bus is emptied.
    std::vector<MarketEvent> drain_ordered() {
        std::stable_sort(pending_.begin(), pending_.end(),
                         [](const MarketEvent& a, const MarketEvent& b) { return a.precedes(b); });
        std::vector<MarketEvent> out;
        out.swap(pending_);
        return out;
    }

    // Deterministic peek without draining.
    std::vector<MarketEvent> ordered() const {
        std::vector<MarketEvent> copy = pending_;
        std::stable_sort(copy.begin(), copy.end(),
                         [](const MarketEvent& a, const MarketEvent& b) { return a.precedes(b); });
        return copy;
    }

private:
    std::vector<MarketEvent> pending_{};
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_DATABUS_H
