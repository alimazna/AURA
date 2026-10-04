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
// A deep blue-black shell holds slightly lighter graphite surfaces so panels read
// as distinct planes without heavy borders or shadows.
constexpr ImVec4 kBackground{0.039f, 0.071f, 0.114f, 1.00f};     // app shell
constexpr ImVec4 kRail{0.031f, 0.059f, 0.098f, 1.00f};          // far-left icon rail
constexpr ImVec4 kSurface{0.075f, 0.118f, 0.169f, 1.00f};       // panels
constexpr ImVec4 kSurfaceRaised{0.086f, 0.133f, 0.184f, 1.00f};  // header / rows
constexpr ImVec4 kSurfaceInset{0.024f, 0.045f, 0.075f, 1.00f};   // chart canvas
constexpr ImVec4 kBorder{0.129f, 0.176f, 0.243f, 1.00f};
constexpr ImVec4 kBorderStrong{0.196f, 0.259f, 0.337f, 1.00f};
constexpr ImVec4 kTextPrimary{0.878f, 0.906f, 0.933f, 1.00f};
constexpr ImVec4 kTextSecondary{0.710f, 0.749f, 0.800f, 1.00f};
constexpr ImVec4 kTextMuted{0.557f, 0.616f, 0.671f, 1.00f};
constexpr ImVec4 kAccent{0.180f, 0.812f, 0.894f, 1.00f};
constexpr ImVec4 kAccentDim{0.106f, 0.478f, 0.529f, 1.00f};
constexpr ImVec4 kGridline{0.129f, 0.176f, 0.243f, 0.50f};

// State colours. Kept separate from the surface palette so a state is never
// mistaken for decoration.
constexpr ImVec4 kHealthy{0.396f, 0.780f, 0.537f, 1.00f};
constexpr ImVec4 kDegraded{0.894f, 0.694f, 0.290f, 1.00f};
constexpr ImVec4 kCritical{0.863f, 0.373f, 0.408f, 1.00f};
constexpr ImVec4 kPaused{0.616f, 0.529f, 0.878f, 1.00f};
constexpr ImVec4 kNeutral{0.529f, 0.561f, 0.620f, 1.00f};
constexpr ImVec4 kUnknown{0.463f, 0.494f, 0.549f, 1.00f};
constexpr ImVec4 kNotAvailable{0.443f, 0.490f, 0.549f, 1.00f};
// The shadow-only identity is deliberately distinct from every health state.
constexpr ImVec4 kShadow{0.894f, 0.694f, 0.290f, 1.00f};

// ---- Spacing scale ---------------------------------------------------------
constexpr float kSpace1 = 4.0f;
constexpr float kSpace2 = 8.0f;
constexpr float kSpace3 = 12.0f;
constexpr float kSpace4 = 16.0f;
constexpr float kSpace5 = 24.0f;
constexpr float kSpace6 = 32.0f;

constexpr float kSidebarWidth = 200.0f;
constexpr float kTopBarHeight = 46.0f;
constexpr float kStatusBarHeight = 24.0f;
constexpr float kActionBarHeight = 40.0f;
constexpr float kIconRailWidth = 52.0f;
constexpr float kPanelRadius = 6.0f;
constexpr float kControlRadius = 4.0f;

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
    ImFont* hero{nullptr};    // brand / instrument (24)
    ImFont* title{nullptr};   // page & panel titles (17)
    ImFont* body{nullptr};    // default UI text (14)
    ImFont* small{nullptr};   // metadata / captions (12)
    ImFont* label{nullptr};   // tiny uppercase module labels (11)
    ImFont* mono{nullptr};    // numeric values, prices, tables (13)
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
inline ImFont* label_font() { return fonts().label != nullptr ? fonts().label : small_font(); }
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
// The tiny uppercase module label used above metric values and panel captions.
inline void text_label(ImVec4 col, const char* fmt, ...) IM_FMTARGS(2);
inline void text_label(ImVec4 col, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ImGui::PushFont(label_font());
    ImGui::TextColoredV(col, fmt, args);
    ImGui::PopFont();
    va_end(args);
}
// Numeric values (prices, counts, scores) in the monospaced face so columns align.
inline void text_mono(ImVec4 col, const char* fmt, ...) IM_FMTARGS(2);
inline void text_mono(ImVec4 col, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ImGui::PushFont(mono_font());
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

    if (ImFont* h = io.Fonts->AddFontFromFileTTF(body_path.c_str(), 23.0f, &cfg)) f.hero = h;
    if (ImFont* t = io.Fonts->AddFontFromFileTTF(body_path.c_str(), 17.0f, &cfg)) f.title = t;
    if (ImFont* b = io.Fonts->AddFontFromFileTTF(body_path.c_str(), 14.0f, &cfg)) f.body = b;
    if (ImFont* s = io.Fonts->AddFontFromFileTTF(body_path.c_str(), 12.0f, &cfg)) f.small = s;
    if (ImFont* l = io.Fonts->AddFontFromFileTTF(body_path.c_str(), 11.0f, &cfg)) f.label = l;
    if (ImFont* m = io.Fonts->AddFontFromFileTTF(mono_path.c_str(), 13.0f, &cfg)) f.mono = m;

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
    style.ChildRounding = kPanelRadius;
    style.FrameRounding = kControlRadius;
    style.PopupRounding = kPanelRadius;
    style.ScrollbarRounding = kControlRadius;
    style.GrabRounding = kControlRadius;
    style.TabRounding = kControlRadius;

    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;

    style.WindowPadding = ImVec2(kSpace3, kSpace2 + 2.0f);
    style.FramePadding = ImVec2(kSpace2, 4.0f);
    style.CellPadding = ImVec2(kSpace2, 4.0f);
    style.ItemSpacing = ImVec2(kSpace3, kSpace2);
    style.ItemInnerSpacing = ImVec2(6.0f, 5.0f);
    style.IndentSpacing = 16.0f;
    style.ScrollbarSize = 9.0f;
    style.GrabMinSize = 11.0f;

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
