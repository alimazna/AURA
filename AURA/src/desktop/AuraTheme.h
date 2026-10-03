#ifndef AURA_DESKTOP_AURATHEME_H
#define AURA_DESKTOP_AURATHEME_H

// AURA Terminal design system.
//
// A muted graphite/navy terminal palette with one restrained cyan accent, a
// small set of true type roles (hero / title / body / small / mono) and a single
// spacing scale. Everything is flat and fixed-function friendly: no gradients,
// no shaders, no textures beyond the font atlas, so the identical design renders
// on the OpenGL 3.3 core backend and the legacy OpenGL 2.1 backend.
//
// State classification lives in StateVisuals.h (pure, ImGui-free) so it is
// unit-testable; this file only maps state to colour and holds the style.

#include "desktop/StateVisuals.h"

#include <imgui.h>

#include <cstdarg>
#include <string>

namespace aura {
namespace desktop {
namespace theme {

// ---- Palette ---------------------------------------------------------------
// Neutral, low-glare surfaces dominate. The accent is used sparingly for the
// selected navigation item, the active timeframe and genuinely important values.
constexpr ImVec4 kBackground{0.043f, 0.055f, 0.075f, 1.00f};     // app chrome
constexpr ImVec4 kSurface{0.071f, 0.086f, 0.114f, 1.00f};        // panels
constexpr ImVec4 kSurfaceRaised{0.094f, 0.113f, 0.145f, 1.00f};  // header / rows
constexpr ImVec4 kSurfaceInset{0.035f, 0.043f, 0.059f, 1.00f};   // chart canvas
constexpr ImVec4 kBorder{0.129f, 0.157f, 0.196f, 1.00f};
constexpr ImVec4 kBorderStrong{0.188f, 0.224f, 0.278f, 1.00f};
constexpr ImVec4 kTextPrimary{0.898f, 0.918f, 0.945f, 1.00f};
constexpr ImVec4 kTextSecondary{0.647f, 0.686f, 0.741f, 1.00f};
constexpr ImVec4 kTextMuted{0.435f, 0.478f, 0.541f, 1.00f};
constexpr ImVec4 kAccent{0.290f, 0.780f, 0.855f, 1.00f};
constexpr ImVec4 kAccentDim{0.161f, 0.435f, 0.494f, 1.00f};
constexpr ImVec4 kGridline{0.129f, 0.157f, 0.196f, 0.55f};

// State colours. Kept separate from the surface palette so a state is never
// mistaken for decoration.
constexpr ImVec4 kHealthy{0.325f, 0.760f, 0.494f, 1.00f};
constexpr ImVec4 kDegraded{0.937f, 0.722f, 0.294f, 1.00f};
constexpr ImVec4 kCritical{0.910f, 0.376f, 0.376f, 1.00f};
constexpr ImVec4 kPaused{0.616f, 0.529f, 0.878f, 1.00f};
constexpr ImVec4 kNeutral{0.529f, 0.561f, 0.620f, 1.00f};
constexpr ImVec4 kUnknown{0.463f, 0.494f, 0.549f, 1.00f};
constexpr ImVec4 kNotAvailable{0.376f, 0.404f, 0.463f, 1.00f};
// The shadow-only identity is deliberately distinct from every health state.
constexpr ImVec4 kShadow{0.937f, 0.596f, 0.247f, 1.00f};

// ---- Spacing scale ---------------------------------------------------------
constexpr float kSpace1 = 4.0f;
constexpr float kSpace2 = 8.0f;
constexpr float kSpace3 = 12.0f;
constexpr float kSpace4 = 16.0f;
constexpr float kSpace5 = 24.0f;
constexpr float kSpace6 = 32.0f;

constexpr float kSidebarWidth = 216.0f;
constexpr float kTopBarHeight = 44.0f;
constexpr float kStatusBarHeight = 24.0f;

// ---- State classification --------------------------------------------------
inline ImVec4 category_color(StateCategory c) {
    switch (c) {
        case StateCategory::Healthy:       return kHealthy;
        case StateCategory::Degraded:      return kDegraded;
        case StateCategory::Critical:      return kCritical;
        case StateCategory::Paused:        return kPaused;
        case StateCategory::Informational: return kNeutral;
        case StateCategory::NotAvailable:  return kNotAvailable;
        case StateCategory::Unknown:       return kUnknown;
    }
    return kUnknown;
}

inline ImVec4 status_color(std::string_view state) {
    return category_color(state_category(state));
}

// ---- Typography ------------------------------------------------------------
// True type roles. When a font could not be loaded the pointer is null and the
// helpers below fall back to ImGui's default font, so the application still runs
// (deterministically, just with the built-in typeface).
struct AuraFonts {
    ImFont* hero{nullptr};    // brand / instrument
    ImFont* title{nullptr};   // page & panel titles
    ImFont* body{nullptr};    // default UI text
    ImFont* small{nullptr};   // metadata / captions
    ImFont* mono{nullptr};    // numeric values, prices, tables
    bool loaded{false};
};

inline AuraFonts& fonts() {
    static AuraFonts f;
    return f;
}

inline ImFont* hero_font() { return fonts().hero != nullptr ? fonts().hero : ImGui::GetFont(); }
inline ImFont* title_font() { return fonts().title != nullptr ? fonts().title : ImGui::GetFont(); }
inline ImFont* body_font() { return fonts().body != nullptr ? fonts().body : ImGui::GetFont(); }
inline ImFont* small_font() { return fonts().small != nullptr ? fonts().small : ImGui::GetFont(); }
inline ImFont* mono_font() { return fonts().mono != nullptr ? fonts().mono : ImGui::GetFont(); }

// Convenience text wrappers that push the requested type role.
inline void text_hero(ImVec4 col, const char* fmt, ...) IM_FMTARGS(2);
inline void text_hero(ImVec4 col, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ImGui::PushFont(hero_font());
    ImGui::TextColoredV(col, fmt, args);
    ImGui::PopFont();
    va_end(args);
}
inline void text_title(ImVec4 col, const char* fmt, ...) IM_FMTARGS(2);
inline void text_title(ImVec4 col, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ImGui::PushFont(title_font());
    ImGui::TextColoredV(col, fmt, args);
    ImGui::PopFont();
    va_end(args);
}
inline void text_body(ImVec4 col, const char* fmt, ...) IM_FMTARGS(2);
inline void text_body(ImVec4 col, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ImGui::PushFont(body_font());
    ImGui::TextColoredV(col, fmt, args);
    ImGui::PopFont();
    va_end(args);
}
inline void text_small(ImVec4 col, const char* fmt, ...) IM_FMTARGS(2);
inline void text_small(ImVec4 col, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ImGui::PushFont(small_font());
    ImGui::TextColoredV(col, fmt, args);
    ImGui::PopFont();
    va_end(args);
}

// Loads the AURA type roles from `font_dir` (the directory the build copies the
// fonts to). Every role is optional: a missing file leaves that role null and the
// UI falls back to the default font. Returns true when at least the body face was
// loaded. Nothing here reads a clock or performs I/O beyond the atlas upload.
inline bool load_fonts(const char* font_dir) {
    ImGuiIO& io = ImGui::GetIO();
    AuraFonts& f = fonts();

    auto join = [](const char* dir, const char* file) {
        std::string s(dir != nullptr ? dir : "");
        if (!s.empty() && s.back() != '/' && s.back() != '\\') s.push_back('/');
        s += file;
        return s;
    };

    // The regular UI face. Roboto-Medium and Cousine-Regular ship with Dear ImGui
    // (misc/fonts); the build copies them next to the executable.
    const std::string body_path = join(font_dir, "Roboto-Medium.ttf");
    const std::string mono_path = join(font_dir, "Cousine-Regular.ttf");

    ImFontConfig cfg;
    cfg.OversampleH = 1;  // crisp, and cheap on legacy GPUs
    cfg.OversampleV = 1;
    cfg.PixelSnapH = true;

    if (ImFont* h = io.Fonts->AddFontFromFileTTF(body_path.c_str(), 24.0f, &cfg)) f.hero = h;
    if (ImFont* t = io.Fonts->AddFontFromFileTTF(body_path.c_str(), 18.0f, &cfg)) f.title = t;
    if (ImFont* b = io.Fonts->AddFontFromFileTTF(body_path.c_str(), 15.0f, &cfg)) f.body = b;
    if (ImFont* s = io.Fonts->AddFontFromFileTTF(body_path.c_str(), 13.0f, &cfg)) f.small = s;
    if (ImFont* m = io.Fonts->AddFontFromFileTTF(mono_path.c_str(), 14.0f, &cfg)) f.mono = m;

    // Nothing loaded (fonts absent): keep ImGui's built-in ProggyClean so the app
    // still runs, and remember that the roles are unavailable.
    if (f.body == nullptr && f.hero == nullptr && f.title == nullptr) {
        io.Fonts->AddFontDefault();
        f.loaded = false;
        return false;
    }
    if (f.body != nullptr) io.FontDefault = f.body;
    f.loaded = true;
    return true;
}

// ---- ImGui style -----------------------------------------------------------
// Applies the terminal look: flat surfaces, hairline borders, tight-but-legible
// padding, small radii. Uses only fixed-function-friendly style properties.
inline void apply_aura_style() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 0.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;

    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;

    style.WindowPadding = ImVec2(kSpace3, kSpace2 + 2.0f);
    style.FramePadding = ImVec2(kSpace2 + 2.0f, 5.0f);
    style.CellPadding = ImVec2(kSpace2, 4.0f);
    style.ItemSpacing = ImVec2(kSpace3, kSpace2);
    style.ItemInnerSpacing = ImVec2(6.0f, 5.0f);
    style.IndentSpacing = 16.0f;
    style.ScrollbarSize = 10.0f;
    style.GrabMinSize = 12.0f;

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
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.141f, 0.169f, 0.212f, 1.0f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.165f, 0.196f, 0.243f, 1.0f);

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
    c[ImGuiCol_ButtonHovered] = ImVec4(0.141f, 0.169f, 0.212f, 1.0f);
    c[ImGuiCol_ButtonActive] = kAccentDim;

    c[ImGuiCol_Header] = ImVec4(0.141f, 0.169f, 0.212f, 1.0f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.169f, 0.208f, 0.259f, 1.0f);
    c[ImGuiCol_HeaderActive] = kAccentDim;

    c[ImGuiCol_Separator] = kBorder;
    c[ImGuiCol_SeparatorHovered] = kAccentDim;
    c[ImGuiCol_SeparatorActive] = kAccent;

    c[ImGuiCol_Tab] = kSurfaceRaised;
    c[ImGuiCol_TabHovered] = kAccentDim;
    c[ImGuiCol_TabActive] = ImVec4(0.141f, 0.169f, 0.212f, 1.0f);

    c[ImGuiCol_TableHeaderBg] = kSurfaceRaised;
    c[ImGuiCol_TableBorderStrong] = kBorderStrong;
    c[ImGuiCol_TableBorderLight] = kBorder;
    c[ImGuiCol_TableRowBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.014f);

    c[ImGuiCol_NavHighlight] = kAccentDim;
    c[ImGuiCol_PlotLines] = kAccent;
    c[ImGuiCol_PlotHistogram] = kAccent;
}

}  // namespace theme
}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_AURATHEME_H
