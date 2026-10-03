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

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

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
    ctx.window = glfwCreateWindow(1360, 860, "AURA Control Center (SHADOW ONLY)", nullptr, nullptr);
    if (ctx.window != nullptr) {
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
            aura::desktop::RendererChoice renderer_choice) {
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
    io.FontGlobalScale = 1.10f;  // comfortable terminal-density type
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

    long rendered = 0;
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

        // ---- Top bar: brand block (left) / real status (right) --------------
        {
            const float bar_h = aura::desktop::theme::kHeaderHeight;
            ImGui::BeginChild("topbar", ImVec2(0.0f, bar_h), false,
                              ImGuiWindowFlags_NoScrollbar);
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const float full_w = ImGui::GetWindowWidth();
            aura::desktop::widgets::filled_rect(
                p, ImVec2(p.x + full_w, p.y + bar_h),
                aura::desktop::theme::kSurfaceRaised, 0.0f);
            // A single accent underline separates the header from the workspace.
            aura::desktop::widgets::filled_rect(
                ImVec2(p.x, p.y + bar_h - 1.0f), ImVec2(p.x + full_w, p.y + bar_h),
                aura::desktop::theme::kAccent, 0.0f);

            // Left: brand / instrument / descriptor.
            ImGui::SetCursorPos(ImVec2(aura::desktop::theme::kSpace3,
                                       (bar_h - ImGui::GetTextLineHeight()) * 0.5f));
            ImGui::TextColored(aura::desktop::theme::kAccent, "AURA");
            ImGui::SameLine(0.0f, aura::desktop::theme::kSpace2);
            ImGui::TextColored(aura::desktop::theme::kTextPrimary, "XAUUSD");
            ImGui::SameLine(0.0f, aura::desktop::theme::kSpace2);
            ImGui::TextColored(aura::desktop::theme::kTextMuted, "MARKET INTELLIGENCE");

            // Right: real, right-aligned status. Widths are measured, not offset
            // by a hard-coded pixel count.
            const std::string tf = state.timeframe_selection().selected_label();
            const std::string market = snap.overview.healthy ? "HEALTHY" : snap.overview.aggregate;
            const std::string renderer = std::string(aura::desktop::to_string(rctx.profile));
            const char* sep = "   ";
            const float w_sep = ImGui::CalcTextSize(sep).x;
            const float total = ImGui::CalcTextSize("TF").x + 6.0f + ImGui::CalcTextSize(tf.c_str()).x +
                                w_sep * 4.0f +
                                ImGui::CalcTextSize("SHADOW ONLY").x + w_sep +
                                ImGui::CalcTextSize("MARKET").x + 6.0f +
                                ImGui::CalcTextSize(market.c_str()).x + w_sep +
                                ImGui::CalcTextSize("RENDERER").x + 6.0f +
                                ImGui::CalcTextSize(renderer.c_str()).x;
            const float right_x = full_w - total - aura::desktop::theme::kSpace3;
            ImGui::SetCursorPosX(right_x > 0.0f ? right_x : full_w * 0.4f);
            ImGui::SetCursorPosY((bar_h - ImGui::GetTextLineHeight()) * 0.5f);
            ImGui::TextColored(aura::desktop::theme::kTextMuted, "TF");
            ImGui::SameLine(0.0f, 6.0f);
            ImGui::TextColored(aura::desktop::theme::kAccent, "%s", tf.c_str());
            ImGui::SameLine(0.0f, aura::desktop::theme::kSpace3);
            ImGui::TextColored(aura::desktop::theme::kShadow, "SHADOW ONLY");
            ImGui::SameLine(0.0f, aura::desktop::theme::kSpace3);
            ImGui::TextColored(aura::desktop::theme::kTextMuted, "MARKET");
            ImGui::SameLine(0.0f, 6.0f);
            ImGui::TextColored(
                aura::desktop::theme::status_color(snap.overview.healthy ? "HEALTHY"
                                                                         : snap.overview.aggregate),
                "%s", market.c_str());
            ImGui::SameLine(0.0f, aura::desktop::theme::kSpace3);
            ImGui::TextColored(aura::desktop::theme::kTextMuted, "RENDERER");
            ImGui::SameLine(0.0f, 6.0f);
            ImGui::TextColored(aura::desktop::theme::kTextSecondary, "%s", renderer.c_str());
            ImGui::EndChild();
        }

        // ---- Body: sidebar + content ----------------------------------------
        const float status_h = aura::desktop::theme::kStatusBarHeight;
        const float body_h = ImGui::GetContentRegionAvail().y - status_h;
        ImGui::BeginChild("body", ImVec2(0.0f, body_h), false,
                          ImGuiWindowFlags_NoScrollbar);

        ImGui::BeginChild("sidebar", ImVec2(aura::desktop::theme::kSidebarWidth, 0.0f), true);
        const float nav_w = ImGui::GetContentRegionAvail().x;
        for (const aura::desktop::NavGroup& g : nav_groups) {
            ImGui::Spacing();
            ImGui::TextColored(aura::desktop::theme::kTextMuted, "%s", g.title.c_str());
            ImGui::Spacing();
            for (const aura::desktop::NavItem& item : g.items) {
                if (aura::desktop::widgets::nav_item(item.label.c_str(), selected == item.index, nav_w))
                    selected = item.index;
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("content", ImVec2(0.0f, 0.0f), true);
        aura::desktop::draw_section(state, report, selected);
        ImGui::EndChild();

        ImGui::EndChild();

        // ---- Status bar: slim, real values only ----------------------------
        {
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const float w = ImGui::GetContentRegionAvail().x;
            aura::desktop::widgets::filled_rect(p, ImVec2(p.x + w, p.y + status_h),
                                                aura::desktop::theme::kSurfaceRaised, 0.0f);
            aura::desktop::widgets::filled_rect(
                ImVec2(p.x, p.y), ImVec2(p.x + w, p.y + 1.0f),
                aura::desktop::theme::kBorder, 0.0f);
            ImGui::SetCursorPos(ImVec2(aura::desktop::theme::kSpace3,
                                       (status_h - ImGui::GetTextLineHeight()) * 0.5f));
            ImGui::TextColored(aura::desktop::theme::kTextMuted, "RUNTIME");
            ImGui::SameLine(0.0f, 5.0f);
            ImGui::TextColored(state.paused() ? aura::desktop::theme::kPaused
                                              : aura::desktop::theme::kHealthy,
                               "%s", state.paused() ? "PAUSED" : "RUNNING");
            ImGui::SameLine(0.0f, aura::desktop::theme::kSpace3);
            ImGui::TextColored(aura::desktop::theme::kTextMuted, "DATA");
            ImGui::SameLine(0.0f, 5.0f);
            ImGui::TextColored(aura::desktop::theme::status_color(snap.overview.aggregate), "%s",
                               snap.overview.aggregate.c_str());
            ImGui::SameLine(0.0f, aura::desktop::theme::kSpace3);
            ImGui::TextColored(aura::desktop::theme::kTextMuted, "PERSISTENCE");
            ImGui::SameLine(0.0f, 5.0f);
            ImGui::TextColored(aura::desktop::theme::status_color(snap.persistence.last_status),
                               "%s", snap.persistence.last_status.c_str());
            ImGui::SameLine(0.0f, aura::desktop::theme::kSpace3);
            ImGui::TextColored(aura::desktop::theme::kTextMuted, "RECOVERY");
            ImGui::SameLine(0.0f, 5.0f);
            ImGui::TextColored(aura::desktop::theme::status_color(snap.persistence.lifecycle),
                               "%s", snap.persistence.lifecycle.c_str());
            ImGui::SameLine(0.0f, aura::desktop::theme::kSpace3);
            ImGui::TextColored(aura::desktop::theme::kShadow, "SHADOW ONLY");
            ImGui::SameLine(0.0f, aura::desktop::theme::kSpace3);
            ImGui::TextColored(aura::desktop::theme::kTextMuted, "RENDERER");
            ImGui::SameLine(0.0f, 5.0f);
            ImGui::TextColored(aura::desktop::theme::kTextSecondary, "%s",
                               aura::desktop::to_string(rctx.profile));
        }

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
        } else if (max_frames > 0) {
            // Bounded smoke: cycle through every V3-37 section so each panel's
            // draw path is exercised (a real render of each section, not just the
            // default one) before the bounded loop exits.
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
    return run_gui(options, max_frames, choice);
}
