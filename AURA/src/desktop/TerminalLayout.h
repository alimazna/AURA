#ifndef AURA_DESKTOP_TERMINALLAYOUT_H
#define AURA_DESKTOP_TERMINALLAYOUT_H

// Pure, GUI-free geometry contract for the AURA terminal composition.
//
// The renderer and the deterministic tests both consume these functions, so a
// layout regression (a mis-ordered timeframe strip, an unbounded chart height, a
// panel that collapses the chart) fails in a plain unit test without a rendered
// surface. Nothing here touches ImGui, runtime state, or the clock.

#include "desktop/CandleChart.h"

#include <cstddef>
#include <string>
#include <vector>

namespace aura {
namespace desktop {
namespace layout {

// Fixed chrome heights (device-independent pixels at 1x). Kept in one place so
// the window shell, the composition and the tests agree.
struct Chrome {
    static constexpr float kTopBar = 48.0f;     // == theme::kTopBarHeight
    static constexpr float kStatusBar = 24.0f;  // == theme::kStatusBarHeight
    static constexpr float kActionBar = 40.0f;  // == theme::kActionBarHeight
    static constexpr float kIconRail = 52.0f;   // == theme::kIconRailWidth
    static constexpr float kSidebar = 300.0f;   // max navigation width
    static constexpr float kMarketHeader = 58.0f;
    static constexpr float kTabStrip = 32.0f;
    static constexpr float kMetricStrip = 62.0f;
    static constexpr float kLowerModules = 150.0f;
    static constexpr float kSecondaryStrip = 56.0f;  // compact secondary-modules strip
    static constexpr float kMinChart = 220.0f;
    static constexpr float kMaxChart = 680.0f;
    static constexpr float kMinPanel = 120.0f;
    static constexpr float kTabWidth = 50.0f;
    static constexpr float kTabGap = 4.0f;
    static constexpr float kMinSidebar = 220.0f;
    static constexpr float kMaxSidebar = 300.0f;
    static constexpr float kAnalyticsMin = 240.0f;
    static constexpr float kAnalyticsMax = 320.0f;
};

// Responsive column widths. The navigation and the right analytics column scale
// with the window but stay clamped, so the chart always keeps the majority of the
// central workspace at 1280x720 and at 1920x1080 alike. Pure functions: the same
// window width always yields the same geometry (no clock, no state).
inline float sidebar_width(float window_w) {
    float w = window_w * 0.185f;
    if (w < Chrome::kMinSidebar) w = Chrome::kMinSidebar;
    if (w > Chrome::kMaxSidebar) w = Chrome::kMaxSidebar;
    return w;
}

inline float analytics_width(float window_w) {
    float w = window_w * 0.185f;
    if (w < Chrome::kAnalyticsMin) w = Chrome::kAnalyticsMin;
    if (w > Chrome::kAnalyticsMax) w = Chrome::kAnalyticsMax;
    return w;
}

// The dashboard chart height for the available content height: the chart takes
// everything left after the market header, tab strip, metric strip, lower module
// row and their spacing, clamped so it can never collapse or grow unbounded.
inline float dashboard_chart_height(float content_h) {
    const float reserved = Chrome::kMarketHeader + Chrome::kTabStrip + Chrome::kMetricStrip +
                           Chrome::kLowerModules + 6.0f * Chrome::kTabGap;
    float h = content_h - reserved;
    if (h < Chrome::kMinChart) h = Chrome::kMinChart;
    if (h > Chrome::kMaxChart) h = Chrome::kMaxChart;
    return h;
}

// Height reserved below the chart for the metric strip and the secondary panels
// (including their inter-item spacing), independent of the viewport. The chart
// takes exactly what is left over.
inline float secondary_reserve() {
    return Chrome::kMetricStrip + 4.0f * Chrome::kTabGap /*spacing*/ + 196.0f;
}

// The chart workspace height for a given content height (the height available
// below the top bar, header and tab strip). Clamped so the chart can never
// collapse below a usable floor nor grow unbounded on very tall viewports.
inline float chart_height(float content_h) {
    float h = content_h - secondary_reserve();
    if (h < Chrome::kMinChart) h = Chrome::kMinChart;
    if (h > Chrome::kMaxChart) h = Chrome::kMaxChart;
    return h;
}

// The secondary-panel height for a given content height and chart height. The
// panels hold the remaining space (never negative); they do not shrink the chart.
inline float panel_height(float content_h, float chart_h) {
    float h = content_h - chart_h - Chrome::kMetricStrip - 4.0f * Chrome::kTabGap;
    return h > Chrome::kMinPanel ? h : Chrome::kMinPanel;
}

struct Row {
    runtime::Timeframe timeframe;
    std::string label;
    float x;
    float width;
};

// The canonical V3-29 authority role of each logical timeframe stream. This is
// presentation-only labelling of the existing runtime identity: it never changes
// which stream a selection maps to. UNKNOWN is not a stream and has no role.
inline const char* timeframe_role(runtime::Timeframe tf) {
    switch (tf) {
        case runtime::Timeframe::M15: return "OPERATIONAL";  // primary setup/trigger
        case runtime::Timeframe::H4:  return "STRUCTURAL";   // primary structural authority
        case runtime::Timeframe::M5:
        case runtime::Timeframe::M1:  return "EXECUTION";    // execution / microstructure context
        case runtime::Timeframe::M30:
        case runtime::Timeframe::H1:  return "CONTEXT";
        case runtime::Timeframe::D1:
        case runtime::Timeframe::W1:
        case runtime::Timeframe::MN1: return "LONG HORIZON";
        default:                      return "";
    }
}

// The ordered timeframe tab geometry: exactly nine tabs in the canonical V3-29
// order, one per supported timeframe (explicit identity, never positional).
inline std::vector<Row> timeframe_rows(float origin_x = 0.0f) {
    std::vector<Row> rows;
    const std::vector<runtime::Timeframe>& tfs = chart_timeframes();
    float x = origin_x;
    for (const runtime::Timeframe tf : tfs) {
        rows.push_back(Row{tf, std::string(runtime::to_string(tf)), x, Chrome::kTabWidth});
        x += Chrome::kTabWidth + Chrome::kTabGap;
    }
    return rows;
}

// Heights of the dashboard rows below the strips: the dominant chart and the
// Signals / Risk / Shadow Positions row. Pure so the composition and the tests
// agree on the same accounting: content_h covers the analytics row, the compact
// secondary strip (kSecondaryStrip + its trailing spacing), the five
// ItemSpacing-sized gaps between the three rows and a 4px slack, so rounding
// can never spill the dashboard into a scrollbar. The analytics row stays a
// minority band; on short viewports the chart floor wins and the panels keep
// only their own usable minimum.
struct DashboardRows {
    float chart_h{0.0f};
    float analytics_h{0.0f};
};

inline DashboardRows dashboard_rows(float content_h, float gap_y = 8.0f) {
    const float reserved = Chrome::kSecondaryStrip + 5.0f * gap_y + 4.0f;
    float analytics = content_h * 0.26f;
    if (analytics < 150.0f) analytics = 150.0f;  // usable minimum for the panel row
    if (analytics > 220.0f) analytics = 220.0f;  // the row stays a minority band
    float chart = content_h - analytics - reserved;
    if (chart > Chrome::kMaxChart) chart = Chrome::kMaxChart;
    if (chart < Chrome::kMinChart) {
        chart = Chrome::kMinChart;  // the chart floor wins
        analytics = content_h - chart - reserved;
        if (analytics < 120.0f) analytics = 120.0f;
    }
    DashboardRows r;
    r.chart_h = chart;
    r.analytics_h = analytics;
    return r;
}

}  // namespace layout
}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_TERMINALLAYOUT_H
