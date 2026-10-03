#ifndef AURA_DESKTOP_CANDLECHARTWIDGET_H
#define AURA_DESKTOP_CANDLECHARTWIDGET_H

#include "desktop/AuraTheme.h"
#include "desktop/AuraWidgets.h"
#include "desktop/CandleChart.h"

#include <imgui.h>

#include <cstddef>
#include <string>

namespace aura {
namespace desktop {

// ImGui rendering for the XAUUSD candlestick chart and its timeframe selector.
//
// Presentation-only: it draws a CandleSeries that was projected from real closed
// bars. It reaches into no engine, mutates no state, and never synthesizes a
// candle. It uses only ImGui's fixed-function draw list (flat rectangles and
// lines), so it renders identically on the OpenGL 3.3 backend and the legacy
// OpenGL 2.1 backend.

namespace chart {

// Compact horizontal timeframe selector. `sel` is the presentation selection
// state; clicking a button switches the chart stream and returns true. The active
// timeframe is drawn with the AURA accent so the current selection is unmistakable.
inline bool timeframe_selector(TimeframeSelection& sel, float button_w = 52.0f,
                               float button_h = 0.0f) {
    bool changed = false;
    const std::vector<runtime::Timeframe>& tfs = chart_timeframes();
    for (std::size_t i = 0; i < tfs.size(); ++i) {
        const runtime::Timeframe tf = tfs[i];
        const bool active = sel.is_selected(tf);
        ImGui::PushStyleColor(ImGuiCol_Button,
                              active ? theme::kAccent : theme::kSurfaceRaised);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              active ? theme::kAccent : theme::kBorderStrong);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, theme::kAccentDim);
        ImGui::PushStyleColor(ImGuiCol_Text,
                              active ? theme::kBackground : theme::kTextSecondary);
        ImGui::PushStyleColor(ImGuiCol_Border, active ? theme::kAccent : theme::kBorder);
        if (ImGui::Button(runtime::to_string(tf).data(), ImVec2(button_w, button_h))) {
            if (sel.select(tf)) changed = true;
        }
        ImGui::PopStyleColor(5);
        if (i + 1 < tfs.size()) ImGui::SameLine(0.0f, theme::kSpace1);
    }
    return changed;
}

// The explicit NO CANDLE DATA state for a timeframe with no real closed bars.
inline void no_candle_data(const CandleSeries& series, float height) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + height),
                      ImGui::GetColorU32(ImVec4(0.043f, 0.051f, 0.071f, 1.0f)));
    dl->AddRect(p, ImVec2(p.x + w, p.y + height), ImGui::GetColorU32(theme::kBorder));

    const float cx = p.x + w * 0.5f;
    float y = p.y + height * 0.5f - 34.0f;
    const char* l1 = "NO CANDLE DATA";
    const char* l2 = "STATUS: NOT AVAILABLE";
    const std::string l3 = "TIMEFRAME: " + series.label;
    const ImVec2 a = ImGui::CalcTextSize(l1);
    dl->AddText(ImVec2(cx - a.x * 0.5f, y), ImGui::GetColorU32(theme::kTextSecondary), l1);
    y += a.y + 10.0f;
    const ImVec2 b = ImGui::CalcTextSize(l3.c_str());
    dl->AddText(ImVec2(cx - b.x * 0.5f, y), ImGui::GetColorU32(theme::kAccent), l3.c_str());
    y += b.y + 6.0f;
    const ImVec2 c = ImGui::CalcTextSize(l2);
    dl->AddText(ImVec2(cx - c.x * 0.5f, y), ImGui::GetColorU32(theme::kNotAvailable), l2);

    ImGui::Dummy(ImVec2(w, height));
}

// Draws a real OHLC candlestick chart for the given closed-bar series, with a
// price scale, a time axis and a subtle grid. Returns the series so callers can
// keep the metadata in sync; it draws nothing but real candle data.
inline void candlestick_chart(const CandleSeries& series, float height = 380.0f) {
    if (!series.available) {
        no_candle_data(series, height);
        return;
    }

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float total_w = ImGui::GetContentRegionAvail().x;
    ImGui::InvisibleButton("##aura_candle_chart", ImVec2(total_w, height));
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Plot geometry: a right-hand price gutter and a bottom time axis.
    const float gutter = 74.0f;
    const float axis_h = 24.0f;
    const float pad_top = 10.0f;
    const float plot_x0 = origin.x + 8.0f;
    const float plot_x1 = origin.x + total_w - gutter;
    const float plot_y0 = origin.y + pad_top;
    const float plot_y1 = origin.y + height - axis_h;
    const float plot_w = plot_x1 - plot_x0;
    const float plot_h = plot_y1 - plot_y0;

    // Dark chart surface, distinct from the surrounding card.
    dl->AddRectFilled(origin, ImVec2(origin.x + total_w, origin.y + height),
                      ImGui::GetColorU32(ImVec4(0.043f, 0.051f, 0.071f, 1.0f)));
    dl->AddRect(origin, ImVec2(origin.x + total_w, origin.y + height),
                ImGui::GetColorU32(theme::kBorder));

    const ChartGeometry g = build_chart_geometry(series, 5, 0.0f);

    // Subtle grid + readable price scale.
    for (std::size_t i = 0; i < g.grid_y.size(); ++i) {
        const float y = plot_y0 + g.grid_y[i] * plot_h;
        dl->AddLine(ImVec2(plot_x0, y), ImVec2(plot_x1, y),
                    ImGui::GetColorU32(ImVec4(0.180f, 0.204f, 0.251f, 0.45f)));
        char buf[24];
        std::snprintf(buf, sizeof(buf), "%.3f", g.grid_price[i]);
        dl->AddText(ImVec2(plot_x1 + 6.0f, y - ImGui::GetTextLineHeight() * 0.5f),
                    ImGui::GetColorU32(theme::kTextMuted), buf);
    }

    // Candles: wick (high-low) plus a body (open-close). Green up, red down.
    const std::size_t n = series.candles.size();
    const float slot = plot_w / static_cast<float>(n > 0 ? n : 1);
    float body_w = slot * 0.6f;
    if (body_w < 2.0f) body_w = 2.0f;
    if (body_w > 16.0f) body_w = 16.0f;
    for (std::size_t i = 0; i < n; ++i) {
        const CandleLayout& l = g.candles[i];
        const float xc = plot_x0 + l.x * plot_w;
        const ImU32 col = ImGui::GetColorU32(l.bullish ? theme::kHealthy : theme::kCritical);

        const float wy0 = plot_y0 + l.wick_top * plot_h;
        const float wy1 = plot_y0 + l.wick_bottom * plot_h;
        dl->AddLine(ImVec2(xc, wy0), ImVec2(xc, wy1), col, 1.0f);

        float by0 = plot_y0 + l.body_top * plot_h;
        float by1 = plot_y0 + l.body_bottom * plot_h;
        if (by1 - by0 < 1.0f) by1 = by0 + 1.0f;
        dl->AddRectFilled(ImVec2(xc - body_w * 0.5f, by0), ImVec2(xc + body_w * 0.5f, by1), col);
    }

    // Time axis: real close times of the first, middle and last closed bars.
    {
        const std::size_t idx[3] = {0, n / 2, n - 1};
        for (std::size_t k = 0; k < 3; ++k) {
            if (n == 0) break;
            const std::string label = format_utc_minute(series.candles[idx[k]].close_time);
            const float x = plot_x0 + g.candles[idx[k]].x * plot_w;
            const float tw = ImGui::CalcTextSize(label.c_str()).x;
            float lx = x - tw * 0.5f;
            if (lx < plot_x0) lx = plot_x0;
            if (lx > plot_x1 - tw) lx = plot_x1 - tw;
            dl->AddText(ImVec2(lx, plot_y1 + 6.0f),
                        ImGui::GetColorU32(theme::kTextMuted), label.c_str());
        }
        dl->AddLine(ImVec2(plot_x0, plot_y1), ImVec2(plot_x1, plot_y1),
                    ImGui::GetColorU32(theme::kBorderStrong));
    }

    // Last-price marker on the price gutter.
    {
        const double last = series.candles.back().close;
        const float y = plot_y0 +
                        static_cast<float>(chart_y_fraction(last, g.price_low, g.price_high)) * plot_h;
        const bool up = series.candles.back().close >= series.candles.back().open;
        const ImU32 col = ImGui::GetColorU32(up ? theme::kHealthy : theme::kCritical);
        dl->AddLine(ImVec2(plot_x0, y), ImVec2(plot_x1, y), col, 1.0f);
        char buf[24];
        std::snprintf(buf, sizeof(buf), "%.3f", last);
        const ImVec2 tp = ImVec2(plot_x1 + 6.0f, y - ImGui::GetTextLineHeight() * 0.5f);
        dl->AddText(tp, col, buf);
    }
}

}  // namespace chart
}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_CANDLECHARTWIDGET_H
