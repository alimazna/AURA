#ifndef AURA_DESKTOP_AURAWIDGETS_H
#define AURA_DESKTOP_AURAWIDGETS_H

// Reusable AURA presentation primitives, built only from ImGui's fixed-function
// draw list (flat rectangles and text). No shaders, no textures, no GPU effects,
// so every widget renders identically on the OpenGL 2.1 legacy backend and the
// OpenGL 3.3 backend.
//
// Widgets are presentation-only: they draw a value they are handed and never
// reach into the runtime. Unknown/absent values are rendered through the neutral
// state treatment so they never read as a healthy value.

#include "desktop/AuraTheme.h"

#include <imgui.h>

#include <cstddef>
#include <string>

namespace aura {
namespace desktop {
namespace widgets {

// Fills a rounded rectangle using only the ImGui draw list (legacy-safe).
inline void filled_rect(ImVec2 lo, ImVec2 hi, ImVec4 col, float rounding) {
    ImGui::GetWindowDrawList()->AddRectFilled(lo, hi, ImGui::GetColorU32(col), rounding);
}

// A bordered card container with a title strip. Call card_end() to close.
inline void card_begin(const char* id, const char* title, ImVec2 size = ImVec2(0, 0),
                       float height = 0.0f) {
    ImGui::BeginChild(id, ImVec2(size.x, height > 0.0f ? height : size.y), true,
                      ImGuiWindowFlags_NoScrollbar);
    if (title != nullptr && title[0] != '\0') {
        ImGui::TextColored(theme::kTextSecondary, "%s", title);
        ImGui::Separator();
        ImGui::Spacing();
    }
}

inline void card_end() { ImGui::EndChild(); }

// A small pill badge. Colour comes from the state category; the label is always
// drawn as text so the state is legible on legacy hardware without colour.
inline void badge(const char* label, ImVec4 color) {
    const ImVec2 pad(theme::kSpace1 * 1.5f, 2.0f);
    const ImVec2 text = ImGui::CalcTextSize(label);
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 lo(pos.x, pos.y);
    const ImVec2 hi(pos.x + text.x + pad.x * 2.0f, pos.y + text.y + pad.y * 2.0f);
    filled_rect(lo, hi, ImVec4(color.x, color.y, color.z, 0.18f), 4.0f);
    ImGui::GetWindowDrawList()->AddRect(lo, hi, ImGui::GetColorU32(color), 4.0f);
    ImGui::SetCursorScreenPos(ImVec2(lo.x + pad.x, lo.y + pad.y));
    ImGui::TextColored(color, "%s", label);
    ImGui::SetCursorScreenPos(ImVec2(hi.x, lo.y));
    ImGui::Dummy(ImVec2(0.0f, text.y + pad.y * 2.0f));
}

// A state badge driven by the shared category map.
inline void state_badge(const std::string& state) {
    const theme::StateCategory c = theme::state_category(state);
    badge(theme::category_label(c), theme::category_color(c));
    ImGui::SameLine();
    // Show the precise contract string next to the category when it adds detail.
    if (std::string(theme::category_label(c)) != state && !state.empty())
        ImGui::TextColored(theme::kTextSecondary, "%s", state.c_str());
}

// A label/value row on one line: muted label, primary value.
inline void kv_row(const char* key, const std::string& value) {
    ImGui::TextColored(theme::kTextMuted, "%s", key);
    ImGui::SameLine();
    const float key_w = ImGui::CalcTextSize(key).x;
    // Right-align the value at a stable column so rows line up.
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (150.0f > key_w ? 150.0f - key_w : 0.0f));
    ImGui::TextColored(theme::is_unknown_like(value) ? theme::kTextMuted : theme::kTextPrimary,
                       "%s", value.c_str());
}

// A label/value row whose value is coloured by its state category.
inline void kv_state_row(const char* key, const std::string& value) {
    ImGui::TextColored(theme::kTextMuted, "%s", key);
    ImGui::SameLine();
    const float key_w = ImGui::CalcTextSize(key).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (150.0f > key_w ? 150.0f - key_w : 0.0f));
    ImGui::TextColored(theme::status_color(value), "%s", value.c_str());
}

// A big "metric" number with a caption. When unavailable the number is replaced
// by an explicit NOT AVAILABLE marker with a reason, never a fake zero.
inline void metric(const char* caption, const std::string& value, bool available,
                   const char* reason = "no data source wired") {
    ImGui::TextColored(theme::kTextMuted, "%s", caption);
    if (available) {
        ImGui::TextColored(theme::kTextPrimary, "%s", value.c_str());
    } else {
        ImGui::TextColored(theme::kNotAvailable, "NOT AVAILABLE");
        ImGui::TextColored(theme::kTextMuted, "%s", reason);
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
    ImGui::TextColored(theme::kTextSecondary, "%s", title);
    ImGui::Spacing();
    ImGui::TextColored(theme::kNotAvailable, "NOT AVAILABLE");
    ImGui::TextWrapped("%s", reason);
}

// A quiet note for a section that IS wired but genuinely has no rows yet (a real
// empty state, distinct from NOT AVAILABLE).
inline void empty_note(const char* reason) {
    ImGui::Spacing();
    ImGui::TextColored(theme::kTextMuted, "%s", reason);
}

// Section header used at the top of the content area.
inline void section_header(const char* title, const char* subtitle) {
    ImGui::TextColored(theme::kTextPrimary, "%s", title);
    if (subtitle != nullptr && subtitle[0] != '\0')
        ImGui::TextColored(theme::kTextMuted, "%s", subtitle);
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
    ImGui::TextColored(theme::status_color(value), "%s", value.c_str());
}

// A cell that shows a value when present, else an explicit NOT AVAILABLE marker.
inline void value_cell(bool available, const std::string& value) {
    if (available) ImGui::TextColored(theme::kTextPrimary, "%s", value.c_str());
    else ImGui::TextColored(theme::kNotAvailable, "NOT AVAILABLE");
}

}  // namespace widgets
}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_AURAWIDGETS_H
