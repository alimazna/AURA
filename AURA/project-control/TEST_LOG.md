# AURA — Test Log

## 2026-10-03 — GUI visual redesign: design system + polished control center (GUI-0009..GUI-0013)

Scope: the desktop control center rendered and behaved correctly but looked prototype-like. This wave
rebuilds the presentation layer into a professional operator surface: an original design system, grouped
navigation, rebuilt panels, a coherent window shell, and a more informative (still honest) Timeframes
view. Presentation only — no runtime, persistence, protocol or MQL5/MT5 change; no live-order path;
shadow-only preserved; nothing fabricated.

Environment: Linux (g++ 14.2.0, CMake 4.4.3, Mesa software GL under Xvfb). No Intel HD 3000 GPU is
available here, so that specific hardware path is NOT exercised (see BLOCK-006); the design was verified
on both the OpenGL 3.3 and OpenGL 2.1 software backends.

Changes under test:
- New `src/desktop/StateVisuals.h`: pure, ImGui-free state classification (`StateCategory`,
  `state_category`, `category_label`, `is_unknown_like`). Invariant: unrecognised/empty states map to
  `UNKNOWN`, never `HEALTHY`; `NOT AVAILABLE` is its own category.
- New `src/desktop/AuraTheme.h`: deep-navy palette with one cyan accent, state→colour map, spacing scale,
  and `apply_aura_style()`. Uses only fixed-function-friendly style properties (flat colours, no
  gradients/textures/shaders).
- New `src/desktop/NavigationModel.h`: groups the 19 canonical V3-37 sections into
  Monitoring / Intelligence / Governance / System with readable labels, preserving section identity and
  order exactly (presentation only).
- New `src/desktop/AuraWidgets.h`: bordered cards, state badges, aligned key/value rows, metric blocks,
  explicit empty states and uniform tables — all via the ImGui draw list (legacy-safe).
- Rewritten `src/desktop/GuiPanels.h`: a Dashboard plus all 19 section panels on the design system.
- `tools/aura_gui.cpp`: new shell (top bar with brand/version/`SHADOW ONLY`/health/actual renderer,
  grouped sidebar, bordered content area, persistent status bar) and safe control-plane shortcuts only
  (`Ctrl+P` pause/resume, `Ctrl+S` checkpoint, `Esc` bounded stop); the bounded smoke now cycles
  `navigation_count()` sections.
- `src/desktop/DesktopModel.h`: `TimeframeRow.sequence` (real processed closed-bar sequence) and
  `.freshness` (deterministic FRESH/LAGGING/UNKNOWN/NOT AVAILABLE from the newest close time in the
  snapshot; no wall clock, no lookahead).
- `src/desktop/DesktopTests.cpp`: added `test_state_category_mapping`, `test_navigation_catalog`,
  `test_freshness_projection`.

Build command: `cmake -S . -B build-gui17 -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17
-DAURA_BUILD_GUI=ON` then `cmake --build build-gui17 -j 4`; default build
`cmake -S . -B build-off -DCMAKE_BUILD_TYPE=Release`; strict test compile
`g++ -std=c++17|-std=c++20 -Wall -Wextra -Werror -Wpedantic -Isrc src/desktop/DesktopTests.cpp
src/foundation/Hasher.cpp`.

Build result: PASS. Project sources compile clean under `-Wall -Wextra -Wpedantic` (no warnings from
`src/desktop/*` or `tools/aura_gui.cpp`).

Test command: `ctest --test-dir build-off`; `ctest --test-dir build-gui17`; `/tmp/dtest17`;
`/tmp/dtest20`; `aura_gui --self-test`; and Xvfb interactive smokes
`aura_gui --gui --renderer auto|legacy --frames 60` plus a **Debug** (assert-enabled) build
`aura_gui --gui --renderer auto|legacy --frames 80`.

Test result: PASS.
- `DesktopTests` ALL PASS under g++ C++17 and C++20 strict (`-Werror -pedantic`), including the three
  new tests.
- Default (GUI OFF) ctest: 18/18 passed. GUI ctest: 19/19 passed (includes `GuiSelfTest`).
- `aura_gui --self-test` PASS (270 frames, 9/9 streams, shadow-only, ledger+version sourced, incidents
  wired, unsourced sections `NOT AVAILABLE`, checkpoint OK, recovery CLEAN_SHUTDOWN resumable).
- Interactive smokes under Xvfb: `--renderer auto` -> `renderer=MODERN_GL33`, 60 frames, clean shutdown;
  `--renderer legacy` -> `renderer=LEGACY_GL21`, 60 frames, clean shutdown. Debug (assert-enabled)
  builds cycled all 19 sections for 80 frames on both backends with exit 0 — this exercises ImGui's own
  Begin/End balance and ID-stack assertions across every panel, so a malformed layout would have aborted.
- Captured a live frame (Xvfb + ImageMagick) of the running app: confirms the new shell (top bar,
  grouped sidebar ~17% width, four-card dashboard row, nine-timeframe table, signal/ledger cards,
  incidents table, status bar). The modern and legacy captures are structurally identical, confirming
  the same design renders on both backends.
- Forced-fallback (`MESA_GL_VERSION_OVERRIDE=2.1`) -> `LEGACY_GL21` with `OpenGL 3.3 unavailable`, clean
  shutdown; forced `--renderer modern` with Mesa capped -> exit 3 with the actionable no-renderer error.

Files changed: `src/desktop/StateVisuals.h` (new), `src/desktop/AuraTheme.h` (new),
`src/desktop/AuraWidgets.h` (new), `src/desktop/NavigationModel.h` (new), `src/desktop/GuiPanels.h`
(rewritten), `src/desktop/DesktopModel.h`, `src/desktop/DesktopTests.cpp`, `tools/aura_gui.cpp`, and the
control-plane files.

Known failures: none. Remote CI on the push of this wave caught one real Windows/MSVC portability defect
that Linux/GCC did not: `src/desktop/AuraWidgets.h` used `min`/`max` as local variable names, and the
`min`/`max` macros that MSVC's Windows headers (pulled in transitively via GLFW) define rewrote
`ImVec2 min(...)` into a function-style cast of a `float` (`error C2440: cannot convert from 'const
float' to 'const ImVec2'`, plus cascading `C2065: 'min'/'max' undeclared`). Fixed by renaming the locals
to `lo`/`hi` and defining `NOMINMAX` in `tools/aura_gui.cpp` before the GLFW include, so the macros
cannot rewrite this translation unit again. Re-verified locally (GUI build 19/19, `--self-test` PASS)
before pushing. Remote CI run 37128118600 (commit `d1c42f0`, after the fix) is ALL GREEN across 6/6
jobs, including `windows-x64-release-package` (Release x64 MSVC build of `aura.exe` + `aura_gui.exe`,
full suite, both exes smoked, ZIP packed and verified) and `desktop-gui-linux` (Dear ImGui + Xvfb bounded
interactive smoke). The failed run 37127787443 (commit `95b2d60`) is the one that surfaced the defect.
The refreshed Windows package from run 37128118600 contains `aura_gui.exe` = 1,134,080 bytes,
`aura.exe` = 440,320 bytes, `AURA_Windows_x64_GUI_Release.zip` = 765,806 bytes (both exes re-ran
`--self-test` PASS on the runner).

Interpretation: the control center now uses an original, consistent, legible design language, and the
new presentation code is verified to build clean, to keep ImGui's layout invariants across all 19
sections on both the modern and legacy backends, and to preserve every honesty invariant (absent data is
`NOT AVAILABLE`, `UNKNOWN` never looks healthy, no fabricated rows, no live-order path). The redesign is
verified structurally and by a captured frame; a human aesthetic review on the real Windows/Intel HD
3000 target is still pending (BLOCK-006). No profitability, calibration, broker-validation or
production-safety claim is made.

## 2026-10-03 — Renderer fallback for legacy GPUs (GUI-0007/GUI-0008)

Scope: make the desktop control center usable on hardware without an OpenGL 3.3 context (legacy Intel
HD Graphics 3000 on Windows). Modern-first (OpenGL 3.3 core) with a safe OpenGL 2.1 compatibility
fallback; never fabricate capability; no live-order path; no MQL5/MT5 changes. Shadow-only preserved.

Environment: Linux (g++ 14.2.0, CMake 4.4.3, Mesa software GL under Xvfb). No Intel HD 3000 GPU is
available here, so that specific hardware path is NOT exercised (see BLOCK-006).

Changes under test:
- New `src/desktop/RendererPolicy.h`: pure, deterministic selection policy (`hints_for`,
  `RendererSelector` with modern->legacy fallback and pinned single-profile mode, `RendererChoice`,
  `renderer_diagnostic`, `no_renderer_error`). No GLFW/GL, no I/O, no clock.
- `tools/aura_gui.cpp`: sets GLFW hints only from the policy; creates the OpenGL 3.3 core context first
  and the OpenGL 2.1 compatibility context on failure; drives `ImGui_ImplOpenGL2` (legacy, GLSL 120) or
  `ImGui_ImplOpenGL3` (modern, GLSL 330); prints the active renderer and shows it in the status bar;
  parses `--renderer auto|modern|legacy`.
- `CMakeLists.txt`: compiles `backends/imgui_impl_opengl2.cpp` into `aura_imgui`.
- `.github/workflows/ci.yml`: desktop-gui job gains modern-pin, legacy, and forced-auto-fallback smokes.
- `src/desktop/DesktopTests.cpp`: five deterministic unit tests for the policy and fallback state
  machine (real code paths, no mocks).

Build command (local):
`cmake -S . -B build-gui17 -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17 -DAURA_BUILD_GUI=ON`
then `cmake --build build-gui17 -j`; likewise `-DCMAKE_CXX_STANDARD=20` into `build-gui20`; and a
default (GUI OFF) build into `build-off`.

Build result: PASS (all three configurations; `aura_gui` links the new OpenGL2 backend).

Test command:
- `ctest --test-dir build-off` ; `ctest --test-dir build-gui17` ; `ctest --test-dir build-gui20`
- `g++ -std=c++17|c++20 -Wall -Wextra -Werror -pedantic -Isrc src/desktop/DesktopTests.cpp
  src/foundation/Hasher.cpp` then run the binary
- `xvfb-run -a -s "-screen 0 1360x860x24" timeout 90 ./build-gui17/aura_gui --gui --frames 40
  --store <p>` (AUTO); with `--renderer modern`; with `--renderer legacy`; and AUTO with
  `MESA_GL_VERSION_OVERRIDE=2.1 MESA_GLSL_VERSION_OVERRIDE=120` (forces the modern attempt to fail)
- `./build-gui17/aura_gui --self-test --store <p>`

Test result: PASS.
- Default build 18/18 ctest; GUI c++17 19/19; GUI c++20 19/19.
- `DesktopTests` ALL PASS under c++17 and c++20 strict (includes the five new renderer tests).
- `--self-test` PASS (270 frames, 9/9 streams, shadow-only, checkpoint OK, recovery CLEAN_SHUTDOWN
  resumable) — unchanged by the fallback.
- AUTO interactive smoke: `renderer=MODERN_GL33`, 40 frames rendered, clean shutdown checkpoint OK.
- `--renderer legacy`: `renderer=LEGACY_GL21`, 40 frames rendered, clean shutdown checkpoint OK.
- AUTO with Mesa capped at 2.1: `GLFW error 65543` on the modern attempt, then
  `renderer=LEGACY_GL21 ...; OpenGL 3.3 unavailable (GLFW error 65543)`, 40 frames rendered, clean
  shutdown checkpoint OK (the fallback path is genuinely exercised, not just linked).
- `--renderer modern` with Mesa capped at 2.1: exit code 3 with the actionable error
  `could not create any OpenGL context. Tried MODERN_GL33 (GLFW error 65543). Actionable: update the
  graphics driver, or use the console host aura.exe` (no pretend-success, no fabricated renderer).

Files changed: `src/desktop/RendererPolicy.h` (new), `tools/aura_gui.cpp`, `CMakeLists.txt`,
`src/desktop/DesktopTests.cpp`, `.github/workflows/ci.yml`, and the control-plane files.

Known failures: none locally. Remote CI (run `37125095527`, commit `f3e1886`) is ALL GREEN across 6/6
jobs: ubuntu gcc c++17/c++20, windows msvc c++17/c++20, `desktop-gui-linux` (now including the
modern-pin, legacy, and forced-auto-fallback smokes), and `windows-x64-release-package` (builds and
packages the GUI with the fallback).

Interpretation: the GUI no longer terminates merely because OpenGL 3.3 is unavailable; it falls back to
OpenGL 2.1 compatibility, names the renderer actually in use, and only fails when no context can be
created. The fallback code path is proven on software GL, but the specific Intel HD Graphics 3000 /
driver 9.17.10.4459 hardware is NOT available in CI and remains UNPROVEN. This does NOT establish
profitability, calibrated probability, broker validation, production safety, or live-trading readiness.

## 2026-10-03 — Phase 9 V3-37 section integration (GUI-0004/0005/0006)

Scope: extend the desktop control center with read-only projections for the remaining V3-37 sections
and wire the ones that have a real source, leaving the rest explicit `NOT AVAILABLE`. Shadow-only; no
live-order path; no MQL5/MT5 changes.

Environment: Linux (g++ 14.2.0, CMake 4.4.3, xvfb + mesa software GL), no network needed for the
core/desktop build.

Changes under test:
- New `src/desktop/ControlCenterPanels.h` (pure, deterministic projections; no GUI toolkit, no I/O, no
  clock, no input mutation).
- `ControlCenterState.h` builds a single `ControlCenterReport`: version context and shadow ledger from
  the live pipeline; incidents from the real adapter stream health via `FailureDetectionEngine`;
  supplied records for the remaining planes. Ledger snapshot cached by size (re-copied only when the
  append-only ledger grows).
- `GuiPanels.h` renders every section; `tools/aura_gui.cpp` cycles all sections in a bounded smoke.

Test commands and results (locally reproduced 2026-10-03):
- `g++ -std=c++17/-std=c++20 -Wall -Wextra -Werror -pedantic -Isrc src/desktop/DesktopTests.cpp
  src/foundation/Hasher.cpp` -> builds clean; run -> `DesktopTests: ALL PASS` for both standards.
  New tests: `test_section_panel_availability` (unsourced sections `NOT AVAILABLE`; wired sections
  available + deterministic; version/ledger/incidents sourced from the real runtime),
  `test_incident_propagation_and_corruption` (a real adapter stream taken offline yields a real
  incident for that stream only; a healthy stream is not reported; a corrupt store surfaces
  `CORRUPTED_STATE` and is refused), `test_single_runtime_owner` (two states are independent; no panel
  snapshot creates a second shell).
- Default (GUI OFF) `cmake --build` + `ctest` -> **18/18 PASS**.
- GUI (`AURA_BUILD_GUI=ON`) c++17 -> `ctest` **19/19 PASS**; c++20 -> `ctest` **19/19 PASS**.
- `aura_gui --self-test` -> PASS: `fed 270 frames`, `snapshot rows=9 streams=9/9 shadow_only=yes`,
  `sections version_available=yes ledger_entries=111 incidents_wired=yes incidents=0`,
  `checkpoint -> OK`, `recovery -> CLEAN_SHUTDOWN (resumable=yes)`, `SELF-TEST PASS` (exit 0). The
  self-test additionally fails if version context is unsourced, if the ledger is not populated from the
  real runtime, if incident detection is not wired, or if any unsourced section fabricates availability.
- Interactive smoke under Xvfb: `xvfb-run -a -s "-screen 0 1360x860x24" aura_gui --gui --frames 57
  --store /tmp/gui_cycle.aura` -> `rendered 57 frames` cycling all 19 sections, `shutdown checkpoint ->
  OK`, exit 0.

Honest limitations: this is local Linux verification. Remote CI for these commits is recorded below
once the push completes. Interactive rendering on a real Windows desktop remains UNPROVEN. No
profitability, calibration, broker-validation or production-safety claim is made.

### Remote CI (GitHub Actions) — run 37118241974, commit e4bde97

All 6 jobs SUCCESS:
- `ubuntu-latest / c++17 / gcc` — SUCCESS (full suite + smoke + install check).
- `ubuntu-latest / c++20 / gcc` — SUCCESS.
- `windows-latest / c++17 / msvc` — SUCCESS.
- `windows-latest / c++20 / msvc` — SUCCESS.
- `desktop-gui-linux (Dear ImGui + Xvfb)` — SUCCESS (full suite incl. `GuiSelfTest`; GUI headless
  integration smoke; bounded interactive-runtime smoke under Xvfb software OpenGL).
- `windows-x64-release-package` — SUCCESS (builds `aura.exe` + `aura_gui.exe`, runs the suite, smokes
  both exes, re-runs both from the extracted package).

This confirms the newly added section-integration code builds and its tests pass on Linux gcc (C++17
and C++20) and Windows MSVC (C++17 and C++20), and that the GUI still builds and its smokes pass.

## 2026-10-03 — Phase 9 desktop productization: real GUI (GUI-0001/0002/0003)

Scope: build and verify a real desktop control-center GUI (Dear ImGui + GLFW + OpenGL 3.3) over the
existing runtime. Shadow-only; no live-order path.

Environment: Linux (g++ 14.2.0, CMake 4.4.3, libgl1-mesa-dev + xorg-dev + xvfb + mesa software GL
installed this session), plus the existing Windows CI runner.

Build commands verified locally:
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17` then `cmake --build build -j`
  (GUI OFF, default) -> BUILD PASS.
- `cmake -S . -B build -DAURA_BUILD_GUI=ON ... -DCMAKE_CXX_STANDARD=17` and `...=20` -> BUILD PASS
  (`aura_gui` links; pinned FetchContent glfw 3.4 + imgui v1.90.9).

Test commands and results (locally reproduced):
- `ctest --test-dir <GUI OFF build>` -> **18/18 PASS** (core matrix unchanged).
- `ctest --test-dir <GUI ON build>` -> **19/19 PASS** (adds `GuiSelfTest`).
- `DesktopTests` (direct, g++ -std=c++17 -Wall -Wextra -Wpedantic) -> `DesktopTests: ALL PASS`; also
  compiles clean under C++20. New tests: `test_desktop_model_empty_state`,
  `test_desktop_model_nine_timeframes`, `test_control_center_state_smoke`.
- `aura_gui --self-test --store /tmp/g20.aura` -> PASS: `fed 270 frames`, `snapshot rows=9 streams=9/9
  shadow_only=yes`, `checkpoint -> OK`, `recovery -> CLEAN_SHUTDOWN (resumable=yes)`, `SELF-TEST PASS`
  (exit 0). The empty-state run also printed `rows=9 shadow_only=yes` with no fabrication.
- **Interactive runtime smoke (genuine window + render + shutdown)**:
  `xvfb-run -a -s "-screen 0 1360x860x24" aura_gui --gui --frames 120 --store /tmp/gui_interactive.aura`
  -> `boot recovery -> UNKNOWN_STATE (resumable=no): no prior persisted state; clean fresh start`,
  `rendered 120 frames (bounded smoke); requesting close`, `shutdown checkpoint -> OK`, exit 0.
  This proves the GLFW window creation, ImGui GL3 render loop, bounded stop and clean-shutdown
  checkpoint actually run — not merely that the target links.

Behavioural checks exercised (real code paths, no mocks):
- Nine timeframes presented individually by identity (M1..MN1), in canonical order; absent streams
  shown `NOT AVAILABLE`, never fabricated as healthy.
- Fed deterministic frames -> 9/9 streams present, accepted > 0.
- `shadow_only` invariant true; no live-order path exists in the GUI.
- Checkpoint -> OK; fresh process cross-process recovery -> `CLEAN_SHUTDOWN`/resumable.
- Corrupted store -> `CORRUPTED_STATE`, refused, not auto-resumed just because the UI is open.

Files changed: `tools/aura_gui.cpp`, `src/desktop/DesktopModel.h`, `src/desktop/ControlCenterState.h`,
`src/desktop/GuiPanels.h`, `src/desktop/DesktopTests.cpp`, `CMakeLists.txt`, `.github/workflows/ci.yml`,
`README.md`, and the control-plane files.

Known failures / unproven: none observed locally. **UNPROVEN:** interactive rendering on a real Windows
desktop (no human `aura_gui --gui` run on Windows yet); the Xvfb smoke is software-rendered on Linux.
**UNPROVEN:** MT5/MetaEditor round-trip and any historical XAUUSD campaign.

Interpretation: Phase 9 now delivers a real desktop control center that builds and runs, with its
integration path and interactive lifecycle verified by reproducible local runs. This is NOT a claim of
Windows visual verification, correctness of unverified sections, profitability, calibration, broker
validation, production safety, or live-trading readiness. Shadow only.

Remote CI (run 37117057898, branch phase13-persistence-recovery, commit aaeb135) — ALL GREEN, 6/6 jobs:
- `desktop-gui-linux (Dear ImGui + Xvfb)`: 19/19 ctest PASS (incl. DesktopTests + GuiSelfTest);
  headless `aura_gui --self-test` PASS (`fed 270 frames`, `rows=9 streams=9/9 shadow_only=yes`,
  checkpoint OK, CLEAN_SHUTDOWN resumable); **interactive Xvfb smoke PASS in CI** (`boot recovery ->
  UNKNOWN_STATE`, `rendered 120 frames`, `shutdown checkpoint -> OK`), i.e. the window + render +
  clean-shutdown lifecycle is proven on the runner, not only on the author machine.
- `windows-x64-release-package`: builds `aura.exe` + `aura_gui.exe` (GUI ON) and passes
  `aura_gui --self-test` on the real Windows runner (`fed 270 frames`, `rows=9 streams=9/9`,
  CLEAN_SHUTDOWN resumable). Sizes: `aura.exe` = 440,320 bytes; `aura_gui.exe` = 1,016,832 bytes;
  `AURA_Windows_x64_GUI_Release.zip` = 714,226 bytes. ZIP verified to contain both exes; the extracted
  `aura_gui.exe` re-ran `--self-test` PASS. Artifacts uploaded: `aura-windows-x64-exe`
  (707,096 bytes: aura.exe + aura_gui.exe) and `AURA_Windows_x64_GUI_Release` (713,997 bytes).
- ubuntu-latest c++17 and c++20 gcc: PASS (18/18, GUI OFF default matrix unchanged).
- windows-latest c++17 and c++20 msvc: PASS.
Remaining UNPROVEN unchanged: real-Windows-desktop interactive visual verification (no human run);
MT5/MetaEditor round-trip; historical XAUUSD campaign.

## Future test entry format

## 2026-10-02 — Baseline repository audit (unverified external material)

Source:
OpenHands read-only audit supplied by the project owner.

Reported:
- CMake configure: PASS with UI disabled
- Build: PASS, 175/175
- CTest: PASS, 40/40
- Repository status after audit: clean
- No commit/push from the audit
- MT5 C++: buildable
- MQL5 adapters: statically checked, not compiled/executed in MetaEditor
- Real C++↔MQL5 runtime connection: not proven
- Host application MT5 wiring: reported missing

Qualification:
These findings are NOT reproducible from the current GitHub repository content
(`alimazna/AURA` at `048a95f`), which contains only `README.md`, `docs/`, and
`project-control/`. They are retained as historical context only and must not be
treated as evidence about the current repository. They do not prove Master V3
implementation, live trading readiness, profitability, broker validation, or production safety.

## 2026-10-02 — Bootstrap normalization verification

Task/Phase: BOOTSTRAP normalization (repository layout + control-plane paths)

Verification performed (read-only inspection; no application source exists yet):
- `git --no-pager log --oneline` -> HEAD `048a95f`; working tree returned to the committed
  nested layout after an incorrect flattening attempt was reverted.
- `git reset` then re-created `AURA/`; `git status --short` shows only the three intended
  control-plane modifications (AI_BOOTSTRAP.md, PROJECT_STATE.md, TASK_MANIFEST.yaml).
- `find . -path ./.git -prune -o -type f -print` -> all 10 original files present under `AURA/`.
- `ls AURA/docs/AURA_MASTER_UNIFIED_PROJECT_v3.0.md` -> exists (354624 bytes).
- `grep -rn XAUUSD_SOVEREIGN_MASTER_UNIFIED_PROJECT AURA/project-control/` -> no matches (exit 1).
- `.git` was not modified.

Result: PASS (structural). No file lost. No stale control-plane reference remains.

Interpretation:
This proves the repository layout and control-plane path consistency. It does NOT prove any
runtime, trading, or architectural correctness. No application source code was built or executed.

## 2026-10-02 — TASK-MANIFEST-001 structural validation

Task/Phase: TASK-MANIFEST-001 (project-control)

Command: `python3` with `yaml.safe_load` on `AURA/project-control/TASK_MANIFEST.yaml`,
followed by structural checks.

Result (recorded values):
- Manifest parses as valid YAML.
- `manifest_version: 2`; 96 tasks; all task IDs unique.
- Status counts: APPROVED 1, IMPLEMENTED 1, READY 12, PLANNED 71, BLOCKED 2, DEFERRED 9.
- Authority counts: CANONICAL 24, PROPOSED 65, PROJECT DECISION 6, OPEN DECISION 1.
- Dependency integrity: zero unresolved task-ID dependencies (excluding file/document references).
- Source output paths: 81, all unique (one output path per source task).
- `FND-0015` (OPEN DECISION) and `FND-0016` are BLOCKED; Phases 3–11 are DEFERRED.

Result: PASS (structural only).

Interpretation:
Structural validation only. It proves the task graph is internally consistent and complete for
the active scope. It does NOT prove that any planned source file is correct, buildable, or that
the architecture has been implemented. No application source code was written.

## 2026-10-02 — FND-0001 EntityId implementation verification

Task/Phase: FND-0001 (Phase 0, Immutable Foundations)

Files changed: `AURA/src/foundation/EntityId.h` (new)

Build command (ad-hoc; no application build system exists):
`g++ -std=c++17 -Wall -Wextra -Werror -pedantic -I AURA/src /tmp/fnd0001_check.cpp`

Build result: PASS (clean compile, c++17). Also PASS under `-std=c++20`.

Test command: `/tmp/fnd0001_check` (runtime structural assertions, no test framework added)

Test result: PASS. Assertions covered:
- default-constructed identity is invalid; `valid()` false and `operator bool` false
- valid identity carries exact bytes (`value()`, `view()`)
- deterministic equality / inequality
- deterministic hashing: equal ids hash equally; `std::hash<EntityId>` specialization resolves
- deterministic ordering (`operator<`)
- usable as a key in `std::unordered_set` (duplicate insert collapses)
- copy preserves identity and hash
- explicit construction from `std::string` works
- no implicit conversion from `const char*` (compile-time `static_assert`)

Additional check: standalone-include self-containment (`#include "foundation/EntityId.h"` only)
compiled and linked cleanly.

Known limitations:
- No application build system, no test framework, and no other source files exist; verification is
  an ad-hoc compile plus a single throwaway program kept outside the repository.
- `EntityId` uses the default `std::hash<std::string>`; this is deterministic within a program run
  but is NOT a stable cross-process/serialization hash. No serialization hash is provided (out of scope).

Interpretation:
FND-0001's header compiles cleanly under strict warnings and its documented structural properties
hold at runtime. This does NOT prove architectural correctness, and does NOT prove anything about
trading, profitability, or live safety. `EntityId` is an immutable value type only.

## 2026-10-02 — Phase 0 Waves 0A–0D + reconciliation verification

Task/Phase: Phase 0 foundation/config/audit/integrity/persistence/guardian tasks
(FND-0002..FND-0014, FND-0017, FND-0010/0011/0013, CFG-0001..0005, AUD-0001..0003,
INT-0001..0003, PER-0001..0004, GDN-0001..0004).

Files added: `AURA/src/foundation/` —
`Timestamp.h`, `Version.h`, `ServiceState.h`, `SystemMode.h`, `HashDigest.h`,
`DataQualityState.h`, `ProtocolVersion.h`, `SchemaVersion.h`, `EventType.h`,
`ErrorSeverity.h`, `RecoveryAction.h`, `EventId.h`, `MessageMetadata.h`, `EventMetadata.h`,
`ConfigurationKey.h`, `ConfigurationValue.h`, `ConfigurationScope.h`,
`ConfigurationSnapshot.h`, `ConfigurationChange.h`, `AuditAction.h`, `AuditOutcome.h`,
`AuditRecord.h`, `HashAlgorithm.h`, `IHasher.h`, `Hasher.cpp`, `PersistenceStatus.h`,
`PersistenceRecordMetadata.h`, `IPersistenceStore.h`, `PersistenceTransaction.h`,
`GuardianStatus.h`, `GuardianPolicy.h`, `IGuardian.h`, `Guardian.h`.

Build command (ad-hoc; no application build system exists):
`g++ -std=c++17 -Wall -Wextra -Werror -pedantic -I AURA/src /tmp/phase0_full_verify.cpp AURA/src/foundation/Hasher.cpp`
Also compiled under `-std=c++20` with the same flags.

Build result: PASS (clean, both standards).

Test result: PASS. Structural assertions covered:
- Version parse/round-trip (`1.2.3`, `2.10.4-beta.1`) and ordering (`1.9.0 < 1.10.0`).
- Timestamp explicit-unit conversions and exact ordering.
- `HashDigest::from_hex`/`to_hex` round-trip and byte-exact equality.
- Canonical enum values: ServiceState (V3-14), SystemMode (V3-14), DataQualityState (V3-25).
- `EventId::to_string()` stable round-trip; `MessageMetadata` carries a `HashDigest` checksum;
  `EventMetadata` keeps `event_time` and `receive_time` distinct with `sequence_id` ordering.
- Configuration typed equality (`integer(7) != decimal(7.0)`), versioned snapshot lookup, effective-change.
- Audit record attributability; failed/rejected outcomes not treated as success.
- Integrity: real FIPS 180-4 SHA-256 known-answer tests passed —
  `sha256("abc") = ba7816bf...15ad` and `sha256("") = e3b0c442...b855`; determinism re-checked;
  `create_hasher(UNKNOWN) == nullptr`.
- Persistence transaction rejects duplicate record identity; applicable predicate.
- Guardian severity mapping (FATAL→HALT, CRITICAL→SAFE_MODE, INFO→NONE); rollback policy requires
  a KNOWN-GOOD anchor (`well_formed()` false without one).

Reconciliation (before continuing beyond Wave 0A):
- Wave 0A canonical set (V3-44) is exactly 12 tasks. Verified all 12 files exist:
  EntityId, Timestamp, Version, ServiceState, SystemMode, HashDigest, DataQualityState,
  ProtocolVersion, SchemaVersion, EventType, ErrorSeverity, RecoveryAction. No task omitted.
  The earlier "11 files" note excluded FND-0001 (already implemented/tested).
- Contract inconsistency found and corrected (not silently frozen): an earlier draft of
  `SystemMode.h` used invented values (`NORMAL/DEGRADED/SAFE/HALT/SHADOW/LIVE`). FND-0005's
  acceptance requires the V3-14 values exactly. Rewritten to
  `STARTING/RECOVERY/NORMAL/DEGRADED/SHADOW/PAUSED/MANUAL/EMERGENCY/HALTED`.
- `RecoveryAction.h` updated to make `UNKNOWN` representable per its acceptance criterion.
- `MessageMetadata.h` updated to carry the V3-23 `HashDigest` checksum per FND-0010 acceptance.
- `EventId.h` updated with a stable `to_string()` serialization per FND-0011 acceptance.
- `Version.h` semantics reviewed against FND-0003: MAJOR.MINOR.PATCH + optional build tag,
  ordering and round-trip are supported by the manifest acceptance and by the Master's versioned
  configuration/provenance requirement (V3-26). The Master does not freeze a concrete version
  grammar, so the grammar is a PROPOSED implementation choice; the file is marked `IMPLEMENTED`,
  not `TESTED`, pending REVIEW.

Known limitations:
- No application build system, test framework, or REVIEW/INTEGRATION/APPROVAL stage yet; verification
  is ad-hoc compile plus a throwaway harness outside the repository.
- `HashDigest` comparison is not constant-time (not a security boundary in Phase 0).
- These checks do not prove architectural or trading correctness.

Interpretation:
The Phase 0 files compile cleanly under strict warnings (c++17/c++20) and their documented
structural properties hold at runtime, including genuine SHA-256 known-answer tests. Status is
`IMPLEMENTED` (not `APPROVED`/`INTEGRATED`) because independent review has not occurred.

## 2026-10-02 — Phase 0 REVIEW / INTEGRATION (Phase 0 only)

Scope: review/integration of the 34 implemented Phase 0 files. No Phase 0.5 work. `FND-0015`
remains OPEN DECISION and `FND-0016` remains BLOCKED; no substitute ErrorCode definitions were
created.

Review method (per file): checked against the exact `TASK_MANIFEST.yaml` acceptance criterion, the
relevant canonical Master V3 section, declared dependencies, namespace/include correctness,
supported C++ standard, determinism, ownership boundaries, dependency direction, accidental
future-phase behaviour, and unnecessary API surface.

Findings:
- Defect (REWORK) `GDN-0002 src/foundation/GuardianPolicy.h`: included `GuardianStatus.h` but never
  used it. Removed the unused include. Reworked and re-verified.
- Defect (REWORK) `PER-0004 src/foundation/PersistenceTransaction.h`: included `PersistenceStatus.h`
  but never used it. Removed the unused include. Reworked and re-verified.
- Benign: several files include foundation headers that are not in their scheduling `dependencies`
  list (e.g. `ProtocolVersion.h`/`SchemaVersion.h` -> `Version.h`; `ConfigurationSnapshot.h` ->
  `ConfigurationScope.h`). This is correct layering (lower-layer only), not a contract mismatch.
  The manifest dependency lists are scheduling dependencies, not the include contract.
- Include graph: acyclic; no self-includes; all includes are lower-layer
  (no hidden higher-layer dependency). Verified programmatically.
- Focused re-checks: `SystemMode.h` uses the exact V3-14 values; `MessageMetadata.h` carries the
  V3-23 `HashDigest` checksum; `EventId.h` provides stable `to_string()`; `RecoveryAction.h` exposes
  `UNKNOWN` and is not treated as safe; `Version.h` parse/round-trip/ordering all hold.

Build command (ad-hoc; no application build system exists):
`g++ -std=c++17 -Wall -Wextra -Werror -pedantic -I AURA/src /tmp/phase0_review_verify.cpp AURA/src/foundation/Hasher.cpp`
Also compiled under `-std=c++20` with the same flags.

Build result: PASS (clean, both standards).

Test result: PASS. Additionally, standalone self-containment was verified for all 33 headers:
each header was compiled alone as a translation unit under `-std=c++17` and `-std=c++20` with
`-Wall -Wextra -Werror -pedantic` (66 TUs) — ALL PASS. Runtime assertions re-confirmed SHA-256
known-answer vectors (`sha256("abc")`, `sha256("")`), enum values against V3-14/V3-25, and the
Guardian severity mapping.

Hygiene:
- 34 files exist under `AURA/src/foundation/`; no duplicate or accidental files; no stray build
  artifacts.
- Manifest `src/foundation/` outputs = 36 (34 present + `ErrorCode.h`/`ErrorRecord.h` still BLOCKED).
- Secret scan of `AURA/**` for token/key/private-key patterns: no matches.

Interpretation:
Every implemented Phase 0 file passed review and integration verification. Status is recorded as
`REVIEW_PENDING` (not `APPROVED`/`INTEGRATED`), because independent approval has not been granted;
compilation and structural checks do not prove architectural or trading correctness.

## 2026-10-02 — FND-0015 ErrorCode + FND-0016 ErrorRecord (implement / review / full Phase 0 verify)

Tasks/Phase: FND-0015 `src/foundation/ErrorCode.h`, FND-0016 `src/foundation/ErrorRecord.h`
(Phase 0). The V3-45 OPEN DECISION was resolved by a human decision recorded in `DECISIONS.md`
(2026-10-02); no taxonomy was invented by the agent.

Build command (ad-hoc; no application build system exists):
`g++ -std=c++17 -Wall -Wextra -Werror -pedantic -I src /tmp/phase0_full_verify.cpp src/foundation/Hasher.cpp`
Also compiled under `-std=c++20` with the same flags.

Build result: PASS (clean, both standards).

Test result: PASS.
- `FND-0015`: exact enum values asserted — `DATA_ERROR=1` … `RECOVERY_ERROR=13` in V3-113 order;
  `to_string()` returns the canonical labels; strongly-typed scoped enum, underlying `uint8_t`.
- `FND-0016`: all eight V3-27 fields round-trip; default record is not `valid()` and defaults
  `recovery_action` to `UNKNOWN` (representable, not safe); empty component ⇒ not valid.
- SHA-256 known-answer tests still pass: `sha256("abc")`, `sha256("")`, and the 448-bit FIPS vector.
- Existing contracts re-checked: `SystemMode` V3-14 values; `ServiceState` names; `is_critical`,
  `is_known`, `is_protective`.

Structural verification (programmatic):
- Self-containment: 35 headers × 2 standards = 70 standalone TUs — ALL PASS
  (`-Wall -Wextra -Werror -pedantic`).
- Include graph: acyclic; no self-includes; no unknown includes; all includes lower-layer.
- Files: 36 expected Phase 0 files present (including `ErrorCode.h`, `ErrorRecord.h`); no missing,
  no stray/duplicate files.
- Duplicate type check: exactly one `enum class ErrorCode` (ErrorCode.h) and one `class ErrorRecord`
  (ErrorRecord.h).
- Secret scan of the repository: no matches.

Review findings: no defect found in either file. `ErrorRecord` includes only `EntityId`, `ErrorCode`,
`ErrorSeverity`, `RecoveryAction`, `ServiceState`, `Timestamp` (all lower-layer); no circular or
hidden higher-layer dependency; API limited to the contract; no logging/recovery/retry/evaluation/
serialization/persistence/networking behaviour.

Interpretation:
Both files compile cleanly and satisfy their structural and contract checks. Status is recorded as
`REVIEW_PENDING` — NOT `APPROVED`/`INTEGRATED` — because independent approval has not been granted;
compilation and structural checks do not prove architectural correctness. `BLOCK-002` is RESOLVED;
the manifest now has no `BLOCKED` tasks. `PHASE-0` (requires all Phase 0 tasks APPROVED) and Phase 0.5
are not started; no task is currently READY.

## 2026-10-02 — Phase 0 final review / integration gate

Scope: all 36 Phase 0 file-level tasks (`src/foundation/`), reviewed against
`TASK_MANIFEST.yaml`, `DECISIONS.md`, `BLOCKED.md`, Master V3, and the files on disk.

Commands (ad-hoc; no application build system exists):
- `g++ -std=c++17 -Wall -Wextra -Werror -pedantic -I src <harness>.cpp src/foundation/Hasher.cpp`
- same with `-std=c++20`
- per-header standalone TUs for all 35 headers under both standards
- programmatic include-graph, duplicate-type, manifest-vs-filesystem, and secret scans

Result: PASS.
- 36/36 canonical Phase 0 outputs exist; no duplicate or stray files; manifest outputs match the
  filesystem.
- 35 headers self-contained: 70 standalone TUs (35 × c++17/c++20) — ALL PASS.
- Full Phase 0 runtime harness passes under both standards, including genuine SHA-256 known-answer
  tests (`abc`, empty, 448-bit FIPS vector) and the Guardian severity→recovery mapping.
- Include graph: acyclic; no self-includes; no unknown includes; all includes lower-layer; no
  duplicate foundation type.
- Canonical enum values re-verified against Master V3: V3-14 `ServiceState`/`SystemMode`, V3-25
  `DataQualityState`, section 113 `ErrorCode` (1..13) and the eight-field `ErrorRecord` shape.
- Secret scan: no matches.

Defect found and fixed:
- 10 Phase 0 tasks under-declared their dependencies in `TASK_MANIFEST.yaml` relative to the actual
  (correct) include contracts: `FND-0008`, `FND-0009`, `FND-0010`, `FND-0016`, `CFG-0003`, `CFG-0005`,
  `AUD-0003`, `PER-0003`, `PER-0004`, `GDN-0003`. The dependency lists were reconciled to the verified
  includes (e.g. `FND-0008`/`FND-0009` → `FND-0003`; `PER-0003` → `PER-0001`, `PER-0002`, `FND-0001`,
  `FND-0006`). No source file was changed; the include graph was already correct. After the fix, no
  genuine dependency omission remains, and the V3-44 Wave 0B dependency sets are all satisfied.

Interpretation:
Every Phase 0 file-level task passes independent review and verification and is classified
`APPROVED`. The `PHASE-0` milestone is `APPROVED` and Phase 0 is complete. No task is `BLOCKED`;
`BLOCK-002` stays RESOLVED. The next READY task is `RS-0001` (Phase 0.5) but it was NOT started.
Compilation and structural checks support — but do not by themselves prove — architectural
correctness; no profitability, calibration, broker-validation or production-safety claim is made.

## 2026-10-02 — RS-0001 ServiceDescriptor (Phase 0.5)

Scope: `RS-0001` → `src/resilience/ServiceDescriptor.h` only. One task, one output file.

Commands (ad-hoc; no application build system exists):
- `g++ -std=c++17 -Wall -Wextra -Werror -pedantic -I src /tmp/sc17.cpp` (standalone self-containment)
- same with `-std=c++20`
- `g++ -std=c++17 ... /tmp/rs0001_check.cpp` and `-std=c++20` (structural harness)
- combined all-headers TU incl. `Hasher.cpp` under both standards

Result: PASS.
- Self-contained: the header compiles alone (no other project header) under c++17 and c++20 strict.
- Structural harness passes under both standards: default descriptor invalid; non-empty name valid;
  empty name invalid; name/state preserved; deterministic equality/inequality on both fields;
  deterministic ordering (name then state); equal values hash equal; usable as `unordered_set` and
  `set` key (equal keys dedup); copy preserves value; all 8 canonical `ServiceState` values
  representable; `is_operational` remains the sole operational authority.
- Includes: only `foundation/ServiceState.h` (FND-0004) plus std headers. Preprocessor check confirms
  `foundation/EntityId.h` is NOT pulled in (the only "EntityId" occurrence is in a comment). No
  higher-layer include. Include graph acyclic.
- No duplicate type name (`ServiceDescriptor` declared once); the `hash` matches are distinct
  `std::hash<...>` specializations, not duplicate types.
- Combined all-headers TU compiles cleanly under c++17 and c++20 strict.

Interpretation:
`RS-0001` satisfies its acceptance ("describes identity and state vocabulary; consistent with
ServiceState; no runtime behaviour") and the V3-13/V3-15/V3-16/V3-40/V3-46/V3-49 checks. Classified
`APPROVED`. No source defect found; no dependency contract was changed. Compilation and structural
checks support — but do not by themselves prove — architectural correctness.

## 2026-10-02 — Phase 0.5 resilience (RS-0001..RS-0020)

Scope: Phase 0.5, `src/resilience/`. 20 file-level tasks; RS-0001 completed earlier, RS-0002..RS-0020
in this session.

Commands (ad-hoc; no application build system exists):
- per-header self-containment: `g++ -std={c++17,c++20} -Wall -Wextra -Werror -pedantic -I src <TU>`
- combined all-headers TU incl. `src/foundation/*.cpp` under both standards
- `g++ -std={c++17,c++20} ... src/resilience/DegradationTests.cpp src/foundation/Hasher.cpp -o t && ./t`
- static include-graph / duplicate-type / dependency-direction analysis (python)

Result: PASS.
- All 19 resilience headers are self-contained (standalone TU) under c++17 and c++20 strict; the
  combined all-headers TU compiles under both.
- `DegradationTests.cpp` passes under both standards, exercising real code paths (no mocks):
  duplicate capability ids rejected; self-edges rejected; cycle detected and topological order empty
  on a cyclic graph; deterministic topological order (dependencies before dependents); dependency
  failure degrades dependents; independent capability stays ONLINE; missing/unregistered health is
  BLOCKED (never fabricated ONLINE); isolation disables only dependents and keeps independent
  capabilities available; non-critical failure is tolerable while critical/unclassified requires
  protection; impact names the exact failed capability; recovery prefers known-valid state and
  escalates to SAFE_MODE when none exists; paused is not treated as failed; freshness distinguishes
  FRESH/STALE/EXPIRED and treats future/incoherent input as UNKNOWN; per-timeframe stale detection
  reports exactly the stale timeframe (M15) while the other eight stay VALID; pause requires a
  reason and an operating mode; the supervisor consults the Guardian and reports HALT/SAFE_MODE
  without escalating itself.
- Include graph: acyclic; resilience headers include only `foundation/` and `resilience/` (no hidden
  higher-layer dependency). No duplicate type names. No future-phase behaviour; no live-trading
  enablement. Secret scan: none.

Interpretation:
Phase 0.5 satisfies the V3-13/V3-15/V3-16 requirements and the `PHASE-0.5` milestone acceptance
("capability dependency graph encoded; degradation/isolation tests prove non-critical failure
isolation"). All 20 Phase 0.5 tasks and the milestone are classified `APPROVED`. Compilation and
structural checks support — but do not by themselves prove — architectural correctness; no
profitability, calibration, broker-validation or production-safety claim is made.

## 2026-10-02 — Phase 1 deterministic runtime (RT-0001..RT-0021)

Scope: Phase 1, `src/runtime/`. 21 file-level tasks.

Commands (ad-hoc; no application build system exists):
- per-header self-containment under c++17/c++20 strict
- combined all-headers TU across foundation/resilience/runtime under both standards
- `g++ -std={c++17,c++20} ... src/runtime/RuntimeTests.cpp src/foundation/Hasher.cpp -o t && ./t`
- static include-graph / duplicate-type / layer-direction analysis (python)

Result: PASS.
- All 20 runtime headers self-contained under c++17 and c++20 strict; combined all-headers TU (74
  headers) compiles under both.
- `RuntimeTests.cpp` passes under both standards (real code paths, no mocks): nine independent MT5
  streams where an M15 failure/recovery leaves H4 untouched and a D1 disconnect leaves M15 healthy;
  unknown timeframe rejected; deterministic data-bus ordering; future-dated event rejected (no
  lookahead); forming bar rejected (no repaint); OHLC validation (INVALID/DEGRADED/VALID); closed-bar
  identity deterministic and per-timeframe; re-finalizing the same close_time rejected; timeframe state
  store is monotonic (regression refused) and persists idempotently; feature computation refuses any
  still-forming bar; H4 structural authority enforced; eligibility gated by data quality (UNKNOWN/STALE
  -> INELIGIBLE); decision/signal identity deterministic and configuration-version dependent; score and
  confidence bounded ranking values (zero confidence on non-VALID quality); market quality UNKNOWN when
  streams unassessed and GOOD when all valid; risk proposal is not an order and requires usable market
  quality; shadow fill is never live and models commission/partial fill; position lifecycle open ->
  partial close -> full close; reconciliation MATCHED within tolerance and DIVERGED/not-trustworthy
  outside it; ledger entry identity deterministic and append idempotent; ledger persist idempotent;
  replay finalizes ten closed bars, rejects the future-dated and forming events, and produces identical
  fingerprints across runs (reproducible).
- Include graph: acyclic; runtime headers include only foundation/resilience/runtime. No duplicate types
  (std::hash specializations only). No live-order path; no future-phase behaviour. Secret scan: none.

Interpretation:
Phase 1 satisfies the `PHASE-1` milestone acceptance ("deterministic runtime proven by replay tests;
shadow ledger append-only and idempotent; no live orders"). All 21 tasks and the milestone are
`APPROVED`. Compilation and structural checks support but do not by themselves prove architectural
correctness; no profitability, calibration, broker-validation or production-safety claim is made, and
the real MT5 connection remains unproven until measured.

## 2026-10-02 — Phase 2 observation and outcomes (OB-0001..OB-0004)

Scope: Phase 2, `src/observation/`. 4 file-level tasks.

Commands (ad-hoc; no application build system exists):
- per-header self-containment under c++17/c++20 strict
- combined all-headers TU across foundation/resilience/runtime/observation under both standards
- `g++ -std={c++17,c++20} ... src/observation/ObservationTests.cpp src/foundation/Hasher.cpp -o t && ./t`
- static include-graph / duplicate-type / layer-direction analysis (python)

Result: PASS.
- All 4 observation headers self-contained under c++17 and c++20 strict; combined all-headers TU (78
  headers) compiles under both.
- `ObservationTests.cpp` passes under both standards (real code paths, no mocks): prediction identity
  deterministic and sensitive to changed fields; ledger append-only, auditable (find by identity),
  idempotent on duplicate and rejecting on conflict; persist idempotent; invalid prediction rejected.
  Outcome resolution: TARGET_HIT / STOP_HIT resolved, ambiguous bar resolves conservatively to
  STOP_HIT, a bar at/before decision time cannot resolve (no lookahead), empty bars -> OPEN and no
  trigger -> EXPIRED (no fabricated win/loss), and resolution is deterministic across runs.
  Failure detection: an ERROR stream yields a valid structured ErrorRecord with its own error id, a
  named component, a non-INFO severity and a known recovery action; a healthy set yields none.
  System health: unassessed -> STARTING and not healthy, all-online -> healthy, one offline stream ->
  unhealthy with the aggregate reflecting the worst stream; severity ordering is total.
- Include graph: acyclic; observation headers include only foundation/resilience/runtime/observation.
  No duplicate types beyond a per-translation-unit test helper class and std::hash specializations. No
  live-order path; no future-phase behaviour. Secret scan: none.

Interpretation:
Phase 2 satisfies the `PHASE-2` milestone acceptance ("prediction ledger and outcome engine
deterministic and auditable; failures structured"). All 4 tasks and the milestone are `APPROVED`.
Compilation and structural checks support but do not by themselves prove architectural correctness;
no profitability, calibration, broker-validation or production-safety claim is made.

## 2026-10-02 - Conceptual Phase 3/4/5 verification + MT5 one-EA/nine-stream boundary

Commits: see `HANDOFF.md` / `PROJECT_STATE.md` (conceptual P3/P4/P5 wave and MT5 wave).

Toolchain: `g++ (Debian 14.2.0-19) 14.2.0`, `-std=c++17` and `-std=c++20`, `-Wall -Wextra -Werror
-pedantic`. Python 3 static audits. No application build system; ad-hoc harness.

Build/test commands:
- per-header self-containment and combined all-headers TU (80 headers) under both standards
- `g++ -std=<std> -Wall -Wextra -Werror -pedantic -I src src/<artifact>.cpp src/foundation/Hasher.cpp -o t && ./t`
  for `RuntimeTests`, `DegradationTests`, `ObservationTests`, `TimeframeStateTests`,
  `AnalysisPipelineTests`, `SignalPipelineTests`, `Mt5ReceiverTests`
- MQL5 static audit (python) and repository static audit (python)

Result: PASS.
- 80 headers self-contained and combined TU OK under c++17/c++20 strict.
- All seven behavioural test artifacts PASS under both standards (real code paths, no mocks):
  - Conceptual P3 (`TimeframeStateTests`): nine explicit stream identities; M1 failure leaves M15
    unchanged; M15 recovery does not reset M1/H4; W1 outage does not alter H1; per-stream sequence;
    duplicate finalized bar and same-close_time repaint rejected; future-dated and forming bars
    rejected (no lookahead/no repaint); per-stream quality; deterministic progress; provenance.
  - Conceptual P4 (`AnalysisPipelineTests`): deterministic bounded features with `based_on`
    provenance; forming bar refuses computation; empty/unknown input -> explicit invalid; H4
    structural authority (M15/M1 refused); deterministic bounded regime (ranking, not probability);
    macro context only D1/W1/MN1; degraded/stale -> INELIGIBLE with reasons; market quality never
    fabricates GOOD.
  - Conceptual P5 (`SignalPipelineTests`): deterministic symbol-sensitive V3-23 identity; eligibility
    gates the signal; M15 trigger under H4 structure; score/confidence deterministic and zero on
    non-VALID data; risk gate refuses unusable quality / zero ATR; shadow fills/positions
    `is_live == false`; attempted live execution blocked (`SHADOW_ONLY`); append-only idempotent
    ledger with deterministic identity and preserved provenance.
  - MT5 (`Mt5ReceiverTests`): encode/decode round-trip; nine-stream routing by explicit timeframe;
    malformed / unknown-timeframe / checksum-mismatch / future-dated rejection; duplicate and
    regression rejection; M1-failure/H4-unaffected; M1-recovery/H4-not-reset; W1-outage/H1-unaffected;
    deterministic per-stream progress.
- MQL5 static audit PASS: exactly one physical EA; nine explicit timeframe labels in canonical order;
  `AURA_STREAM_COUNT == 9`; single shared transport; zero `OrderSend`/`PositionModify`/`CTrade`
  tokens; includes resolve; brackets balanced; `OnInit`/`OnTick`/`OnTimer`/`OnDeinit` present.
- Repository static audit: no include cycles; correct layer direction; no duplicate class
  definitions; no live-execution tokens in C++; no secrets.

Interpretation:
The conceptual Phase 3/4/5 gates PASSED (verified against manifest Phase 1, which they map to per
`PHASE_RECONCILIATION.md`). The MT5 one-EA/nine-stream boundary is implemented to the statically
verified level. MetaEditor/MT5 are NOT available, so MQL5 compilation and real terminal connectivity
are UNPROVEN. Live trading is NOT enabled. Compilation and structural checks support but do not prove
architectural correctness; no profitability, calibration, broker-validation or production-safety
claim is made.


## 2026-10-02 - Canonical Phase 3 (Self-Learning) + Phase 4 (Research Plane)

Toolchain: `g++ (Debian 14.2.0-19) 14.2.0`, `-std=c++17` and `-std=c++20`,
`-Wall -Wextra -Werror -pedantic`. Python 3 static audits.

Promotion: canonical Phases 3-11 were promoted into active scope by the `DECISIONS.md` entry
"2026-10-02 - Session authorization: promote canonical Phases 3-11 into active scope".

Phase 3 (Self-Learning, `src/learning/`): KnowledgeObject, KnowledgeStore, KnowledgeLifecycle,
ContextLearning, ContradictionEngine, KnowledgeDecay, FailureMemory, LearningTests.
- Build/test: `g++ -std=<std> -Wall -Wextra -Werror -pedantic -I src src/learning/LearningTests.cpp
  src/foundation/Hasher.cpp -o t && ./t` -> PASS c++17/c++20.
- Proven: point-in-time consistency (assessment cannot precede observation; future evidence
  rejected); versioned append-only store (no silent overwrite, no rewind, idempotent, conflicting
  revision rejected, history retained); explicit lifecycle only; context learning excludes
  observations whose outcome postdates the assessment and counts the exclusion (no future-label
  leakage); contradiction preserved as a first-class object with order-independent identity;
  decay/revalidation never erases; failure memory excludes not-yet-known outcomes and aggregates
  deterministically.

Phase 4 (Research Plane, `src/research/`): Hypothesis, Experiment, ExperimentFingerprint,
ResearchBudget, ExperimentLedger, ResearchPlanner, ResearchSandbox, ResearchTests.
- Build/test: `g++ -std=<std> ... src/research/ResearchTests.cpp src/foundation/Hasher.cpp` -> PASS
  c++17/c++20.
- Proven: falsifiability mandatory; fingerprint deterministic, ignores transient fields
  (timestamp/result), changes with identity-relevant fields; append-only ledger with idempotent
  re-record, duplicate-fingerprint detection, failed experiments retained; budget GREEN/YELLOW/RED/
  FROZEN with no overspend/partial spend; validation protocol fixed before evaluation and immutable
  within a family; sandbox never mutates runtime and admits only recorded hypotheses with a fixed plan.

Header self-containment + combined TU verified for all Phase 3/4 headers under both standards.
Interpretation: Phase 3 and 4 satisfy their V3-40 capability sets to the statically verified,
behaviourally tested level. No live path; no profitability/calibration claim; research cannot mutate
runtime.


## 2026-10-02 - Canonical Phase 5 (Evolution) / Phase 6 (Validation) / Phase 7 (Governance)

Toolchain: `g++ (Debian 14.2.0-19) 14.2.0`, `-std=c++17`/`-std=c++20`,
`-Wall -Wextra -Werror -pedantic`.

Phase 5 (`src/evolution/`): Candidate, CandidateRegistry, EvolutionGraph, CandidateComparison,
EvolutionTests -> PASS c++17/c++20. Proven: explicit candidate lifecycle with no hidden transitions
(cannot reach PROMOTED without human-approval path); append-only registry retains history and refuses
illegal jumps; matched comparison yields INSUFFICIENT_EVIDENCE when unmatched and VETOED on a failed
hard constraint; evolution graph acyclic (unknown parent rejected) with reproducible lineage.

Phase 6 (`src/validation/`): ValidationFirewall, EvidenceFirewall, EvaluatorFirewall,
RewardHackingDefense, StatisticalControls, ValidationTests -> PASS c++17/c++20. Proven: missing check
is NOT_RUN not PASS; any FAIL is NOT_CREDIBLE; holdout budget enforced, malformed request refused, no
contamination on refused query; contamination is monotone (no silent reset); evaluator-family mismatch
refused; hard constraints veto a high composite metric; missing statistical evidence is not passing.

Phase 7 (`src/governance/`): PolicyEngine, ForbiddenBehavior, PromotionGate, HumanDecision,
AuditLedger, GovernanceTests -> PASS c++17/c++20. Proven: policy by change type (risk/evaluator/
research-method need human approval); forbidden behaviors rejected (only a trusted maintainer may
request governed deployment actions, never the integrity anchors); promotion requires every gate incl.
human approval and a shadow-stage candidate, with no self-promotion; automated actors cannot record an
ACCEPTED human decision; audit ledger append-only with no mutation API.

Header self-containment + combined TU verified for all Phase 5/6/7 headers under both standards.
Interpretation: Phases 5-7 satisfy their V3-40 capability sets to the statically verified,
behaviourally tested level. No live path; no profitability/calibration claim; governance cannot
self-promote or mutate runtime.


## 2026-10-02 - Canonical Phases 8-11 (Operating Window, Desktop, Telegram, Real-World Validation)

Toolchain: g++ 14.2.0, -std=c++17/-std=c++20, -Wall -Wextra -Werror -pedantic.
Phase 8 (src/operatingwindow/): OperatingWindow, Checkpoint, ResourceBudget, WindowOrchestrator,
OperatingWindowTests -> PASS c++17/c++20. Proven: human-bounded 3-8h window with self-extension
refused; automated sub-minimum schedule refused; phase transitions ACTIVE/DRAINING/OFFLINE;
recovery prefers a valid compatible checkpoint else known-good and never guesses; corrupt/incompatible
checkpoints unusable; no partial resource spend; uncheckpointed resumable work refused, not discarded.
Phase 9 (src/desktop/): ControlCenterViewModels, DashboardProjector, DesktopTests -> PASS
c++17/c++20. Proven: read-only projections from trusted layers; source values unchanged after
projection; an UNKNOWN tile is surfaced as unknown, never fabricated as healthy; deterministic
explicit-order graph projection. Headless, no GUI toolkit.
Phase 10 (src/telegram/): TelegramConfig, TelegramMessage, ApprovalRequest, TelegramGateway,
NotificationPolicy, TelegramTests -> PASS c++17/c++20. Proven: config is secret-free (no embedded
token, not configured until a human sets it); a disabled gateway is a safe no-op and is never a core
dependency; informational messages only; the approval pipeline enforces all six verification steps
and any failure stops before promotion; ERROR/INCIDENT/ROLLBACK/APPROVAL_NEEDED notifications cannot
be suppressed.
Phase 11 (src/realworld/): RealWorldValidation, ValidationRegistry, RealWorldTests -> PASS
c++17/c++20. Proven: all readiness gates required (missing = unmet, never assumed); never automatic;
explicitly makes no profitability/production-safety claim; append-only validation-run registry.
Full-suite regression: all 16 test suites PASS under both c++17 and c++20.
Interpretation: Phase 11 is a gate/registry only. No live trading path is enabled; real MT5
execution readiness remains UNPROVEN in this environment (no MetaEditor/MT5 on Linux).

## 2026-10-02 - Phase 12 Integration / Application (gap-audit closure)
Toolchain: g++ 14.2.0; CMake (pip) configure/build; -std=c++17 and -std=c++20,
-Wall -Wextra -Werror -pedantic (direct) / -Wall -Wextra -Wpedantic (CMake).
New sources under test: src/runtime/ApplicationPipeline.h (APP-0001),
src/runtime/ApplicationShell.h (APP-0002), tools/run_pipeline.cpp (APP-0003),
src/runtime/EndToEndTests.cpp (E2E-0001), CMakeLists.txt (BUILD-0001).
Results:
- CMake configure + build: SUCCESS (static lib aura_foundation, 17 test targets, aura executable).
- CTest: 17/17 tests PASSED (including EndToEndTests).
- Direct strict compile of all 17 suites: ALL PASS under both c++17 and c++20.
- New headers self-contained (standalone TU) under c++17 and c++20.
- EndToEndTests (real loopback TCP, no mocks): proves nine-stream availability, explicit-timeframe
  routing, closed-bar acceptance, duplicate/malformed/future-dated rejection, per-stream isolation,
  deterministic decision/proposal identity and ledger size across identical runs, and a strictly
  shadow-only path (proposal.is_order == false).
- aura --replay of 360 real canonical frames: 9/9 streams HEALTHY, 360 accepted, 0 rejected,
  0 malformed, 38 signals, 38 risk proposals, 38 shadow fills, 151 ledger entries, aggregate ONLINE.
- aura --serve smoke test over a real socket: same counts; clean SIGTERM shutdown (no hang).
- Live-lifecycle fix verified: accept() is now interruptible (select with timeout), so the host loop
  observes shutdown without a client connected.
Interpretation: This closes the audit's integration P0 for the environment-independent parts. A real
end-to-end MT5 -> analysis -> shadow path now exists and is proven over a real socket, but a real MT5
terminal round-trip, a native Windows GUI, and a historical dataset campaign remain BLOCKED (no
toolchain/terminal/dataset here). No profitability, calibration, broker-validation, production-safety
or live-trading claim is made.

## 2026-10-03 — Phase 13: durable persistence + crash recovery + packaging/CI

Task/Phase: PERSIST-0001, PERSIST-0002, BUILD-0002 (Phase 13 Integration).

Build command:
`cmake -S . -B <build> -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17|20 && cmake --build <build> -j4`

Build result: PASS for both C++17 and C++20 (g++, `-Wall -Wextra -Wpedantic`), including the new
`aura`, `PersistenceTests`, and all pre-existing targets. Header-only code also passes
`-Wall -Wextra -Werror -pedantic -fsyntax-only` under both standards.

Test command: `ctest --test-dir <build> --output-on-failure`

Test result: `100% tests passed out of 18` under C++17 AND under C++20 (the count rose from 17 to 18
with the new `PersistenceTests`). `PersistenceTests` covers, over real code paths (no mocks):
- append idempotency (identical re-append = OK, conflicting payload = CONFLICT);
- atomic flush + reload round-trip preserving records, digests and payloads;
- byte-identical artifacts for identical append sequences (determinism);
- tampered byte -> `CORRUPT`; checksum-less/truncated file -> `CORRUPT` (never silently OK);
- V2-36 lifecycle classification (CLEAN_SHUTDOWN / EXPECTED_PAUSE / INTERRUPTED_WORK /
  CORRUPTED_STATE / UNKNOWN_STATE), version-incompatible -> refused, corrupted -> refused,
  uncheckpointed interruption -> refused;
- application restart over real encoded frames: a fresh shell restores nine timeframe states and the
  ledger, accepts strictly-newer bars (no repaint), and re-persist is idempotent.

Real host verification (not compilation alone):
- `aura --serve 12001 --store /tmp/serve.aura`: a Python client sent all 360 canonical frames over a
  real loopback socket; on Ctrl-C the process persisted and printed: transport connected, 9/9 streams
  healthy, 360 accepted, 0 rejected, 0 malformed, 38 signals / 38 risk proposals / 38 shadow fills,
  151 ledger entries, HEALTHY (ONLINE), SHADOW ONLY, persisted store 161 records. This matches the
  Phase 12 replay result exactly, so persistence did not perturb the pipeline.
- `aura --replay /tmp/frames.txt --store /tmp/replay.aura`: 360 frames, 9/9 streams, 360 accepted,
  38/38/38, 151 ledger entries, HEALTHY, persist OK (161 records).
- `aura --recover /tmp/replay.aura` (good store) -> lifecycle CLEAN_SHUTDOWN, resumable yes, exit 0.
  After flipping one payload byte, `--recover` -> CORRUPTED_STATE, resumable no, exit 1.
- `aura --self-test` -> PASS: 30 deterministic steps x 9 streams -> 270 accepted, 28/28/28, 111 ledger
  entries; persist OK (121 records); recovery CLEAN_SHUTDOWN resumable; restores 9 timeframes + 111
  ledger entries.
- `cmake --install <build> --prefix <dir>` -> installs `bin/aura`, `include/aura/**.h`, and the doc.

Files changed: `src/foundation/FilePersistenceStore.h` (new), `src/runtime/ApplicationRecovery.h` (new),
`src/runtime/PersistenceTests.cpp` (new), `src/mt5/Mt5StreamManager.h`, `src/runtime/ApplicationPipeline.h`,
`src/runtime/ApplicationShell.h`, `tools/run_pipeline.cpp`, `CMakeLists.txt`,
`.github/workflows/ci.yml` (new), `README.md`, and the control-plane files.

Known failures: none. Remote CI on PR #1 now has ALL FOUR jobs green: Linux gcc C++17 and C++20
(build + ctest + smoke + install) and Windows MSVC C++17 and C++20 (build + 18/18 ctest). This proves
the console host compiles, links (after the `ws2_32` fix) and passes its full suite under MSVC. Two
real portability defects were found by CI and fixed rather than hidden: (1) `ws2_32` (Winsock) was not
linked on Windows (`LNK2019 __imp_socket/...`); (2) `std::rename` does not overwrite an existing file
on Windows, so the atomic flush now clears the target before renaming (same-directory swap). Each fix
was re-verified locally (Linux 18/18 unaffected) before pushing. The Windows/MSVC CI job remains
non-blocking by policy, but it is currently passing.

Interpretation: durable persistence and the V2-36 crash-recovery decision now exist in the running
application and are verified by unit tests and a real socket run. A corrupted or version-incompatible
store is refused, never silently resumed; shadow-only behaviour is preserved (no proposal is ever an
order). This does NOT establish profitability, calibrated probability, broker validation, production
safety, or live-trading readiness.

## Phase 14 - canonical Windows x64 Release package (2026-10-03)

- Date: 2026-10-03
- Commit: 64ce035 (merged into the branch; CI run 37115746002)
- Task/Phase: BUILD-0003 / Phase 14
- Build command (CI, windows-latest): `cmake -S AURA -B AURA/build -A x64 -DCMAKE_BUILD_TYPE=Release
  -DCMAKE_CXX_STANDARD=17` then `cmake --build AURA/build --config Release -j 4`
- Build result: PASS. `aura.exe` produced (Release, x64, MSVC, static CRT) = 440,320 bytes.
- Test command: `ctest --test-dir AURA/build -C Release --output-on-failure` plus a PowerShell smoke:
  `aura.exe --self-test --store <p> --keep`; `aura.exe --recover <p>`; `aura.exe --dump-frames <f>`;
  `aura.exe --replay <f> --store <p>`; a bounded `--serve 12345` process check (single instance + TCP
  connect + forced stop); package + secret scan + ZIP verification (extract and re-run the packaged exe).
- Test result: PASS. Full suite 18/18; self-test PASS (9 timeframes, 111 ledger entries restored);
  cross-process recover => CLEAN_SHUTDOWN/resumable; replay 270 frames => HEALTHY, 111 ledger entries,
  last proposal is not an order; serve: started, stayed up, exactly 1 aura process, accepted a TCP
  connection, stopped cleanly (no immediate crash); secret scan OK; ZIP contains aura.exe and the
  extracted exe re-runs self-test PASS.
- Names/sizes: `aura.exe` = 440,320 bytes; `AURA_Windows_x64_Release.zip` = 220,103 bytes. Artifacts:
  `aura-windows-x64-exe` (217,039 bytes as uploaded) and `AURA_Windows_x64_Release` (220,131 bytes).
- Files changed: `tools/run_pipeline.cpp`, `CMakeLists.txt`, `.github/workflows/ci.yml`, and the
  control-plane files.
- Known failures: none.
- Interpretation: the canonical AURA executable now exists as a real, self-contained Windows x64
  Release `.exe` and is packaged and published as a CI artifact. Its startup/init/single-instance/
  clean-stop and its persistence/recovery/replay paths are verified on a real Windows runner by a
  bounded smoke. The native Windows GUI is NOT included and its runtime is UNPROVEN. MT5/MetaEditor is
  NOT exercised. No live order path; shadow only. No profitability/calibration/production-safety claim.

## 2026-10-03 — Phase 9 GUI: XAUUSD candlestick chart + timeframe selector (GUI-0015..GUI-0020)

- Date: 2026-10-03
- Commit: (recorded with this entry; branch `phase13-persistence-recovery`)
- Task/Phase: GUI-0015 (read-only `ApplicationPipeline::bar_series`/`last_bar`), GUI-0016
  (`src/desktop/CandleChart.h`), GUI-0017 (`ControlCenterState.h` chart projection + selection),
  GUI-0018 (`src/desktop/CandleChartWidget.h` + Dashboard wiring), GUI-0019 (`aura_gui` self-test +
  `--replay` feed), GUI-0020 (`DesktopTests.cpp`).
- Build command: `cmake --build build-gui17 -j4` (GUI build, `-DAURA_BUILD_GUI=ON`, g++ C++17).
- Build result: PASS. `DesktopTests`, `aura`, `aura_gui` all link and build clean (one initial
  compile error fixed: a stray `AuraTheme.h` include in `DesktopTests.cpp` pulled ImGui into the
  ImGui-free test target; removed).
- Test command: `./build-gui17/DesktopTests`; `ctest --test-dir build-gui17`;
  `./build-gui17/aura_gui --self-test --store <p>`;
  `DISPLAY=:99 aura_gui --gui --replay <frames> --renderer {modern,legacy}` + `import` capture.
- Test result: PASS.
  - `DesktopTests: ALL PASS` — includes the seven new chart tests (M15 default + nine-timeframe
    identity switching + UNKNOWN refusal; foreign-timeframe/non-closed bar exclusion; NaN rejection;
    bullish/bearish wick+body geometry and 5-level grid determinism; report bound to the selected
    stream over real runtime frames; empty -> unavailable with zero candles; deterministic UTC labels).
  - `ctest`: **19/19 passed** (GUI build).
  - `aura_gui --self-test`: SELF-TEST PASS, and prints
    `chart nine timeframes OK, default M15, each stream has real candles`.
  - Xvfb captures: with the all-bullish builtin set the chart region has ~5,444 green candle px and
    0 red; with an alternating M15 series it has ~11,919 green + ~12,848 red candle px. Identical
    counts under `MODERN_GL33` and `LEGACY_GL21` (real candles render on both backends).
- Files changed: `src/runtime/ApplicationPipeline.h` (`bar_series`/`last_bar`),
  `src/runtime/ApplicationShell.h` (const `pipeline()`), `src/desktop/CandleChart.h` (new),
  `src/desktop/CandleChartWidget.h` (new), `src/desktop/ControlCenterState.h` (report.chart +
  selection + `chart_series`), `src/desktop/GuiPanels.h` (`draw_market_chart`, `draw_dashboard`,
  `draw_section` signature), `src/desktop/DesktopTests.cpp` (7 tests), `tools/aura_gui.cpp`
  (self-test checks + `--replay` feed in GUI mode).
- Known failures: none.
- Interpretation: the Dashboard now shows a real XAUUSD candlestick chart with an above-chart
  timeframe selector (`[M1]..[MN1]`, M15 default). Candles come only from the runtime's retained
  **closed** bars for the explicitly selected timeframe; non-closed/foreign/non-finite bars are
  dropped and an empty stream shows NO CANDLE DATA — nothing is fabricated. The chart renders
  identically on the OpenGL 3.3 and legacy OpenGL 2.1 backends. The frames used for the captures are
  deterministic closed bars produced by the trusted encoder path (synthetic, not real historical
  XAUUSD data). Interactive rendering on a real Windows desktop / Intel HD 3000 remains UNPROVEN
  (BLOCK-006). No live-order path; shadow only. No profitability/calibration/broker/production claim.

### Remote CI for this work

- Run `37131254459` (commit `02d3d5b`, chart + control-plane updates): **ALL GREEN 6/6** —
  `ubuntu-latest / c++17 / gcc`, `ubuntu-latest / c++20 / gcc`, `windows-latest / c++17 / msvc`,
  `windows-latest / c++20 / msvc`, `desktop-gui-linux (Dear ImGui + Xvfb)`, and
  `windows-x64-release-package` (builds/packages `aura_gui.exe` with the chart). Note: CI verifies
  build + tests + bounded software-GL smokes; it does **not** prove visual quality or real-Windows /
  Intel HD 3000 rendering.

## 2026-10-03 — Premium GUI overhaul (GUI-0021..GUI-0024)

- Date: 2026-10-03
- Commit: (recorded at commit time; this entry is part of the same commit)
- Task/Phase: Phase 9 / `GUI-0021` (design system + shell), `GUI-0022` (chart-first Dashboard),
  `GUI-0023` (real score/confidence/outcome projection), `GUI-0024` (deterministic projection test).
- Build command: `cmake --build build-gui17 -j4` (GUI ON, c++17) and `cmake --build build-off -j4`
  (GUI OFF); header-only C++17.
- Build result: clean (no errors; only pre-existing warnings, if any).
- Test command: `./build-gui17/DesktopTests`; `ctest --test-dir build-gui17`; `ctest --test-dir
  build-off`; `aura_gui --self-test`; Xvfb captures via ImageMagick `import` + OCR via `tesseract`.
- Test result:
  - `DesktopTests: ALL PASS` (includes the new `test_real_metrics_projection`).
  - GUI build `ctest`: **19/19 passed** (2.35 s). GUI-OFF build `ctest`: **18/18 passed**.
  - `aura_gui --self-test` PASS (270 frames, 9/9 streams, `chart nine timeframes OK, default M15,
    each stream has real candles`, checkpoint OK, recovery CLEAN_SHUTDOWN resumable).
  - Xvfb captures at **1600x900** and **1280x720** under **MODERN_GL33** and **LEGACY_GL21**; OCR reads
    the header (`AURA XAUUSD MARKET INTELLIGENCE ... TF M15 SHADOW ONLY MARKET HEALTHY RENDERER
    MODERN_GL33`), the market strip (`LAST CLOSED 141.808 AT 01-12 15:43 STREAM ONLINE QUALITY VALID
    BARS 32`) and the six-column status strip (`SYSTEM HEALTHY | DATA STREAMS 9/9 | SCORE 0.97 |
    SIGNAL LONG | RISK PROPOSED | MODE SHADOW`) in one aligned row on both backends.
- Files changed: `src/desktop/AuraTheme.h`, `src/desktop/AuraWidgets.h`, `src/desktop/GuiPanels.h`,
  `src/runtime/ApplicationPipeline.h`, `src/desktop/DesktopModel.h`, `src/desktop/DesktopTests.cpp`,
  `tools/aura_gui.cpp`; control plane (`PROJECT_STATE.md`, `TASK_MANIFEST.yaml`, `TEST_LOG.md`,
  `HANDOFF.md`).
- Known failures: none. One defect was found and fixed during this work: the first six-column status
  strip used manual `SameLine` offsets that cascaded vertically (values rendered as a diagonal), caught
  by the OCR pass; replaced with a borderless `ImGui::BeginTable` (verified aligned afterwards).
- Interpretation: the control center is now a chart-first premium terminal and shows AURA's **real**
  deterministic SCORE (RT-0011) and CONFIDENCE (RT-0012) plus the realized shadow success rate. These
  are ranking/derived values, explicitly labelled "not a probability" / "not calibrated"; AURA computes
  no calibrated probability and the GUI claims none. `test_real_metrics_projection` proves the projected
  values equal the pipeline's own values, are bounded to `[0,1]`, and are marked unavailable when absent
  (never a fabricated 0%). Shadow-only invariant and no-order-path invariant unchanged. Visual quality is
  verified structurally + by capture/OCR under both GL backends, **not** by a human aesthetic review on
  the target Windows/Intel HD 3000 machine (still UNPROVEN, `BLOCK-006`). No profitability/calibration/
  broker/production claim.

## Future test entry format

- Date
- Commit
- Task/Phase
- Build command
- Build result
- Test command
- Test result
- Files changed
- Known failures
- Interpretation
