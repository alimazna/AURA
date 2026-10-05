#ifndef AURA_DESKTOP_CANDLECHARTWIDGET_H
#define AURA_DESKTOP_CANDLECHARTWIDGET_H

#include "desktop/AuraTheme.h"
#include "desktop/AuraWidgets.h"
#include "desktop/CandleChart.h"
#include "desktop/TerminalLayout.h"

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
//
// The chart is treated as a structured financial chart: a chart header, an inset
// canvas with a subtle grid, an integrated price gutter on the right, an
// integrated time axis along the bottom and an optional last-price marker.

namespace chart {

// The explicit NO CANDLE DATA state for a timeframe with no real closed bars.
// A finished, centred empty state, not a bare rectangle.
inline void no_candle_data(const CandleSeries& series, float height) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + height),
                      ImGui::GetColorU32(theme::kSurfaceInset), 3.0f);
    dl->AddRect(p, ImVec2(p.x + w, p.y + height), ImGui::GetColorU32(theme::kBorder), 3.0f);

    const float cx = p.x + w * 0.5f;
    float y = p.y + height * 0.5f - 46.0f;
    ImGui::PushFont(theme::title_font());
    const char* l1 = "NO CANDLE DATA";
    const ImVec2 a = ImGui::CalcTextSize(l1);
    dl->AddText(ImVec2(cx - a.x * 0.5f, y), ImGui::GetColorU32(theme::kTextSecondary), l1);
    ImGui::PopFont();
    y += a.y + 14.0f;
    // "XAUUSD · M15" plus the stream's canonical V3-29 authority role.
    const char* role = layout::timeframe_role(series.timeframe);
    const std::string l2 = std::string("XAUUSD \u00b7 ") + series.label.c_str() +
                           (role[0] != '\0' ? std::string("  \u00b7  ") + role : std::string());
    ImGui::PushFont(theme::small_font());
    const ImVec2 b = ImGui::CalcTextSize(l2.c_str());
    dl->AddText(ImVec2(cx - b.x * 0.5f, y), ImGui::GetColorU32(theme::kAccent), l2.c_str());
    y += b.y + 8.0f;
    const char* l3 = "MARKET STREAM  NOT AVAILABLE";
    const ImVec2 c = ImGui::CalcTextSize(l3);
    dl->AddText(ImVec2(cx - c.x * 0.5f, y), ImGui::GetColorU32(theme::kNotAvailable), l3);
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(w, height));
}

// Draws a real OHLC candlestick chart for the given closed-bar series, with an
// integrated price scale, a time axis and a subtle grid. Never draws anything
// but real candle data. `grid_lines` is the price-grid density and `dash`
// selects the dashed (institutional-style) last-price marker line.
inline void candlestick_chart(const CandleSeries& series, float height = 380.0f,
                              int grid_lines = 7, bool dash = true) {
    if (!series.available) {
        no_candle_data(series, height);
        return;
    }

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float total_w = ImGui::GetContentRegionAvail().x;
    ImGui::InvisibleButton("##aura_candle_chart", ImVec2(total_w, height));
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Chart surface: distinct inset canvas with a hairline border.
    dl->AddRectFilled(origin, ImVec2(origin.x + total_w, origin.y + height),
                      ImGui::GetColorU32(theme::kSurfaceInset), theme::kPanelRadius);
    dl->AddRect(origin, ImVec2(origin.x + total_w, origin.y + height),
                ImGui::GetColorU32(theme::kBorder), theme::kPanelRadius);

    // Plot geometry: a right-hand price gutter and a bottom time axis.
    const float gutter = 82.0f;
    const float axis_h = 24.0f;
    const float pad_top = 14.0f;
    const float pad_left = 12.0f;
    const float plot_x0 = origin.x + pad_left;
    const float plot_x1 = origin.x + total_w - gutter;
    const float plot_y0 = origin.y + pad_top;
    const float plot_y1 = origin.y + height - axis_h;
    const float plot_w = plot_x1 - plot_x0;
    const float plot_h = plot_y1 - plot_y0;

    // A faint separator between the plot and the integrated price gutter.
    dl->AddLine(ImVec2(plot_x1, plot_y0 - 2.0f), ImVec2(plot_x1, plot_y1 + 2.0f),
                ImGui::GetColorU32(theme::kBorder));

    const ChartGeometry g = build_chart_geometry(series, grid_lines, 0.0f);

    // Subtle grid + readable price scale on the integrated gutter.
    for (std::size_t i = 0; i < g.grid_y.size(); ++i) {
        const float y = plot_y0 + g.grid_y[i] * plot_h;
        dl->AddLine(ImVec2(plot_x0, y), ImVec2(plot_x1, y), ImGui::GetColorU32(theme::kGridline));
        char buf[24];
        std::snprintf(buf, sizeof(buf), "%.3f", g.grid_price[i]);
        ImGui::PushFont(theme::mono_font());
        dl->AddText(ImVec2(plot_x1 + 8.0f, y - ImGui::GetTextLineHeight() * 0.5f),
                    ImGui::GetColorU32(theme::kTextMuted), buf);
        ImGui::PopFont();
    }
    // Vertical gridlines at the labelled time positions so the canvas reads as a
    // structured chart rather than a floating bitmap.
    const std::size_t n = series.candles.size();
    const std::size_t tidx[3] = {0, n / 2, n > 0 ? n - 1 : 0};
    for (std::size_t k = 0; k < 3; ++k) {
        if (n == 0) break;
        const float x = plot_x0 + g.candles[tidx[k]].x * plot_w;
        dl->AddLine(ImVec2(x, plot_y0), ImVec2(x, plot_y1), ImGui::GetColorU32(theme::kGridline));
    }

    // Candles: wick (high-low) plus a body (open-close). Green up, red down.
    const float slot = plot_w / static_cast<float>(n > 0 ? n : 1);
    float body_w = slot * 0.66f;
    if (body_w < 2.0f) body_w = 2.0f;
    if (body_w > 22.0f) body_w = 22.0f;
    for (std::size_t i = 0; i < n; ++i) {
        const CandleLayout& l = g.candles[i];
        const float xc = plot_x0 + l.x * plot_w;
        const ImVec4 cc = l.bullish ? theme::kHealthy : theme::kCritical;
        const ImU32 col = ImGui::GetColorU32(cc);

        const float wy0 = plot_y0 + l.wick_top * plot_h;
        const float wy1 = plot_y0 + l.wick_bottom * plot_h;
        dl->AddLine(ImVec2(xc, wy0), ImVec2(xc, wy1), col, 1.2f);

        float by0 = plot_y0 + l.body_top * plot_h;
        float by1 = plot_y0 + l.body_bottom * plot_h;
        if (by1 - by0 < 1.5f) by1 = by0 + 1.5f;
        // A subtle border on the body edge reads as a solid candlestick.
        dl->AddRectFilled(ImVec2(xc - body_w * 0.5f, by0), ImVec2(xc + body_w * 0.5f, by1), col);
        dl->AddRect(ImVec2(xc - body_w * 0.5f, by0), ImVec2(xc + body_w * 0.5f, by1),
                    ImGui::GetColorU32(ImVec4(cc.x * 0.7f, cc.y * 0.7f, cc.z * 0.7f, 1.0f)));
    }

    // Time axis: real close times of the first, middle and last closed bars,
    // labelled per the selected timeframe's granularity.
    {
        ImGui::PushFont(theme::small_font());
        for (std::size_t k = 0; k < 3; ++k) {
            if (n == 0) break;
            const std::string label =
                format_axis_label(series.candles[tidx[k]].close_time, series.timeframe);
            const float x = plot_x0 + g.candles[tidx[k]].x * plot_w;
            const float tw = ImGui::CalcTextSize(label.c_str()).x;
            float lx = x - tw * 0.5f;
            if (lx < plot_x0) lx = plot_x0;
            if (lx > plot_x1 - tw) lx = plot_x1 - tw;
            dl->AddText(ImVec2(lx, plot_y1 + 6.0f), ImGui::GetColorU32(theme::kTextMuted),
                        label.c_str());
        }
        ImGui::PopFont();
        dl->AddLine(ImVec2(plot_x0, plot_y1), ImVec2(plot_x1, plot_y1),
                    ImGui::GetColorU32(theme::kBorderStrong));
    }

    // Last-price marker on the integrated price gutter: a labelled tag on the
    // right edge, coloured by the last candle's direction.
    {
        const Candle& last = series.candles.back();
        const float y = plot_y0 +
                        static_cast<float>(chart_y_fraction(last.close, g.price_low, g.price_high)) *
                            plot_h;
        const bool up = last.close >= last.open;
        const ImVec4 cc = up ? theme::kHealthy : theme::kCritical;
        const ImU32 col = ImGui::GetColorU32(cc);
        // Dashed last-price line (2 px on / 3 px off) reads as a marker, not a
        // gridline; when the range is too flat the line is still drawn.
        if (dash) {
            float seg = plot_x0;
            while (seg < plot_x1) {
                const float end = (seg + 2.0f < plot_x1) ? seg + 2.0f : plot_x1;
                dl->AddLine(ImVec2(seg, y), ImVec2(end, y), col, 1.0f);
                seg = end + 3.0f;
            }
        } else {
            dl->AddLine(ImVec2(plot_x0, y), ImVec2(plot_x1, y), col, 1.0f);
        }

        char buf[24];
        std::snprintf(buf, sizeof(buf), "%.3f", last.close);
        ImGui::PushFont(theme::mono_font());
        const ImVec2 ts = ImGui::CalcTextSize(buf);
        const float tag_h = ts.y + 4.0f;
        // The tag is right-flush against the gutter edge and vertically clamped
        // to the plot, so a close at the top or bottom of the range can never
        // push it into the canvas padding or over the time axis.
        const float ty = y < plot_y0 + tag_h * 0.5f   ? plot_y0 + tag_h * 0.5f
                         : y > plot_y1 - tag_h * 0.5f ? plot_y1 - tag_h * 0.5f
                                                      : y;
        const ImVec2 hi(plot_x1 + 63.0f, ty + tag_h * 0.5f);
        const ImVec2 lo(hi.x - ts.x - theme::kSpace2, hi.y - tag_h);
        dl->AddRectFilled(lo, hi, col, 2.0f);
        dl->AddText(ImVec2(lo.x + theme::kSpace1, ty - ts.y * 0.5f),
                    ImGui::GetColorU32(theme::kBackground), buf);
        ImGui::PopFont();
    }

    // In-canvas O/H/L/C legend for the last real closed bar (top-left). This is
    // the terminal's OHLC read-out, not a fabricated value; it exists only when
    // the series does.
    {
        const Candle& last = series.candles.back();
        ImGui::PushFont(theme::small_font());
        float lx = origin.x + 12.0f;
        const float ly = origin.y + 12.0f;
        auto legend = [&](const char* k, double v, const ImVec4& c) {
            char vb[16];
            std::snprintf(vb, sizeof(vb), "%.3f", v);
            dl->AddText(ImVec2(lx, ly), ImGui::GetColorU32(theme::kTextMuted), k);
            const float kw = ImGui::CalcTextSize(k).x;
            dl->AddText(ImVec2(lx + kw + 4.0f, ly), ImGui::GetColorU32(c), vb);
            lx += kw + 4.0f + ImGui::CalcTextSize(vb).x + 14.0f;
        };
        legend("O", last.open, theme::kTextPrimary);
        legend("H", last.high, theme::kTextPrimary);
        legend("L", last.low, theme::kTextPrimary);
        legend("C", last.close, last.close >= last.open ? theme::kHealthy : theme::kCritical);
        ImGui::PopFont();
    }
}

// A professional chart header strip above the canvas: instrument + timeframe on
// the left and the last real closed bar's OHLC + change on the right. Every value
// is drawn from the real closed-bar series; an absent value shows N/A.
inline void chart_header(const CandleSeries& series, const std::string& ohlc, float height = 26.0f) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    ImGui::GetWindowDrawList()->AddLine(ImVec2(p.x, p.y + height - 1.0f),
                                        ImVec2(p.x + w, p.y + height - 1.0f),
                                        ImGui::GetColorU32(theme::kBorder));
    theme::text_label(theme::kTextMuted, "XAUUSD");
    ImGui::SameLine(0.0f, theme::kSpace2);
    theme::text_label(theme::kAccent, "%s", series.label.c_str());
    ImGui::SameLine(0.0f, theme::kSpace3);
    if (!series.available || series.candles.empty()) {
        theme::text_label(theme::kNotAvailable, "NO DATA");
    } else {
        const Candle& last = series.candles.back();
        const double change = last.close - last.open;
        theme::text_mono(theme::kTextSecondary, "O %s", [&] {
            char b[16]; std::snprintf(b, sizeof(b), "%.3f", last.open); return std::string(b);
        }().c_str());
        ImGui::SameLine(0.0f, theme::kSpace2);
        theme::text_mono(theme::kTextSecondary, "H %s", [&] {
            char b[16]; std::snprintf(b, sizeof(b), "%.3f", last.high); return std::string(b);
        }().c_str());
        ImGui::SameLine(0.0f, theme::kSpace2);
        theme::text_mono(theme::kTextSecondary, "L %s", [&] {
            char b[16]; std::snprintf(b, sizeof(b), "%.3f", last.low); return std::string(b);
        }().c_str());
        ImGui::SameLine(0.0f, theme::kSpace2);
        theme::text_mono(last.close >= last.open ? theme::kHealthy : theme::kCritical, "C %s",
                         [&] {
                             char b[16]; std::snprintf(b, sizeof(b), "%.3f", last.close);
                             return std::string(b);
                         }().c_str());
        ImGui::SameLine(0.0f, theme::kSpace2);
        theme::text_label(last.close >= last.open ? theme::kHealthy : theme::kCritical, "(%+.3f)",
                          change);
    }
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + height));
    (void)ohlc;
}

}  // namespace chart
}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_CANDLECHARTWIDGET_H
