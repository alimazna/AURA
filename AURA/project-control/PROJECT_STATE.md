# AURA — Project State

This is the human-readable snapshot of where the project currently stands.

## Repository layout

- GitHub repository: `alimazna/AURA`
- The project is intentionally nested under the `AURA/` directory.
- The repository root contains only the `AURA/` project directory.
- All project-control paths are relative to the `AURA/` project root.
- Architecture reference: `docs/AURA_MASTER_UNIFIED_PROJECT_v3.0.md`
  (from the GitHub repository root: `AURA/docs/AURA_MASTER_UNIFIED_PROJECT_v3.0.md`).

## Current status

- Project: AURA
- Architecture reference: `docs/AURA_MASTER_UNIFIED_PROJECT_v3.0.md`
- Build mode: incremental
- Primary mode target: SHADOW
- Current objective: build the Master architecture incrementally, beginning with the defined MVP scope
- Live automation: deferred
- Profitability: unproven
- Probability calibration: not established
- Broker validation: required

## Repository content audit (2026-10-02)

- At audit time the GitHub HEAD was `048a95f` ("Add files via upload").
- The repository then contained only `README.md`, `docs/`, and `project-control/`.
  (This changed on 2026-10-02 with the first source file, `src/foundation/EntityId.h`.)
- No CMake build system, tests, or MQL5 adapters are present in the repository.
- The earlier "baseline repository audit" (reported tag `v1.0-fixed`, `175/175` build, `40/40` CTest)
  is NOT reproducible from the current repository content and is treated as unverified external
  material, not as evidence about this repository.
- No application build system exists; individual headers are verified by ad-hoc compile checks only.

## Bootstrap normalization (2026-10-02)

- The nested `AURA/` layout was confirmed intentional. An earlier flattening attempt was reverted
  using Git-aware operations; no files were lost and `.git` was not modified.
- Stale Master references pointing to
  `docs/XAUUSD_SOVEREIGN_MASTER_UNIFIED_PROJECT_v3.0.md` were corrected to
  `docs/AURA_MASTER_UNIFIED_PROJECT_v3.0.md` in the control plane.
- The architecture document itself was not modified.

## Task graph

- `TASK-MANIFEST-001` created the detailed file-level task graph (`manifest_version: 2`, 96 tasks).
- Phase 0 (36 tasks) uses the V3-42 canonical filenames; the `src/foundation/` layout is PROPOSED.
- Phase 0.5 (20 tasks), Phase 1 (21 tasks), and Phase 2 (4 tasks) file names are PROPOSED
  decomposition (V3-16, V3-46) and are subject to integration review.
- Phases 3–11 were later promoted into active scope and implemented (see the Phase 5–11 sections);
  deferred Master capabilities (research gaps, live trading, multi-asset expansion, real MT5 validation)
  remain visible in the manifest as `DEFERRED`.
- The two Phase 0 contracts that were blocked on human architectural decisions
  (`FND-0015 ErrorCode.h` OPEN DECISION, `FND-0016 ErrorRecord.h` BLOCKED) are now resolved and
  `APPROVED`; see the Phase 0 integration gate below.

## Phase 0 implementation (2026-10-02)

- `src/foundation/EntityId.h` was created — the first application source file in the repository.
- `FND-0001` is implemented and verified structurally; status recorded as `TESTED` in the manifest.
- The remainder of Phase 0's foundation, configuration, audit, integrity, persistence and guardian
  contract files were then created under `src/foundation/` (34 source tasks total, including FND-0001):
  - Immutable foundations: `Timestamp`, `Version`, `ServiceState`, `SystemMode`, `HashDigest`,
    `DataQualityState`, `ProtocolVersion`, `SchemaVersion`, `EventType`, `ErrorSeverity`,
    `RecoveryAction`, `EventId`, `MessageMetadata`, `EventMetadata`.
  - Configuration: `ConfigurationKey`, `ConfigurationValue`, `ConfigurationScope`,
    `ConfigurationSnapshot`, `ConfigurationChange`.
  - Audit: `AuditAction`, `AuditOutcome`, `AuditRecord`.
  - Integrity: `HashAlgorithm`, `IHasher`, `Hasher.cpp` (real FIPS 180-4 SHA-256).
  - Persistence: `PersistenceStatus`, `PersistenceRecordMetadata`, `IPersistenceStore`,
    `PersistenceTransaction`.
  - Guardian: `GuardianStatus`, `GuardianPolicy`, `IGuardian`, `Guardian`.
- All of the above were recorded as `IMPLEMENTED` at that time, pending independent review; the
  Phase 0 integration gate (below) subsequently advanced every Phase 0 file to `APPROVED`.
- Verification: clean compile under `g++ -std=c++17` and `-std=c++20` with
  `-Wall -Wextra -Werror -pedantic`; runtime structural assertions passed, including genuine
  SHA-256 known-answer tests. Details in `TEST_LOG.md`.
- No application build system exists yet; none was invented for these tasks.

## Reconciliation (2026-10-02)

- Wave 0A (V3-44) is exactly 12 tasks; all 12 files verified present, none omitted. The earlier
  "11 files" note excluded `FND-0001`, which was already implemented and tested.
- A contract inconsistency was found and corrected rather than silently frozen:
  an early `SystemMode.h` used invented values; it now uses the exact V3-14 system operating modes.
  `RecoveryAction` exposes `UNKNOWN`; `MessageMetadata` carries the V3-23 `HashDigest` checksum;
  `EventId` exposes a stable `to_string()`.
- PROPOSED implementation choices (not canonical): the `EntityId` opaque-string representation,
  the `Version` MAJOR.MINOR.PATCH grammar, and the `src/` layout. These require integration review.

## Phase 0 review / integration (2026-10-02)

- All 34 implemented Phase 0 files were reviewed against their manifest acceptance, canonical Master
  V3 sections, dependencies, namespace/include correctness, supported C++ standard, determinism,
  ownership boundaries, dependency direction, and future-phase behaviour.
- Two files required rework (unused includes) and were fixed: `GDN-0002 GuardianPolicy.h` and
  `PER-0004 PersistenceTransaction.h`.
- The include graph is acyclic, lower-layer only, with no self-includes or hidden higher-layer
  dependencies.
- Verification: all 33 headers are self-contained (66 standalone TUs, c++17/c++20, strict warnings);
  the full runtime harness passes including SHA-256 known-answer tests. Details in `TEST_LOG.md`.
- Status was recorded as `REVIEW_PENDING` for the reviewed files at that time; it was later advanced
  to `APPROVED` at the Phase 0 integration gate (see below). Compilation alone was never treated as
  proof of correctness.
- No Phase 0.5 work was started. (At that time `FND-0015` was OPEN DECISION and `FND-0016` BLOCKED;
  both were subsequently resolved — see below.)

## Phase 0 error contracts — FND-0015 / FND-0016 (2026-10-02)

- The V3-45 OPEN DECISION for `ErrorCode` was resolved by an explicit human decision recorded in
  `DECISIONS.md` (2026-10-02): a closed, strongly typed `enum class` over the 13 V3 section 113 ERROR
  categories, with explicit stable integral identities (1..13) independent of compiler ordering, no
  `UNKNOWN`/`OTHER`, and severity kept as an independent record-level field.
- `src/foundation/ErrorCode.h` (FND-0015) and `src/foundation/ErrorRecord.h` (FND-0016) were
  implemented. `ErrorRecord` carries exactly the eight V3-27 fields and is a value/contract type only.
- Verification: both headers are self-contained (70 standalone TUs across 35 headers × c++17/c++20,
  strict warnings); the full Phase 0 runtime harness passes including SHA-256 known-answer tests; the
  include graph is acyclic and lower-layer only. Details in `TEST_LOG.md`.
- Status: `FND-0015` and `FND-0016` were implemented and reviewed here; they are now `APPROVED` at
  the Phase 0 integration gate (see below). `BLOCK-002` is RESOLVED; the manifest has no `BLOCKED`
  tasks.
- No Phase 0.5 work was started.

## Phase 0 final review / integration gate (2026-10-02)

- All 36 Phase 0 file-level tasks were independently reviewed at the integration gate against their
  manifest acceptance, `DECISIONS.md`, the canonical Master V3 sections, output path, namespace,
  includes, C++17/C++20 strict self-containment, dependency direction, determinism, value semantics,
  provenance/versioning, error semantics, and absence of future-phase behaviour.
- Verification (re-run): 35 headers are self-contained (70 standalone TUs, c++17/c++20,
  `-Wall -Wextra -Werror -pedantic`); the full Phase 0 runtime harness passes including genuine
  SHA-256 known-answer tests; the include graph is acyclic and lower-layer only; 36/36 canonical
  outputs present with no duplicate/stray files; no credentials/secrets.
- One real defect was found and fixed: the manifest dependency lists under-declared the actual
  include contracts for `FND-0008`, `FND-0009`, `FND-0010`, `FND-0016`, `CFG-0003`, `CFG-0005`,
  `AUD-0003`, `PER-0003`, `PER-0004`, `GDN-0003`. The lists were reconciled to match the verified
  includes; no source file was changed and the include graph was already correct.
- Outcome: all 36 Phase 0 file-level tasks are `APPROVED`; the `PHASE-0` milestone is `APPROVED`.
  Phase 0 is complete. No task is BLOCKED. `BLOCK-002` remains RESOLVED.
- The next READY task is `RS-0001` (Phase 0.5) but it was NOT started; Phase 0.5 awaits explicit
  authorization.

## Phase 0.5 implementation — RS-0001 (2026-10-02)

- Phase 0.5 (Resilience & Graceful Degradation) was explicitly authorized. Exactly one task was
  executed: `RS-0001` → `src/resilience/ServiceDescriptor.h`.
- `RS-0001` defines an immutable `ServiceDescriptor` value type in `aura::resilience`: a stable
  non-empty service name (empty name yields the invalid default) plus a `foundation::ServiceState`
  declared default state. It is a descriptive contract only — no supervision, monitoring,
  transition, recovery, isolation, scheduling, or I/O.
- It depends only on `FND-0004 ServiceState.h`; it deliberately does not carry an `EntityId` (RS-0002
  owns capability identity) and introduces no higher-layer dependency.
- Verification: self-contained standalone TU and structural harness pass under `g++ -std=c++17` and
  `g++ -std=c++20` with `-Wall -Wextra -Werror -pedantic`; deterministic equality/ordering/hashing,
  value semantics, all 8 canonical `ServiceState` values representable, and combined all-headers
  compile verified. Details in `TEST_LOG.md`.
- Review classification: `RS-0001` = `APPROVED`.
- `PHASE-0.5` remains `PLANNED` (1 of 20 Phase 0.5 tasks complete). `RS-0002` is the next READY task
  but was NOT started.

## Phase 0.5 completion (2026-10-02)

- Phase 0.5 (Resilience & Graceful Degradation) is COMPLETE. All 20 file-level tasks
  (`RS-0001`..`RS-0020`) are `APPROVED`; the `PHASE-0.5` milestone is `APPROVED`.
- New source under `src/resilience/`: capability identity and descriptors (`CapabilityId`,
  `CapabilityDescriptor`, `DependencyDescriptor`), health/freshness (`HealthSnapshot`,
  `FreshnessState`, `DataFreshnessMonitor`), the explicit capability dependency graph
  (`CapabilityRegistry`, `DependencyGraph`), impact/degradation (`DegradationImpact`,
  `CriticalityPolicy`, `ImpactResolver`, `SubsystemIsolationManager`, `GracefulDegradationManager`),
  recovery/pause (`RecoveryManager`, `PauseResumeManager`), coordination (`HealthStateEngine`,
  `SystemSupervisor`), and the phase test artifact `DegradationTests.cpp`.
- Verification: 19 resilience headers self-contained and combined TU compiles under
  `g++ -std=c++17` and `-std=c++20` with `-Wall -Wextra -Werror -pedantic`; `DegradationTests.cpp`
  passes under both standards and proves non-critical failure isolation, exact stale-timeframe
  reporting, deterministic health/impact/recovery, and cycle-safe traversal. Details in `TEST_LOG.md`.
- The capability dependency graph is encoded explicitly (V3-15) rather than implied. No runtime
  behaviour, no live-trading enablement, no future-phase leakage.

## MT5 deployment decision (2026-10-02)

- Recorded in `DECISIONS.md`: one physical MT5 EA, one chart, one transport, carrying nine logically
  independent timeframe streams (M1, M5, M15, M30, H1, H4, D1, W1, MN1). The nine logical adapter
  responsibilities and the V3-29 authority hierarchy are unchanged. The task graph already used a
  single adapter boundary (`RT-0001 AdapterManager.h`) and enumerated no nine physical `.mq5` files,
  so no task replacement was needed; the `RT-0001` and `RT-0021` contracts were refined to require
  explicit per-stream timeframe identity and independent stream state. Master V3 is unmodified.

## Phase 1 completion (2026-10-02)

- Phase 1 (Deterministic Runtime) is COMPLETE. All 21 file-level tasks (`RT-0001`..`RT-0021`) and the
  `PHASE-1` milestone are `APPROVED`.
- New source under `src/runtime/`: `AdapterManager` (nine logical MT5 streams with explicit timeframe
  identity, one EA / one transport), `DataBus`, `DataValidator`, `BarFinalizer`, `TimeframeStateStore`,
  `FeatureEngine`, `StructureEngine`, `RegimeEngine`, `EligibilityEngine`, `SignalEngine`, `ScoreEngine`,
  `ConfidenceEngine`, `MacroContextEngine`, `MarketQualityEngine`, `RiskEngine`, `ShadowExecutionEngine`,
  `PositionSimulator`, `ReconciliationEngine`, `ShadowLedger`, `ReplayEngine`, plus the contract document
  `Mt5Boundary.md` and the test artifact `RuntimeTests.cpp`.
- Determinism, no-lookahead and no-repaint are enforced: future-dated events and forming/non-advancing
  bars are rejected; decision/signal identity follows the V3-23 formula; replay is reproducible.
- Shadow-only: `RiskProposal::is_order`, `SimulatedFill::is_live` and `SimulatedPosition::is_live` are
  always false. No live order path exists in this phase.
- Verification: 20 runtime headers self-contained and combined all-headers TU (74 headers across
  foundation/resilience/runtime) compile under `g++ -std=c++17` and `-std=c++20` with
  `-Wall -Wextra -Werror -pedantic`; `RuntimeTests.cpp` passes under both standards. Details in `TEST_LOG.md`.

## Phase 2 completion (2026-10-02)

- Phase 2 (Observation & Outcomes) is COMPLETE. All 4 file-level tasks (`OB-0001`..`OB-0004`) and the
  `PHASE-2` milestone are `APPROVED`.
- New source under `src/observation/`: `PredictionLedger` (append-only, deterministic identity,
  idempotent), `OutcomeEngine` (deterministic resolution, no lookahead, no repaint, conservative on
  ambiguity), `FailureDetectionEngine` (structured `foundation::ErrorRecord` objects, never plain
  strings), `SystemHealthMonitor` (deterministic worst-state aggregation, no fabricated healthy state),
  plus the test artifact `ObservationTests.cpp`.
- Verification: 4 observation headers self-contained and combined all-headers TU (78 headers across
  foundation/resilience/runtime/observation) compile under `g++ -std=c++17` and `-std=c++20` with
  `-Wall -Wextra -Werror -pedantic`; `ObservationTests.cpp` passes under both standards. Details in
  `TEST_LOG.md`.

## Conceptual Phase 3/4/5 and MT5 one-EA/nine-stream (2026-10-02)

- The continuation request's conceptual "Phase 3 = Timeframe State, Phase 4 = Feature/Structure/
  Regime, Phase 5 = Signal" was reconciled with the canonical Master phase order (V3-40) and mapped
  onto the already-approved manifest Phase 1. The mapping and rationale are recorded in
  `DECISIONS.md` and `PHASE_RECONCILIATION.md`; the Master was not modified and no deferred manifest
  phase was promoted.
- Conceptual Phase 3 (timeframe state) verification: `src/runtime/TimeframeStateTests.cpp`
  (`PH3-0001`) — nine explicit streams, per-stream independence, closed-bar-only processing,
  duplicate/repaint rejection, no-lookahead, provenance. `APPROVED`.
- Conceptual Phase 4 (feature/structure/regime) verification:
  `src/runtime/AnalysisPipelineTests.cpp` (`PH4-0001`) — deterministic bounded features, no future
  input, H4 structural authority, deterministic regime, degraded-input gating, macro context from
  D1/W1/MN1 only. `APPROVED`.
- Conceptual Phase 5 (signal) verification: `src/runtime/SignalPipelineTests.cpp` (`PH5-0001`) —
  deterministic V3-23 identity, eligibility gating, M15-under-H4 authority, score/confidence as
  ranking values, risk gate, shadow-only execution with a live-execution-blocked negative test,
  append-only ledger provenance. `APPROVED`.
- One additive change to the owning Phase 1 file `ShadowExecutionEngine.h` (`RT-0016`): an explicit
  `request_live_execution()` guard returning `SHADOW_ONLY`. No live path added.
- MT5 one-physical-EA / nine-logical-stream boundary implemented under `src/mt5/`:
  MQL5 (`Common`, `Protocol`, `SocketClient`, `TimeframeStream`, `StreamManager`,
  `ShadowOrderGateway`, `AURA_MT5_EA`) and C++ (`ProtocolCodec`, `Mt5StreamManager`,
  `Mt5ReceiverTests`). Physical EA count = 1; logical stream count = 9; transport count = 1;
  explicit per-stream timeframe identity; shadow-only.
- Verification: 80 headers self-contained + combined TU under c++17/c++20 strict; all seven
  behavioural test artifacts PASS under both standards; MQL5 static audit PASS; repository static
  audit clean. MetaEditor/MT5 NOT available — real terminal connectivity is UNPROVEN. Details in
  `TEST_LOG.md` and `FINAL_VERIFICATION_REPORT.md`.

## Canonical Phase 3/4 — Self-Learning and Research Plane (2026-10-02)

- Canonical Phases 3–11 were promoted into active scope by an explicit `DECISIONS.md` entry ("Session
  authorization: promote canonical Phases 3–11"), per the `IMPLEMENTATION_SCOPE.md` scope-change rule.
  Master V3-40 is authoritative; the older section-109 phase names are legacy.
- Phase 3 (Self-Learning) is COMPLETE and `APPROVED` (`src/learning/`): `KnowledgeObject` (scoped,
  versioned, point-in-time), `KnowledgeStore` (append-only, versioned, no silent overwrite),
  `KnowledgeLifecycle` (explicit transitions only), `ContextLearning` (excludes future-outcome
  observations), `ContradictionEngine` (contradictions preserved, never resolved by overwrite),
  `KnowledgeDecay` (aged knowledge never erased), `FailureMemory` (structured error memory), and
  `LearningTests.cpp`.
- Phase 4 (Research Plane) is COMPLETE and `APPROVED` (`src/research/`): `Hypothesis` (falsifiability
  mandatory), `Experiment`, `ExperimentFingerprint` (deterministic SHA-256 over stable fields),
  `ResearchBudget` (explicit budget states, no overspend), `ExperimentLedger` (append-only, duplicate
  detection), `ResearchPlanner` (protocol fixed before evaluation), `ResearchSandbox` (never mutates
  runtime), and `ResearchTests.cpp`.
- Verification: all Phase 3/4 headers self-contained + combined TU under c++17/c++20 strict;
  `LearningTests.cpp` and `ResearchTests.cpp` PASS under both standards.
- No live path, no profitability/calibration claim, and research cannot mutate runtime behaviour.

## Canonical Phases 5–11 — Evolution through Controlled Real-World Validation (2026-10-02)

- Phase 5 (Evolution) COMPLETE and `APPROVED`: `src/evolution/` — `Candidate.h`,
  `CandidateRegistry.h`, `EvolutionGraph.h`, `CandidateComparison.h`, `EvolutionTests.cpp`
  (`EVOL-0001`..`EVOL-0005`). Explicit candidate lifecycle, no auto-promotion, acyclic graph,
  constraints treated as vetoes.
- Phase 6 (Validation) COMPLETE and `APPROVED`: `src/validation/` — `ValidationFirewall.h`,
  `EvidenceFirewall.h`, `EvaluatorFirewall.h`, `RewardHackingDefense.h`, `StatisticalControls.h`,
  `ValidationTests.cpp` (`VALID-0001`..`VALID-0006`). Missing check is NOT_RUN not PASS; monotone
  contamination; evaluator firewall; holdout budget; no profitability claim.
- Phase 7 (Governance) COMPLETE and `APPROVED`: `src/governance/` — `PolicyEngine.h`,
  `ForbiddenBehavior.h`, `PromotionGate.h`, `HumanDecision.h`, `AuditLedger.h`,
  `GovernanceTests.cpp` (`GOV-0001`..`GOV-0006`). Policy-by-change-type; hard-forbidden boundary;
  promotion requires every gate incl. human approval and shadow state; append-only audit; no
  self-promotion.
- Phase 8 (Operating Window & Recovery) COMPLETE and `APPROVED`: `src/operatingwindow/` —
  `OperatingWindow.h`, `Checkpoint.h`, `ResourceBudget.h`, `WindowOrchestrator.h`,
  `OperatingWindowTests.cpp` (`WIN-0001`..`WIN-0005`). Human-bounded 3–8h window with no
  self-extension; recovery prefers valid checkpoint/known-good and never guesses; controlled drain.
- Phase 9 (Desktop Control Center) COMPLETE and `APPROVED` as headless deterministic view-models:
  `src/desktop/` — `ControlCenterViewModels.h`, `DashboardProjector.h`, `DesktopTests.cpp`
  (`DESK-0001`..`DESK-0003`). No GUI toolkit; read-only projections; unknown never fabricated.
- Phase 10 (Auxiliary Telegram) COMPLETE and `APPROVED`, secret-free: `src/telegram/` —
  `TelegramConfig.h`, `TelegramMessage.h`, `ApprovalRequest.h`, `TelegramGateway.h`,
  `NotificationPolicy.h`, `TelegramTests.cpp` (`TG-0001`..`TG-0006`). No embedded token; disabled
  gateway is a safe no-op; bounded approval pipeline; failure notifications never suppressed.
- Phase 11 (Controlled Real-World Validation) COMPLETE and `APPROVED` as a gate/registry, NOT a
  live path: `src/realworld/` — `RealWorldValidation.h`, `ValidationRegistry.h`,
  `RealWorldTests.cpp` (`RW-0001`..`RW-0003`). All readiness gates required; never automatic; no
  profitability/safety claim; execution remains a separate human action.

- Verification: every new header self-contained + combined TU under c++17/c++20 strict; all 16 test
  suites PASS under both standards (`LearningTests`, `ResearchTests`, `EvolutionTests`,
  `ValidationTests`, `GovernanceTests`, `OperatingWindowTests`, `DesktopTests`, `TelegramTests`,
  `RealWorldTests`, plus the pre-existing suites).
- Manifest now 172 tasks; Phases 0–11 are all decomposed into file-level tasks with acceptance
  criteria and evidence.

## Current task

- All canonical Phases 0–11 are COMPLETE and `APPROVED` at the statically-verified, behaviourally
  tested level. Phase 11 is deliberately gated: real MT5 execution readiness is UNPROVEN in this
  environment and no unattended live trading is enabled.
- Real MT5 terminal validation remains the final gated step and is environment-limited (no
  MetaEditor/MT5 on Linux).

## Next action

1. When a MetaEditor/MT5 environment becomes available, run Phase 11 controlled validation
   (demo/shadow only) behind the readiness gate; record real results in `TEST_LOG.md`.
2. Keep the control plane and manifest updated per wave; preserve state in Git.
3. Do not enable unattended live trading or make profitability/safety claims without evidence.

## Update rule

Update this file after each meaningful implementation session.
