#ifndef AURA_RUNTIME_BARFINALIZER_H
#define AURA_RUNTIME_BARFINALIZER_H

#include "foundation/DataQualityState.h"
#include "foundation/Timestamp.h"
#include "runtime/DataBus.h"
#include "runtime/DataValidator.h"

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>

namespace aura {
namespace runtime {

// Deterministic closed-bar identity: symbol + timeframe + close_time.
//
// RT-0004 / Phase 1 deterministic runtime. The identity is a pure function of
// stable fields, so the same closed bar always yields the same identity across
// processes, and two different timeframes can never collide.
class BarIdentity {
public:
    BarIdentity() = default;

    BarIdentity(std::string symbol, Timeframe timeframe, foundation::Timestamp close_time)
        : symbol_(std::move(symbol)), timeframe_(timeframe), close_time_(close_time) {}

    const std::string& symbol() const noexcept { return symbol_; }
    Timeframe timeframe() const noexcept { return timeframe_; }
    foundation::Timestamp close_time() const noexcept { return close_time_; }

    bool valid() const noexcept {
        return !symbol_.empty() && timeframe_ != Timeframe::UNKNOWN;
    }

    // Canonical, reproducible serialization.
    std::string canonical_string() const {
        return symbol_ + "|" + std::string(to_string(timeframe_)) + "|" +
               std::to_string(close_time_.nanoseconds());
    }

    friend bool operator==(const BarIdentity& a, const BarIdentity& b) noexcept {
        return a.symbol_ == b.symbol_ && a.timeframe_ == b.timeframe_ &&
               a.close_time_ == b.close_time_;
    }
    friend bool operator!=(const BarIdentity& a, const BarIdentity& b) noexcept { return !(a == b); }
    friend bool operator<(const BarIdentity& a, const BarIdentity& b) noexcept {
        if (a.symbol_ != b.symbol_) return a.symbol_ < b.symbol_;
        if (a.timeframe_ != b.timeframe_) return a.timeframe_ < b.timeframe_;
        return a.close_time_ < b.close_time_;
    }

    std::size_t hash() const noexcept {
        std::size_t h = std::hash<std::string>{}(symbol_);
        h ^= std::hash<std::uint8_t>{}(static_cast<std::uint8_t>(timeframe_)) +
             0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        h ^= close_time_.hash() + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        return h;
    }

private:
    std::string symbol_{};
    Timeframe timeframe_{Timeframe::UNKNOWN};
    foundation::Timestamp close_time_{};
};

// A finalized (immutable) closed bar with its quality verdict.
struct FinalizedBar {
    BarIdentity identity{};
    MarketBar bar{};
    foundation::DataQualityState quality{foundation::DataQualityState::UNKNOWN};
};

// Finalizes closed bars exactly once.
//
// RT-0004 / Phase 1 deterministic runtime. Only closed bars are finalized. A bar
// whose close_time is not strictly after the last finalized bar for that
// timeframe is rejected, so a bar can never be repainted, re-finalized or
// replaced. Bar identity is deterministic (BarIdentity). Bars failing validation
// are still finalized with their non-VALID quality so downstream gating can see
// them; the finalizer never fabricates or repairs data.
class BarFinalizer {
public:
    BarFinalizer() = default;

    // Attempts to finalize a bar. Returns false, with no change, when the bar is
    // not closed, has an unknown timeframe, or would repaint/replace an already
    // finalized bar for that timeframe.
    bool finalize(const MarketEvent& event, FinalizedBar& out) {
        const MarketBar& bar = event.bar;
        if (!bar.closed) return false;
        if (bar.timeframe == Timeframe::UNKNOWN) return false;
        if (bar.close_time <= last_close_[bar.timeframe]) return false;

        const std::string symbol = event.metadata.symbol();
        FinalizedBar finalized;
        finalized.identity = BarIdentity(symbol, bar.timeframe, bar.close_time);
        finalized.bar = bar;
        finalized.quality = validator_.validate(bar).quality;

        last_close_[bar.timeframe] = bar.close_time;
        out = std::move(finalized);
        return true;
    }

    bool has_finalized(Timeframe timeframe) const {
        return last_close_.find(timeframe) != last_close_.end();
    }

    foundation::Timestamp last_close(Timeframe timeframe) const {
        const auto it = last_close_.find(timeframe);
        return it == last_close_.end() ? foundation::Timestamp{} : it->second;
    }

private:
    DataValidator validator_{};
    std::map<Timeframe, foundation::Timestamp> last_close_{};
};

}  // namespace runtime
}  // namespace aura

namespace std {
template <>
struct hash<aura::runtime::BarIdentity> {
    std::size_t operator()(const aura::runtime::BarIdentity& id) const noexcept { return id.hash(); }
};
}  // namespace std

#endif  // AURA_RUNTIME_BARFINALIZER_H
