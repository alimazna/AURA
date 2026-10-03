#ifndef AURA_DESKTOP_AURATHEME_H
#define AURA_DESKTOP_AURATHEME_H

// AURA Desktop Control Center design system.
//
// An original visual language for AURA: a deep-navy surface with a single cyan
// accent, and a state colour map that keeps HEALTHY, DEGRADED, CRITICAL,
// UNKNOWN and NOT AVAILABLE visually distinct. UNKNOWN never looks healthy and
// NOT AVAILABLE never looks like a zero.
//
// The state-classification functions are pure and deterministic (no ImGui, no
// GL, no I/O), so they are unit-testable anywhere. `apply_aura_style()` is the
// only part that touches ImGuiStyle; it uses fixed-function-friendly primitives
// (flat colours, no shaders) so the same theme renders on OpenGL 2.1 legacy
// hardware.

#include "desktop/StateVisuals.h"

#include <imgui.h>

#include <cstdint>
#include <string_view>

namespace aura {
namespace desktop {
namespace theme {

// ---- Palette ---------------------------------------------------------------
// Deep, low-glare navy surfaces with one cyan accent. Deliberately flat: no
// gradients, no GPU effects, legacy-renderer safe.
constexpr ImVec4 kBackground{0.055f, 0.063f, 0.086f, 1.00f};
constexpr ImVec4 kSurface{0.086f, 0.098f, 0.129f, 1.00f};
constexpr ImVec4 kSurfaceRaised{0.114f, 0.129f, 0.165f, 1.00f};
constexpr ImVec4 kBorder{0.180f, 0.204f, 0.251f, 1.00f};
constexpr ImVec4 kBorderStrong{0.247f, 0.278f, 0.337f, 1.00f};
constexpr ImVec4 kTextPrimary{0.902f, 0.918f, 0.949f, 1.00f};
constexpr ImVec4 kTextSecondary{0.639f, 0.667f, 0.729f, 1.00f};
constexpr ImVec4 kTextMuted{0.451f, 0.478f, 0.541f, 1.00f};
constexpr ImVec4 kAccent{0.298f, 0.780f, 0.855f, 1.00f};
constexpr ImVec4 kAccentDim{0.180f, 0.470f, 0.529f, 1.00f};

// State colours. Kept separate from the surface palette so a state is never
// mistaken for decoration.
constexpr ImVec4 kHealthy{0.361f, 0.784f, 0.510f, 1.00f};
constexpr ImVec4 kDegraded{0.945f, 0.741f, 0.322f, 1.00f};
constexpr ImVec4 kCritical{0.925f, 0.408f, 0.404f, 1.00f};
constexpr ImVec4 kPaused{0.639f, 0.541f, 0.898f, 1.00f};
constexpr ImVec4 kNeutral{0.549f, 0.573f, 0.635f, 1.00f};
constexpr ImVec4 kUnknown{0.478f, 0.502f, 0.565f, 1.00f};
constexpr ImVec4 kNotAvailable{0.404f, 0.427f, 0.486f, 1.00f};
// The shadow-only identity is deliberately its own colour so the mode banner is
// never confused with a health state.
constexpr ImVec4 kShadow{0.949f, 0.596f, 0.259f, 1.00f};

// ---- Spacing / typography scale -------------------------------------------
constexpr float kSpace1 = 4.0f;
constexpr float kSpace2 = 8.0f;
constexpr float kSpace3 = 12.0f;
constexpr float kSpace4 = 16.0f;
constexpr float kSpace5 = 24.0f;

constexpr float kSidebarWidth = 232.0f;
constexpr float kHeaderHeight = 46.0f;
constexpr float kStatusBarHeight = 30.0f;

// ---- State classification --------------------------------------------------
// state_category(), category_label() and is_unknown_like() live in StateVisuals.h
// (pure, ImGui-free) so they can be unit-tested without a GUI toolkit. This file
// only adds the colour mapping.

inline ImVec4 category_color(StateCategory c) {
    switch (c) {
        case StateCategory::Healthy:      return kHealthy;
        case StateCategory::Degraded:     return kDegraded;
        case StateCategory::Critical:     return kCritical;
        case StateCategory::Paused:       return kPaused;
        case StateCategory::Informational: return kNeutral;
        case StateCategory::NotAvailable: return kNotAvailable;
        case StateCategory::Unknown:      return kUnknown;
    }
    return kUnknown;
}

// Colour lookup for a raw state string. Routes everything through the one
// category map so the app has a single, consistent state language.
inline ImVec4 status_color(std::string_view state) {
    return category_color(state_category(state));
}

// ---- ImGui style -----------------------------------------------------------
// Applies the AURA look: flat surfaces, thin borders, generous padding, and a
// single accent. Uses only fixed-function-friendly style properties.
inline void apply_aura_style() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 0.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;

    style.WindowPadding = ImVec2(kSpace3, kSpace3);
    style.FramePadding = ImVec2(kSpace2, 5.0f);
    style.CellPadding = ImVec2(kSpace2, 5.0f);
    style.ItemSpacing = ImVec2(kSpace2, kSpace2);
    style.ItemInnerSpacing = ImVec2(6.0f, 5.0f);
    style.IndentSpacing = 18.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 10.0f;

    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.5f);

    style.AntiAliasedLines = true;
    style.AntiAliasedFill = true;

    ImVec4* c = style.Colors;
    c[ImGuiCol_WindowBg] = kBackground;
    c[ImGuiCol_ChildBg] = kSurface;
    c[ImGuiCol_PopupBg] = kSurfaceRaised;
    c[ImGuiCol_Border] = kBorder;
    c[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

    c[ImGuiCol_Text] = kTextPrimary;
    c[ImGuiCol_TextDisabled] = kTextMuted;

    c[ImGuiCol_FrameBg] = kSurfaceRaised;
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.157f, 0.180f, 0.227f, 1.0f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.180f, 0.208f, 0.263f, 1.0f);

    c[ImGuiCol_TitleBg] = kSurface;
    c[ImGuiCol_TitleBgActive] = kSurface;
    c[ImGuiCol_TitleBgCollapsed] = kSurface;
    c[ImGuiCol_MenuBarBg] = kSurface;

    c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab] = kBorderStrong;
    c[ImGuiCol_ScrollbarGrabHovered] = kNeutral;
    c[ImGuiCol_ScrollbarGrabActive] = kAccentDim;

    c[ImGuiCol_CheckMark] = kAccent;
    c[ImGuiCol_SliderGrab] = kAccentDim;
    c[ImGuiCol_SliderGrabActive] = kAccent;

    c[ImGuiCol_Button] = kSurfaceRaised;
    c[ImGuiCol_ButtonHovered] = ImVec4(0.157f, 0.180f, 0.227f, 1.0f);
    c[ImGuiCol_ButtonActive] = kAccentDim;

    c[ImGuiCol_Header] = ImVec4(0.157f, 0.196f, 0.243f, 1.0f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.196f, 0.243f, 0.302f, 1.0f);
    c[ImGuiCol_HeaderActive] = kAccentDim;

    c[ImGuiCol_Separator] = kBorder;
    c[ImGuiCol_SeparatorHovered] = kAccentDim;
    c[ImGuiCol_SeparatorActive] = kAccent;

    c[ImGuiCol_Tab] = kSurfaceRaised;
    c[ImGuiCol_TabHovered] = kAccentDim;
    c[ImGuiCol_TabActive] = ImVec4(0.157f, 0.196f, 0.243f, 1.0f);

    c[ImGuiCol_TableHeaderBg] = kSurfaceRaised;
    c[ImGuiCol_TableBorderStrong] = kBorderStrong;
    c[ImGuiCol_TableBorderLight] = kBorder;
    c[ImGuiCol_TableRowBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.018f);

    c[ImGuiCol_NavHighlight] = kAccentDim;
    c[ImGuiCol_PlotLines] = kAccent;
    c[ImGuiCol_PlotHistogram] = kAccent;
}

}  // namespace theme
}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_AURATHEME_H
