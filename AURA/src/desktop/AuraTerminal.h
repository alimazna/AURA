#ifndef AURA_DESKTOP_AURATERMINAL_H
#define AURA_DESKTOP_AURATERMINAL_H

// AURA professional market terminal — shell composition.
//
// This file owns the cohesive terminal chrome and the chart-first dashboard
// composition: the top terminal bar, the navigation sidebar, the instrument
// (market) header, the timeframe tab strip, the large candlestick workspace, the
// compact metric strip, the secondary information panels and the slim status bar.
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

// A compact terminal header: brand + instrument + descriptor on the left, real
// status on the right. Height is fixed so the chart keeps its space.
inline void top_bar(const ControlCenterSnapshot& s, const std::string& tf_label,
                    const std::string& renderer, bool paused) {
    const float bar_h = theme::kTopBarHeight;
    ImGui::BeginChild("topbar", ImVec2(0.0f, bar_h), false, ImGuiWindowFlags_NoScrollbar);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float full_w = ImGui::GetWindowWidth();
    filled_rect(p, ImVec2(p.x + full_w, p.y + bar_h), theme::kSurfaceRaised, 0.0f);
    filled_rect(ImVec2(p.x, p.y + bar_h - 1.0f), ImVec2(p.x + full_w, p.y + bar_h), theme::kBorder, 0.0f);

    const float line_h = ImGui::GetTextLineHeight();
    const float cy = (bar_h - line_h) * 0.5f;

    // Left: brand.
    ImGui::SetCursorPos(ImVec2(theme::kSpace4, cy));
    theme::text_hero(theme::kAccent, "AURA");
    ImGui::SameLine(0.0f, theme::kSpace3);
    {
        const ImVec2 dp = ImGui::GetCursorScreenPos();
        const float dy = dp.y + line_h * 0.5f;
        ImGui::GetWindowDrawList()->AddLine(ImVec2(dp.x, dy - 9.0f), ImVec2(dp.x, dy + 9.0f),
                                            ImGui::GetColorU32(theme::kBorderStrong));
        ImGui::Dummy(ImVec2(1.0f, line_h));
        ImGui::SameLine(0.0f, theme::kSpace3);
        theme::text_hero(theme::kTextPrimary, "XAUUSD");
        ImGui::SameLine(0.0f, theme::kSpace3);
        ImGui::SetCursorPosY(cy + 4.0f);
        theme::text_small(theme::kTextMuted, "MARKET INTELLIGENCE");
    }

    // Right: real, right-aligned status. Widths are measured, not hard-coded.
    struct Item {
        const char* label;
        std::string value;
        ImVec4 col;
    };
    const std::string market =
        s.overview.streams_total == 0
            ? std::string(s.overview.aggregate)
            : std::to_string(s.overview.streams_healthy) + "/" +
                  std::to_string(s.overview.streams_total);
    const std::string mode = paused ? "PAUSED" : "SHADOW ONLY";
    const Item items[] = {
        {"RENDERER", renderer, theme::kTextSecondary},
        {"DATA", market + (s.overview.healthy ? " ONLINE" : " " + s.overview.aggregate),
         theme::status_color(s.overview.healthy ? "HEALTHY" : s.overview.aggregate)},
        {"TF", tf_label, theme::kAccent},
    };

    // Sum widths.
    float total = 0.0f;
    for (const Item& it : items) {
        total += text_w(it.label) + 5.0f + text_w(it.value.c_str()) + theme::kSpace4;
    }
    total += text_w(mode.c_str()) + theme::kSpace3;

    float x = full_w - total - theme::kSpace4;
    if (x < full_w * 0.42f) x = full_w * 0.42f;
    ImGui::SetCursorPos(ImVec2(x, cy));
    for (const Item& it : items) {
        theme::text_small(theme::kTextMuted, "%s", it.label);
        ImGui::SameLine(0.0f, 5.0f);
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

// ---- Navigation sidebar ----------------------------------------------------

// Returns the clicked section index, or -1. `selected` is the real current page.
inline int sidebar(const std::vector<NavGroup>& groups, int selected, float width) {
    int clicked = -1;
    ImGui::BeginChild("sidebar", ImVec2(width, 0.0f), true);
    const float nav_w = ImGui::GetContentRegionAvail().x;
    for (std::size_t gi = 0; gi < groups.size(); ++gi) {
        const NavGroup& g = groups[gi];
        if (gi != 0) ImGui::Spacing();
        ImGui::SetCursorPosX(theme::kSpace3);
        theme::text_small(theme::kTextMuted, "%s", g.title.c_str());
        ImGui::Spacing();
        for (const NavItem& item : g.items) {
            if (widgets::nav_item(item.label.c_str(), icons::icon_for_section(item.section.c_str()),
                                  selected == item.index, nav_w))
                clicked = item.index;
        }
    }
    ImGui::EndChild();
    return clicked;
}

// ---- Timeframe tab strip ---------------------------------------------------

// Terminal-style timeframe tabs. `sel` is the presentation selection state.
// Clicking a tab switches the chart stream (explicit timeframe identity) and
// returns true. The active tab has an accent underline and brighter text.
inline bool timeframe_tabs(TimeframeSelection& sel, float height = 30.0f) {
    bool changed = false;
    const std::vector<runtime::Timeframe>& tfs = chart_timeframes();
    const float tab_w = 50.0f;
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
                              ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.045f)), 3.0f);
            dl->AddRectFilled(ImVec2(p.x, p.y + height - 2.0f), ImVec2(p.x + tab_w, p.y + height),
                              ImGui::GetColorU32(theme::kAccent), 0.0f);
        } else if (hovered) {
            dl->AddRectFilled(p, ImVec2(p.x + tab_w, p.y + height),
                              ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.025f)), 3.0f);
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
        if (i + 1 < tfs.size()) ImGui::SameLine(0.0f, theme::kSpace1);
    }
    // A single hairline baseline under the whole strip ties the tabs together.
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
    const float h = 58.0f;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    filled_rect(p, ImVec2(p.x + avail, p.y + h), theme::kSurface, 3.0f);
    outlined_rect(p, ImVec2(p.x + avail, p.y + h), theme::kBorder, 3.0f);

    // Left: instrument + timeframe badge. (The terminal identity is stated once,
    // in the top bar; the header carries only the instrument and its real state.)
    ImGui::SetCursorScreenPos(ImVec2(p.x + theme::kSpace4, p.y + theme::kSpace2 + 4.0f));
    theme::text_hero(theme::kTextPrimary, "XAUUSD");
    ImGui::SameLine(0.0f, theme::kSpace3);
    {
        const ImVec2 bp = ImGui::GetCursorScreenPos();
        const std::string tf(cs.label);
        const ImVec2 ts = ImGui::CalcTextSize(tf.c_str());
        const ImVec2 lo(bp.x, bp.y + 2.0f);
        const ImVec2 hi(bp.x + ts.x + theme::kSpace2, bp.y + ts.y + 4.0f);
        filled_rect(lo, hi, ImVec4(theme::kAccent.x, theme::kAccent.y, theme::kAccent.z, 0.15f), 3.0f);
        outlined_rect(lo, hi, ImVec4(theme::kAccent.x, theme::kAccent.y, theme::kAccent.z, 0.5f), 3.0f);
        ImGui::SetCursorScreenPos(ImVec2(lo.x + theme::kSpace1, lo.y + 1.0f));
        theme::text_body(theme::kAccent, "%s", tf.c_str());
    }
    ImGui::SetCursorScreenPos(ImVec2(p.x + theme::kSpace4, p.y + 34.0f));
    theme::text_small(theme::kTextMuted, "MARKET INTELLIGENCE");

    // Right: real price + stream state, right-aligned in measured columns.
    double close_v = 0.0;
    std::string close_t;
    const bool have_close = last_close(cs, close_v, close_t);
    const bool have_row = row != nullptr && row->present;
    const std::string stream_state =
        have_row ? row_health(*row) : std::string("NOT AVAILABLE");
    const std::string quality = (have_row && row->quality != "NOT AVAILABLE")
                                    ? row->quality
                                    : std::string("NOT AVAILABLE");
    const std::string bars = have_row && row->sequence.available ? row->sequence.value
                                                                 : std::string("N/A");

    // The price readout.
    {
        const float col_w = 150.0f;
        float x = p.x + avail - col_w - theme::kSpace4;
        ImGui::SetCursorScreenPos(ImVec2(x, p.y + theme::kSpace2));
        theme::text_small(theme::kTextMuted, "LAST CLOSED");
        ImGui::SetCursorScreenPos(ImVec2(x, p.y + theme::kSpace2 + 15.0f));
        if (have_close)
            theme::text_title(theme::kTextPrimary, "%s", price(close_v).c_str());
        else
            theme::text_title(theme::kNotAvailable, "N/A");
        ImGui::SetCursorScreenPos(ImVec2(x, p.y + theme::kSpace2 + 34.0f));
        theme::text_small(theme::kTextMuted, "%s", have_close ? close_t.c_str() : "\u2014");

        // Stream / quality / bars to the left of the price.
        const float x2 = x - 250.0f;
        ImGui::SetCursorScreenPos(ImVec2(x2, p.y + theme::kSpace2 + 2.0f));
        theme::text_small(theme::kTextMuted, "MARKET STREAM");
        ImGui::SameLine(0.0f, theme::kSpace3);
        theme::text_small(theme::kTextMuted, "QUALITY");
        ImGui::SameLine(0.0f, theme::kSpace3);
        theme::text_small(theme::kTextMuted, "BARS");
        ImGui::SetCursorScreenPos(ImVec2(x2, p.y + theme::kSpace2 + 20.0f));
        theme::text_body(theme::status_color(stream_state), "%s", stream_state.c_str());
        ImGui::SameLine(0.0f, theme::kSpace3);
        theme::text_body(theme::status_color(quality), "%s", quality.c_str());
        ImGui::SameLine(0.0f, theme::kSpace3);
        theme::text_body(theme::kTextPrimary, "%s", bars.c_str());
    }

    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + theme::kSpace2));
}

// ---- Metric strip ----------------------------------------------------------

// A compact single-row metric strip (not five giant cards). Only real values.
inline void metric_strip(const ControlCenterReport& r) {
    const ControlCenterSnapshot& s = r.snapshot;
    char score_buf[16] = {0};
    char conf_buf[16] = {0};
    if (s.signal.score_available)
        std::snprintf(score_buf, sizeof(score_buf), "%.2f", s.signal.score);
    if (s.signal.confidence_available)
        std::snprintf(conf_buf, sizeof(conf_buf), "%.2f", s.signal.confidence);
    char rate_buf[16] = {0};
    if (s.signal.outcomes_available)
        std::snprintf(rate_buf, sizeof(rate_buf), "%.0f%%",
                      static_cast<double>(s.signal.wins) /
                          static_cast<double>(s.signal.positions_closed) * 100.0);

    const float h = 62.0f;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float avail = ImGui::GetContentRegionAvail().x;
    filled_rect(p, ImVec2(p.x + avail, p.y + h), theme::kSurface, 3.0f);
    outlined_rect(p, ImVec2(p.x + avail, p.y + h), theme::kBorder, 3.0f);

    const TimeframeRow* row = row_for(s.timeframes, r.chart.timeframe);
    const std::string freshness = (row != nullptr && row->present) ? row->freshness
                                                                   : std::string("NOT AVAILABLE");

    struct Cell {
        const char* label;
        std::string value;
        bool available;
        const char* reason;
        ImVec4 col;
    };
    const Cell cells[] = {
        {"SCORE", score_buf, s.signal.score_available, "no signal", theme::kAccent},
        {"CONFIDENCE", conf_buf, s.signal.confidence_available, "no signal", theme::kHealthy},
        {"REALIZED SUCCESS", rate_buf, s.signal.outcomes_available, "no closed positions",
         theme::kHealthy},
        {"DATA", freshness, row != nullptr && row->present, "stream silent", theme::kTextPrimary},
        {"RISK", s.risk.available ? "PROPOSED" : "", s.risk.available, "none yet",
         theme::status_color(s.risk.direction)},
        {"MODE", s.shadow_only ? "SHADOW" : "VIOLATED", true, "", theme::kShadow},
    };
    const int n = static_cast<int>(sizeof(cells) / sizeof(cells[0]));
    const float col_w = avail / static_cast<float>(n);
    for (int i = 0; i < n; ++i) {
        const float x = p.x + theme::kSpace4 + col_w * static_cast<float>(i);
        if (i > 0)
            ImGui::GetWindowDrawList()->AddLine(
                ImVec2(p.x + col_w * static_cast<float>(i), p.y + 10.0f),
                ImVec2(p.x + col_w * static_cast<float>(i), p.y + h - 10.0f),
                ImGui::GetColorU32(theme::kBorder));
        ImGui::SetCursorScreenPos(ImVec2(x, p.y + theme::kSpace2 + 2.0f));
        theme::text_small(theme::kTextMuted, "%s", cells[i].label);
        ImGui::SetCursorScreenPos(ImVec2(x, p.y + theme::kSpace2 + 20.0f));
        if (cells[i].available)
            theme::text_title(cells[i].col, "%s", cells[i].value.c_str());
        else {
            theme::text_title(theme::kNotAvailable, "N/A");
            ImGui::SameLine(0.0f, theme::kSpace2);
            ImGui::SetCursorPosY(p.y + theme::kSpace2 + 24.0f);
            theme::text_small(theme::kTextMuted, "%s", cells[i].reason);
        }
    }
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + h + theme::kSpace2));
}

// ---- Lower panels ----------------------------------------------------------

// Compact sign/analytics + system/streams panels. No large empty boxes; equal
// weight is deliberately avoided (the chart above is dominant).
inline void lower_panels(const ControlCenterReport& r) {
    const ControlCenterSnapshot& s = r.snapshot;
    const float gap = theme::kSpace3;
    const float avail = ImGui::GetContentRegionAvail().x;
    const float left_w = avail * 0.46f;
    const float right_w = avail - left_w - gap;

    // Supporting panels shrink their content on shorter viewports instead of
    // pushing the work off-screen; the chart above stays the visual anchor.
    const float avail_h = ImGui::GetContentRegionAvail().y;
    const float panel_h = avail_h > 150.0f ? avail_h - theme::kSpace2 : 120.0f;

    // ---- Signals / analytics (left) ----
    widgets::panel_begin("term_signals", "SIGNAL / ANALYTICS", icons::Icon::Signals,
                         ImVec2(left_w, 0.0f), panel_h, /*scroll=*/true);
    if (!s.signal.available) {
        widgets::empty_state("No signal data", "the runtime has not produced a signal yet");
    } else {
        const ImVec4 dir_col =
            s.signal.direction == runtime::to_string(runtime::SignalDirection::SHORT)
                ? theme::kCritical
                : theme::kHealthy;
        widgets::badge(s.signal.direction.c_str(), dir_col);
        ImGui::SameLine(0.0f, theme::kSpace2);
        theme::text_body(theme::kTextSecondary, "on %s", s.signal.trigger_timeframe.c_str());
        ImGui::Spacing();
        widgets::kv_row("Strategy family", s.signal.family);
        widgets::kv_row("Symbol", s.signal.symbol);
        widgets::kv_row("Closed bar", s.signal.closed_bar);
        widgets::kv_row("Bar close (UTC)", s.signal.closed_bar_close_time.available
                                               ? s.signal.closed_bar_close_time.value
                                               : std::string("NOT AVAILABLE"));
        widgets::kv_row("Decision id", s.signal.decision_id);
    }
    widgets::panel_end();

    ImGui::SameLine(0.0f, gap);

    // ---- Data streams matrix (right) ----
    widgets::panel_begin("term_streams", "DATA STREAMS", icons::Icon::Market,
                         ImVec2(right_w, 0.0f), panel_h, /*scroll=*/true);
    if (ImGui::BeginTable("term_streams_tbl", 5,
                          ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp |
                              ImGuiTableFlags_NoBordersInBody)) {
        ImGui::TableSetupColumn("TF", ImGuiTableColumnFlags_WidthFixed, 42.0f);
        ImGui::TableSetupColumn("STATE");
        ImGui::TableSetupColumn("QUALITY");
        ImGui::TableSetupColumn("FRESH");
        ImGui::TableSetupColumn("BARS");
        for (const TimeframeRow& row : s.timeframes.rows) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            theme::text_body(theme::kTextPrimary, "%s", row.label.c_str());
            ImGui::TableNextColumn();
            if (row.present) {
                const theme::StateCategory c = theme::state_category(row_health(row));
                const ImVec2 dp = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddCircleFilled(
                    ImVec2(dp.x + 3.5f, dp.y + ImGui::GetTextLineHeight() * 0.5f), 3.0f,
                    ImGui::GetColorU32(theme::category_color(c)));
                ImGui::Dummy(ImVec2(10.0f, ImGui::GetTextLineHeight()));
                ImGui::SameLine(0.0f, 0.0f);
                theme::text_body(theme::category_color(c), "%s",
                                 theme::category_label(c));
            } else {
                theme::text_body(theme::kNotAvailable, "N/A");
            }
            ImGui::TableNextColumn();
            widgets::state_cell(row.present ? row.quality : std::string("N/A"));
            ImGui::TableNextColumn();
            widgets::state_cell(row.present ? row.freshness : std::string("N/A"));
            ImGui::TableNextColumn();
            theme::text_body(theme::kTextPrimary, "%s",
                             row.sequence.available ? row.sequence.value.c_str() : "N/A");
        }
        ImGui::EndTable();
    }
    widgets::panel_end();
}

// ---- Status bar ------------------------------------------------------------

// A very slim footer with real runtime state only.
inline void status_bar(ControlCenterState& state, const ControlCenterReport& r,
                       const std::string& renderer) {
    const float status_h = theme::kStatusBarHeight;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    filled_rect(p, ImVec2(p.x + w, p.y + status_h), theme::kSurfaceRaised, 0.0f);
    filled_rect(ImVec2(p.x, p.y), ImVec2(p.x + w, p.y + 1.0f), theme::kBorder, 0.0f);
    ImGui::SetCursorPos(ImVec2(theme::kSpace4, (status_h - ImGui::GetTextLineHeight()) * 0.5f));
    const ControlCenterSnapshot& s = r.snapshot;
    theme::text_small(theme::kTextMuted, "DATA");
    ImGui::SameLine(0.0f, 5.0f);
    theme::text_small(theme::status_color(s.overview.aggregate), "%s", s.overview.aggregate.c_str());
    ImGui::SameLine(0.0f, theme::kSpace4);
    theme::text_small(theme::kTextMuted, "PERSISTENCE");
    ImGui::SameLine(0.0f, 5.0f);
    theme::text_small(theme::status_color(s.persistence.last_status), "%s",
                      s.persistence.last_status.c_str());
    ImGui::SameLine(0.0f, theme::kSpace4);
    theme::text_small(theme::kTextMuted, "RECOVERY");
    ImGui::SameLine(0.0f, 5.0f);
    theme::text_small(theme::status_color(s.persistence.lifecycle), "%s",
                      s.persistence.lifecycle.c_str());
    ImGui::SameLine(0.0f, theme::kSpace4);
    theme::text_small(theme::kTextMuted, "RUNTIME");
    ImGui::SameLine(0.0f, 5.0f);
    theme::text_small(state.paused() ? theme::kPaused : theme::kHealthy, "%s",
                      state.paused() ? "PAUSED" : "RUNNING");
    ImGui::SameLine(0.0f, theme::kSpace4);
    theme::text_small(theme::kTextMuted, "RENDERER");
    ImGui::SameLine(0.0f, 5.0f);
    theme::text_small(theme::kTextSecondary, "%s", renderer.c_str());
    // Right-aligned shadow-only identity.
    const std::string mode = "SHADOW ONLY";
    ImGui::SameLine();
    ImGui::SetCursorPosX(w - text_w(mode.c_str()) - theme::kSpace4);
    theme::text_small(theme::kShadow, "%s", mode.c_str());
}

}  // namespace terminal
}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_AURATERMINAL_H
