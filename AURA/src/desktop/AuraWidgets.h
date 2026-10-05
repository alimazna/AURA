#ifndef AURA_DESKTOP_AURAWIDGETS_H
#define AURA_DESKTOP_AURAWIDGETS_H

// Reusable AURA presentation primitives for the terminal, built only from
// Dear ImGui's fixed-function draw list (flat rectangles, lines, text). No
// shaders, no textures, no GPU effects, so every widget renders identically on
// the OpenGL 2.1 legacy backend and the OpenGL 3.3 backend.
//
// Widgets are presentation-only: they draw a value they are handed and never
// reach into the runtime. Unknown/absent values are rendered through the neutral
// state treatment so they never read as a healthy value or a zero.

#include "desktop/AuraIcons.h"
#include "desktop/AuraTheme.h"

#include <imgui.h>

#include <cstddef>
#include <cstdio>
#include <string>

namespace aura {
namespace desktop {
namespace widgets {

// Fills a rectangle using only the ImGui draw list (legacy-safe).
inline void filled_rect(ImVec2 lo, ImVec2 hi, ImVec4 col, float rounding = 0.0f) {
    ImGui::GetWindowDrawList()->AddRectFilled(lo, hi, ImGui::GetColorU32(col), rounding);
}

// A hairline border helper.
inline void outlined_rect(ImVec2 lo, ImVec2 hi, ImVec4 col, float rounding = 0.0f,
                          float thickness = 1.0f) {
    ImGui::GetWindowDrawList()->AddRect(lo, hi, ImGui::GetColorU32(col), rounding, 0, thickness);
}

// A flat, titled panel that visually belongs to the terminal. The title is a
// small uppercase caption in the secondary text colour with an optional accent
// icon, separated from the body by a hairline. Call panel_end() to close it.
// Panels use the shared surface colour and a single hairline border rather than
// heavy card shadows.
inline void panel_begin(const char* id, const char* title, icons::Icon ic = icons::Icon::None,
                        ImVec2 size = ImVec2(0.0f, 0.0f), float height = 0.0f,
                        bool scroll = false) {
    ImGui::BeginChild(id, ImVec2(size.x, height > 0.0f ? height : size.y), true,
                      scroll ? 0 : ImGuiWindowFlags_NoScrollbar);
    if (title != nullptr && title[0] != '\0') {
        const float line_h = ImGui::GetTextLineHeight();
        if (ic != icons::Icon::None) {
            const ImVec2 p = ImGui::GetCursorScreenPos();
            icons::draw_icon(ImGui::GetWindowDrawList(), ic,
                             ImVec2(p.x + line_h * 0.5f, p.y + line_h * 0.5f), line_h * 0.86f,
                             ImGui::GetColorU32(theme::kTextMuted));
            ImGui::Dummy(ImVec2(line_h + theme::kSpace1, line_h));
            ImGui::SameLine(0.0f, 0.0f);
        }
        theme::text_small(theme::kTextMuted, "%s", title);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }
}

inline void panel_end() { ImGui::EndChild(); }

// Backwards-compatible alias used by the section panels.
inline void card_begin(const char* id, const char* title, ImVec2 size = ImVec2(0, 0),
                       float height = 0.0f) {
    panel_begin(id, title, icons::Icon::None, size, height);
}
inline void card_end() { panel_end(); }

// A small pill badge. Colour comes from the state category; the label is always
// drawn as text so the state is legible on legacy hardware without colour.
inline void badge(const char* label, ImVec4 color, float text_scale = 1.0f) {
    const ImVec2 pad(theme::kSpace1 * 1.5f, 2.0f);
    const ImVec2 text = ImGui::CalcTextSize(label);
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 lo(pos.x, pos.y);
    const ImVec2 hi(pos.x + text.x * text_scale + pad.x * 2.0f,
                    pos.y + text.y * text_scale + pad.y * 2.0f);
    filled_rect(lo, hi, ImVec4(color.x, color.y, color.z, 0.16f), 3.0f);
    outlined_rect(lo, hi, ImVec4(color.x, color.y, color.z, 0.55f), 3.0f);
    ImGui::SetCursorScreenPos(ImVec2(lo.x + pad.x, lo.y + pad.y));
    if (text_scale != 1.0f) ImGui::SetWindowFontScale(text_scale);
    ImGui::TextColored(color, "%s", label);
    if (text_scale != 1.0f) ImGui::SetWindowFontScale(1.0f);
    ImGui::SetCursorScreenPos(ImVec2(hi.x, lo.y));
    ImGui::Dummy(ImVec2(0.0f, text.y * text_scale + pad.y * 2.0f));
}

// A solid state dot + label. Compact, used in dense rows.
inline void state_dot(const char* label, ImVec4 color) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float r = 3.5f;
    const float line_h = ImGui::GetTextLineHeight();
    ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(p.x + r, p.y + line_h * 0.5f), r,
                                                ImGui::GetColorU32(color));
    ImGui::Dummy(ImVec2(r * 2.0f + theme::kSpace1, line_h));
    ImGui::SameLine(0.0f, 0.0f);
    theme::text_body(theme::kTextSecondary, "%s", label);
}

// A state badge driven by the shared category map.
inline void state_badge(const std::string& state) {
    const theme::StateCategory c = theme::state_category(state);
    badge(theme::category_label(c), theme::category_color(c));
    ImGui::SameLine();
    if (std::string(theme::category_label(c)) != state && !state.empty())
        theme::text_body(theme::kTextSecondary, "%s", state.c_str());
}

// A label/value row on one line: muted label, primary value, aligned to a column.
inline void kv_row(const char* key, const std::string& value, float key_col = 148.0f) {
    theme::text_body(theme::kTextMuted, "%s", key);
    ImGui::SameLine();
    const float key_w = ImGui::CalcTextSize(key).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (key_col > key_w ? key_col - key_w : 0.0f));
    if (theme::is_unknown_like(value))
        theme::text_body(theme::kTextMuted, "%s", value.c_str());
    else
        theme::text_body(theme::kTextPrimary, "%s", value.c_str());
}

// A label/value row whose value is coloured by its state category.
inline void kv_state_row(const char* key, const std::string& value, float key_col = 148.0f) {
    theme::text_body(theme::kTextMuted, "%s", key);
    ImGui::SameLine();
    const float key_w = ImGui::CalcTextSize(key).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (key_col > key_w ? key_col - key_w : 0.0f));
    theme::text_body(theme::status_color(value), "%s", value.c_str());
}

// A big "metric" number with a caption. When unavailable the number is replaced
// by an explicit NOT AVAILABLE marker with a reason, never a fake zero.
inline void metric(const char* caption, const std::string& value, bool available,
                   const char* reason = "no data source wired") {
    theme::text_small(theme::kTextMuted, "%s", caption);
    if (available)
        theme::text_title(theme::kTextPrimary, "%s", value.c_str());
    else {
        theme::text_title(theme::kNotAvailable, "N/A");
        theme::text_small(theme::kTextMuted, "%s", reason);
    }
}

inline void metric_u64(const char* caption, unsigned long long value, bool available,
                       const char* reason = "no data source wired") {
    metric(caption, std::to_string(value), available, reason);
}

// A prominent, professional empty-state panel: title, an explicit NOT AVAILABLE
// marker and a reason naming the missing data source. Never fabricates a row.
inline void empty_state(const char* title, const char* reason) {
    ImGui::Spacing();
    theme::text_title(theme::kTextSecondary, "%s", title);
    ImGui::Spacing();
    theme::text_body(theme::kNotAvailable, "NOT AVAILABLE");
    ImGui::PushTextWrapPos(0.0f);
    theme::text_small(theme::kTextMuted, "%s", reason);
    ImGui::PopTextWrapPos();
}

// A quiet note for a section that IS wired but genuinely has no rows yet (a real
// empty state, distinct from NOT AVAILABLE).
inline void empty_note(const char* reason) {
    ImGui::Spacing();
    theme::text_small(theme::kTextMuted, "%s", reason);
}

// Section header used at the top of a content page.
inline void section_header(const char* title, const char* subtitle,
                           icons::Icon ic = icons::Icon::None) {
    const float line_h = ImGui::GetTextLineHeight();
    if (ic != icons::Icon::None) {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        icons::draw_icon(ImGui::GetWindowDrawList(), ic, ImVec2(p.x + 9.0f, p.y + line_h * 0.5f),
                         15.0f, ImGui::GetColorU32(theme::kAccent));
        ImGui::Dummy(ImVec2(24.0f, line_h));
        ImGui::SameLine(0.0f, 0.0f);
    }
    theme::text_title(theme::kTextPrimary, "%s", title);
    if (subtitle != nullptr && subtitle[0] != '\0')
        theme::text_small(theme::kTextMuted, "%s", subtitle);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

// A thin horizontal rule with vertical breathing room.
inline void rule() {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
}

// Standard table flags so every table in the app looks the same.
inline ImGuiTableFlags table_flags(bool scroll_y = false, bool scroll_x = false) {
    ImGuiTableFlags f = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH |
                        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_SizingStretchProp;
    if (scroll_y) f |= ImGuiTableFlags_ScrollY;
    if (scroll_x) f |= ImGuiTableFlags_ScrollX;
    return f;
}

// A coloured cell whose colour follows the state category.
inline void state_cell(const std::string& value) {
    theme::text_body(theme::status_color(value), "%s", value.c_str());
}

// A cell that shows a value when present, else an explicit NOT AVAILABLE marker.
inline void value_cell(bool available, const std::string& value) {
    if (available)
        theme::text_body(theme::kTextPrimary, "%s", value.c_str());
    else
        theme::text_body(theme::kNotAvailable, "N/A");
}

// ---- Compact metric strip items -------------------------------------------

// One cell of a compact information strip: a muted caption above a strong value.
// Unavailable values are an explicit N/A marker, never a fabricated zero.
inline void metric_block(const char* label, const std::string& value, bool available,
                         const char* reason = nullptr) {
    theme::text_small(theme::kTextMuted, "%s", label);
    if (available) {
        theme::text_title(theme::kTextPrimary, "%s", value.c_str());
    } else {
        theme::text_title(theme::kNotAvailable, "N/A");
        if (reason != nullptr && reason[0] != '\0') theme::text_small(theme::kTextMuted, "%s", reason);
    }
}

// ---- Success meter ---------------------------------------------------------
// A labelled 0..100% meter with a horizontal bar. `value` is a fraction in [0,1].
// This renders AURA's real deterministic score/confidence (RT-0011/RT-0012) or a
// realized shadow success rate. None of these is a calibrated probability, and
// when the value is absent the meter shows N/A rather than a fake percentage.
inline void success_meter(const char* label, const char* sublabel, bool available, double value,
                          ImVec4 color) {
    theme::text_small(theme::kTextMuted, "%s", label);
    if (!available) {
        theme::text_title(theme::kNotAvailable, "N/A");
        if (sublabel != nullptr && sublabel[0] != '\0')
            theme::text_small(theme::kTextMuted, "%s", sublabel);
        return;
    }
    double clamped = value < 0.0 ? 0.0 : (value > 1.0 ? 1.0 : value);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%.0f%%", clamped * 100.0);
    theme::text_title(color, "%s", buf);

    const float w = ImGui::GetContentRegionAvail().x;
    const float h = 5.0f;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), ImGui::GetColorU32(theme::kSurfaceRaised), 2.0f);
    dl->AddRectFilled(p, ImVec2(p.x + w * static_cast<float>(clamped), p.y + h),
                      ImGui::GetColorU32(color), 2.0f);
    ImGui::Dummy(ImVec2(w, h));
    if (sublabel != nullptr && sublabel[0] != '\0')
        theme::text_small(theme::kTextMuted, "%s", sublabel);
}

// ---- Shell chrome items ----------------------------------------------------

// A compact "LABEL value" status item for the top header / status bar. The label
// is muted, the value carries the state colour. Returns the width consumed so
// callers can lay items out responsively instead of at fixed pixel offsets.
inline float status_item(const char* label, const std::string& value, ImVec4 color) {
    const float x0 = ImGui::GetCursorPosX();
    theme::text_small(theme::kTextMuted, "%s", label);
    ImGui::SameLine(0.0f, 5.0f);
    theme::text_small(color, "%s", value.c_str());
    return ImGui::GetCursorPosX() - x0;
}

// A full-width sidebar navigation row with a small icon, a label and (for the
// selected row) a left accent bar and a subtle raised background. Selection is
// driven by the caller's real page state, never by string comparison.
inline bool nav_item(const char* label, icons::Icon ic, bool selected, float width) {
    const float h = 24.0f;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const bool clicked = ImGui::InvisibleButton(label, ImVec2(width, h));
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (selected) {
        dl->AddRectFilled(p, ImVec2(p.x + width, p.y + h),
                          ImGui::GetColorU32(theme::kSurfaceRaised), 3.0f);
        dl->AddRectFilled(p, ImVec2(p.x + 3.0f, p.y + h), ImGui::GetColorU32(theme::kAccent), 2.0f);
    } else if (hovered) {
        dl->AddRectFilled(p, ImVec2(p.x + width, p.y + h),
                          ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.035f)), 3.0f);
    }
    const ImU32 icol = ImGui::GetColorU32(selected ? theme::kAccent : theme::kTextMuted);
    const ImVec2 icon_c(p.x + theme::kSpace2 + 7.0f, p.y + h * 0.5f);
    icons::draw_icon(dl, ic, icon_c, 14.0f, icol);
    const float tx = p.x + theme::kSpace2 + 22.0f;
    ImGui::PushFont(theme::body_font());
    const ImVec2 ts = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(tx, p.y + (h - ts.y) * 0.5f),
                ImGui::GetColorU32(selected ? theme::kTextPrimary : theme::kTextSecondary), label);
    ImGui::PopFont();
    return clicked;
}

// ---- Far-left icon rail ----------------------------------------------------

// A square, icon-only navigation button for the far-left rail. `selected` draws
// the accent active indicator (a short bar on the left edge plus a tinted tile).
// Compact by design: the rail never carries text.
inline bool rail_item(const char* id, icons::Icon ic, bool selected, float size) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const bool clicked = ImGui::InvisibleButton(id, ImVec2(size, size));
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (selected) {
        dl->AddRectFilled(p, ImVec2(p.x + size, p.y + size),
                          ImGui::GetColorU32(ImVec4(theme::kAccent.x, theme::kAccent.y,
                                                    theme::kAccent.z, 0.14f)),
                          4.0f);
        dl->AddRectFilled(p, ImVec2(p.x + 2.5f, p.y + size), ImGui::GetColorU32(theme::kAccent),
                          1.0f);
    } else if (hovered) {
        dl->AddRectFilled(p, ImVec2(p.x + size, p.y + size),
                          ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.05f)), 4.0f);
    }
    const ImU32 col = ImGui::GetColorU32(selected   ? theme::kAccent
                                         : hovered ? theme::kTextSecondary
                                                   : theme::kTextMuted);
    icons::draw_icon(dl, ic, ImVec2(p.x + size * 0.5f, p.y + size * 0.5f), 18.0f, col, 1.5f);
    return clicked;
}

// ---- Sidebar footer controls -----------------------------------------------

// A full-width application-exit control anchored to the bottom of the navigation
// sidebar. Deliberately quiet and neutral at rest (it is a lifecycle action, not
// a trading action): a subtle raised fill with muted text, warming to a restrained
// red only on hover. It returns true exactly once per click; the caller routes the
// request through the normal shutdown lifecycle — it never terminates the process.
inline bool exit_button(const char* label, float width, float height = 36.0f) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const bool clicked = ImGui::InvisibleButton(label, ImVec2(width, height));
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec4 fill = hovered ? ImVec4(theme::kCritical.x, theme::kCritical.y, theme::kCritical.z,
                                         0.22f)
                                : theme::kSurfaceRaised;
    dl->AddRectFilled(p, ImVec2(p.x + width, p.y + height), ImGui::GetColorU32(fill),
                      theme::kControlRadius);
    dl->AddRect(p, ImVec2(p.x + width, p.y + height),
                ImGui::GetColorU32(hovered ? theme::kCritical : theme::kBorder),
                theme::kControlRadius);
    ImGui::PushFont(theme::label_font());
    const ImVec2 ts = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(p.x + (width - ts.x) * 0.5f, p.y + (height - ts.y) * 0.5f),
                ImGui::GetColorU32(hovered ? theme::kCritical : theme::kTextSecondary), label);
    ImGui::PopFont();
    return clicked;
}

// ---- Action bar controls ---------------------------------------------------

// A compact, professional control-bar button. Flat until hovered; an accent tint
// marks the active/pressed control (e.g. a paused transport). Text only — there
// is no order/buy/sell control anywhere on this surface.
inline bool action_button(const char* label, bool active = false, float min_w = 84.0f) {
    const ImVec2 pad(theme::kSpace3, 5.0f);
    const ImVec2 ts = ImGui::CalcTextSize(label);
    const ImVec2 size(ts.x + pad.x * 2.0f > min_w ? ts.x + pad.x * 2.0f : min_w,
                      ts.y + pad.y * 2.0f);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const bool clicked = ImGui::InvisibleButton(label, size);
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec4 base = active ? theme::kAccentDim : theme::kSurfaceRaised;
    ImVec4 fill = base;
    if (hovered && !active) fill = ImVec4(0.129f, 0.157f, 0.196f, 1.0f);
    if (hovered && active) fill = ImVec4(theme::kAccent.x, theme::kAccent.y, theme::kAccent.z, 0.35f);
    dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), ImGui::GetColorU32(fill),
                      theme::kControlRadius);
    dl->AddRect(p, ImVec2(p.x + size.x, p.y + size.y),
                ImGui::GetColorU32(active ? theme::kAccentDim : theme::kBorder),
                theme::kControlRadius);
    ImGui::PushFont(theme::small_font());
    const ImVec2 tsz = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(p.x + (size.x - tsz.x) * 0.5f, p.y + (size.y - tsz.y) * 0.5f),
                ImGui::GetColorU32(active ? theme::kAccent : theme::kTextSecondary), label);
    ImGui::PopFont();
    return clicked;
}

// ---- Metric modules --------------------------------------------------------

// A compact information module: a small uppercase label, a prominent value and a
// subtle status dot. This is the visual building block of the top metric row and
// the right analytics column — deliberately not a large rounded "card". When the
// value is unavailable the module shows an explicit N/A, never a fabricated zero.
inline void metric_module(const char* label, const std::string& value, bool available, ImVec4 color,
                          const char* caption = nullptr) {
    theme::text_label(theme::kTextMuted, "%s", label);
    const float line_h = ImGui::GetTextLineHeight();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    if (available) {
        ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(p.x + 3.0f, p.y + line_h * 0.55f), 3.0f,
                                                    ImGui::GetColorU32(color));
        ImGui::Dummy(ImVec2(10.0f, line_h));
        ImGui::SameLine(0.0f, 0.0f);
        theme::text_title(color, "%s", value.c_str());
    } else {
        theme::text_title(theme::kNotAvailable, "N/A");
    }
    if (caption != nullptr && caption[0] != '\0')
        theme::text_label(theme::kTextMuted, "%s", caption);
}

// A thin horizontal progress bar used inside analytics panels. `value` is a
// fraction in [0,1]; a non-finite/absent value must be handled by the caller.
inline void mini_bar(float value, ImVec4 color, float width = 0.0f, float height = 4.0f) {
    float clamped = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
    const float w = width > 0.0f ? width : ImGui::GetContentRegionAvail().x;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + height), ImGui::GetColorU32(theme::kSurfaceRaised),
                      2.0f);
    dl->AddRectFilled(p, ImVec2(p.x + w * clamped, p.y + height), ImGui::GetColorU32(color), 2.0f);
    ImGui::Dummy(ImVec2(w, height));
}

}  // namespace widgets
}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_AURAWIDGETS_H
