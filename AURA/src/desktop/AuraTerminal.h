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

// Shared identity strings, typed once so the descriptor and the execution-mode
// label never drift apart between the top bar, the sidebar, the metric strip
// and the status bar.
inline constexpr const char* kDescriptor = "XAUUSD MARKET INTELLIGENCE";
inline constexpr const char* kShadowOnly = "SHADOW ONLY";

// Colour for a real LONG/SHORT direction label. NONE (or anything else) is
// informational, never a positive colour — an absent direction must not read
// as a green light.
inline ImVec4 direction_color(const std::string& d) {
    if (d == runtime::to_string(runtime::SignalDirection::LONG)) return theme::kHealthy;
    if (d == runtime::to_string(runtime::SignalDirection::SHORT)) return theme::kCritical;
    return theme::kNeutral;
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

// A brand mark to draw in the chrome: a real GL texture plus its aspect ratio
// (width / height) so it is never distorted. `texture == 0` means no real asset
// was supplied, in which case the caller draws the typographic wordmark alone.
struct BrandMarkView {
    ImTextureID texture{0};
    float aspect{1.0f};
    bool valid() const { return texture != 0; }
};

// Draw the brand mark at the requested pixel height, preserving aspect ratio.
inline void brand_mark_image(const BrandMarkView& mark, float height) {
    if (!mark.valid() || height <= 0.0f) return;
    const float w = height * (mark.aspect > 0.0f ? mark.aspect : 1.0f);
    ImGui::Image(mark.texture, ImVec2(w, height));
}

// A compact terminal header: brand + instrument + descriptor on the left, the
// current page title, and real status on the right. Height is fixed so the chart
// keeps its space. When a real ASTRA brand mark texture is available it is drawn
// before the wordmark; otherwise the typographic wordmark stands alone (no
// replacement logo is ever synthesized).
inline void top_bar(const ControlCenterSnapshot& s, const std::string& page_title,
                    const std::string& renderer, bool paused,
                    const BrandMarkView& brand_mark = {}) {
    const float bar_h = theme::kTopBarHeight;
    ImGui::BeginChild("topbar", ImVec2(0.0f, bar_h), false, ImGuiWindowFlags_NoScrollbar);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float full_w = ImGui::GetWindowWidth();
    filled_rect(p, ImVec2(p.x + full_w, p.y + bar_h), theme::kSurfaceRaised, 0.0f);
    filled_rect(ImVec2(p.x, p.y + bar_h - 1.0f), ImVec2(p.x + full_w, p.y + bar_h), theme::kBorder,
                0.0f);

    const float line_h = ImGui::GetTextLineHeight();
    const float cy = (bar_h - line_h) * 0.5f;

    // Left: brand mark (if provided) + ASTRA wordmark + the single descriptor.
    // The descriptor is the product tagline; the instrument identity lives in the
    // dashboard's market header, so "XAUUSD" is not repeated here.
    ImGui::SetCursorPos(ImVec2(theme::kSpace4, cy));
    if (brand_mark.valid()) {
        const float mark_h = line_h + 8.0f;
        ImGui::SetCursorPosY((bar_h - mark_h) * 0.5f);
        brand_mark_image(brand_mark, mark_h);
        ImGui::SameLine(0.0f, theme::kSpace2);
        ImGui::SetCursorPosY(cy);
    }
    theme::text_title(theme::kBrand, "ASTRA");
    ImGui::SameLine(0.0f, theme::kSpace3);
    {
        // A hairline separator between the wordmark and the descriptor, then the
        // descriptor in one muted small line.
        const ImVec2 dp = ImGui::GetCursorScreenPos();
        const float dy = dp.y + line_h * 0.5f;
        ImGui::GetWindowDrawList()->AddLine(ImVec2(dp.x, dy - 10.0f), ImVec2(dp.x, dy + 10.0f),
                                            ImGui::GetColorU32(theme::kBorderStrong));
        ImGui::Dummy(ImVec2(1.0f, line_h));
        ImGui::SameLine(0.0f, theme::kSpace3);
        theme::text_small(theme::kTextSecondary, "%s", kDescriptor);
    }

    // Right: real, right-aligned status. Widths are measured, not hard-coded,
    // and measured up front so the page title can be clamped to the actual
    // cluster boundary at every window width (never a pinned pixel offset).
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
    const std::string mode = paused ? "PAUSED" : kShadowOnly;
    const Item items[] = {
        {"SYSTEM", std::string(s.overview.healthy ? "HEALTHY" : s.overview.aggregate),
         theme::status_color(s.overview.healthy ? "HEALTHY" : s.overview.aggregate)},
        {"DATA", streams, theme::status_color(s.overview.healthy ? "HEALTHY" : s.overview.aggregate)},
        {"RENDERER", renderer, theme::kTextSecondary},
    };

    float cluster_w = 0.0f;
    for (const Item& it : items) {
        if (it.label[0] != '\0') cluster_w += text_w(it.label) + 5.0f;
        cluster_w += text_w(it.value.c_str()) + theme::kSpace4;
    }
    cluster_w += text_w(mode.c_str()) + theme::kSpace3;
    const float cluster_x = full_w - cluster_w - theme::kSpace4;

    // Centre-left: current page title (real navigation state), measured and
    // clamped so it never runs into the measured status cluster.
    if (!page_title.empty()) {
        const float brand_end = ImGui::GetCursorPosX();
        float tx = brand_end + theme::kSpace5 * 1.5f;
        ImGui::PushFont(theme::title_font());
        const float title_w = ImGui::CalcTextSize(page_title.c_str()).x;
        ImGui::PopFont();
        const float right_edge = cluster_x - theme::kSpace5;
        if (tx + title_w > right_edge && right_edge - title_w > brand_end + theme::kSpace2)
            tx = right_edge - title_w;
        ImGui::SetCursorPos(ImVec2(tx, cy + 1.0f));
        theme::text_title(theme::kTextSecondary, "%s", page_title.c_str());
    }

    float x = cluster_x;
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
    // SHADOW ONLY / PAUSED identity pill. Steel/off-white identity treatment:
    // the execution mode is an identity, not a warning, so it never borrows a
    // health colour. PAUSED keeps the violet paused tone.
    {
        const ImVec2 sp = ImGui::GetCursorScreenPos();
        const ImVec2 ts = ImGui::CalcTextSize(mode.c_str());
        const ImVec2 lo(sp.x, sp.y - 1.0f);
        const ImVec2 hi(sp.x + ts.x + theme::kSpace2 + 8.0f, sp.y + ts.y + 1.0f);
        const ImVec4 col = paused ? theme::kPaused : theme::kShadow;
        filled_rect(lo, hi, ImVec4(col.x, col.y, col.z, 0.10f), 3.0f);
        outlined_rect(lo, hi, ImVec4(col.x, col.y, col.z, 0.55f), 3.0f);
        // Identity dot (the reference chip's filled marker).
        ImGui::GetWindowDrawList()->AddCircleFilled(
            ImVec2(lo.x + 7.0f, (lo.y + hi.y) * 0.5f), 2.2f, ImGui::GetColorU32(col));
        ImGui::SetCursorScreenPos(ImVec2(lo.x + 12.0f, lo.y + 1.0f));
        theme::text_small(col, "%s", mode.c_str());
    }
    ImGui::EndChild();
}

// ---- Far-left icon rail ----------------------------------------------------

// A narrow vertical icon rail. One icon per navigation group (the group's first
// section); clicking jumps to that group's first page. The active group carries
// the accent indicator. The real ASTRA brand mark (when the build supplied one)
// tops the rail; without an asset the rail starts directly with the navigation
// icons — no invented monogram is drawn. Returns the clicked section index, or -1.
inline int icon_rail(const std::vector<NavGroup>& groups, int selected,
                     const BrandMarkView& brand_mark = {}) {
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

    // Brand mark: the real ASTRA asset when the build supplied one, otherwise
    // nothing. No invented monogram is drawn here — the reference identity is
    // the real mark or the typographic wordmark, never a synthesized glyph.
    if (brand_mark.valid()) {
        const float mark_w = tile * (brand_mark.aspect > 0.0f ? brand_mark.aspect : 1.0f);
        ImGui::SetCursorPos(ImVec2((w - mark_w) * 0.5f, theme::kSpace3));
        brand_mark_image(brand_mark, tile);
        ImGui::Dummy(ImVec2(tile, tile));
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + theme::kSpace3);
    } else {
        ImGui::SetCursorPosY(theme::kSpace3);
    }
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
// whether the operator pressed the footer EXIT control (which opens the
// confirmation dialog; it is not itself a shutdown).
struct SidebarResult {
    int clicked = -1;
    bool exit_pressed = false;
};

// `selected` is the real current page. Group titles are muted captions; rows are
// compact with an accent active bar. The EXIT control is pinned to the bottom of
// the sidebar so it stays in the lower-left corner at every window size; it only
// signals intent — the caller routes the request through the shutdown lifecycle.
inline SidebarResult sidebar(const std::vector<NavGroup>& groups, int selected, float width,
                             const BrandMarkView& brand_mark = {}) {
    SidebarResult result;
    ImGui::BeginChild("sidebar", ImVec2(width, 0.0f), false);
    // ASTRA identity block: the real brand mark (when supplied) above the
    // product wordmark and the full descriptor, matching the reference sidebar.
    // When no asset is present the typographic treatment stands alone (no
    // replacement logo is synthesized here).
    ImGui::SetCursorPos(ImVec2(theme::kSpace3, theme::kSpace3));
    if (brand_mark.valid()) {
        brand_mark_image(brand_mark, ImGui::GetTextLineHeight() * 1.4f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + theme::kSpace1);
    }
    theme::text_title(theme::kBrand, "ASTRA");
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1.0f);
    theme::text_label(theme::kTextMuted, "%s", kDescriptor);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + theme::kSpace2);
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
            result.exit_pressed = true;
    }
    ImGui::EndChild();
    return result;
}

// ---- Exit confirmation -----------------------------------------------------

// A modal confirmation for the sidebar EXIT control. Returns true exactly once,
// when the operator confirms; the caller then routes the request through the
// normal application shutdown lifecycle (a clean close that still persists a
// checkpoint) — never a process kill. Cancelling returns false.
//
// `open` is the caller's per-frame intent to raise the dialog; the caller keeps
// it set while the dialog should remain visible.
inline bool exit_confirmation(bool& open) {
    bool confirmed = false;
    if (open) {
        ImGui::OpenPopup("Exit ASTRA?");
        open = false;
    }
    if (ImGui::BeginPopupModal("Exit ASTRA?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        theme::text_body(theme::kTextPrimary, "Close ASTRA?");
        theme::text_small(theme::kTextMuted,
                          "The application will shut down cleanly. A final checkpoint is "
                          "persisted and the recovery lifecycle stays intact.");
        ImGui::Spacing();
        if (widgets::action_button("CANCEL", false, 110.0f)) ImGui::CloseCurrentPopup();
        ImGui::SameLine(0.0f, theme::kSpace2);
        if (widgets::action_button("EXIT", true, 110.0f)) {
            confirmed = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    return confirmed;
}

// ---- Timeframe tab strip ---------------------------------------------------

// Terminal-style segmented timeframe strip (the institutional selector from the
// ASTRA reference): one inset container with a bordered segment per timeframe, a
// separator between segments, an elevated active segment with an accent
// underline, and a small dot marking the structural authority (H4). Clicking a
// tab switches the chart stream by explicit timeframe identity and returns true.
// A caption right of the strip names the selected stream's canonical V3-29
// authority role (OPERATIONAL / STRUCTURAL / ...).
inline bool timeframe_tabs(TimeframeSelection& sel, float height = 32.0f) {
    bool changed = false;
    const std::vector<runtime::Timeframe>& tfs = chart_timeframes();
    const float tab_w = 52.0f;
    const float y0 = ImGui::GetCursorScreenPos().y;
    const float x_start = ImGui::GetCursorScreenPos().x;
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Segmented container: one inset strip holding all nine segments.
    const float strip_w = tfs.size() * tab_w;
    dl->AddRectFilled(ImVec2(x_start, y0), ImVec2(x_start + strip_w, y0 + height),
                      ImGui::GetColorU32(theme::kSurface), 3.0f);
    dl->AddRect(ImVec2(x_start, y0), ImVec2(x_start + strip_w, y0 + height),
                ImGui::GetColorU32(theme::kBorder), 3.0f);

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
                              ImGui::GetColorU32(theme::kSurfaceRaised), 0.0f);
            dl->AddRectFilled(ImVec2(p.x, p.y + height - 2.0f), ImVec2(p.x + tab_w, p.y + height),
                              ImGui::GetColorU32(theme::kAccent), 0.0f);
        } else if (hovered) {
            dl->AddRectFilled(p, ImVec2(p.x + tab_w, p.y + height),
                              ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.03f)), 0.0f);
        }
        // Segment separator (reference-style shared borders).
        if (i > 0)
            dl->AddLine(ImVec2(p.x, y0 + 6.0f), ImVec2(p.x, y0 + height - 6.0f),
                        ImGui::GetColorU32(theme::kBorder));
        ImGui::PushFont(theme::body_font());
        const ImVec2 ts = ImGui::CalcTextSize(label.c_str());
        dl->AddText(ImVec2(p.x + (tab_w - ts.x) * 0.5f, p.y + (height - ts.y) * 0.5f),
                    ImGui::GetColorU32(active   ? theme::kTextPrimary
                                       : hovered ? theme::kTextSecondary
                                                 : theme::kTextMuted),
                    label.c_str());
        ImGui::PopFont();
        // A small dot marks the primary structural authority (H4), matching the
        // reference's role hint. Identity comes from the explicit timeframe.
        if (tf == runtime::Timeframe::H4 && !active) {
            dl->AddCircleFilled(ImVec2(p.x + tab_w - 6.0f, p.y + 6.0f), 1.6f,
                                ImGui::GetColorU32(theme::kTextMuted));
        }
        ImGui::PopID();
        if (clicked && sel.select(tf)) changed = true;
        if (i + 1 < tfs.size()) ImGui::SameLine(0.0f, 0.0f);
    }

    // Right of the strip: the selected stream's authority role (real identity,
    // never positional), drawn on the strip's centreline. The cursor is parked
    // past the strip with a reserved dummy so following widgets do not overlap.
    const char* role = layout::timeframe_role(sel.selected());
    const std::string caption =
        role[0] != '\0' ? std::string(role) + " \u00b7 " + sel.selected_label() : std::string();
    ImGui::SameLine(0.0f, theme::kSpace3);
    ImVec2 ts(0.0f, 0.0f);
    if (!caption.empty()) {
        ImGui::PushFont(theme::label_font());
        ts = ImGui::CalcTextSize(caption.c_str());
        ImGui::PopFont();
    }
    // Overflow guard: on a narrow window the nine-tab strip wins and the role
    // caption is dropped (never clipped or wrapped onto a second line).
    if (!caption.empty() && ts.x + theme::kSpace2 <= ImGui::GetContentRegionAvail().x) {
        const float cy = y0 + (height - ts.y) * 0.5f;
        dl->AddText(ImVec2(x_start + strip_w + theme::kSpace3, cy),
                    ImGui::GetColorU32(theme::kTextMuted), caption.c_str());
        ImGui::Dummy(ImVec2(theme::kSpace3 + ts.x, height));
    } else {
        ImGui::Dummy(ImVec2(0.0f, height));
    }
    return changed;
}

// ---- Instrument (market) header --------------------------------------------

// Compact instrument header above the chart: identity, selected timeframe with
// its canonical authority role, the real last closed price as the hero value
// (with the change against the previous real closed bar when one exists) and
// the selected stream's real state/quality/bars. Layout is measured, not pinned
// to hard-coded pixel offsets, so it stays coherent with any font fallback and
// at every supported resolution. Absent values render N/A / NOT AVAILABLE.
inline void market_header(const ControlCenterReport& r) {
    const ControlCenterSnapshot& s = r.snapshot;
    const CandleSeries& cs = r.chart;
    const TimeframeRow* row = row_for(s.timeframes, cs.timeframe);

    const float avail = ImGui::GetContentRegionAvail().x;
    const float h = 58.0f;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    filled_rect(p, ImVec2(p.x + avail, p.y + h), theme::kSurface, 3.0f);
    outlined_rect(p, ImVec2(p.x + avail, p.y + h), theme::kBorder, 3.0f);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // ---- Left: instrument identity + timeframe badge + role -----------------
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
    ImGui::SameLine(0.0f, theme::kSpace2);
    {
        const char* role = layout::timeframe_role(cs.timeframe);
        if (role[0] != '\0') theme::text_label(theme::kTextMuted, "%s", role);
    }
    ImGui::SetCursorScreenPos(ImVec2(p.x + theme::kSpace3, p.y + 35.0f));
    theme::text_label(theme::kTextMuted, "MARKET INTELLIGENCE");

    // ---- Real values --------------------------------------------------------
    const bool have_row = row != nullptr && row->present;
    const std::string stream_state = have_row ? row_health(*row) : std::string("NOT AVAILABLE");
    const std::string quality = (have_row && row->quality != "NOT AVAILABLE")
                                    ? row->quality
                                    : std::string("NOT AVAILABLE");
    const std::string bars =
        have_row && row->sequence.available ? row->sequence.value : std::string("N/A");

    const bool have_close = cs.available && !cs.candles.empty();
    double close_v = 0.0;
    std::string close_t;
    if (have_close) {
        close_v = cs.candles.back().close;
        close_t = format_utc_minute(cs.candles.back().close_time);
    }
    // Change against the previous real closed bar (present only when at least
    // two real closed bars exist; never computed from anything else).
    bool have_change = false;
    double change = 0.0;
    if (have_close && cs.candles.size() >= 2) {
        change = close_v - cs.candles[cs.candles.size() - 2].close;
        have_change = true;
    }

    // ---- Middle: hero price + change ---------------------------------------
    {
        const float hero_x = p.x + avail * 0.42f;
        // Keep the formatted price alive for the whole draw (never a temporary
        // c_str()).
        const std::string pv_str = have_close ? price(close_v) : std::string("N/A");
        const char* pv = pv_str.c_str();
        ImGui::PushFont(theme::hero_font());
        const ImVec2 ps = ImGui::CalcTextSize(pv);
        ImGui::PopFont();
        const float hero_y = p.y + (h - ps.y - 14.0f) * 0.5f;
        dl->AddText(ImVec2(hero_x, hero_y),
                    ImGui::GetColorU32(have_close ? theme::kTextPrimary : theme::kNotAvailable),
                    pv);
        if (have_close) {
            ImGui::PushFont(theme::label_font());
            const std::string cap = "LAST CLOSED \u00b7 " + close_t;
            dl->AddText(ImVec2(hero_x, hero_y + ps.y + 2.0f),
                        ImGui::GetColorU32(theme::kTextMuted), cap.c_str());
            ImGui::PopFont();
        }
        if (have_change) {
            char cb[32];
            std::snprintf(cb, sizeof(cb), "%+.3f", change);
            const ImVec2 csz = ImGui::CalcTextSize(cb);
            const float cx = hero_x + ps.x + theme::kSpace3;
            ImGui::PushFont(theme::mono_font());
            dl->AddText(ImVec2(cx, hero_y + ps.y - csz.y),
                        ImGui::GetColorU32(change >= 0.0 ? theme::kHealthy : theme::kCritical),
                        cb);
            ImGui::PopFont();
        }
    }

    // ---- Right: stream / quality / bars, right-aligned, measured ------------
    {
        struct Stat {
            const char* label;
            std::string value;
            ImVec4 col;
            bool mono;  // numeric values use the monospaced face so digits align
        };
        const Stat stats[] = {
            {"STREAM", stream_state, theme::status_color(stream_state), false},
            {"QUALITY", quality, theme::status_color(quality), false},
            {"BARS", bars, theme::kTextPrimary, true},
        };
        // Measure the whole cluster with the exact fonts it will draw in, then
        // place it flush right.
        auto label_w = [](const char* txt) {
            ImGui::PushFont(theme::label_font());
            const float w = ImGui::CalcTextSize(txt).x;
            ImGui::PopFont();
            return w;
        };
        auto value_w = [](const std::string& v, bool mono) {
            ImGui::PushFont(mono ? theme::mono_font() : theme::body_font());
            const float w = ImGui::CalcTextSize(v.c_str()).x;
            ImGui::PopFont();
            return w;
        };
        float cluster_w = 0.0f;
        for (const Stat& st : stats)
            cluster_w += label_w(st.label) + 5.0f + value_w(st.value, st.mono) + theme::kSpace4;
        float x = p.x + avail - cluster_w - theme::kSpace3;
        const float min_x = p.x + avail * 0.68f;  // never collide with the hero price
        if (x < min_x) x = min_x;
        const float ly = p.y + theme::kSpace2 + 1.0f;
        const float vy = p.y + theme::kSpace2 + 15.0f;
        for (const Stat& st : stats) {
            ImGui::PushFont(theme::label_font());
            dl->AddText(ImVec2(x, ly), ImGui::GetColorU32(theme::kTextMuted), st.label);
            const float lw = ImGui::CalcTextSize(st.label).x;
            ImGui::PopFont();
            ImGui::PushFont(st.mono ? theme::mono_font() : theme::body_font());
            dl->AddText(ImVec2(x, vy), ImGui::GetColorU32(st.col), st.value.c_str());
            ImGui::PopFont();
            x += lw + 5.0f + value_w(st.value, st.mono) + theme::kSpace4;
        }
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
         "no proposal", s.risk.is_order ? "order flag" : "no live order",
         theme::status_color(s.risk.available ? s.risk.direction : "UNKNOWN")},
        {"EXECUTION MODE", s.shadow_only ? kShadowOnly : "VIOLATED", true, "",
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

// ---- Analytics panels ------------------------------------------------------

// The right-hand analytics content: Signals, Risk and Shadow Positions. Each
// panel is exposed separately so the vertical analytics column and the
// dashboard's horizontal Signals/Risk/Shadow row render the identical content
// from one implementation. Compact, professionally aligned, real values only.

// The SIGNALS panel body (most recent decision, real score/confidence).
inline void signals_panel(const ControlCenterReport& r, float width, float height) {
    const ControlCenterSnapshot& s = r.snapshot;
    widgets::panel_begin("an_signals", "SHADOW SIGNALS", icons::Icon::Signals, ImVec2(width, 0.0f),
                         height, true);
    if (!s.signal.available) {
        widgets::empty_state("Signal", "no signal produced yet");
    } else {
        const ImVec4 dir_col = direction_color(s.signal.direction);
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
}

// The RISK panel body (shadow-only sizing proposal; never an order).
inline void risk_panel(const ControlCenterReport& r, float width, float height) {
    const ControlCenterSnapshot& s = r.snapshot;
    widgets::panel_begin("an_risk", "RISK", icons::Icon::Risk, ImVec2(width, 0.0f), height, true);
    if (!s.risk.available) {
        widgets::empty_state("Risk proposal", "no proposal yet");
    } else {
        const ImVec4 dir_col = direction_color(s.risk.direction);
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
}

// The SHADOW POSITIONS panel body (simulated fills only; SHADOW ONLY identity).
inline void shadow_positions_panel(const ControlCenterReport& r, float width, float height) {
    const ControlCenterSnapshot& s = r.snapshot;
    widgets::panel_begin("an_positions", "SHADOW POSITIONS", icons::Icon::Positions,
                         ImVec2(width, 0.0f), height, true);
    // An open shadow position is informational, not degraded: amber stays
    // reserved for genuinely degraded states, and nothing here implies a live
    // order.
    widgets::badge(s.positions.has_open_position ? "OPEN" : "NONE",
                   s.positions.has_open_position ? theme::kNeutral
                                                 : theme::status_color("NONE"));
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
    theme::text_label(theme::kShadow, "%s \u00b7 simulated fills", kShadowOnly);
    widgets::panel_end();
}

// The vertical analytics column: Signals, Risk and Shadow Positions stacked.
// Deliberately narrower than the chart.
inline void analytics_column(const ControlCenterReport& r, float width, float height) {
    ImGui::BeginChild("analytics", ImVec2(width, height), false);

    const float panel_h = (height - theme::kSpace3 * 2.0f) / 3.0f;
    signals_panel(r, width, panel_h);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + theme::kSpace2);
    risk_panel(r, width, panel_h);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + theme::kSpace2);
    shadow_positions_panel(r, width, panel_h);

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
    const std::string mode = kShadowOnly;
    ImGui::SameLine();
    ImGui::SetCursorPosX(w - text_w(mode.c_str()) - theme::kSpace4);
    theme::text_label(theme::kShadow, "%s", mode.c_str());
}

}  // namespace terminal
}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_AURATERMINAL_H
