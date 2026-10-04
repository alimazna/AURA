#ifndef AURA_DESKTOP_AURATERMINAL_H
#define AURA_DESKTOP_AURATERMINAL_H

// AURA professional market terminal — shell composition.
//
// This file owns the cohesive terminal chrome and the chart-first dashboard
// composition: the far-left icon rail, the navigation sidebar, the top terminal
// bar, the instrument (market) header, the timeframe tab strip, the large
// candlestick workspace, the compact metric modules, the right-hand analytics
// column, the lower information modules, the action bar and the slim status bar.
//
// Presentation-only: every value is read from the pre-built ControlCenterReport
// (single runtime owner) and no value is fabricated. Absent data renders as an
// explicit N/A / NOT AVAILABLE state, never a fake zero or placeholder. The chart
// is drawn from the real closed-bar CandleSeries; when a stream has no closed
// bars the chart shows a finished "NO CANDLE DATA" state.
//
// Only ImGui's fixed-function draw list is used (flat rects, lines, text), so the
// same composition renders on OpenGL 3.3 and legacy OpenGL 2.1.

#include "desktop/AuraIcons.h"
#include "desktop/AuraTheme.h"
#include "desktop/AuraWidgets.h"
#include "desktop/CandleChartWidget.h"
#include "desktop/ControlCenterState.h"
#include "desktop/NavigationModel.h"
#include "desktop/TerminalLayout.h"

#include <imgui.h>

#include <cstddef>
#include <cstdio>
#include <string>

namespace aura {
namespace desktop {
namespace terminal {

using widgets::filled_rect;
using widgets::outlined_rect;

// ---- small helpers ---------------------------------------------------------

inline float text_w(const char* s) { return ImGui::CalcTextSize(s).x; }

// Formats a price with the terminal's fixed 3-decimal convention.
inline std::string price(double v) {
    char b[32];
    std::snprintf(b, sizeof(b), "%.3f", v);
    return std::string(b);
}

// The nine-timeframe row whose explicit identity matches (never positional).
inline const TimeframeRow* row_for(const TimeframePanel& panel, runtime::Timeframe tf) {
    for (const TimeframeRow& r : panel.rows)
        if (r.timeframe == tf) return &r;
    return nullptr;
}

// The selected chart series' last real candle close, or unavailable.
inline bool last_close(const CandleSeries& cs, double& out, std::string& when) {
    if (!cs.available || cs.candles.empty()) return false;
    out = cs.candles.back().close;
    when = format_utc_minute(cs.candles.back().close_time);
    return true;
}

// ---- Top terminal bar ------------------------------------------------------

// A compact terminal header: brand + instrument + descriptor on the left, the
// current page title, and real status on the right. Height is fixed so the chart
// keeps its space.
inline void top_bar(const ControlCenterSnapshot& s, const std::string& page_title,
                    const std::string& renderer, bool paused) {
    const float bar_h = theme::kTopBarHeight;
    ImGui::BeginChild("topbar", ImVec2(0.0f, bar_h), false, ImGuiWindowFlags_NoScrollbar);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float full_w = ImGui::GetWindowWidth();
    filled_rect(p, ImVec2(p.x + full_w, p.y + bar_h), theme::kSurfaceRaised, 0.0f);
    filled_rect(ImVec2(p.x, p.y + bar_h - 1.0f), ImVec2(p.x + full_w, p.y + bar_h), theme::kBorder,
                0.0f);

    const float line_h = ImGui::GetTextLineHeight();
    const float cy = (bar_h - line_h) * 0.5f;

    // Left: brand + instrument + descriptor.
    ImGui::SetCursorPos(ImVec2(theme::kSpace4, cy));
    theme::text_hero(theme::kAccent, "AURA");
    ImGui::SameLine(0.0f, theme::kSpace3);
    {
        const ImVec2 dp = ImGui::GetCursorScreenPos();
        const float dy = dp.y + line_h * 0.5f;
        ImGui::GetWindowDrawList()->AddLine(ImVec2(dp.x, dy - 10.0f), ImVec2(dp.x, dy + 10.0f),
                                            ImGui::GetColorU32(theme::kBorderStrong));
        ImGui::Dummy(ImVec2(1.0f, line_h));
        ImGui::SameLine(0.0f, theme::kSpace3);
        theme::text_hero(theme::kTextPrimary, "XAUUSD");
        ImGui::SameLine(0.0f, theme::kSpace2);
        ImGui::SetCursorPosY(cy + 5.0f);
        theme::text_small(theme::kTextMuted, "MARKET INTELLIGENCE");
    }

    // Centre-left: current page title (real navigation state).
    if (!page_title.empty()) {
        ImGui::SetCursorPos(ImVec2(full_w * 0.34f, cy + 1.0f));
        theme::text_title(theme::kTextSecondary, "%s", page_title.c_str());
    }

    // Right: real, right-aligned status. Widths are measured, not hard-coded.
    struct Item {
        const char* label;
        std::string value;
        ImVec4 col;
    };
    const std::string streams =
        s.overview.streams_total == 0
            ? std::string(s.overview.aggregate)
            : std::to_string(s.overview.streams_healthy) + "/" +
                  std::to_string(s.overview.streams_total);
    const std::string mode = paused ? "PAUSED" : "SHADOW ONLY";
    const Item items[] = {
        {"SYSTEM", std::string(s.overview.healthy ? "HEALTHY" : s.overview.aggregate),
         theme::status_color(s.overview.healthy ? "HEALTHY" : s.overview.aggregate)},
        {"DATA", streams, theme::status_color(s.overview.healthy ? "HEALTHY" : s.overview.aggregate)},
        {"RENDERER", renderer, theme::kTextSecondary},
    };

    float total = 0.0f;
    for (const Item& it : items) {
        if (it.label[0] != '\0') total += text_w(it.label) + 5.0f;
        total += text_w(it.value.c_str()) + theme::kSpace4;
    }
    total += text_w(mode.c_str()) + theme::kSpace3;

    float x = full_w - total - theme::kSpace4;
    if (x < full_w * 0.55f) x = full_w * 0.55f;
    ImGui::SetCursorPos(ImVec2(x, cy));
    for (const Item& it : items) {
        if (it.label[0] != '\0') {
            theme::text_small(theme::kTextMuted, "%s", it.label);
            ImGui::SameLine(0.0f, 5.0f);
        }
        theme::text_small(it.col, "%s", it.value.c_str());
        ImGui::SameLine(0.0f, theme::kSpace4);
    }
    // SHADOW ONLY / PAUSED identity pill.
    {
        const ImVec2 sp = ImGui::GetCursorScreenPos();
        const ImVec2 ts = ImGui::CalcTextSize(mode.c_str());
        const ImVec2 lo(sp.x, sp.y - 1.0f);
        const ImVec2 hi(sp.x + ts.x + theme::kSpace2, sp.y + ts.y + 1.0f);
        const ImVec4 col = paused ? theme::kPaused : theme::kShadow;
        filled_rect(lo, hi, ImVec4(col.x, col.y, col.z, 0.16f), 3.0f);
        outlined_rect(lo, hi, ImVec4(col.x, col.y, col.z, 0.55f), 3.0f);
        ImGui::SetCursorScreenPos(ImVec2(lo.x + theme::kSpace1, lo.y + 1.0f));
        theme::text_small(col, "%s", mode.c_str());
    }
    ImGui::EndChild();
}

// ---- Far-left icon rail ----------------------------------------------------

// A narrow vertical icon rail. One icon per navigation group (the group's first
// section); clicking jumps to that group's first page. The active group carries
// the accent indicator. Returns the clicked section index, or -1.
inline int icon_rail(const std::vector<NavGroup>& groups, int selected) {
    int clicked = -1;
    ImGui::BeginChild("rail", ImVec2(theme::kIconRailWidth, 0.0f), false,
                      ImGuiWindowFlags_NoScrollbar);
    const ImVec2 p0 = ImGui::GetWindowPos();
    const float w = theme::kIconRailWidth;
    const float h = ImGui::GetWindowHeight();
    filled_rect(p0, ImVec2(p0.x + w, p0.y + h), theme::kRail, 0.0f);
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p0.x + w - 1.0f, p0.y),
                                        ImVec2(p0.x + w - 1.0f, p0.y + h),
                                        ImGui::GetColorU32(theme::kBorder));

    const float tile = 36.0f;
    const float x = (w - tile) * 0.5f;

    // Brand mark: an accent tile with a drawn "A" mark (no font dependency).
    ImGui::SetCursorPos(ImVec2(x, theme::kSpace3));
    {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        filled_rect(p, ImVec2(p.x + tile, p.y + tile),
                    ImVec4(theme::kAccent.x, theme::kAccent.y, theme::kAccent.z, 0.16f), 5.0f);
        outlined_rect(p, ImVec2(p.x + tile, p.y + tile),
                      ImVec4(theme::kAccent.x, theme::kAccent.y, theme::kAccent.z, 0.5f), 5.0f);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float cx = p.x + tile * 0.5f;
        const float cy = p.y + tile * 0.5f;
        const float a = 8.0f;
        dl->AddLine(ImVec2(cx - a, cy + a), ImVec2(cx, cy - a), ImGui::GetColorU32(theme::kAccent),
                    1.8f);
        dl->AddLine(ImVec2(cx, cy - a), ImVec2(cx + a, cy + a), ImGui::GetColorU32(theme::kAccent),
                    1.8f);
        dl->AddLine(ImVec2(cx - a * 0.55f, cy + a * 0.15f), ImVec2(cx + a * 0.55f, cy + a * 0.15f),
                    ImGui::GetColorU32(theme::kAccent), 1.4f);
        ImGui::Dummy(ImVec2(tile, tile));
    }
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + theme::kSpace3);
    ImGui::Separator();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + theme::kSpace2);

    for (std::size_t gi = 0; gi < groups.size(); ++gi) {
        const NavGroup& g = groups[gi];
        if (g.items.empty()) continue;
        bool in_group = false;
        for (const NavItem& it : g.items)
            if (it.index == selected) in_group = true;
        const icons::Icon ic = icons::icon_for_section(g.items.front().section.c_str());
        ImGui::SetCursorPosX(x);
        ImGui::PushID(static_cast<int>(gi));
        if (widgets::rail_item("##rail", ic, in_group, tile)) clicked = g.items.front().index;
        ImGui::PopID();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + theme::kSpace2);
    }
    ImGui::EndChild();
    return clicked;
}

// ---- Navigation sidebar ----------------------------------------------------

// Sidebar interaction result: the clicked navigation index (-1 when none) and
// whether the operator requested a clean application exit from the footer button.
struct SidebarResult {
    int clicked = -1;
    bool exit_requested = false;
};

// `selected` is the real current page. Group titles are muted captions; rows are
// compact with an accent active bar. The EXIT control is pinned to the bottom of
// the sidebar so it stays in the lower-left corner at every window size; it only
// signals intent — the caller routes the request through the shutdown lifecycle.
inline SidebarResult sidebar(const std::vector<NavGroup>& groups, int selected, float width) {
    SidebarResult result;
    ImGui::BeginChild("sidebar", ImVec2(width, 0.0f), false);
    ImGui::SetCursorPos(ImVec2(theme::kSpace3, theme::kSpace3));
    theme::text_label(theme::kTextMuted, "NAVIGATION");
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + theme::kSpace1);
    ImGui::Separator();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + theme::kSpace2);

    // Tight vertical rhythm so all navigation groups fit at 720p without a
    // scrollbar; rows are compact by design (this is chrome, not content).
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 2.0f));
    const float nav_w = ImGui::GetContentRegionAvail().x;
    for (std::size_t gi = 0; gi < groups.size(); ++gi) {
        const NavGroup& g = groups[gi];
        if (gi != 0) ImGui::SetCursorPosY(ImGui::GetCursorPosY() + theme::kSpace1 + 2.0f);
        ImGui::SetCursorPosX(theme::kSpace3);
        theme::text_label(theme::kTextMuted, "%s", g.title.c_str());
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1.0f);
        for (const NavItem& item : g.items) {
            ImGui::SetCursorPosX(theme::kSpace2);
            if (widgets::nav_item(item.label.c_str(), icons::icon_for_section(item.section.c_str()),
                                  selected == item.index, nav_w))
                result.clicked = item.index;
        }
    }
    ImGui::PopStyleVar();

    // Bottom-anchored EXIT control (lower-left corner of the application).
    constexpr float kExitH = 36.0f;
    const float footer_h = kExitH + theme::kSpace3 * 2.0f;
    const float exit_y = ImGui::GetWindowHeight() - footer_h;
    if (exit_y > ImGui::GetCursorPosY()) {
        ImGui::SetCursorPosX(theme::kSpace2);
        ImGui::SetCursorPosY(exit_y + theme::kSpace3);
        if (widgets::exit_button("EXIT", width - theme::kSpace2 * 2.0f, kExitH))
            result.exit_requested = true;
    }
    ImGui::EndChild();
    return result;
}

// ---- Timeframe tab strip ---------------------------------------------------

// Terminal-style timeframe tabs. `sel` is the presentation selection state.
// Clicking a tab switches the chart stream (explicit timeframe identity) and
// returns true. The active tab has an accent underline and brighter text.
inline bool timeframe_tabs(TimeframeSelection& sel, float height = 32.0f) {
    bool changed = false;
    const std::vector<runtime::Timeframe>& tfs = chart_timeframes();
    const float tab_w = 52.0f;
    const float y0 = ImGui::GetCursorScreenPos().y;
    const float x_start = ImGui::GetCursorScreenPos().x;
    ImDrawList* dl = ImGui::GetWindowDrawList();

    for (std::size_t i = 0; i < tfs.size(); ++i) {
        const runtime::Timeframe tf = tfs[i];
        const std::string label(runtime::to_string(tf));
        const bool active = sel.is_selected(tf);
        ImGui::PushID(static_cast<int>(i));
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const bool clicked = ImGui::InvisibleButton("##tf", ImVec2(tab_w, height));
        const bool hovered = ImGui::IsItemHovered();
        if (active) {
            dl->AddRectFilled(p, ImVec2(p.x + tab_w, p.y + height),
                              ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.05f)), 3.0f);
            dl->AddRectFilled(ImVec2(p.x, p.y + height - 2.0f), ImVec2(p.x + tab_w, p.y + height),
                              ImGui::GetColorU32(theme::kAccent), 0.0f);
        } else if (hovered) {
            dl->AddRectFilled(p, ImVec2(p.x + tab_w, p.y + height),
                              ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.03f)), 3.0f);
        }
        ImGui::PushFont(theme::body_font());
        const ImVec2 ts = ImGui::CalcTextSize(label.c_str());
        dl->AddText(ImVec2(p.x + (tab_w - ts.x) * 0.5f, p.y + (height - ts.y) * 0.5f),
                    ImGui::GetColorU32(active   ? theme::kTextPrimary
                                       : hovered ? theme::kTextSecondary
                                                 : theme::kTextMuted),
                    label.c_str());
        ImGui::PopFont();
        ImGui::PopID();
        if (clicked && sel.select(tf)) changed = true;
        if (i + 1 < tfs.size()) ImGui::SameLine(0.0f, layout::Chrome::kTabGap);
    }
    const float x_end = ImGui::GetCursorScreenPos().x;
    dl->AddLine(ImVec2(x_start, y0 + height + 1.0f), ImVec2(x_end, y0 + height + 1.0f),
                ImGui::GetColorU32(theme::kBorder));
    return changed;
}

// ---- Instrument (market) header --------------------------------------------

// Compact instrument header above the chart: identity, selected timeframe, real
// last closed price/time and the selected stream's real state/quality/bars.
inline void market_header(const ControlCenterReport& r) {
    const ControlCenterSnapshot& s = r.snapshot;
    const CandleSeries& cs = r.chart;
    const TimeframeRow* row = row_for(s.timeframes, cs.timeframe);

    const float avail = ImGui::GetContentRegionAvail().x;
    const float h = 56.0f;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    filled_rect(p, ImVec2(p.x + avail, p.y + h), theme::kSurface, 3.0f);
    outlined_rect(p, ImVec2(p.x + avail, p.y + h), theme::kBorder, 3.0f);

    ImGui::SetCursorScreenPos(ImVec2(p.x + theme::kSpace3, p.y + theme::kSpace2));
    theme::text_hero(theme::kTextPrimary, "XAUUSD");
    ImGui::SameLine(0.0f, theme::kSpace2);
    {
        const ImVec2 bp = ImGui::GetCursorScreenPos();
        const std::string tf(cs.label);
        const ImVec2 ts = ImGui::CalcTextSize(tf.c_str());
        const ImVec2 lo(bp.x, bp.y + 3.0f);
        const ImVec2 hi(bp.x + ts.x + theme::kSpace2, bp.y + ts.y + 5.0f);
        filled_rect(lo, hi, ImVec4(theme::kAccent.x, theme::kAccent.y, theme::kAccent.z, 0.15f), 3.0f);
        outlined_rect(lo, hi, ImVec4(theme::kAccent.x, theme::kAccent.y, theme::kAccent.z, 0.5f), 3.0f);
        ImGui::SetCursorScreenPos(ImVec2(lo.x + theme::kSpace1, lo.y + 1.0f));
        theme::text_body(theme::kAccent, "%s", tf.c_str());
    }
    ImGui::SetCursorScreenPos(ImVec2(p.x + theme::kSpace3, p.y + 33.0f));
    theme::text_label(theme::kTextMuted, "MARKET INTELLIGENCE");

    double close_v = 0.0;
    std::string close_t;
    const bool have_close = last_close(cs, close_v, close_t);
    const bool have_row = row != nullptr && row->present;
    const std::string stream_state = have_row ? row_health(*row) : std::string("NOT AVAILABLE");
    const std::string quality = (have_row && row->quality != "NOT AVAILABLE")
                                    ? row->quality
                                    : std::string("NOT AVAILABLE");
    const std::string bars =
        have_row && row->sequence.available ? row->sequence.value : std::string("N/A");

    {
        const float col_w = 150.0f;
        float x = p.x + avail - col_w - theme::kSpace3;
        ImGui::SetCursorScreenPos(ImVec2(x, p.y + theme::kSpace2));
        theme::text_label(theme::kTextMuted, "LAST CLOSED");
        ImGui::SetCursorScreenPos(ImVec2(x, p.y + theme::kSpace2 + 14.0f));
        if (have_close)
            theme::text_mono(theme::kTextPrimary, "%s", price(close_v).c_str());
        else
            theme::text_mono(theme::kNotAvailable, "N/A");
        ImGui::SetCursorScreenPos(ImVec2(x, p.y + theme::kSpace2 + 33.0f));
        theme::text_label(theme::kTextMuted, "%s", have_close ? close_t.c_str() : "\u2014");

        const float x2 = x - 240.0f;
        ImGui::SetCursorScreenPos(ImVec2(x2, p.y + theme::kSpace2));
        theme::text_label(theme::kTextMuted, "STREAM");
        ImGui::SameLine(0.0f, theme::kSpace3);
        theme::text_label(theme::kTextMuted, "QUALITY");
        ImGui::SameLine(0.0f, theme::kSpace3);
        theme::text_label(theme::kTextMuted, "BARS");
        ImGui::SetCursorScreenPos(ImVec2(x2, p.y + theme::kSpace2 + 16.0f));
        theme::text_body(theme::status_color(stream_state), "%s", stream_state.c_str());
        ImGui::SameLine(0.0f, theme::kSpace3);
        theme::text_body(theme::status_color(quality), "%s", quality.c_str());
        ImGui::SameLine(0.0f, theme::kSpace3);
        theme::text_mono(theme::kTextPrimary, "%s", bars.c_str());
    }

    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + theme::kSpace2));
}

// ---- Metric strip ----------------------------------------------------------

// A compact row of five information modules (SYSTEM HEALTH / DATA STREAMS /
// SIGNALS / RISK / EXECUTION MODE). Only real values; unavailable values are an
// explicit N/A, never a fake zero. SCORE/CONFIDENCE ride on the SIGNALS card
// caption and the realized success rate lives in the Shadow Positions panel, so
// no real metric is dropped from the surface.
inline void metric_strip(const ControlCenterReport& r) {
    const ControlCenterSnapshot& s = r.snapshot;

    char score_buf[32] = {0};
    if (s.signal.score_available && s.signal.confidence_available)
        std::snprintf(score_buf, sizeof(score_buf), "score %.2f \u00b7 conf %.2f", s.signal.score,
                      s.signal.confidence);
    else if (s.signal.score_available)
        std::snprintf(score_buf, sizeof(score_buf), "score %.2f", s.signal.score);

    const std::string streams =
        s.overview.streams_total == 0
            ? std::string(s.overview.aggregate)
            : std::to_string(s.overview.streams_healthy) + " / " +
                  std::to_string(s.overview.streams_total);

    struct Cell {
        const char* label;
        std::string value;
        bool available;
        const char* reason;
        const char* caption;
        ImVec4 col;
    };
    const Cell cells[] = {
        {"SYSTEM HEALTH", std::string(s.overview.healthy ? "HEALTHY" : s.overview.aggregate), true,
         "", s.overview.healthy ? "Operational" : "degraded",
         theme::status_color(s.overview.healthy ? "HEALTHY" : s.overview.aggregate)},
        {"DATA STREAMS", streams, true, "",
         s.overview.streams_total == 0 ? "no streams" : "closed-bar feeds", theme::kTextPrimary},
        {"SIGNALS", s.signal.available ? s.signal.direction : std::string(), s.signal.available,
         "no signal", score_buf[0] != '\0' ? score_buf : "shadow",
         theme::status_color(s.signal.direction)},
        {"RISK", s.risk.available ? s.risk.direction : std::string(), s.risk.available,
         "no proposal", s.risk.is_order ? "order flag" : "no live order", theme::kAccent},
        {"EXECUTION MODE", s.shadow_only ? "SHADOW ONLY" : "VIOLATED", true, "",
         s.shadow_only ? "no orders" : "live path", theme::kShadow},
    };
    const int n = static_cast<int>(sizeof(cells) / sizeof(cells[0]));
    const float h = layout::Chrome::kMetricStrip;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float avail = ImGui::GetContentRegionAvail().x;
    filled_rect(p, ImVec2(p.x + avail, p.y + h), theme::kSurface, theme::kPanelRadius);
    outlined_rect(p, ImVec2(p.x + avail, p.y + h), theme::kBorder, theme::kPanelRadius);

    const float col_w = avail / static_cast<float>(n);
    for (int i = 0; i < n; ++i) {
        const float x = p.x + theme::kSpace3 + col_w * static_cast<float>(i);
        if (i > 0)
            ImGui::GetWindowDrawList()->AddLine(
                ImVec2(p.x + col_w * static_cast<float>(i), p.y + 10.0f),
                ImVec2(p.x + col_w * static_cast<float>(i), p.y + h - 10.0f),
                ImGui::GetColorU32(theme::kBorder));
        ImGui::SetCursorScreenPos(ImVec2(x, p.y + theme::kSpace2 + 2.0f));
        widgets::metric_module(cells[i].label, cells[i].value, cells[i].available, cells[i].col,
                               cells[i].available ? cells[i].caption : cells[i].reason);
    }
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + theme::kSpace2));
}

// ---- Lower information modules ---------------------------------------------

// One compact lower module: a caption, a hairline and a real value (or an
// explicit NOT AVAILABLE when the plane is not wired).
inline void lower_cell(const char* id, const char* title, bool available,
                       const std::string& value, const char* unit, float width) {
    const float h = layout::Chrome::kLowerModules;
    ImGui::BeginChild(id, ImVec2(width, h), true);
    theme::text_label(theme::kTextMuted, "%s", title);
    ImGui::Separator();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + theme::kSpace1);
    if (available) {
        theme::text_title(theme::kAccent, "%s", value.c_str());
        if (unit != nullptr && unit[0] != '\0') theme::text_small(theme::kTextMuted, "%s", unit);
    } else {
        theme::text_title(theme::kNotAvailable, "N/A");
        if (unit != nullptr && unit[0] != '\0') theme::text_small(theme::kTextMuted, "%s", unit);
    }
    ImGui::EndChild();
}

// A compact six-module row: Research / Knowledge / Candidates / Validation /
// Approval Center / Schedule. Real values where wired, NOT AVAILABLE otherwise.
inline void lower_modules(const ControlCenterReport& r) {
    const float gap = theme::kSpace3;
    const float avail = ImGui::GetContentRegionAvail().x;
    const float w = (avail - gap * 5.0f) / 6.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::BeginGroup();
    lower_cell("lm_research", "RESEARCH", r.research.available,
               std::to_string(r.research.total), "experiments", w);
    ImGui::SameLine(0.0f, gap);
    lower_cell("lm_knowledge", "KNOWLEDGE", r.knowledge.available,
               std::to_string(r.knowledge.identities), "objects", w);
    ImGui::SameLine(0.0f, gap);
    lower_cell("lm_candidates", "CANDIDATES", r.candidates.available,
               std::to_string(r.candidates.population), "population", w);
    ImGui::SameLine(0.0f, gap);
    lower_cell("lm_validation", "VALIDATION", r.validation.available,
               std::to_string(r.validation.campaigns), "no campaign", w);
    ImGui::SameLine(0.0f, gap);
    lower_cell("lm_approvals", "APPROVAL CENTER", r.approvals.available,
               std::to_string(r.approvals.total), "decisions", w);
    ImGui::SameLine(0.0f, gap);
    lower_cell("lm_schedule", "SCHEDULE", r.schedule.available, r.schedule.window_phase,
               "no window", w);
    ImGui::EndGroup();
    ImGui::PopStyleVar();
}

// ---- Analytics column ------------------------------------------------------

// The right-hand analytics column: Signals, Risk and Shadow Positions. Compact,
// professionally aligned, real values only. Deliberately narrower than the chart.
inline void analytics_column(const ControlCenterReport& r, float width, float height) {
    const ControlCenterSnapshot& s = r.snapshot;
    ImGui::BeginChild("analytics", ImVec2(width, height), false);

    const float panel_h = (height - theme::kSpace3 * 2.0f) / 3.0f;

    // ---- SIGNALS ----
    widgets::panel_begin("an_signals", "SIGNALS", icons::Icon::Signals, ImVec2(width, 0.0f), panel_h,
                         true);
    if (!s.signal.available) {
        widgets::empty_state("Signal", "no signal produced yet");
    } else {
        const ImVec4 dir_col =
            s.signal.direction == runtime::to_string(runtime::SignalDirection::SHORT)
                ? theme::kCritical
                : theme::kHealthy;
        widgets::badge(s.signal.direction.c_str(), dir_col);
        ImGui::SameLine(0.0f, theme::kSpace2);
        theme::text_small(theme::kTextSecondary, "on %s", s.signal.trigger_timeframe.c_str());
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + theme::kSpace1);
        widgets::kv_row("Family", s.signal.family, 92.0f);
        widgets::kv_row("Symbol", s.signal.symbol, 92.0f);
        widgets::kv_row("Closed bar", s.signal.closed_bar, 92.0f);
        if (s.signal.score_available) {
            char b[16];
            std::snprintf(b, sizeof(b), "%.2f", s.signal.score);
            widgets::kv_row("Score", b, 92.0f);
        }
        if (s.signal.confidence_available) {
            char b[16];
            std::snprintf(b, sizeof(b), "%.2f", s.signal.confidence);
            widgets::kv_row("Confidence", b, 92.0f);
        }
    }
    widgets::panel_end();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + theme::kSpace2);

    // ---- RISK ----
    widgets::panel_begin("an_risk", "RISK", icons::Icon::Risk, ImVec2(width, 0.0f), panel_h, true);
    if (!s.risk.available) {
        widgets::empty_state("Risk proposal", "no proposal yet");
    } else {
        const ImVec4 dir_col =
            s.risk.direction == runtime::to_string(runtime::SignalDirection::SHORT)
                ? theme::kCritical
                : theme::kHealthy;
        widgets::badge(s.risk.direction.c_str(), dir_col);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + theme::kSpace1);
        char b[32];
        std::snprintf(b, sizeof(b), "%.3f", s.risk.position_size);
        widgets::kv_row("Size", b, 92.0f);
        std::snprintf(b, sizeof(b), "%.3f", s.risk.stop_distance);
        widgets::kv_row("Stop dist", b, 92.0f);
        std::snprintf(b, sizeof(b), "%.3f", s.risk.atr);
        widgets::kv_row("ATR", b, 92.0f);
        widgets::kv_state_row("Live order", s.risk.is_order ? "YES" : "NO", 92.0f);
    }
    widgets::panel_end();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + theme::kSpace2);

    // ---- SHADOW POSITIONS ----
    widgets::panel_begin("an_positions", "SHADOW POSITIONS", icons::Icon::Positions,
                         ImVec2(width, 0.0f), panel_h, true);
    widgets::badge(s.positions.has_open_position ? "OPEN" : "NONE",
                   theme::status_color(s.positions.has_open_position ? "DEGRADED" : "NONE"));
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + theme::kSpace1);
    widgets::kv_row("Ledger", std::to_string(s.positions.ledger_entries), 92.0f);
    widgets::kv_row("Fills", std::to_string(s.positions.shadow_fills), 92.0f);
    widgets::kv_row("Opened", std::to_string(s.positions.positions_opened), 92.0f);
    if (s.positions.positions_closed > 0) {
        char b[40];
        std::snprintf(b, sizeof(b), "%llu", (unsigned long long)s.positions.positions_closed);
        widgets::kv_row("Closed", b, 92.0f);
        std::snprintf(b, sizeof(b), "%.0f%%",
                      static_cast<double>(s.positions.wins) /
                          static_cast<double>(s.positions.positions_closed) * 100.0);
        widgets::kv_row("Success",
                        std::string(b) + " (" + std::to_string(s.positions.wins) + " up)", 92.0f);
    }
    ImGui::Spacing();
    theme::text_label(theme::kShadow, "SHADOW ONLY \u00b7 simulated fills");
    widgets::panel_end();

    ImGui::EndChild();
}

// ---- Action bar ------------------------------------------------------------

// A compact action/control bar with only the legitimate control-plane operations.
// Returns a bitmask of requested actions. There is deliberately no order/buy/sell
// control here and none is reachable from the UI.
enum ActionFlags : unsigned {
    kActionNone = 0,
    kActionRefresh = 1u << 0,
    kActionCheckpoint = 1u << 1,
    kActionPauseToggle = 1u << 2,
    kActionStop = 1u << 3,
    kActionRecovery = 1u << 4,
};

inline unsigned action_bar(ControlCenterState& state, const ControlCenterReport& r) {
    unsigned actions = kActionNone;
    const float bar_h = theme::kActionBarHeight;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    filled_rect(p, ImVec2(p.x + w, p.y + bar_h), theme::kSurfaceRaised, 0.0f);
    filled_rect(ImVec2(p.x, p.y), ImVec2(p.x + w, p.y + 1.0f), theme::kBorder, 0.0f);

    ImGui::SetCursorScreenPos(ImVec2(p.x + theme::kSpace3, p.y + 6.0f));
    if (widgets::action_button("REFRESH")) actions |= kActionRefresh;
    ImGui::SameLine(0.0f, theme::kSpace2);
    if (widgets::action_button("CHECKPOINT")) actions |= kActionCheckpoint;
    ImGui::SameLine(0.0f, theme::kSpace2);
    if (widgets::action_button(state.paused() ? "RESUME" : "PAUSE", state.paused()))
        actions |= kActionPauseToggle;
    ImGui::SameLine(0.0f, theme::kSpace2);
    if (widgets::action_button("RECOVERY")) actions |= kActionRecovery;
    ImGui::SameLine(0.0f, theme::kSpace2);
    if (widgets::action_button("STOP", false, 72.0f)) actions |= kActionStop;

    // Right: real transport/mode context, not a console sentence.
    const ControlCenterSnapshot& s = r.snapshot;
    const std::string ctx = std::string("TRANSPORT ") +
                            (s.overview.transport_connected ? "UP" : "LOCAL") + "   \u00b7   " +
                            (state.paused() ? "PAUSED" : "RUNNING");
    const float tw = text_w(ctx.c_str());
    ImGui::SetCursorScreenPos(ImVec2(p.x + w - tw - theme::kSpace3, p.y + bar_h * 0.5f - 7.0f));
    theme::text_small(theme::kTextMuted, "%s", ctx.c_str());
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + bar_h));
    return actions;
}

// ---- Status bar ------------------------------------------------------------

// A very slim footer with real runtime state only, presented as compact items
// rather than a console sentence.
inline void status_bar(ControlCenterState& state, const ControlCenterReport& r,
                       const std::string& renderer) {
    const float status_h = theme::kStatusBarHeight;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    filled_rect(p, ImVec2(p.x + w, p.y + status_h), theme::kSurfaceRaised, 0.0f);
    filled_rect(ImVec2(p.x, p.y), ImVec2(p.x + w, p.y + 1.0f), theme::kBorder, 0.0f);
    ImGui::SetCursorPos(ImVec2(theme::kSpace4, (status_h - ImGui::GetTextLineHeight()) * 0.5f));
    const ControlCenterSnapshot& s = r.snapshot;

    struct Item {
        const char* label;
        std::string value;
        ImVec4 col;
    };
    const Item items[] = {
        {"DATA", s.overview.aggregate, theme::status_color(s.overview.aggregate)},
        {"PERSIST", s.persistence.last_status, theme::status_color(s.persistence.last_status)},
        {"RECOVERY", s.persistence.lifecycle, theme::status_color(s.persistence.lifecycle)},
        {"RUNTIME", state.paused() ? "PAUSED" : "RUNNING",
         state.paused() ? theme::kPaused : theme::kHealthy},
        {"RENDERER", renderer, theme::kTextSecondary},
    };
    for (const Item& it : items) {
        theme::text_label(theme::kTextMuted, "%s", it.label);
        ImGui::SameLine(0.0f, 5.0f);
        theme::text_small(it.col, "%s", it.value.c_str());
        ImGui::SameLine(0.0f, theme::kSpace4);
    }
    const std::string mode = "SHADOW ONLY";
    ImGui::SameLine();
    ImGui::SetCursorPosX(w - text_w(mode.c_str()) - theme::kSpace4);
    theme::text_label(theme::kShadow, "%s", mode.c_str());
}

}  // namespace terminal
}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_AURATERMINAL_H
