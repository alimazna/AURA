// AURA desktop control center (Phase 9 / GUI-0001).
//
// Single canonical entrypoint for the AURA Windows desktop application. It owns
// exactly one runtime (ControlCenterState -> ApplicationShell) and renders the
// human control surface with Dear ImGui + GLFW, preferring an OpenGL 3.3 core
// context and falling back to OpenGL 2.1 compatibility on legacy hardware. It is
// presentation over the existing runtime: it reads state, offers only safe
// control-plane operations (pause/resume the local transport, refresh,
// checkpoint, bounded stop, read-only recovery report), and never places an
// order.
//
// Execution mode is SHADOW ONLY. There is no live-trading action anywhere on
// this surface. Unknown/absent values are shown as NOT AVAILABLE / UNKNOWN, not
// fabricated as healthy. Corrupted or version-incompatible persisted state is
// reported and refused, never auto-resumed.
//
// Modes:
//   aura --gui    [--store <path>] [--serve <port>] [--replay <frames>] [--keep]
//   aura --self-test [--store <path>] [--keep]   headless integration smoke (no window)
//   aura --gui --self-test                        the same, explicitly
//
// This program does not claim profitability, calibration, broker validation or
// production safety.

#include "desktop/AuraTheme.h"
#include "desktop/AuraTerminal.h"
#include "desktop/ControlCenterState.h"
#include "desktop/GuiPanels.h"
#include "desktop/NavigationModel.h"
#include "desktop/RendererPolicy.h"

// Windows headers (pulled in transitively by GLFW) define `min`/`max` macros
// that collide with ordinary identifiers; keep them from rewriting this TU.
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl2.h>
#include <imgui_impl_opengl3.h>

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

// MSVC's system <GL/gl.h> declares OpenGL 1.1 only; GL_CLAMP_TO_EDGE arrived in
// OpenGL 1.2. Define it when the platform header has not, so the brand-mark
// texture upload compiles on Windows without pulling in an extension loader for a
// single enum.
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

// Optional ASTRA brand mark. When the build provides stb_image and a real asset
// exists on disk it is uploaded as a GL texture and drawn in the header; when the
// asset is absent (or the loader is unavailable) the header renders the
// typographic wordmark alone. No logo is ever synthesized or redrawn.
#if defined(AURA_HAVE_STB_IMAGE)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include "stb_image.h"
#endif

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace {

bool has_flag(int argc, char** argv, const char* flag) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == flag) return true;
    }
    return false;
}

std::string value_of(int argc, char** argv, const char* name) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == name) return argv[i + 1];
    }
    return {};
}

int usage() {
    std::fprintf(stderr,
                 "usage:\n"
                 "  aura --gui    [--store <path>] [--serve <port>] [--replay <frames>] [--keep]\n"
                 "                  [--frames <n>]  bounded render for CI smoke\n"
                 "                  [--section <n>] pin one navigation section (0..18)\n"
                 "                  [--renderer auto|modern|legacy]  (default auto: GL3.3 then GL2.1)\n"
                 "  aura --self-test [--store <path>] [--keep]\n"
                 "  (run without --gui and without --self-test, or with --help, for this text)\n"
                 "Execution mode: SHADOW ONLY. No live order path.\n");
    return 2;
}

aura::desktop::ControlCenterOptions parse_options(int argc, char** argv) {
    aura::desktop::ControlCenterOptions options;
    options.store_path = value_of(argc, argv, "--store");
    options.replay_path = value_of(argc, argv, "--replay");
    const std::string port = value_of(argc, argv, "--serve");
    if (!port.empty()) options.serve_port = static_cast<std::uint16_t>(std::atoi(port.c_str()));
    options.keep_store = has_flag(argc, argv, "--keep");
    return options;
}

// Headless integration smoke. Exercises the exact runtime ownership, recovery
// evaluation and snapshot projection the GUI uses, without a window. This is what
// CI can prove; interactive rendering is not faked.
int headless_self_test(const aura::desktop::ControlCenterOptions& options) {
    const std::string store = options.store_path.empty() ? std::string("aura_gui_selftest.aura")
                                                         : options.store_path;
    aura::desktop::ControlCenterOptions o = options;
    o.store_path = store;

    std::printf("aura-gui: self-test starting (store=%s)\n", store.c_str());
    {
        aura::desktop::ControlCenterState state(o);
        // Exercise the nine-stream projection with real deterministic state. If a
        // replay file was supplied, use it; otherwise use the built-in set.
        const std::size_t frames = options.replay_path.empty()
                                       ? state.feed_builtin(30)
                                       : state.feed_replay(options.replay_path);
        std::printf("aura-gui: fed %zu frames\n", frames);
        if (frames == 0) {
            std::printf("aura-gui: SELF-TEST FAIL (no frames fed)\n");
            return 1;
        }
        const auto& snap = state.refresh();
        if (snap.timeframes.rows.size() != 9) {
            std::printf("aura-gui: SELF-TEST FAIL (nine-timeframe display: %zu rows)\n",
                        snap.timeframes.rows.size());
            return 1;
        }
        if (!snap.timeframes.any_present) {
            std::printf("aura-gui: SELF-TEST FAIL (no timeframe stream present after frames)\n");
            return 1;
        }
        if (!snap.shadow_only) {
            std::printf("aura-gui: SELF-TEST FAIL (shadow-only invariant)\n");
            return 1;
        }
        std::printf("aura-gui: snapshot rows=%zu streams=%zu/%zu shadow_only=%s\n",
                    snap.timeframes.rows.size(), snap.overview.streams_healthy,
                    snap.overview.streams_total, snap.shadow_only ? "yes" : "no");

        // ---- XAUUSD candlestick chart / timeframe selector -------------------
        auto& sel = state.timeframe_selection();
        if (sel.selected_label() != "M15") {
            std::printf("aura-gui: SELF-TEST FAIL (chart default timeframe is not M15)\n");
            return 1;
        }
        if (aura::desktop::chart_timeframes().size() != 9) {
            std::printf("aura-gui: SELF-TEST FAIL (chart does not expose nine timeframes)\n");
            return 1;
        }
        // Switch through all nine controls; each must map to its own explicit
        // stream and, since every stream was fed, show real candles.
        for (const aura::runtime::Timeframe tf : aura::desktop::chart_timeframes()) {
            if (!sel.select(tf)) {
                std::printf("aura-gui: SELF-TEST FAIL (could not select %s)\n",
                            std::string(aura::runtime::to_string(tf)).c_str());
                return 1;
            }
            if (sel.selected() != tf || sel.selected_label() != aura::runtime::to_string(tf)) {
                std::printf("aura-gui: SELF-TEST FAIL (selection identity for %s)\n",
                            std::string(aura::runtime::to_string(tf)).c_str());
                return 1;
            }
            const aura::desktop::CandleSeries cs = state.chart_series(tf);
            if (cs.timeframe != tf || !cs.available || cs.candles.empty()) {
                std::printf("aura-gui: SELF-TEST FAIL (no real candles for %s)\n",
                            std::string(aura::runtime::to_string(tf)).c_str());
                return 1;
            }
        }
        // An unsupported selection is refused, never mapped to a bogus stream.
        if (sel.select(aura::runtime::Timeframe::UNKNOWN)) {
            std::printf("aura-gui: SELF-TEST FAIL (UNKNOWN timeframe was accepted)\n");
            return 1;
        }
        std::printf("aura-gui: chart nine timeframes OK, default M15, each stream has real candles\n");

        // Additional V3-37 section projections (read-only; no fabrication).
        const auto& rep = state.refresh_report();
        std::printf("aura-gui: sections version_available=%s ledger_entries=%zu "
                    "incidents_wired=%s incidents=%zu\n",
                    rep.version.available ? "yes" : "no", rep.shadow_ledger.total,
                    rep.incidents.source_wired ? "yes" : "no", rep.incidents.count);
        if (!rep.version.available || rep.version.schema_version.empty()) {
            std::printf("aura-gui: SELF-TEST FAIL (version context not sourced)\n");
            return 1;
        }
        if (rep.shadow_ledger.total == 0 || !rep.shadow_ledger.available) {
            std::printf("aura-gui: SELF-TEST FAIL (shadow ledger not populated from real runtime)\n");
            return 1;
        }
        if (!rep.incidents.source_wired) {
            std::printf("aura-gui: SELF-TEST FAIL (incident detector not wired)\n");
            return 1;
        }
        // These planes have no source wired in the control center yet: they must be
        // NOT AVAILABLE, never fabricated.
        if (rep.predictions.available || rep.knowledge.available || rep.research.available ||
            rep.candidates.available || rep.validation.available || rep.schedule.available) {
            std::printf("aura-gui: SELF-TEST FAIL (an unsourced section fabricated availability)\n");
            return 1;
        }
        const auto status = state.persist_checkpoint();
        std::printf("aura-gui: checkpoint -> %s\n",
                    std::string(aura::foundation::to_string(status)).c_str());
        if (status != aura::foundation::PersistenceStatus::OK) {
            std::printf("aura-gui: SELF-TEST FAIL (checkpoint persist)\n");
            return 1;
        }
    }
    // Cross-process recovery: a fresh owner must see CLEAN_SHUTDOWN and be
    // resumable, and must NOT auto-resume corrupted state.
    {
        aura::desktop::ControlCenterState state(o);
        const auto& outcome = state.evaluate_recovery(false);
        std::printf("aura-gui: recovery -> %s (resumable=%s)\n",
                    std::string(aura::runtime::to_string(outcome.detected)).c_str(),
                    outcome.resumable ? "yes" : "no");
        if (!outcome.resumable) {
            std::printf("aura-gui: SELF-TEST FAIL (recovery not resumable)\n");
            return 1;
        }
        const auto& snap = state.refresh();
        if (snap.persistence.lifecycle.empty()) {
            std::printf("aura-gui: SELF-TEST FAIL (no lifecycle presented)\n");
            return 1;
        }
    }
    if (!options.keep_store) std::remove(store.c_str());
    std::printf("aura-gui: SELF-TEST PASS\n");
    return 0;
}

void glfw_error_callback(int error, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

// A loaded brand mark: a GL texture plus its pixel size. `texture` is 0 when no
// real asset was found, in which case the header draws the wordmark alone.
struct BrandMark {
    unsigned int texture{0};
    int width{0};
    int height{0};
    bool loaded() const { return texture != 0; }
};

// Directory containing the running executable, so the packaged app finds its
// staged assets next to itself regardless of the working directory. Falls back to
// an empty string when it cannot be determined.
std::string executable_dir() {
#if defined(_WIN32)
    char buf[1024];
    const DWORD n = GetModuleFileNameA(nullptr, buf, sizeof(buf));
    if (n == 0 || n >= sizeof(buf)) return {};
    std::string p(buf, n);
    const auto pos = p.find_last_of("\\/");
    return pos == std::string::npos ? std::string{} : p.substr(0, pos);
#else
    char buf[1024];
    const ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) return {};
    buf[n] = '\0';
    std::string p(buf);
    const auto pos = p.find_last_of('/');
    return pos == std::string::npos ? std::string{} : p.substr(0, pos);
#endif
}

std::string join_path(const std::string& dir, const std::string& leaf) {
    if (dir.empty()) return leaf;
    std::string s = dir;
    if (s.back() != '/' && s.back() != '\\') s.push_back('/');
    s += leaf;
    return s;
}

// Attempts to load the real ASTRA brand mark. It looks next to the executable
// first (the packaged/staged location), then in the build's asset directory.
// Returns an empty mark when no asset exists or the loader is unavailable. It
// never draws a substitute logo.
BrandMark load_brand_mark() {
    BrandMark mark;
#if defined(AURA_HAVE_STB_IMAGE)
    const std::string exe_dir = executable_dir();
    const char* names[] = {"astra-mark.png", "astra-logo.png", "astra.png"};
    std::vector<std::string> candidates;
    for (const char* name : names) {
        if (!exe_dir.empty()) {
            candidates.push_back(join_path(join_path(exe_dir, "assets/brand"), name));
            candidates.push_back(join_path(exe_dir, name));
        }
#if defined(AURA_BRAND_DIR)
        candidates.push_back(join_path(AURA_BRAND_DIR, name));
#endif
    }
    for (const std::string& path : candidates) {
        int w = 0, h = 0, comp = 0;
        stbi_set_flip_vertically_on_load(0);
        unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &comp, 4);
        if (pixels == nullptr) continue;
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        stbi_image_free(pixels);
        mark.texture = tex;
        mark.width = w;
        mark.height = h;
        std::printf("aura-gui: loaded ASTRA brand mark %s (%dx%d)\n", path.c_str(), w, h);
        break;
    }
#endif
    return mark;
}

// A created window/context plus which ImGui backend must drive it.
struct RendererContext {
    GLFWwindow* window{nullptr};
    aura::desktop::RendererProfile profile{aura::desktop::RendererProfile::NONE};
    const char* glsl_version{"#version 120"};
};

// Create a window/context by applying the pure policy's hints for `profile`.
// This is the ONLY place GLFW window hints are set. The legacy profile never
// requests a core profile, so legacy drivers (Intel HD 3000) are not rejected.
RendererContext create_context(aura::desktop::RendererProfile profile) {
    RendererContext ctx;
    const aura::desktop::RendererHints hints = aura::desktop::hints_for(profile);
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, hints.context_version_major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, hints.context_version_minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, hints.request_core_profile ? GLFW_OPENGL_CORE_PROFILE
                                                                   : GLFW_OPENGL_ANY_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, hints.forward_compatible ? GLFW_TRUE : GLFW_FALSE);
    // Size the terminal to the display work area so it fills the screen at
    // whatever resolution the operator has (a trading terminal is used full-size).
    // Falls back to a sane 1440x900 when no monitor is reported.
    int win_w = 1440, win_h = 900;
    if (GLFWmonitor* mon = glfwGetPrimaryMonitor()) {
        int wx = 0, wy = 0, ww = 0, wh = 0;
        glfwGetMonitorWorkarea(mon, &wx, &wy, &ww, &wh);
        if (ww >= 1024 && wh >= 640) { win_w = ww; win_h = wh; }
    }
    ctx.window = glfwCreateWindow(win_w, win_h, "ASTRA — XAUUSD Market Intelligence (SHADOW ONLY)", nullptr, nullptr);
    if (ctx.window != nullptr) {
        // Start maximized so the terminal fills the desktop work area immediately
        // at launch. This is a normal, resizable maximized window (not an exclusive
        // fullscreen mode), so standard Windows usability is preserved. The layout
        // derives from the actual framebuffer size, so it adapts to the real screen.
        glfwMaximizeWindow(ctx.window);
        ctx.profile = profile;
        ctx.glsl_version = hints.glsl_version;
    }
    return ctx;
}

// Try the modern OpenGL 3.3 core renderer, then the legacy OpenGL 2.1
// compatibility renderer. Records each observed attempt (never fabricates
// capability) and returns the active context, or a context with a null window
// when no renderer could be created.
//
// `choice` lets CI pin one path: MODERN forces only OpenGL 3.3 (proves the
// modern renderer), LEGACY forces only OpenGL 2.1 (a legacy-renderer smoke on
// software GL), AUTO tries modern then falls back.
RendererContext create_best_context(aura::desktop::RendererChoice choice) {
    aura::desktop::RendererSelector selector;
    if (choice == aura::desktop::RendererChoice::MODERN) {
        selector = aura::desktop::RendererSelector::only(aura::desktop::RendererProfile::MODERN_GL33);
    } else if (choice == aura::desktop::RendererChoice::LEGACY) {
        selector = aura::desktop::RendererSelector::only(aura::desktop::RendererProfile::LEGACY_GL21);
    }
    while (!selector.done()) {
        const aura::desktop::RendererProfile candidate = selector.candidate();
        RendererContext ctx = create_context(candidate);
        aura::desktop::RendererAttempt attempt;
        attempt.profile = candidate;
        attempt.created = ctx.window != nullptr;
        attempt.glfw_error = ctx.window == nullptr ? glfwGetError(nullptr) : 0;
        if (ctx.window == nullptr) {
            std::fprintf(stderr, "aura-gui: %s context unavailable (GLFW error %d)\n",
                         aura::desktop::to_string(candidate), attempt.glfw_error);
        }
        if (selector.record(attempt)) {
            // The modern attempt's GLFW error (0 if modern succeeded) so the
            // diagnostic can report "OpenGL 3.3 unavailable" only when it truly was.
            const int modern_error = selector.error_for(aura::desktop::RendererProfile::MODERN_GL33);
            std::printf("%s\n",
                        aura::desktop::renderer_diagnostic(selector.active(), modern_error,
                                                           "context created")
                            .c_str());
            std::fflush(stdout);
            return ctx;
        }
        if (ctx.window != nullptr) glfwDestroyWindow(ctx.window);
    }
    std::fprintf(stderr, "%s\n", aura::desktop::no_renderer_error(selector.attempts()).c_str());
    RendererContext none;
    return none;
}

// Interactive control center. Returns the process exit code. `max_frames` bounds
// the render loop (0 = unbounded); a non-zero bound is used by CI to prove the
// window/render lifecycle starts and shuts down cleanly, then exits.
int run_gui(const aura::desktop::ControlCenterOptions& options, long max_frames,
            aura::desktop::RendererChoice renderer_choice, int pin_section = -1) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::fprintf(stderr, "aura-gui: GLFW init failed (no windowing system?)\n");
        return 3;
    }

    // Modern-first with a safe legacy fallback. Never terminate merely because
    // OpenGL 3.3 is unavailable; only fail when no context at all can be made.
    const RendererContext rctx = create_best_context(renderer_choice);
    if (rctx.window == nullptr) {
        glfwTerminate();
        return 3;
    }
    GLFWwindow* window = rctx.window;
    const bool legacy = rctx.profile == aura::desktop::RendererProfile::LEGACY_GL21;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;  // deterministic; no user-path ini file
    // Real type hierarchy: load the AURA font roles (Roboto-Medium / Cousine) from
    // the directory the build copies them to. Missing fonts fall back to ImGui's
    // built-in face, so the terminal still runs.
    aura::desktop::theme::load_fonts(AURA_FONT_DIR);
    aura::desktop::theme::apply_aura_style();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    if (legacy) {
        // Legacy OpenGL 2.1 compatibility context: use the fixed-function ImGui
        // OpenGL2 backend (GLSL #version 120), not the OpenGL3 core backend.
        ImGui_ImplOpenGL2_Init();
    } else {
        ImGui_ImplOpenGL3_Init(rctx.glsl_version);
    }

    aura::desktop::ControlCenterState state(options);
    // Optional real brand mark (present only when a real asset shipped). The
    // texture must be created while the GL context is current, which it is here.
    const BrandMark brand = load_brand_mark();
    aura::desktop::terminal::BrandMarkView brand_view;
    if (brand.loaded()) {
        brand_view.texture = reinterpret_cast<ImTextureID>(static_cast<intptr_t>(brand.texture));
        brand_view.aspect =
            brand.height > 0 ? static_cast<float>(brand.width) / static_cast<float>(brand.height)
                             : 1.0f;
    }
    // Report-only recovery evaluation at boot; it never auto-resumes state.
    {
        const aura::runtime::RecoveryOutcome& outcome = state.evaluate_recovery(false);
        std::printf("aura-gui: boot recovery -> %s (resumable=%s): %s\n",
                    std::string(aura::runtime::to_string(outcome.detected)).c_str(),
                    outcome.resumable ? "yes" : "no", outcome.reason.c_str());
    }
    if (!state.start_transport()) {
        std::fprintf(stderr, "aura-gui: failed to bind serve port %u\n",
                     static_cast<unsigned>(options.serve_port));
    }

    // If a canonical frames file was supplied, feed it once so the window opens
    // on real retained closed bars (this is the same trusted pipeline path the
    // headless self-test and the console host use; no synthetic chart data).
    if (!options.replay_path.empty()) {
        const std::size_t fed = state.feed_replay();
        std::printf("aura-gui: fed %zu replay frames from %s\n", fed,
                    options.replay_path.c_str());
    }

    const std::vector<aura::desktop::NavGroup> nav_groups =
        aura::desktop::navigation_groups();
    int selected = 0;
    if (pin_section >= 0 &&
        pin_section < static_cast<int>(aura::desktop::navigation_count()))
        selected = pin_section;

    long rendered = 0;
    bool exit_dialog_open = false;
    bool exit_confirmed = false;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Advance the transport by one bounded cycle so the loop stays responsive.
        state.poll_transport(0);
        const auto& report = state.refresh_report();
        const auto& snap = report.snapshot;

        ImGui_ImplGlfw_NewFrame();
        if (legacy) {
            ImGui_ImplOpenGL2_NewFrame();
        } else {
            ImGui_ImplOpenGL3_NewFrame();
        }
        ImGui::NewFrame();

        // Safe control-plane shortcuts only: pause/resume the local transport,
        // persist a checkpoint, request a bounded stop. There is deliberately no
        // order/buy/sell shortcut and none is reachable from the UI.
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_P)) state.toggle_pause();
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_S)) state.persist_checkpoint();
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) state.request_stop();

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("AURA", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar);
        ImGui::PopStyleVar();

        const std::string renderer = std::string(aura::desktop::to_string(rctx.profile));

        // ---- Top terminal bar -------------------------------------------------
        const std::string page_title =
            aura::desktop::section_title_for(selected);
        aura::desktop::terminal::top_bar(snap, page_title, renderer, state.paused(), brand_view);

        // ---- Body: icon rail + navigation + main workspace ---------------------
        const float status_h = aura::desktop::theme::kStatusBarHeight;
        const float action_h = aura::desktop::theme::kActionBarHeight;
        const float body_h = ImGui::GetContentRegionAvail().y - status_h - action_h;
        ImGui::BeginChild("body", ImVec2(0.0f, body_h), false,
                          ImGuiWindowFlags_NoScrollbar);

        const float window_w = ImGui::GetMainViewport()->Size.x;
        const int rail_clicked = aura::desktop::terminal::icon_rail(nav_groups, selected);
        if (rail_clicked >= 0) selected = rail_clicked;
        ImGui::SameLine(0.0f, 0.0f);

        const aura::desktop::terminal::SidebarResult nav =
            aura::desktop::terminal::sidebar(nav_groups, selected,
                                             aura::desktop::layout::sidebar_width(window_w),
                                             brand_view);
        if (nav.clicked >= 0) selected = nav.clicked;
        // EXIT (lower-left sidebar footer) opens the confirmation dialog; a
        // confirmed exit is routed through the normal shutdown lifecycle below.
        if (nav.exit_pressed) exit_dialog_open = true;

        ImGui::SameLine();

        ImGui::BeginChild("content", ImVec2(0.0f, 0.0f), false);
        aura::desktop::draw_section(state, report, selected);
        ImGui::EndChild();

        ImGui::EndChild();

        // ---- Compact action / control bar (safe control-plane ops only) --------
        const unsigned actions = aura::desktop::terminal::action_bar(state, report);
        if (actions & aura::desktop::terminal::kActionRefresh) state.refresh_report();
        if (actions & aura::desktop::terminal::kActionCheckpoint) state.persist_checkpoint();
        if (actions & aura::desktop::terminal::kActionPauseToggle) state.toggle_pause();
        if (actions & aura::desktop::terminal::kActionStop) state.request_stop();
        if (actions & aura::desktop::terminal::kActionRecovery) state.evaluate_recovery(false);

        // ---- Slim status bar: real runtime state only -------------------------
        aura::desktop::terminal::status_bar(state, report, renderer);

        // ---- EXIT confirmation (modal) ----------------------------------------
        if (aura::desktop::terminal::exit_confirmation(exit_dialog_open))
            exit_confirmed = true;

        ImGui::End();

        ImGui::Render();
        int w = 0, h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.07f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        if (legacy) {
            ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        } else {
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
        glfwSwapBuffers(window);

        if (max_frames > 0 && ++rendered >= max_frames) {
            std::printf("aura-gui: rendered %ld frames (bounded smoke); requesting close\n",
                        rendered);
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        } else if (exit_confirmed) {
            std::printf("aura-gui: EXIT confirmed; requesting clean close\n");
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        } else if (max_frames > 0 && pin_section < 0) {
            // Bounded smoke: cycle through every V3-37 section so each panel's
            // draw path is exercised (a real render of each section, not just the
            // default one) before the bounded loop exits. A pinned section stays
            // on one page (used for deterministic screenshots).
            selected = (selected + 1) % static_cast<int>(aura::desktop::navigation_count());
        }
    }

    // Clean shutdown: persist a checkpoint with clean intent.
    const auto status = state.persist_checkpoint();
    std::printf("aura-gui: shutdown checkpoint -> %s\n",
                std::string(aura::foundation::to_string(status)).c_str());

    if (legacy) {
        ImGui_ImplOpenGL2_Shutdown();
    } else {
        ImGui_ImplOpenGL3_Shutdown();
    }
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    const bool gui = has_flag(argc, argv, "--gui");
    const bool self_test = has_flag(argc, argv, "--self-test");
    if (!gui && !self_test) return usage();

    const aura::desktop::ControlCenterOptions options = parse_options(argc, argv);
    if (self_test) return headless_self_test(options);
    const std::string frames = value_of(argc, argv, "--frames");
    const long max_frames = frames.empty() ? 0 : std::atol(frames.c_str());
    const aura::desktop::RendererChoice choice =
        aura::desktop::renderer_choice_from_string(value_of(argc, argv, "--renderer"));
    const std::string sec = value_of(argc, argv, "--section");
    const int pin_section = sec.empty() ? -1 : std::atoi(sec.c_str());
    return run_gui(options, max_frames, choice, pin_section);
}
