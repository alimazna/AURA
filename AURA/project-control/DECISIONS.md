# AURA — Human Architectural Decisions

Only explicit human decisions belong here.

## 2026-10-02 — Initial control-plane decision

- The Master V3 document is the architectural reference.
- The Master is important but is not interpreted as “implement every line immediately”.
- Implementation is incremental and scope-controlled.
- GitHub is the persistent memory of the project.
- Different AI agents may be used during the project.
- Any AI must resume from repository state, not chat history.
- The current priority is to build the Master-described system incrementally.
- Existing repository code is reusable implementation material only when compatible.
- Initial active scope follows the Master MVP guidance rather than the full end-state architecture.
- Live automation is deferred during the initial build.
- MT5 is initially a market/data boundary; later execution capabilities require validation and explicit promotion.
- Deferred capabilities require explicit human promotion into active scope.

## 2026-10-02 — Phase 0 ErrorCode taxonomy decision (resolves V3-45 OPEN DECISION)

- **Date:** 2026-10-02
- **Decision:** The canonical Phase 0 `ErrorCode` taxonomy is the 13 ERROR categories listed in
  Master V3 section 113, represented as a strongly typed `enum class`:
  `DATA_ERROR`, `SCHEMA_ERROR`, `CLOCK_ERROR`, `CONNECTION_ERROR`, `BROKER_ERROR`, `RISK_ERROR`,
  `EXECUTION_ERROR`, `RECONCILIATION_ERROR`, `PERSISTENCE_ERROR`, `MODEL_ERROR`, `CONFIG_ERROR`,
  `TELEGRAM_ERROR`, `RECOVERY_ERROR`.
  Each code has an explicit, documented, stable integral identity (explicit numeric assignment in
  the canonical order above; identity does not depend on compiler enum ordering). The taxonomy is a
  closed canonical set: no `UNKNOWN`, `OTHER`, or additional categories are added, and adding or
  changing codes in future requires a new explicit architectural/project decision. `ErrorSeverity`
  remains an independent record-level field; `ErrorCode` does not embed or derive severity. The
  stable `ErrorCode` identity is the explicit numeric value; no serialization, persistence, network,
  JSON or database code is implied.
- **Rationale:** Master V3 section 113 names the categories but does not freeze them as a closed,
  numbered enumeration (V3-45 marks this an OPEN DECISION). V3-02 makes an explicit human
  architectural decision the highest source of truth, and V3-03/V3-11 forbid an implementation agent
  from silently freezing an OPEN DECISION. This decision supplies the missing authority so that
  `FND-0015`/`FND-0016` can proceed without inventing a taxonomy.
- **Affected tasks:** `FND-0015 ErrorCode.h` (unblocked), `FND-0016 ErrorRecord.h` (unblocked once
  its remaining dependency conditions are met); later error/audit/persistence/Guardian contracts that
  consume error identity.
- **Scope-change flag:** No. This freezes an existing OPEN DECISION and does not expand the active
  implementation scope; the Master V3 document is not modified.
- **V3-45 status:** This decision resolves the V3-45 OPEN DECISION for `FND-0015 ErrorCode.h`.

## 2026-10-02 — MT5 one-EA / nine logical timeframe stream deployment decision

- **Date:** 2026-10-02
- **Decision:** The physical MT5 deployment model is ONE MQL5 Expert Advisor attached to ONE
  chart using ONE transport connection, carrying NINE logically independent timeframe streams
  (M1, M5, M15, M30, H1, H4, D1, W1, MN1). The nine logical streams keep independent identity,
  symbol identity, closed-bar identity, bar timestamp, capture/receive timestamp, sequence,
  data-quality state, stream health, provenance, last successful update, last error, and
  independent closed-bar tracking. One stream failing must not cause the other streams to be
  reported as unhealthy; one stream recovering must not overwrite another stream's state. Each
  message/event must carry explicit timeframe identity; positional multiplexing (e.g. "first
  message = M1") is forbidden unless the protocol contract makes identity explicit and stable.
  The canonical timeframe authority hierarchy (V3-29 / §4) is unchanged: H4 = primary
  structural authority, M15 = primary operational setup/trigger, M5/M1/TICK = execution /
  microstructure context, D1/W1/MN1 = long-horizon context.
- **Rationale:** The earlier design implied nine physical `.mq5` files (one per timeframe). The
  human project owner approved a one-physical-EA deployment that preserves the nine canonical
  logical stream responsibilities. This is an implementation/deployment decomposition decision,
  not an architectural change: the nine logical adapters/streams remain canonical (V3-29); only
  the physical packaging changes. The existing task graph already used a single adapter boundary
  (`RT-0001 AdapterManager.h`) and did not enumerate nine physical `.mq5` tasks, so no task
  replacement was required; the affected task contracts were refined to encode the nine-stream
  identity requirements.
- **Affected tasks:** `RT-0001 AdapterManager.h` (adapter boundary must carry nine explicit stream
  identities with independent health and closed-bar state), `RT-0021 Mt5Boundary.md` (boundary
  contract must require explicit per-stream timeframe identity and forbid positional
  multiplexing). No new source tasks are created by this decision.
- **Scope-change flag:** No. This is a deployment/decomposition decision within the
  already-authorized scope; it promotes no deferred phase and does not modify Master V3.
- **Master V3 status:** No modification required. V3-29 ("Nine MT5/MQL5 timeframe adapters are
  canonical") is satisfied by nine logical stream responsibilities under one physical EA.

## 2026-10-02 — Phase numbering reconciliation — conceptual Phase 3/4/5 vs manifest phase order

- **Date:** 2026-10-02
- **Decision:** The continuation request uses a conceptual/working decomposition ("PHASE 3 =
  Timeframe State", "PHASE 4 = Feature / Structure / Regime", "PHASE 5 = Signal"). These names do NOT
  correspond to the Master V3 authoritative phase numbers (§V3-40), where Phase 3 = Self-Learning,
  Phase 4 = Research Plane, Phase 5 = Evolution. The Master phase order is preserved and is not
  renamed. The conceptual Phase 3/4/5 content is mapped onto the already-authorized manifest Phase 1
  (Deterministic Runtime) and is completed as **hardening-and-proof phases** of that existing
  implementation, not as duplicate reimplementations. The mapping and evidence are recorded in
  `project-control/PHASE_RECONCILIATION.md`. New verification artifacts are
  `src/runtime/TimeframeStateTests.cpp` (P3), `src/runtime/AnalysisPipelineTests.cpp` (P4),
  `src/runtime/SignalPipelineTests.cpp` (P5).
- **Rationale:** `IMPLEMENTATION_SCOPE.md` and `DECISIONS.md` require that a deferred capability enter
  active scope only through an explicit human decision and task creation, and the Master phase order
  is authoritative (V3-40). The conceptual Phase 3/4/5 work does not map to any deferred capability;
  it maps to Phase 1, which is already approved. Treating the conceptual names as new phases would
  either (a) wrongly promote deferred Phases 3–11, or (b) duplicate approved Phase 1 code. Recording
  the mapping avoids both.
- **Affected tasks:** new `PH3-0001`, `PH4-0001`, `PH5-0001` (verification artifacts); no Phase 1
  behaviour file is replaced. If a genuine defect is found it is corrected in its owning Phase 1 file.
- **Scope-change flag:** No. No deferred phase is promoted; the Master V3 document is not modified.
- **V3-40 status:** Preserved unchanged.

## 2026-10-02 — MT5 one-EA / nine-stream boundary implementation authorization

- **Date:** 2026-10-02
- **Decision:** The approved MT5 one-EA / nine-logical-timeframe-stream deployment decision (recorded
  above on 2026-10-02) is implemented in real source: one physical MQL5 Expert Advisor entry point
  (`src/mt5/mql5/AURA_MT5_EA.mq5`) with supporting modules (`Common`, `Protocol`, `SocketClient`,
  `StreamManager`, per-stream state, `RealTimeQuote`), and a C++ protocol codec plus receiver stream
  manager under `src/mt5/` (`include/mt5/`). The physical EA count is 1; the logical stream count is 9;
  the transport count is 1. Positional timeframe multiplexing is forbidden: every stream message
  carries an explicit timeframe identity. Shadow-only is preserved; the EA contains no order-send,
  no position-modification and no `OrderSend`/`PositionModify` path. The exact module paths,
  responsibilities and task ownership are recorded in
  `project-control/MT5_ONE_EA_NINE_STREAM_CONTROL.md` and `TASK_MANIFEST.yaml` (`MT5-0001`..`MT5-0005`).
- **Rationale:** `RT-0021 Mt5Boundary.md` defined the boundary contract but no real source existed.
  This decision authorizes implementing that already-approved contract without changing the
  architecture. The Master names nine MT5/MQL5 adapters (V3-29) and a MT5/MQL5 adapter boundary; the
  physical packaging is one EA per the prior decision.
- **Affected tasks:** `MT5-0001`..`MT5-0005` (new); `RT-0021` is unchanged and remains the contract
  of record.
- **Scope-change flag:** No. This implements an already-authorized boundary within the active
  "MT5 market-data boundary" build-order item; no deferred phase is promoted.
- **Master V3 status:** No modification required.

## 2026-10-02 — MT5 boundary module paths finalized + shadow-only live-execution guard

- **Date:** 2026-10-02
- **Decision:** Finalize the MT5 boundary decomposition authorized above:
  - C++ headers live directly under `src/mt5/` (repo include convention `"mt5/..."`), not a nested
    `include/mt5/`; files are `ProtocolCodec.h`, `Mt5StreamManager.h`, `Mt5ReceiverTests.cpp`.
  - MQL5 sources remain under `src/mt5/mql5/` (`Common`, `Protocol`, `SocketClient`,
    `TimeframeStream`, `StreamManager`, `ShadowOrderGateway` `.mqh`, and `AURA_MT5_EA.mq5`).
  - The task set is `MT5-0001`..`MT5-0010` plus the `PHASE-MT5` milestone.
  - `src/runtime/ShadowExecutionEngine.h` (`RT-0016`) gains an explicit `request_live_execution()`
    guard that always returns `LiveExecutionBlockReason::SHADOW_ONLY` (`executed = false`). This adds
    no live path; it makes the shadow-only invariant explicitly testable (the required negative test).
- **Rationale:** The module path was PROPOSED decomposition requiring integration review
  (`HANDOFF.md` authority note). `src/mt5/*` matches the existing layer convention and keeps the
  C++ codec/receiver behind the one physical EA. The live-execution guard is additive and prevents a
  vague/untested shadow-only claim.
- **Affected tasks:** `MT5-0001`..`MT5-0010`, `PHASE-MT5` (all `APPROVED`); `RT-0016` (additive,
  `APPROVED`); `RT-0021` unchanged.
- **Scope-change flag:** No. Implements an already-authorized boundary; no deferred phase promoted.
- **Master V3 status:** Preserved unchanged; shadow-only (V3-32) upheld; live trading not enabled.

## 2026-10-02 — Session authorization: promote canonical Phases 3–11 into active scope

- **Date:** 2026-10-02
- **Decision:** Under the current human execution authorization ("FULL AUTONOMOUS CONTINUATION —
  Execute All Remaining Canonical Phases"), the canonical deferred phases of the Master V3
  **authoritative order (V3-40)** are promoted into active implementation scope, sequentially and
  respecting dependencies:
  - Phase 3 — Self-Learning (`src/learning/`)
  - Phase 4 — Research Plane (`src/research/`)
  - Phase 5 — Evolution (`src/evolution/`)
  - Phase 6 — Validation (`src/validation/`)
  - Phase 7 — Governance (`src/governance/`)
  - Phase 8 — Operating Window & Recovery (`src/operatingwindow/`)
  - Phase 9 — Desktop Control Center (`src/desktop/`)
  - Phase 10 — Auxiliary Telegram (`src/telegram/`)
  - Phase 11 — Controlled Real-World Validation (`src/realworld/`)
- **Scope:** Implement the capability sets named in V3-40 for each phase as **contract/logic layers
  in the `src/` C++ tree**, with real behavioural tests, preserving determinism, causality,
  provenance, versioning, idempotency and auditability.
- **Non-goals:** No live/real-money trading; no unattended execution; no profitability or calibrated
  probability claims; no automatic production self-modification; no GUI toolkit or network I/O
  (Phase 9/10 are deterministic offline logic with injected/fake transports); no real Telegram token;
  no promotion of a candidate without explicit human governance (Phase 7).
- **Safety constraints:** Shadow-only is preserved end-to-end; Telegram is reporting-only and never a
  live-command channel; the research/evolution layers cannot mutate runtime behaviour; validation is
  a firewall between "interesting" and "credible"; rollback is bounded and versioned.
- **Dependencies:** Each phase depends on the preceding phases per V3-40 and on the approved
  foundation/resilience/runtime/observation layers.
- **Approval basis:** The human execution authorization in this session, which explicitly directs the
  agent to execute all currently authorized canonical phases and to create the smallest accurate
  decision record needed to authorize promotion. This does not modify Master V3.
- **Scope-change flag:** Yes (promotion of deferred phases), recorded here per the
  `IMPLEMENTATION_SCOPE.md` scope-change rule.
- **Master V3 status:** Preserved unchanged. The older section-109 phase names ("Phase 3 = Timeframe
  State") remain a historical/legacy listing; V3-40 is authoritative, consistent with the earlier
  2026-10-02 reconciliation decision.

## 2026-10-02 — Phase 9 desktop represented as headless deterministic view-models

- **Date:** 2026-10-02
- **Decision:** Master V3-37 / Phase 9 (Desktop Control Center) is implemented in this environment as
  **headless, deterministic, read-only view-model projection types** over the existing registered
  components (knowledge, research, evolution, governance, validation, operating-window, observation,
  audit). No GUI toolkit, windowing system, network or filesystem I/O is introduced.
- **Rationale:** The authoritative capability set for Phase 9 (dashboard, research UI, knowledge UI,
  candidate UI, approval center, evolution graph UI, incident UI, schedule UI, audit UI) describes
  presentation surfaces. In a headless CI/Linux environment the verifiable, architecture-conforming
  deliverable is the deterministic data-projection layer the UI would consume. This preserves the
  separation between a presentation shell and the trusted logic it displays, and avoids fabricating
  behaviour (no fabricated health, no fabricated confidence).
- **Affected tasks:** Phase 9 file-level tasks (`src/desktop/`).
- **Scope-change flag:** No. Implements the Phase 9 capability set at the projection layer.
- **Master V3 status:** Preserved unchanged.

## 2026-10-02 — Phases 5–11 completion; Telegram secret-free; Phase 11 gated, no live trading

- **Date:** 2026-10-02
- **Decision:**
  1. Canonical Phases 5–7 (Evolution, Validation, Governance), Phase 8 (Operating Window & Recovery),
     Phase 9 (Desktop Control Center, headless view-models per the earlier decision), Phase 10
     (Auxiliary Telegram) and Phase 11 (Controlled Real-World Validation) are implemented at the
     statically-verified, behaviourally-tested level and marked `APPROVED`.
  2. **Telegram remains secret-free.** No bot token is embedded in source, no default token is
     invented, and the gateway is not configured until a human operator supplies the token and
     authorized user ids out-of-band. A disabled/unconfigured gateway is a safe no-op and is never a
     core dependency.
  3. **Phase 11 is a gate and registry only, not a live path.** It decides whether readiness
     preconditions are satisfied and records controlled runs immutably. It never enables unattended
     live trading, never executes automatically, and makes no profitability, calibrated-probability,
     broker-validation or production-safety claim. Real MT5 execution readiness remains UNPROVEN in
     this environment (no MetaEditor/MT5 on Linux); execution remains a separate, gated human action.
- **Rationale:** V3-38 makes Telegram auxiliary and forbids it from becoming source of truth or
  required for core safety; V3-40 Phase 11 is explicitly "only after the preceding layers are stable
  and the applicable evidence/safety gates have been satisfied". Recording these as explicit
  decisions preserves the safety model rather than silently authorising a live path.
- **Affected tasks:** `EVOL-*`, `VALID-*`, `GOV-*`, `WIN-*`, `DESK-*`, `TG-*`, `RW-*`; Phases 5–11.
- **Scope-change flag:** No. Implements the promoted scope; execution authority is unchanged.
- **Master V3 status:** Preserved unchanged.

## 2026-10-02 — Gap-audit review; Phase 12 Integration/Application promoted into scope

- **Date:** 2026-10-02
- **Decision:**
  1. The Master-to-repository gap audit (`GAP_AUDIT_2026-10-02`) is accepted as an accurate
     characterization: the repository is an advanced engineered foundation/prototype platform, not a
     finished Windows trading product. Any report claiming a finished product/penetration is treated
     as unverified external material, not repository evidence.
  2. Phase 12 (Integration / Application) is promoted into active scope: real end-to-end pipeline
     wiring, a real dependency-free transport, a real host executable, and an authoritative CMake
     build with a CTest-registered end-to-end test. These are the uncoupled, environment-independent
     parts of the audit's P0 list.
  3. The audited items that require an unavailable environment are recorded as real blockers rather
     than implemented with invented substitutes: native Windows GUI (no Windows toolchain), real
     MT5/MetaEditor run (not available on Linux), and the historical XAUUSD validation campaign (no
     licensed dataset).
  4. Live trading remains disabled; Phase 11 remains a gate/registry; no profitability, calibrated
     probability, broker-validation, or production-safety claim is made.
- **Rationale:** The audit correctly identifies INTEGRATION, not more architecture, as the need. This
  decision authorizes exactly that integration while keeping the safety model and refusing to fake
  environment-bound proof.
- **Affected tasks:** `APP-0001`..`APP-0003`, `E2E-0001`, `BUILD-0001`, and the blocked
  `GUI-0001`/`MT5-REAL-0001`/`VAL-EVID-0001`; new `PERSIST-0001`.
- **Scope-change flag:** Yes (Phase 12 promoted). Execution authority is unchanged.
- **Master V3 status:** Preserved unchanged.

## 2026-10-03 — Phase 13: durable persistence, crash recovery, packaging/CI

- **Date:** 2026-10-03
- **Decision:**
  1. `PERSIST-0001` is implemented as `FilePersistenceStore` + `ApplicationRecovery` and wired into
     `ApplicationShell`. The concrete store keeps the existing `IPersistenceStore` contract
     (PER-0003) and adds a payload-retaining record, an atomic (temp + fsync + rename) flush, and a
     whole-file SHA-256 checksum. No new persistence interface was invented.
  2. Crash recovery follows the Master V2-36/V3-21 taxonomy exactly (CLEAN_SHUTDOWN, EXPECTED_PAUSE,
     INTERRUPTED_WORK, CORRUPTED_STATE, UNKNOWN_STATE). The system REFUSES to resume corrupted,
     version-incompatible, or uncheckpointed-interruption state rather than continue on unverified
     state; a known-good fallback is used only when explicitly available.
  3. `PERSIST-0002` (`aura --self-test`) and `BUILD-0002` (CMake install rules + CI) are in scope.
     CI's Linux jobs are blocking; the MSVC Windows job is marked experimental/non-blocking because the
     Windows toolchain is not yet proven (BLOCKED.md), so a failure is visible rather than hidden.
  4. Shadow mode remains the sole execution path; no live order path is added. No profitability,
     calibration, broker-validation, or production-safety claim is made.
- **Rationale:** Persistence and recovery are required for long-running operation and are
  environment-independent; they can be implemented and proven here without inventing toolchains or
  datasets. The Master already fixes the recovery taxonomy, so this implements an existing decision.
- **Affected tasks:** `PERSIST-0001`, `PERSIST-0002`, `BUILD-0002`.
- **Scope-change flag:** Yes (Phase 13 promoted). Execution authority is unchanged.
- **Master V3 status:** Preserved unchanged.

## 2026-10-03 — Phase 9 desktop productization: real GUI over the existing runtime

- **Date:** 2026-10-03
- **Decision:**
  1. The Phase 9 Desktop Control Center gains a **real GUI application** built with **Dear ImGui +
     GLFW + OpenGL 3.3**, as a single canonical entrypoint (`tools/aura_gui.cpp`) that owns exactly
     one `ApplicationShell` (no second runtime, no duplicate process). This supersedes the earlier
     headless-view-models-only interpretation (2026-10-02 decision) as the *deliverable form* of
     Phase 9 while keeping the DESK-0001..0003 projection layer unchanged and authoritative for
     read-only data projection.
  2. The GUI is a **human control surface, not a source of truth**: it only presents read-only
     projections (`DesktopModel`) and exposes only safe control-plane operations (pause/resume of the
     local transport loop, refresh, checkpoint, bounded stop, read-only recovery report). No GUI path
     places a live order or bypasses any governance/safety contract.
  3. **Unknown stays unknown:** absent/undefined state is presented as `NOT AVAILABLE` / `UNKNOWN`,
     never fabricated as `HEALTHY`. The nine logical timeframes are shown individually by explicit
     identity, never merged into one anonymous status.
  4. **Recovery is report-only:** the GUI reports the V2-36 lifecycle decision and never auto-resumes
     corrupted or version-incompatible state merely because the window is open.
  5. The GUI is the repository's approved GUI approach. It is enabled by the CMake option
     `AURA_BUILD_GUI` (default OFF) so the core/console build and Linux test matrix remain
     dependency-free; dependencies are pinned and fetched reproducibly.
- **Rationale:** The Master V3 states the Desktop Application is the primary human control center
  (V3-37 / Phase 9) and the repository now has a proven Windows toolchain (the Windows x64 Release
  job). Building the GUI is a Phase 9 productization of already-promoted scope, not a new phase and
  not an architectural change. It uses the project's approved GUI stack rather than inventing one.
- **Affected tasks:** `GUI-0001` (BLOCKED → IMPLEMENTED), new `GUI-0002` (tests), new `GUI-0003`
  (CMake/CI/packaging); `PHASE-9-MILESTONE` (APPROVED → TESTED); `DESK-0001..0003` unchanged.
- **Scope-change flag:** No new phase. Phase 9 was already promoted on 2026-10-02; this records the
  GUI-toolkit decision and the productization of that already-authorized phase.
- **Master V3 status:** Preserved unchanged. Shadow-only upheld; live trading not enabled; no
  profitability/calibration/broker/production-safety claim.

## 2026-10-03 — GUI renderer compatibility: OpenGL 3.3 preferred, OpenGL 2.1 fallback

- **Date:** 2026-10-03
- **Decision:**
  1. The desktop control center prefers an **OpenGL 3.3 core** context but MUST NOT terminate merely
     because one is unavailable. When the driver cannot provide it (observed on Intel HD Graphics 3000 /
     driver 9.17.10.4459, where the core-profile request fails with `WGL_ARB_create_context_profile`
     unavailable, GLFW error 65543), the GUI falls back to an **OpenGL 2.1 compatibility** context driven
     by the ImGui OpenGL2 backend (GLSL 120). The legacy profile never requests a core profile.
  2. Renderer selection is a **pure, deterministic policy** (`src/desktop/RendererPolicy.h`) with no
     GLFW/GL calls, so it is unit-testable without a GPU. The window/context creation stays in
     `tools/aura_gui.cpp` and applies only the policy's hints.
  3. **No fabricated capability:** the renderer actually in use is printed at startup and shown in the
     status bar (`MODERN_GL33` or `LEGACY_GL21`); OpenGL 3.3 is reported unavailable only when that
     attempt truly failed. If no context can be created, the process exits non-zero with an actionable
     message and points at the headless console host `aura.exe`.
  4. A `--renderer auto|modern|legacy` selector pins a path for diagnostics/CI (default `auto`). This
     does not change the GUI's read-only, shadow-only behavior or the `--self-test` path.
- **Rationale:** A GUI that refuses to start on a large class of existing Windows machines is not a
  usable control surface. A compatibility fallback is the minimal, standard fix and does not alter the
  architecture or the safety posture. It is recorded here so a future AI does not "simplify" it back to
  an unconditional OpenGL 3.3 request.
- **Affected tasks:** new `GUI-0007` (fallback, TESTED), new `GUI-0008` (tests, TESTED);
  `PHASE-9-MILESTONE` evidence updated.
- **Scope-change flag:** No new phase; a compatibility refinement of the already-authorized Phase 9 GUI.
- **Master V3 status:** Preserved unchanged. Shadow-only upheld; live trading not enabled; no
  profitability/calibration/broker/production-safety claim. The specific legacy-GPU hardware remains
  UNPROVEN (`BLOCK-006`).



## Decision format

For each future material decision record:

- date
- decision
- rationale
- affected phases/tasks
- whether it changes scope or architecture
