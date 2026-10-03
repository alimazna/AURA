// AURA desktop control center (Phase 9 / GUI-0001).
//
// Single canonical entrypoint for the AURA Windows desktop application. It owns
// exactly one runtime (ControlCenterState -> ApplicationShell) and renders the
// human control surface with Dear ImGui + GLFW + OpenGL 3.3. It is presentation
// over the existing runtime: it reads state, offers only safe control-plane
// operations (pause/resume the local transport, refresh, checkpoint, bounded
// stop, read-only recovery report), and never places an order.
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

#include "desktop/ControlCenterState.h"
#include "desktop/GuiPanels.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
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

// Interactive control center. Returns the process exit code. `max_frames` bounds
// the render loop (0 = unbounded); a non-zero bound is used by CI to prove the
// window/render lifecycle starts and shuts down cleanly, then exits.
int run_gui(const aura::desktop::ControlCenterOptions& options, long max_frames) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::fprintf(stderr, "aura-gui: GLFW init failed (no windowing system?)\n");
        return 3;
    }

    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window =
        glfwCreateWindow(1360, 860, "AURA Control Center (SHADOW ONLY)", nullptr, nullptr);
    if (window == nullptr) {
        std::fprintf(stderr, "aura-gui: failed to create window / OpenGL 3.3 context\n");
        glfwTerminate();
        return 3;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;  // deterministic; no user-path ini file
    ImGui::StyleColorsDark();
    ImGui::GetStyle().FrameRounding = 3.0f;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

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

    std::vector<std::string> sections = aura::desktop::DesktopModel::sections();
    int selected = 0;

    long rendered = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Advance the transport by one bounded cycle so the loop stays responsive.
        state.poll_transport(0);
        const auto& report = state.refresh_report();
        const auto& snap = report.snapshot;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::Begin("AURA", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar);

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Checkpoint now")) state.persist_checkpoint();
                if (ImGui::MenuItem(state.paused() ? "Resume transport" : "Pause transport"))
                    state.toggle_pause();
                ImGui::Separator();
                if (ImGui::MenuItem("Quit")) state.request_stop();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Shadow only (fixed)", nullptr, false, false);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("About")) {}
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Sidebar navigation.
        const float sidebar_w = 260.0f;
        ImGui::BeginChild("nav", ImVec2(sidebar_w, 0), true);
        ImGui::TextColored(ImVec4(0.70f, 0.80f, 1.0f, 1.0f), "AURA");
        ImGui::TextColored(ImVec4(0.62f, 0.62f, 0.66f, 1.0f), "Control Center");
        ImGui::Separator();
        for (int i = 0; i < static_cast<int>(sections.size()); ++i) {
            if (ImGui::Selectable(sections[static_cast<std::size_t>(i)].c_str(), selected == i))
                selected = i;
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // Content area.
        ImGui::BeginChild("content", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);
        aura::desktop::draw_section(report, selected);
        ImGui::EndChild();

        // Status bar.
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.90f, 0.55f, 0.20f, 1.0f), "SHADOW ONLY");
        ImGui::SameLine();
        ImGui::Text("| streams %zu/%zu | health", snap.overview.streams_healthy,
                    snap.overview.streams_total);
        ImGui::SameLine();
        ImGui::TextColored(aura::desktop::status_color(snap.overview.aggregate), "%s",
                           snap.overview.aggregate.c_str());
        ImGui::SameLine();
        ImGui::Text("| %s", state.paused() ? "PAUSED" : "RUNNING");

        ImGui::End();

        ImGui::Render();
        int w = 0, h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.07f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        if (max_frames > 0 && ++rendered >= max_frames) {
            std::printf("aura-gui: rendered %ld frames (bounded smoke); requesting close\n",
                        rendered);
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        } else if (max_frames > 0) {
            // Bounded smoke: cycle through every V3-37 section so each panel's
            // draw path is exercised (a real render of each section, not just the
            // default one) before the bounded loop exits.
            selected = (selected + 1) % static_cast<int>(sections.size());
        }
    }

    // Clean shutdown: persist a checkpoint with clean intent.
    const auto status = state.persist_checkpoint();
    std::printf("aura-gui: shutdown checkpoint -> %s\n",
                std::string(aura::foundation::to_string(status)).c_str());

    ImGui_ImplOpenGL3_Shutdown();
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
    return run_gui(options, max_frames);
}
