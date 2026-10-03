# AURA -- AI Handoff

## Current handoff (2026-10-03)

Read `AI_BOOTSTRAP.md` -> `IMPLEMENTATION_SCOPE.md` -> `PROJECT_STATE.md` -> `TASK_MANIFEST.yaml` ->
`DECISIONS.md` -> `BLOCKED.md` -> `HANDOFF.md` -> `TEST_LOG.md` in that order, then inspect the source.
Do not rely on any prior conversation memory.

### Where we are

- Repository: `alimazna/AURA`, project nested under `AURA/`. Authority: the Master V3 is the
  architecture reference; `IMPLEMENTATION_SCOPE.md` defines the active scope.
- All canonical Phases 0-11 are COMPLETE and `APPROVED`. Phase 12 (integration/application), Phase 13
  (durable persistence + crash recovery + packaging/CI) and the Windows x64 Release package are
  implemented and `TESTED`. Phase 9's Desktop Control Center now includes a **real GUI**
  (`GUI-0001`, IMPLEMENTED) plus read-only projections for every remaining V3-37 section
  (`GUI-0004..GUI-0006`, APPROVED).
- Authoritative build: `AURA/CMakeLists.txt` (header-only C++17/20 + the `aura_foundation` SHA-256 TU).
  It builds the `aura` console host, 18 behavioural test executables, and installs `bin/aura` +
  headers + docs. `ctest` is 18/18 green under C++17 AND C++20. On Windows/MSVC the CRT is linked
  statically so `aura.exe` is self-contained.
- Real host path (shadow-only): `aura --replay`, `aura --serve <port>` (interruptible accept),
  `aura --self-test [--keep]` (bounded offline smoke), `aura --dump-frames <file>`, `aura --recover
  <store>` (report the V2-36 decision). `--store <path>` enables file-backed persistence of
  per-timeframe progress + the shadow ledger.
- Manifest graph: 199 tasks -- 178 `APPROVED`, 7 `TESTED` (PERSIST-0001, PERSIST-0002, BUILD-0002,
  BUILD-0003, GUI-0002, GUI-0003, PHASE-9-MILESTONE), 2 `IMPLEMENTED` (TASK-MANIFEST-001, GUI-0001),
  10 `DEFERRED`, 2 `BLOCKED`.
- CI: `.github/workflows/ci.yml` builds/tests Linux C++17+C++20 (blocking) with the smoke test and an
  install check, plus MSVC Windows C++17/C++20, plus a `desktop-gui` job (Dear ImGui + GLFW + Xvfb
  bounded interactive smoke), plus a `windows-x64-release-package` job that builds `aura.exe` and
  `aura_gui.exe`, runs the full suite, smokes both exes, packs `AURA_Windows_x64_GUI_Release.zip`,
  verifies it and uploads the artifacts. CI previously caught and we fixed two real Windows
  portability defects (missing `ws2_32` linkage; `std::rename` not overwriting) -- see TEST_LOG.md.
- Remote CI run 37117057898 (commit `aaeb135`) is ALL GREEN across 6/6 jobs (ubuntu c++17/c++20,
  windows msvc c++17/c++20, `desktop-gui`, `windows-x64-release-package`). GUI artifacts:
  `aura_gui.exe` = 1,016,832 bytes, `aura.exe` = 440,320 bytes, `AURA_Windows_x64_GUI_Release.zip`
  = 714,226 bytes (verified to contain both exes; each re-ran `--self-test` PASS).
- Verified but still NOT claimed: profitability, calibrated probability, broker validation,
  production safety, or live trading. Shadow mode remains the only execution path. Real-Windows-desktop
  interactive GUI rendering is UNPROVEN (the Xvfb smoke is software-rendered on Linux); MT5/MetaEditor
  is UNPROVEN.

### Phase 9 V3-37 section integration deliverables (this session)

- `src/desktop/ControlCenterPanels.h` (new) — pure, deterministic read-only projectors for the
  remaining V3-37 sections (prediction, shadow ledger, incidents, knowledge, research, candidates,
  validation, approvals, evolution, schedule, checkpoints, audit, configuration/version) over a
  `PanelSources` set of copied records. No GUI toolkit, no I/O, no clock, no input mutation.
- `src/desktop/ControlCenterState.h` — builds a single `ControlCenterReport` (core snapshot + all
  section projections). Version context and the shadow ledger come from the live pipeline; incidents
  come from the real adapter stream health via `FailureDetectionEngine`; the remaining planes accept
  supplied records. Ledger snapshot cached by size to avoid re-copying every frame.
- `src/desktop/GuiPanels.h` — renders every section; wired sections bind to live data, unsourced
  sections show explicit `NOT AVAILABLE`. `draw_section` now takes the full `ControlCenterReport`.
- `tools/aura_gui.cpp` — refresh via `refresh_report()`; `--self-test` now asserts version/ledger/
  incident sourcing and `NOT AVAILABLE` for unsourced planes; bounded `--frames` smoke cycles all 19
  sections.
- `src/desktop/DesktopTests.cpp` — 3 new tests (`test_section_panel_availability`,
  `test_incident_propagation_and_corruption`, `test_single_runtime_owner`).
- Verification: DesktopTests ALL PASS c++17/c++20 strict; default 18/18, GUI 19/19; `aura_gui
  --self-test` PASS; Xvfb interactive smoke cycles all sections and shuts down cleanly. Recorded in
  `TEST_LOG.md`.

### Phase 14 deliverables (this session)

- `tools/run_pipeline.cpp` -- added `--dump-frames <file>` and `--self-test --keep` so a separate
  process can exercise cross-process `--recover` and `--replay` in CI.
- `CMakeLists.txt` -- `CMAKE_MSVC_RUNTIME_LIBRARY` static CRT for a self-contained `aura.exe`.
- `.github/workflows/ci.yml` -- `windows-x64-release-package` job: Release/x64 build, full CTest,
  bounded serve smoke (startup/init/single-instance/clean-stop), packaging, secret scan, ZIP creation
  and verification, and artifact upload (`aura-windows-x64-exe`, `AURA_Windows_x64_Release`).
- Artifacts: `aura.exe` (440,320 bytes), `AURA_Windows_x64_Release.zip` (220,103 bytes).

### Phase 9 GUI deliverables (this session)

- `tools/aura_gui.cpp` — single GUI entrypoint. `--gui` (window) and `--self-test` (headless
  integration smoke). Owns exactly one `ApplicationShell`.
- `src/desktop/DesktopModel.h` — read-only projection of the live runtime into
  `ControlCenterSnapshot` (nine timeframe rows by explicit identity; `NOT AVAILABLE` for absent
  data; `shadow_only` invariant).
- `src/desktop/ControlCenterState.h` — single runtime owner + safe control-plane ops
  (pause/resume transport, refresh, checkpoint, bounded stop, read-only recovery report) and the
  shared `builtin_frames` set.
- `src/desktop/GuiPanels.h` — ImGui rendering of the V3-37 sections; unwired sections show
  `NOT AVAILABLE`.
- `src/desktop/DesktopTests.cpp` — 3 new tests (nine rows/no-fabrication; fed 9/9 streams; checkpoint
  + cross-process CLEAN_SHUTDOWN recovery + CORRUPTED_STATE refused).
- `CMakeLists.txt` — `AURA_BUILD_GUI` (default OFF) + pinned FetchContent (glfw 3.4, imgui v1.90.9) +
  `aura_gui` target + `GuiSelfTest`.
- `.github/workflows/ci.yml` — new `desktop-gui` job (Linux + Xvfb bounded interactive smoke);
  `windows-release` now builds the GUI, runs `aura_gui --self-test`, and packages
  `AURA_Windows_x64_GUI_Release.zip` (aura_gui.exe + aura.exe).

### Next actions

1. Remaining work is environment/data-unproven only: real-Windows-desktop interactive GUI (UNPROVEN —
   have a human run `aura_gui --gui` on Windows to promote it), real MetaEditor/MT5 round-trip
   (`MT5-REAL-0001`), and the historical XAUUSD validation campaign (`VAL-EVID-0001`). See
   `BLOCKED.md`. Do not invent datasets/toolchains.
2. Wire the still-`NOT AVAILABLE` panels (prediction/observation, knowledge, research, candidates,
   validation evidence, approvals, evolution, schedule, operating-window checkpoints) to their existing
   read-only ledgers once the control center runs those planes; do not fabricate.
3. Optional hardening: a SIGKILL-during-write recovery test for the atomic rename path.
4. If new GUI work starts, keep the approved stack (Dear ImGui + GLFW + OpenGL).

### Do not

- silently redesign the architecture or invent contracts/dependencies
- treat existing source as automatically authoritative
- enable unattended live trading or claim profitability/safety without evidence
- rely on previous AI conversation memory

---

## Historical handoff (2026-10-02) -- superseded

## Current handoff

Date: 2026-10-02

### Where we are

- Repository layout: the project lives under the nested `AURA/` directory inside `alimazna/AURA`;
  the repository root contains only `AURA/`.
- Phases 0, 0.5, 1 and 2 are COMPLETE and `APPROVED`, plus the conceptual Phase 3/4/5 verification
  gates and the MT5 one-EA/nine-stream boundary (`PHASE-MT5`).
- Source layers under `AURA/src/`: `foundation/` (35 headers), `resilience/` (19), `runtime/` (20),
  `observation/` (4), and the new `mt5/` boundary (3 C++ headers + 7 MQL5 sources).
- There is still NO application build system and no test framework; verification is ad-hoc via the
  existing harness convention. No CMake/application build exists.
- The earlier baseline build/test audit remains non-reproducible historical material
  (see `TEST_LOG.md`).
- `TASK-MANIFEST-001` produced the detailed file-level task graph
  (`project-control/TASK_MANIFEST.yaml`, `manifest_version: 2`, now 113 tasks).

### Task graph summary

- COMPLETE/APPROVED: Phase 0 (36 tasks), Phase 0.5 (20), Phase 1 (21), Phase 2 (4), conceptual
  Phase 3/4/5 verification (6), MT5 boundary (11), plus phase milestones.
- Deferred and still visible: canonical manifest Phases 311 (`DEFERRED`).
- New this continuation: `PH3-0001`, `PHASE-3-TS`, `PH4-0001`, `PHASE-4-FSR`, `PH5-0001`,
  `PHASE-5-SIG`, `MT5-0001`..`MT5-0010`, `PHASE-MT5` -- all `APPROVED`.
- Authority note: Phase 0 filenames come from V3-42. The `src/` layout, the `EntityId`
  representation, the `Version` grammar, and the Phase 0.5/1/2/MT5 filenames are PROPOSED
  decomposition requiring integration review; they are not canonical architecture.

### Reconciliation note (2026-10-02)

- Wave 0A (V3-44) is exactly 12 tasks; all 12 files verified present, none omitted.
- A contract inconsistency was found and corrected rather than silently frozen: `SystemMode.h`
  now uses the exact V3-14 values; `RecoveryAction` exposes `UNKNOWN`; `MessageMetadata` carries
  the V3-23 `HashDigest` checksum; `EventId` exposes stable `to_string()`.

### Prepared control files

- Master architecture (`docs/AURA_MASTER_UNIFIED_PROJECT_v3.0.md`)
- AI bootstrap instructions
- Current implementation scope
- Project state
- Task manifest
- Human decisions
- Blocked state
- Test log
- AI handoff
- Phase reconciliation (`project-control/PHASE_RECONCILIATION.md`)
- MT5 one-EA / nine-stream control (`project-control/MT5_ONE_EA_NINE_STREAM_CONTROL.md`)
- Final verification report (`project-control/FINAL_VERIFICATION_REPORT.md`)

### Next AI action

1. Read `project-control/AI_BOOTSTRAP.md`.
2. Read `project-control/IMPLEMENTATION_SCOPE.md`.
3. Read `project-control/PROJECT_STATE.md`.
4. Read `project-control/TASK_MANIFEST.yaml` and pick the next actionable task.
5. Phases 0, 0.5, 1, 2 are COMPLETE and `APPROVED`; conceptual Phase 3/4/5 are VERIFIED and the MT5
   one-EA/nine-stream boundary is IMPLEMENTED (MQL5 static + C++ tests). There is no READY task in the
   active scope.
6. To continue: either promote the next canonical deferred phase (3/4/5/6..11) with an explicit
   `DECISIONS.md` entry, or compile `src/mt5/mql5/AURA_MT5_EA.mq5` in MetaEditor on a Windows host
   and test real terminal connectivity.
7. Run appropriate deterministic checks; record real results in `TEST_LOG.md`.
8. Preserve implementation + state in Git before ending the session.

### Session end state (2026-10-02, Phase 0 integration gate)

- Phase 0 is COMPLETE: all 36 file-level Phase 0 tasks and the `PHASE-0` milestone are `APPROVED`.
- The integration gate re-ran the strongest available verification (70 standalone TUs across 35
  headers  c++17/c++20 strict; full Phase 0 harness incl. SHA-256 KATs; acyclic include graph;
  36/36 outputs; no strays/secrets) and reconciled 10 under-declared manifest dependency lists.
- No task is BLOCKED; `BLOCK-001` and `BLOCK-002` are RESOLVED.
- Next READY task is `RS-0001` (Phase 0.5). It was NOT started and must not start without explicit
  authorization.
- Pushed to `origin/main`; remote URL is token-free and no credential is persisted anywhere.

### Session end state (2026-10-02, Phase 2 complete)

- Phase 2 (Observation & Outcomes) is COMPLETE: all 4 file-level tasks (`OB-0001`..`OB-0004`) and the
  `PHASE-2` milestone are `APPROVED`.
- `src/observation/` now holds the append-only prediction ledger, the deterministic outcome engine, the
  structured failure-detection engine and the deterministic system health monitor, plus
  `ObservationTests.cpp`.
- Verification: 4 observation headers self-contained + combined all-headers TU (78 headers) under
  c++17/c++20 strict; `ObservationTests.cpp` passes under both standards. Details in `TEST_LOG.md`.
- Remaining phases (3..11) are `DEFERRED` with no file-level decomposition. Promotion requires an
  explicit `DECISIONS.md` entry.
- Pushed to `origin/main`; remote URL is token-free and no credential is persisted anywhere.

### Session end state (2026-10-02, gap audit + Phase 12 integration)

- Reviewed the Master-to-repository gap audit against the real repository and accepted its central
  finding: AURA was a large tested foundation + real MT5 source + shadow pipeline, but had no
  integrated application runtime, build system, or real end-to-end path. The "finished Windows
  product" claim is not supported by repository evidence.
- Phase 12 (Integration/Application) implemented and `APPROVED`:
  - APP-0001 `src/runtime/ApplicationPipeline.h` -- one real frame -> full runtime -> shadow -> ledger.
  - APP-0002 `src/runtime/ApplicationShell.h` -- real dependency-free socket transport + host loop
    (POSIX/Winsock), interruptible accept, newline frame reassembly, clean drain.
  - APP-0003 `tools/run_pipeline.cpp` -- real `aura` executable (`--replay`, `--serve`), shadow only.
  - E2E-0001 `src/runtime/EndToEndTests.cpp` -- one reproducible end-to-end test over a real socket.
  - BUILD-0001 `CMakeLists.txt` -- static lib + all tests + `aura` + CTest.
- Verification: `ctest` 17/17 green; all 17 suites pass under c++17 and c++20 strict; `aura --replay`
  of 360 frames gave 9/9 streams, 360 accepted, 0 rejected, 38 signals/proposals/fills, 151 ledger
  entries, HEALTHY; `aura --serve` smoke-tested over a real socket with clean SIGTERM shutdown.
- Real blockers recorded: GUI-0001 (Windows GUI, needs a Windows toolchain), MT5-REAL-0001
  (MetaEditor/MT5 unavailable on Linux), VAL-EVID-0001 (no licensed historical XAUUSD dataset).
- Next unblocked step: PERSIST-0001, wire a file-backed `IPersistenceStore` into the application so
  timeframe state and ledgers survive restart. No live trading; no profitability/safety claims.

### Session end state (2026-10-02, canonical Phases 8-11)

- Phase 8 (Operating Window & Recovery) COMPLETE and `APPROVED`: `src/operatingwindow/` --
  `OperatingWindow`, `Checkpoint`, `ResourceBudget`, `WindowOrchestrator`, `OperatingWindowTests.cpp`
  (`WIN-0001`..`WIN-0005`). Human-bounded 3-8h window, no self-extension; recovery prefers a valid
  checkpoint/known-good and never guesses; controlled drain.
- Phase 9 (Desktop Control Center) COMPLETE and `APPROVED` as headless deterministic view-models:
  `src/desktop/` -- `ControlCenterViewModels`, `DashboardProjector`, `DesktopTests.cpp`
  (`DESK-0001`..`DESK-0003`). No GUI toolkit; read-only projections; unknown never fabricated.
- Phase 10 (Auxiliary Telegram) COMPLETE and `APPROVED`, secret-free: `src/telegram/` --
  `TelegramConfig`, `TelegramMessage`, `ApprovalRequest`, `TelegramGateway`, `NotificationPolicy`,
  `TelegramTests.cpp` (`TG-0001`..`TG-0006`). No embedded token; disabled gateway is a safe no-op;
  bounded approval pipeline; failure notifications never suppressed.
- Phase 11 (Controlled Real-World Validation) COMPLETE and `APPROVED` as a gate/registry, NOT a live
  path: `src/realworld/` -- `RealWorldValidation`, `ValidationRegistry`, `RealWorldTests.cpp`
  (`RW-0001`..`RW-0003`). All readiness gates required; never automatic; no profitability/safety claim.
- Verification: all Phase 8/9/10/11 headers self-contained + combined TU under c++17/c++20 strict; all
  16 test suites PASS under both standards. No live trading path; Phase 11 execution remains a
  separate, gated human action; real MT5 readiness still UNPROVEN.
- Manifest now 182 entries: Phases 0-11 decomposed plus 10 visible DEFERRED Master capabilities.


### Session end state (2026-10-02, canonical Phases 57)

- Phase 5 (Evolution) COMPLETE and `APPROVED`: `src/evolution/` -- `Candidate`, `CandidateRegistry`,
  `EvolutionGraph`, `CandidateComparison`, `EvolutionTests.cpp` (`EVOL-0001`..`EVOL-0005`).
- Phase 6 (Validation) COMPLETE and `APPROVED`: `src/validation/` -- `ValidationFirewall`,
  `EvidenceFirewall`, `EvaluatorFirewall`, `RewardHackingDefense`, `StatisticalControls`,
  `ValidationTests.cpp` (`VALID-0001`..`VALID-0006`).
- Phase 7 (Governance) COMPLETE and `APPROVED`: `src/governance/` -- `PolicyEngine`,
  `ForbiddenBehavior`, `PromotionGate`, `HumanDecision`, `AuditLedger`, `GovernanceTests.cpp`
  (`GOV-0001`..`GOV-0006`).
- Verification: all Phase 5/6/7 headers self-contained + combined TU under c++17/c++20 strict;
  `EvolutionTests.cpp`, `ValidationTests.cpp`, `GovernanceTests.cpp` PASS under both standards.
  No live path; constraints are vetoes; governance cannot self-promote or mutate runtime.
- Manifest now 151 tasks. Next: canonical Phases 811 (Operating Window & Recovery, Desktop Control
  Center, Telegram, Controlled Real-World Validation).

### Session end state (2026-10-02, canonical Phases 34)

- Under the session authorization, canonical Phases 311 were promoted into active scope via
  `DECISIONS.md` ("Session authorization: promote canonical Phases 311"). Presentation-phase (9)
  scope is recorded as headless deterministic view-models.
- Phase 3 (Self-Learning) COMPLETE and `APPROVED`: `src/learning/` holds `KnowledgeObject`,
  `KnowledgeStore`, `KnowledgeLifecycle`, `ContextLearning`, `ContradictionEngine`, `KnowledgeDecay`,
  `FailureMemory`, `LearningTests.cpp` (`LEARN-0001`..`LEARN-0008`, `PHASE-3-MILESTONE`).
- Phase 4 (Research Plane) COMPLETE and `APPROVED`: `src/research/` holds `Hypothesis`, `Experiment`,
  `ExperimentFingerprint`, `ResearchBudget`, `ExperimentLedger`, `ResearchPlanner`, `ResearchSandbox`,
  `ResearchTests.cpp` (`RESEARCH-0001`..`RESEARCH-0008`, `PHASE-4-MILESTONE`).
- Verification: all Phase 3/4 headers self-contained + combined TU under c++17/c++20 strict;
  `LearningTests.cpp` and `ResearchTests.cpp` PASS under both standards. No future-outcome leakage;
  research cannot mutate runtime; no live path. Details in `TEST_LOG.md`.
- Manifest now 131 tasks. Next: canonical Phases 511 (Evolution, Validation, Governance, Operating
  Window & Recovery, Desktop Control Center, Telegram, Controlled Real-World Validation).

### Session end state (2026-10-02, conceptual Phase 3/4/5 + MT5 one-EA/nine-stream)

- Continuation request's conceptual "Phase 3/4/5" (Timeframe State / Feature-Structure-Regime /
  Signal) was reconciled against the canonical Master order (V3-40): that content belongs to the
  approved manifest Phase 1, so it was delivered as verification artifacts over Phase 1 rather than
  as new architecture. Mapping recorded in `PHASE_RECONCILIATION.md` and `DECISIONS.md`; the Master
  was NOT modified and no deferred manifest phase was promoted.
- New verification artifacts (`APPROVED`): `src/runtime/TimeframeStateTests.cpp` (`PH3-0001`),
  `src/runtime/AnalysisPipelineTests.cpp` (`PH4-0001`), `src/runtime/SignalPipelineTests.cpp`
  (`PH5-0001`), plus milestones `PHASE-3-TS`/`PHASE-4-FSR`/`PHASE-5-SIG`.
- MT5 one-physical-EA / nine-logical-stream boundary implemented (`APPROVED`):
  `src/mt5/mql5/{Common,Protocol,SocketClient,TimeframeStream,StreamManager,ShadowOrderGateway}.mqh`
  and `src/mt5/mql5/AURA_MT5_EA.mq5` (`MT5-0001`..`MT5-0007`); C++ counterpart
  `src/mt5/{ProtocolCodec.h,Mt5StreamManager.h,Mt5ReceiverTests.cpp}` (`MT5-0008`..`MT5-0010`);
  milestone `PHASE-MT5`.
- Physical EA count = 1; logical stream count = 9; transport count = 1; explicit per-stream
  timeframe identity (never positional); shadow-only (no live order path).
- One additive Phase 1 edit: `src/runtime/ShadowExecutionEngine.h` (`RT-0016`) gained an explicit
  `request_live_execution()` guard returning `LiveExecutionBlockReason::SHADOW_ONLY`; no live path.
- Verification: 80 headers self-contained + combined TU under c++17/c++20 strict; all seven
  behavioural test artifacts PASS under both standards; MQL5 static audit PASS; repository static
  audit clean. Details in `TEST_LOG.md` and `FINAL_VERIFICATION_REPORT.md`.
- MetaEditor/MT5 are NOT available: MQL5 compilation and real terminal connectivity are UNPROVEN.
  Live trading is NOT enabled. No profitability/calibration/broker-validation/production-safety
  claim is made.
- Next READY: none in the active scope. Canonical manifest Phases 311 remain `DEFERRED`; promotion
  requires a `DECISIONS.md` entry. Alternatively, compile the MQL5 EA on a Windows/MetaEditor host.

### Session end state (2026-10-02, Phase 1 complete)

- Phase 1 (Deterministic Runtime) is COMPLETE: all 21 file-level tasks (`RT-0001`..`RT-0021`) and the
  `PHASE-1` milestone are `APPROVED`.
- `src/runtime/` now holds the nine-stream MT5 adapter boundary, data bus, validator, closed-bar
  finalizer, timeframe state store, feature/structure/regime/eligibility/signal/score/confidence/macro/
  market-quality/risk engines, shadow execution, position simulator, reconciliation, append-only ledger,
  deterministic replay, the `Mt5Boundary.md` contract, and the `RuntimeTests.cpp` test artifact.
- Verification: 20 runtime headers self-contained + combined all-headers TU (74 headers) under
  c++17/c++20 strict; `RuntimeTests.cpp` passes under both standards. Details in `TEST_LOG.md`.
- Shadow-only: no live-order path exists; `is_order`/`is_live` are always false.
- Next READY: Phase 2, `OB-0001` (`src/observation/PredictionLedger.h`, deps `RT-0010`, `RT-0019`).
- Pushed to `origin/main`; remote URL is token-free and no credential is persisted anywhere.

### Session end state (2026-10-02, Phase 0.5 complete)

- Phase 0.5 is COMPLETE: all 20 file-level tasks (`RS-0001`..`RS-0020`) and the `PHASE-0.5`
  milestone are `APPROVED`.
- `src/resilience/` now holds capability identity/descriptors, health/freshness, the explicit
  capability dependency graph, impact/degradation, recovery/pause, coordination, and the
  `DegradationTests.cpp` behavioural test artifact.
- Verification: 19 resilience headers self-contained + combined TU under c++17/c++20 strict;
  `DegradationTests.cpp` passes under both standards. Details in `TEST_LOG.md`.
- MT5 one-EA / nine-logical-stream deployment decision recorded in `DECISIONS.md`; `RT-0001` and
  `RT-0021` contracts refined accordingly. Master V3 unmodified.
- Next READY: Phase 1, `RT-0001` (`src/runtime/AdapterManager.h`, deps `RS-0019`, `FND-0010`).
- Pushed to `origin/main`; remote URL is token-free and no credential is persisted anywhere.

### Session end state (2026-10-02, RS-0001)

- Phase 0.5 started under explicit authorization. Exactly one task executed: `RS-0001`
  (`src/resilience/ServiceDescriptor.h`), classified `APPROVED`.
- Verification: self-contained standalone TU + structural harness pass under c++17/c++20 strict;
  deterministic equality/ordering/hashing; all 8 canonical `ServiceState` values representable;
  combined all-headers TU compiles.
- `RS-0002` is the next READY task; it was NOT started. No other Phase 0.5/1/2 task was started.
- `PHASE-0.5` remains `PLANNED` (1 of 20 tasks complete).
- Pushed to `origin/main`; remote URL is token-free and no credential is persisted anywhere.

### Open blockers

- None. `BLOCK-001` and `BLOCK-002` are both RESOLVED.
- Remaining (non-blocking) state: the `src/` layout and Phase 0.5/1/2 filenames remain PROPOSED and
  need integration review as those phases begin.

### Do not

- begin by blindly repairing existing code (there is none here)
- treat current source as automatically authoritative
- implement the whole Master in one pass
- silently promote deferred phases
- flatten the repository again
- rely on previous AI conversation memory
- claim live trading/profitability readiness without evidence
