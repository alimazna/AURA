#ifndef AURA_DESKTOP_CANDLECHART_H
#define AURA_DESKTOP_CANDLECHART_H

#include "foundation/Timestamp.h"
#include "runtime/AdapterManager.h"
#include "runtime/DataBus.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace aura {
namespace desktop {

// UTC formatter for a bar close time (MM-DD HH:MM from the epoch nanoseconds).
// The axis shows the real close time of the closed bar; there is no wall clock.
inline std::string format_utc_minute(foundation::Timestamp t) {
    foundation::Timestamp::rep secs = t.seconds();
    if (secs < 0) secs = 0;
    const std::int64_t days = secs / 86400;
    const std::int64_t rem = secs % 86400;
    const int hh = static_cast<int>(rem / 3600);
    const int mm = static_cast<int>((rem % 3600) / 60);
    // Civil date from days since 1970-01-01 (Howard Hinnant's algorithm).
    const std::int64_t z = days + 719468;
    const std::int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const std::int64_t doe = z - era * 146097;
    const std::int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const std::int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const std::int64_t mp = (5 * doy + 2) / 153;
    const std::int64_t d = doy - (153 * mp + 2) / 5 + 1;
    const std::int64_t m = mp + (mp < 10 ? 3 : -9);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02lld-%02lld %02d:%02d", static_cast<long long>(m),
                  static_cast<long long>(d), hh, mm);
    return std::string(buf);
}


// Pure, ImGui-free chart model for the XAUUSD candlestick view.
//
// This header owns three responsibilities, all deterministic and unit-testable
// without a GPU or a window:
//   1. the timeframe selector state (default M15) and the explicit
//      timeframe <-> stream mapping;
//   2. the closed-bar series projection (real bars only, never synthesized);
//   3. the chart geometry (price range and normalised candle positions).
//
// It reaches into no engine, reads no clock and mutates nothing. Candles are
// copied from an already-accepted closed-bar series, so the forming bar can
// never appear and history can never repaint. A timeframe with no real bars is
// reported as unavailable and rendered as an explicit empty state, never padded
// with invented candles.

// The nine supported chart timeframes, in display order. This is presentation
// order only; identity is always the explicit runtime::Timeframe value.
inline const std::vector<runtime::Timeframe>& chart_timeframes() {
    static const std::vector<runtime::Timeframe> kAll = {
        runtime::Timeframe::M1,  runtime::Timeframe::M5,  runtime::Timeframe::M15,
        runtime::Timeframe::M30, runtime::Timeframe::H1,  runtime::Timeframe::H4,
        runtime::Timeframe::D1,  runtime::Timeframe::W1,  runtime::Timeframe::MN1,
    };
    return kAll;
}

// The default chart timeframe (M15, the operational stream).
inline runtime::Timeframe default_chart_timeframe() { return runtime::Timeframe::M15; }

// ---- Timeframe selector state ----------------------------------------------

// Presentation-only selection state for the timeframe selector. Holds an
// explicit runtime::Timeframe (never an index), so a switch always resolves to
// the correct logical stream. The selector can only ever hold one of the nine
// supported chart timeframes.
class TimeframeSelection {
public:
    TimeframeSelection() : selected_(default_chart_timeframe()) {}

    runtime::Timeframe selected() const noexcept { return selected_; }
    std::string selected_label() const { return std::string(runtime::to_string(selected_)); }

    // Selects a timeframe. An unsupported value (UNKNOWN or anything outside the
    // nine) is refused and the selection is left unchanged, so the selector can
    // never point at a stream that does not exist.
    bool select(runtime::Timeframe tf) {
        for (const runtime::Timeframe candidate : chart_timeframes()) {
            if (candidate == tf) {
                selected_ = tf;
                return true;
            }
        }
        return false;
    }

    // True when the given timeframe is the current selection (drives the active
    // visual state of the corresponding button).
    bool is_selected(runtime::Timeframe tf) const noexcept { return tf == selected_; }

private:
    runtime::Timeframe selected_;
};

// ---- Closed-bar series projection ------------------------------------------

// One closed bar reduced to exactly what the chart draws. `close_time` is the
// bar identity's close; it is a real accepted value, never a synthetic index.
struct Candle {
    double open{0.0};
    double high{0.0};
    double low{0.0};
    double close{0.0};
    foundation::Timestamp close_time{};
};

// The render-ready series for one timeframe. `available` is true only when at
// least one real closed bar exists for that exact timeframe; otherwise the chart
// shows the explicit NO CANDLE DATA state.
struct CandleSeries {
    runtime::Timeframe timeframe{runtime::Timeframe::UNKNOWN};
    std::string label{"UNKNOWN"};
    bool available{false};
    std::vector<Candle> candles{};
    foundation::Timestamp first_close{};
    foundation::Timestamp last_close{};
    double low{0.0};
    double high{0.0};
};

// Projects an authoritative closed-bar series into chart candles. Bars whose
// OHLC is non-finite are skipped (never drawn as a fabricated candle); the
// remaining candles are kept in their original chronological order.
inline CandleSeries make_candle_series(runtime::Timeframe tf,
                                       const std::vector<runtime::MarketBar>& bars) {
    CandleSeries s;
    s.timeframe = tf;
    s.label = std::string(runtime::to_string(tf));
    for (const runtime::MarketBar& b : bars) {
        if (b.timeframe != tf) continue;  // explicit identity, never positional
        if (!b.closed) continue;          // never draw a still-forming bar
        if (!std::isfinite(b.open) || !std::isfinite(b.high) || !std::isfinite(b.low) ||
            !std::isfinite(b.close))
            continue;
        Candle c;
        c.open = b.open;
        c.high = b.high;
        c.low = b.low;
        c.close = b.close;
        c.close_time = b.close_time;
        s.candles.push_back(c);
    }
    s.available = !s.candles.empty();
    if (s.available) {
        s.first_close = s.candles.front().close_time;
        s.last_close = s.candles.back().close_time;
        s.low = s.candles.front().low;
        s.high = s.candles.front().high;
        for (const Candle& c : s.candles) {
            if (c.low < s.low) s.low = c.low;
            if (c.high > s.high) s.high = c.high;
        }
    }
    return s;
}

// ---- Chart geometry --------------------------------------------------------

// Normalised candle positions. y is a fraction of the plot height measured from
// the top (0 = highest price in view, 1 = lowest), so the model carries no
// pixels and can be tested exactly. wick_* share the candle's x centre.
struct CandleLayout {
    float x{0.0f};         // candle centre, fraction of plot width [0,1]
    float body_top{0.0f};  // top of the body (the higher of open/close)
    float body_bottom{0.0f};
    float wick_top{0.0f};   // == high
    float wick_bottom{0.0f};  // == low
    bool bullish{false};   // close >= open
};

struct ChartGeometry {
    double price_low{0.0};
    double price_high{0.0};
    float plot_left{0.0f};   // fraction of total width reserved for the price axis
    float plot_right{1.0f};
    std::vector<CandleLayout> candles{};
    // Horizontal reference lines (fractions of the plot height from the top)
    // and their prices, so the price scale can be drawn and labelled.
    std::vector<float> grid_y{};
    std::vector<double> grid_price{};
};

inline double chart_y_fraction(double price, double lo, double hi) {
    if (!(hi > lo)) return 0.5;  // flat or degenerate range -> centre
    double f = (hi - price) / (hi - lo);
    if (f < 0.0) f = 0.0;
    if (f > 1.0) f = 1.0;
    return f;
}

// Builds the deterministic geometry for a candle series inside a plot whose
// price-axis gutter occupies `plot_left` of the width. Gridlines are evenly
// spaced in price across the visible range.
inline ChartGeometry build_chart_geometry(const CandleSeries& series, int grid_lines = 5,
                                          float plot_left = 0.0f) {
    ChartGeometry g;
    g.plot_left = plot_left;
    g.plot_right = 1.0f;
    if (!series.available) return g;

    g.price_low = series.low;
    g.price_high = series.high;
    if (!(g.price_high > g.price_low)) {
        // A flat range (e.g. a single bar with high == low) is expanded
        // symmetrically so the candle is still visible; this is geometry only,
        // never a fabricated price.
        const double pad = (g.price_high == 0.0) ? 1.0 : std::abs(g.price_high) * 0.001;
        g.price_low -= pad;
        g.price_high += pad;
    }

    const float left = plot_left;
    const float width = 1.0f - plot_left;
    const std::size_t n = series.candles.size();
    for (std::size_t i = 0; i < n; ++i) {
        const Candle& c = series.candles[i];
        CandleLayout l;
        l.x = left + width * (n == 1 ? 0.5f : static_cast<float>(i) / static_cast<float>(n - 1));
        l.wick_top = static_cast<float>(chart_y_fraction(c.high, g.price_low, g.price_high));
        l.wick_bottom = static_cast<float>(chart_y_fraction(c.low, g.price_low, g.price_high));
        const float o = static_cast<float>(chart_y_fraction(c.open, g.price_low, g.price_high));
        const float cl = static_cast<float>(chart_y_fraction(c.close, g.price_low, g.price_high));
        l.body_top = (o < cl) ? o : cl;
        l.body_bottom = (o < cl) ? cl : o;
        l.bullish = c.close >= c.open;
        g.candles.push_back(l);
    }

    if (grid_lines > 0) {
        for (int i = 0; i < grid_lines; ++i) {
            const float t = (grid_lines == 1) ? 0.5f
                                              : static_cast<float>(i) / static_cast<float>(grid_lines - 1);
            g.grid_y.push_back(t);
            g.grid_price.push_back(g.price_high - t * (g.price_high - g.price_low));
        }
    }
    return g;
}

}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_CANDLECHART_H
