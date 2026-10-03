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

## Decision format

For each future material decision record:

- date
- decision
- rationale
- affected phases/tasks
- whether it changes scope or architecture
