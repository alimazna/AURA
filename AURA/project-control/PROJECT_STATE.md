# AURA  Project State

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
- Phases 311 were later promoted into active scope and implemented (see the Phase 511 sections);
  deferred Master capabilities (research gaps, live trading, multi-asset expansion, real MT5 validation)
  remain visible in the manifest as `DEFERRED`.
- The two Phase 0 contracts that were blocked on human architectural decisions
  (`FND-0015 ErrorCode.h` OPEN DECISION, `FND-0016 ErrorRecord.h` BLOCKED) are now resolved and
  `APPROVED`; see the Phase 0 integration gate below.

## Phase 0 implementation (2026-10-02)

- `src/foundation/EntityId.h` was created  the first application source file in the repository.
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
  both were subsequently resolved  see below.)

## Phase 0 error contracts  FND-0015 / FND-0016 (2026-10-02)

- The V3-45 OPEN DECISION for `ErrorCode` was resolved by an explicit human decision recorded in
  `DECISIONS.md` (2026-10-02): a closed, strongly typed `enum class` over the 13 V3 section 113 ERROR
  categories, with explicit stable integral identities (1..13) independent of compiler ordering, no
  `UNKNOWN`/`OTHER`, and severity kept as an independent record-level field.
- `src/foundation/ErrorCode.h` (FND-0015) and `src/foundation/ErrorRecord.h` (FND-0016) were
  implemented. `ErrorRecord` carries exactly the eight V3-27 fields and is a value/contract type only.
- Verification: both headers are self-contained (70 standalone TUs across 35 headers  c++17/c++20,
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

## Phase 0.5 implementation  RS-0001 (2026-10-02)

- Phase 0.5 (Resilience & Graceful Degradation) was explicitly authorized. Exactly one task was
  executed: `RS-0001`  `src/resilience/ServiceDescriptor.h`.
- `RS-0001` defines an immutable `ServiceDescriptor` value type in `aura::resilience`: a stable
  non-empty service name (empty name yields the invalid default) plus a `foundation::ServiceState`
  declared default state. It is a descriptive contract only  no supervision, monitoring,
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
  (`PH3-0001`)  nine explicit streams, per-stream independence, closed-bar-only processing,
  duplicate/repaint rejection, no-lookahead, provenance. `APPROVED`.
- Conceptual Phase 4 (feature/structure/regime) verification:
  `src/runtime/AnalysisPipelineTests.cpp` (`PH4-0001`)  deterministic bounded features, no future
  input, H4 structural authority, deterministic regime, degraded-input gating, macro context from
  D1/W1/MN1 only. `APPROVED`.
- Conceptual Phase 5 (signal) verification: `src/runtime/SignalPipelineTests.cpp` (`PH5-0001`) 
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
  audit clean. MetaEditor/MT5 NOT available  real terminal connectivity is UNPROVEN. Details in
  `TEST_LOG.md` and `FINAL_VERIFICATION_REPORT.md`.

## Canonical Phase 3/4  Self-Learning and Research Plane (2026-10-02)

- Canonical Phases 311 were promoted into active scope by an explicit `DECISIONS.md` entry ("Session
  authorization: promote canonical Phases 311"), per the `IMPLEMENTATION_SCOPE.md` scope-change rule.
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

## Canonical Phases 511  Evolution through Controlled Real-World Validation (2026-10-02)

- Phase 5 (Evolution) COMPLETE and `APPROVED`: `src/evolution/`  `Candidate.h`,
  `CandidateRegistry.h`, `EvolutionGraph.h`, `CandidateComparison.h`, `EvolutionTests.cpp`
  (`EVOL-0001`..`EVOL-0005`). Explicit candidate lifecycle, no auto-promotion, acyclic graph,
  constraints treated as vetoes.
- Phase 6 (Validation) COMPLETE and `APPROVED`: `src/validation/`  `ValidationFirewall.h`,
  `EvidenceFirewall.h`, `EvaluatorFirewall.h`, `RewardHackingDefense.h`, `StatisticalControls.h`,
  `ValidationTests.cpp` (`VALID-0001`..`VALID-0006`). Missing check is NOT_RUN not PASS; monotone
  contamination; evaluator firewall; holdout budget; no profitability claim.
- Phase 7 (Governance) COMPLETE and `APPROVED`: `src/governance/`  `PolicyEngine.h`,
  `ForbiddenBehavior.h`, `PromotionGate.h`, `HumanDecision.h`, `AuditLedger.h`,
  `GovernanceTests.cpp` (`GOV-0001`..`GOV-0006`). Policy-by-change-type; hard-forbidden boundary;
  promotion requires every gate incl. human approval and shadow state; append-only audit; no
  self-promotion.
- Phase 8 (Operating Window & Recovery) COMPLETE and `APPROVED`: `src/operatingwindow/` 
  `OperatingWindow.h`, `Checkpoint.h`, `ResourceBudget.h`, `WindowOrchestrator.h`,
  `OperatingWindowTests.cpp` (`WIN-0001`..`WIN-0005`). Human-bounded 38h window with no
  self-extension; recovery prefers valid checkpoint/known-good and never guesses; controlled drain.
- Phase 9 (Desktop Control Center) COMPLETE and `APPROVED` as headless deterministic view-models:
  `src/desktop/`  `ControlCenterViewModels.h`, `DashboardProjector.h`, `DesktopTests.cpp`
  (`DESK-0001`..`DESK-0003`). No GUI toolkit; read-only projections; unknown never fabricated.
- Phase 10 (Auxiliary Telegram) COMPLETE and `APPROVED`, secret-free: `src/telegram/` 
  `TelegramConfig.h`, `TelegramMessage.h`, `ApprovalRequest.h`, `TelegramGateway.h`,
  `NotificationPolicy.h`, `TelegramTests.cpp` (`TG-0001`..`TG-0006`). No embedded token; disabled
  gateway is a safe no-op; bounded approval pipeline; failure notifications never suppressed.
- Phase 11 (Controlled Real-World Validation) COMPLETE and `APPROVED` as a gate/registry, NOT a
  live path: `src/realworld/`  `RealWorldValidation.h`, `ValidationRegistry.h`,
  `RealWorldTests.cpp` (`RW-0001`..`RW-0003`). All readiness gates required; never automatic; no
  profitability/safety claim; execution remains a separate human action.

- Verification: every new header self-contained + combined TU under c++17/c++20 strict; all 16 test
  suites PASS under both standards (`LearningTests`, `ResearchTests`, `EvolutionTests`,
  `ValidationTests`, `GovernanceTests`, `OperatingWindowTests`, `DesktopTests`, `TelegramTests`,
  `RealWorldTests`, plus the pre-existing suites).
- Manifest now 172 tasks; Phases 0-11 are all decomposed into file-level tasks with acceptance
  criteria and evidence.

## Gap audit and Phase 12 integration (2026-10-02)

- A comprehensive Master-to-repository gap audit (`GAP_AUDIT_2026-10-02`) was reviewed against
  the actual repository. Its central finding is accurate and accepted: AURA has a large,
  tested architectural/contract/gate foundation plus a real one-EA MT5 source and a real shadow
  decision pipeline, but did NOT yet have an integrated application runtime, a build system, or a
  real end-to-end path. The CLAIMED "Penetration 1" report that this repository is a finished
  Windows trading product is NOT supported by repository evidence.
- Phase 12 (Integration / Application) was promoted into active scope and implemented:
  - `src/runtime/ApplicationPipeline.h` (APP-0001) wires one real MT5 wire frame through the whole
    approved runtime: codec -> nine-stream receiver -> adapter boundary -> H4 structure/regime ->
    M15 features -> eligibility -> signal -> score/confidence/market-quality -> risk -> shadow fill
    -> simulated position -> append-only shadow ledger -> health. No live path; no fabrication.
  - `src/runtime/ApplicationShell.h` (APP-0002) provides a real, dependency-free socket transport
    (POSIX on Linux, Winsock on Windows) with an interruptible accept/read loop and newline frame
    reassembly, plus a host-driven application loop.
  - `tools/run_pipeline.cpp` (APP-0003) is a real application entrypoint (`aura --replay`,
    `aura --serve`), deliberately shadow-only and console-hosted.
  - `src/runtime/EndToEndTests.cpp` (E2E-0001) is one reproducible end-to-end integration test over
    a real loopback socket covering all nine streams, rejection paths, isolation and determinism.
  - `CMakeLists.txt` (BUILD-0001) is the authoritative build: a static library, all test targets, the
    `aura` executable, and CTest registration.
- Verification: CMake configure/build succeeds; `ctest` runs 17/17 suites green; all 17 suites also
  pass under `g++ -std=c++17` and `-std=c++20` strict. The `aura` executable replayed 360 real frames
  (9/9 streams healthy, 360 accepted, 0 rejected, 38 signals/proposals/fills, 151 ledger entries,
  HEALTHY) both offline and over a real socket via `--serve`.
- Still deliberately NOT claimed: profitability, calibrated probability, broker validation,
  production safety, or live trading. Phase 11 remains a gate/registry only.

## Durable persistence and crash recovery (2026-10-03)

- PERSIST-0001 was the active unblocked task and is now implemented, verified and recorded `TESTED`:
  - `src/foundation/FilePersistenceStore.h` is the concrete file-backed `IPersistenceStore`
    (PER-0003). It is append-only, idempotent on record identity (a conflicting payload is a
    `CONFLICT`), and crash-safe: `flush()` writes a temp file, fsyncs, and atomically renames. The
    whole file carries a SHA-256 checksum verified on load, so a torn or tampered file is reported
    `CORRUPT` and a truncated/decodable-error file is `UNAVAILABLE`  never silently accepted. Each
    record stores its payload digest and (for restoration) the payload text; a payload containing a
    tab/newline is refused.
  - `src/runtime/ApplicationRecovery.h` implements the V2-36/V3-21 crash-recovery path: it persists
    per-timeframe progress plus the append-only shadow ledger, and classifies the prior lifecycle as
    one of CLEAN_SHUTDOWN, EXPECTED_PAUSE, INTERRUPTED_WORK, CORRUPTED_STATE or UNKNOWN_STATE. The
    boot decision verifies schema/strategy/configuration identity and REFUSES to resume state that is
    corrupted, version-incompatible, or an uncheckpointed interruption; it returns a known-good
    fallback only when one is available. An incomplete manifest is INTERRUPTED_WORK, never
    CLEAN_SHUTDOWN.
  - `src/runtime/ApplicationPipeline.h` / `ApplicationShell.h` wire persistence in
    (`persist_state`, `persist_pause`, `recover`, `apply_resume`); `Mt5StreamManager::restore_progress`
    repopulates per-stream state from a verified store without reprocessing history.
    `tools/run_pipeline.cpp` gains `--store`, `--self-test` and `--recover`.
- Verification: `src/runtime/PersistenceTests.cpp` (PERSIST-0001) PASSES  idempotency, atomic
  round-trip, byte-determinism, tamper + truncation -> CORRUPT, every lifecycle decision,
  incompatible/corrupted -> refused, and an application restart restoring 9 timeframe states + the
  ledger then accepting strictly-newer bars (no repaint). CTest is now 18/18 green under C++17 and
  C++20. Real `aura --serve` received 360 socket frames -> 9/9 streams, 360 accepted, 38
  signals/proposals/fills, 151 ledger entries, HEALTHY, persisting 161 records; `aura --recover`
  reports CLEAN_SHUTDOWN/resumable for the good store and CORRUPTED_STATE/refused (exit 1) for a
  tampered store. `aura --self-test` PASSES.
- BUILD-0002 adds an installable package (CMake install rules for the host + headers + docs) and a CI
  workflow building/testing on Linux under C++17 and C++20 with the smoke test, plus an MSVC Windows
  job (non-blocking by policy, but currently passing). Remote CI on PR #1 is ALL GREEN across four jobs
  (Linux gcc C++17/C++20 and Windows MSVC C++17/C++20). CI found and we fixed two real portability
  defects: missing `ws2_32` (Winsock) linkage on Windows, and `std::rename` not overwriting on Windows.
- BUILD-0003 produces the canonical runnable Windows x64 Release package. CI job
  `windows-x64-release-package` (run 37115746002, GREEN) builds a self-contained `aura.exe` (Release,
  x64, MSVC, static CRT; 440,320 bytes), runs the full 18/18 suite, and passes a bounded runtime smoke
  on the real Windows runner: `--self-test` PASS; cross-process `--recover` => CLEAN_SHUTDOWN/resumable;
  `--dump-frames` + `--replay` (270 frames) => HEALTHY, 111 ledger entries; `--serve` starts, stays up,
  is a single process (no duplicate copies), accepts a TCP connection and stops cleanly. A secret scan
  is clean. `AURA_Windows_x64_Release.zip` (220,103 bytes) is created and verified to contain `aura.exe`;
  the packaged exe is extracted and re-runs `--self-test` PASS. Both the exe and the ZIP are uploaded as
  Actions artifacts (`aura-windows-x64-exe`, `AURA_Windows_x64_Release`). The package needs no DLLs, no
  data files and no config; it reads no developer absolute paths (only the user-supplied `--store` path).
- Still deliberately NOT claimed: profitability, calibrated probability, broker validation,
  production safety, or live trading. All of the above is shadow-only. Real-Windows-desktop interactive
  GUI rendering is UNPROVEN (the Xvfb smoke is software-rendered on Linux); MT5/MetaEditor is UNPROVEN.
  Phase 9 now also ships a real GUI (`aura_gui`, see the Current task section).

## Renderer compatibility fallback for legacy GPUs (2026-10-03)

A real Windows 10 machine with **Intel HD Graphics 3000** (driver `9.17.10.4459`) exposed a real
compatibility gap: `aura_gui.exe --self-test` passed, but `aura_gui.exe --gui` failed with
`GLFW error 65543: WGL: OpenGL profile requested but WGL_ARB_create_context_profile is unavailable`,
because the previous build requested an OpenGL 3.3 core-profile context unconditionally and exited when
it could not be created.

- `GUI-0007` (renderer fallback, `TESTED`): added a pure, deterministic selection policy
  (`src/desktop/RendererPolicy.h`: `hints_for`, `RendererSelector`, `RendererChoice`,
  `renderer_diagnostic`, `no_renderer_error`; no GLFW/GL, unit-testable). `tools/aura_gui.cpp` now sets
  GLFW hints only from that policy, creates the **OpenGL 3.3 core** context first and, on failure, the
  **OpenGL 2.1 compatibility** context, driving `ImGui_ImplOpenGL2` (legacy) or `ImGui_ImplOpenGL3`
  (modern). The startup line and the status bar name the renderer actually in use (`MODERN_GL33` /
  `LEGACY_GL21`); the legacy profile never requests a core profile and uses GLSL version 120. If no
  context can be created at all, the program exits non-zero with an actionable message naming the
  attempts and pointing at the headless console host `aura.exe`. A `--renderer auto|modern|legacy`
  selector allows pinning a path for diagnostics and CI. `CMakeLists.txt` now compiles
  `backends/imgui_impl_opengl2.cpp` into `aura_imgui`.
- `GUI-0008` (fallback tests, `TESTED`): added five deterministic unit tests
  (`test_renderer_policy_hints`, `test_renderer_selector_modern_success`,
  `test_renderer_selector_legacy_fallback`, `test_renderer_selector_both_fail`,
  `test_renderer_selector_pinned`) over real code paths (no mocks, no GL/GLFW).
- The GUI remains **read-only / shadow-only**; no live-order path was added. `--self-test` is unchanged.
- **Unproven:** the specific Intel HD 3000 hardware could not be exercised here (no such GPU in CI; the
  Linux Xvfb runner uses software Mesa). The fallback code path is proven by forcing the legacy renderer
  and by capping Mesa at GL 2.1 to force the AUTO fallback, but that is not the same as a verified
  legacy-GPU render. See `BLOCK-006`.

## Explicit blockers (2026-10-03)

- GUI-0001 (real desktop control center) — RESOLVED 2026-10-03: a Dear ImGui + GLFW + OpenGL GUI now
  builds and runs (`aura_gui`), with a headless integration smoke and a bounded Xvfb interactive smoke.
  One remainder is UNPROVEN: interactive rendering on a real Windows desktop has not been human-verified.
- BLOCK-006 (legacy-GPU GUI compatibility) — BLOCKED on hardware-specific verification only: the
  OpenGL 3.3 -> 2.1 fallback is implemented and `TESTED` on software GL, but the Intel HD Graphics 3000
  path is UNPROVEN (no such GPU in CI). Needs a re-run of the new `aura_gui.exe` on that machine.
- MT5-REAL-0001 (MetaEditor compile + live terminal run) — BLOCKED (no MetaEditor/MT5 on Linux).
- VAL-EVID-0001 (historical XAUUSD validation campaign) — BLOCKED on a licensed dataset.
See `project-control/BLOCKED.md`.

## Current task

- All canonical Phases 0-11 are COMPLETE and `APPROVED`. Phase 12 (Integration/Application), Phase 13
  (durable persistence + crash recovery + packaging/CI) and the Windows x64 Release package are
  implemented and `TESTED`.
- Phase 9 Desktop Control Center now includes a **real GUI application** (GUI-0001, Dear ImGui + GLFW +
  OpenGL 3.3; single entrypoint `tools/aura_gui.cpp`): read-only projections (`DesktopModel.h`), a
  single runtime owner (`ControlCenterState.h`), section rendering (`GuiPanels.h`), safe control-plane
  actions only, and **no live-order path**. It displays the nine timeframes by explicit identity and
  shows unknown/absent state as `NOT AVAILABLE`. Reports the V2-36 recovery decision but never
  auto-resumes corrupted/incompatible state. Build with `-DAURA_BUILD_GUI=ON`; executable `aura_gui`.
- Phase 9 integration wave (GUI-0004..GUI-0006, `IMPLEMENTED` 2026-10-03): the remaining V3-37
  control-center sections are now projected read-only (`src/desktop/ControlCenterPanels.h`) and rendered
  (`GuiPanels.h`), combined into a single `ControlCenterReport` (`ControlCenterState.h`). Sections bound
  to a real source are live: **configuration/version context** (from the pipeline config) and the
  **shadow ledger** (from the append-only runtime ledger); **incidents** are detected from the real
  adapter stream health via the existing Phase 2 engine (so an empty list is a genuine "no incidents").
  Sections with no source wired yet — prediction/observation, knowledge, research, candidates,
  validation evidence, approvals, evolution graph, schedule/operating window, operating-window
  checkpoints — are shown explicitly as **`NOT AVAILABLE`**, never fabricated. The GUI exposes no
  live-order path; the shadow-only invariant is asserted in tests and self-test.
- GUI visual redesign (GUI-0009..GUI-0013, `TESTED` 2026-10-03): the control center was visually poor
  and prototype-like, so the presentation layer was rebuilt on an original design system. New
  `src/desktop/StateVisuals.h` (pure, ImGui-free state classification with the invariant that
  `UNKNOWN`/`NOT AVAILABLE` never map to healthy) and `src/desktop/AuraTheme.h` (deep-navy palette, one
  cyan accent, state→colour map, spacing scale, `apply_aura_style()`; flat fixed-function-friendly
  properties so the same theme renders on both backends). New `src/desktop/NavigationModel.h` groups the
  19 canonical V3-37 sections into Monitoring / Intelligence / Governance / System with readable labels
  while preserving section identity and order exactly. New `src/desktop/AuraWidgets.h` (bordered cards,
  state badges, aligned key/value rows, metric blocks, explicit empty states, uniform tables) and a
  rewritten `src/desktop/GuiPanels.h` (a Dashboard plus all 19 sections). `tools/aura_gui.cpp` now has a
  coherent shell: top bar (brand, version, `SHADOW ONLY`, health, actual renderer), grouped sidebar,
  bordered content area, and a persistent status bar, with safe control-plane shortcuts only
  (`Ctrl+P` pause/resume, `Ctrl+S` checkpoint, `Esc` bounded stop) and no order path. `DesktopModel.h`
  now exposes the real processed closed-bar `sequence` per stream and a deterministic relative
  `freshness` (FRESH / LAGGING / UNKNOWN / NOT AVAILABLE) computed from the newest close time in the
  snapshot — no wall clock, no lookahead. Absent values remain explicit `NOT AVAILABLE`; a wired-but-empty
  source is a distinct real empty state.
- XAUUSD candlestick chart + timeframe selector (GUI-0015..GUI-0020, `TESTED` 2026-10-03): the Dashboard
  now leads with a real XAUUSD candlestick chart and an above-chart horizontal timeframe selector
  `[M1][M5][M15][M30][H1][H4][D1][W1][MN1]` with **M15 active by default**. The chart is a pure,
  ImGui-free model (`src/desktop/CandleChart.h`: `make_candle_series`, `build_chart_geometry`,
  `TimeframeSelection`, `format_utc_minute`) projected from the closed bars the runtime actually retained
  — only bars whose **explicit timeframe matches**, whose `closed` flag is set, and whose OHLC are finite
  are drawn; non-closed/foreign/non-finite bars are dropped, never fabricated. A stream with no real bars
  renders an explicit **NO CANDLE DATA / STATUS: NOT AVAILABLE / TIMEFRAME** state, never placeholder
  candles. The selection holds an explicit `Timeframe` (never a row index) and refuses `UNKNOWN`. The
  widget (`src/desktop/CandleChartWidget.h`) draws wicks+bodies, a price scale, a time axis and a grid
  using only fixed-function draw-list primitives, so it renders identically on `MODERN_GL33` and
  `LEGACY_GL21`; the active timeframe is drawn with the AURA accent. The read-only projection uses a new
  const `ApplicationPipeline::bar_series(Timeframe)` accessor (`GUI-0015`); the report carries the
  selected `CandleSeries` (`GUI-0017`). No order path, no lookahead, no repaint, shadow-only.
- Premium GUI overhaul (GUI-0021..GUI-0024, `TESTED` 2026-10-03): the control center was transformed
  from a card-heavy prototype into a premium, chart-first financial terminal, preserving the AURA
  architecture (single runtime owner, read-only projections), the deep-navy/cyan identity and the
  shadow-only invariant. `apply_aura_style()` was refined (rounder panels, hairline child borders,
  no frame borders, more generous `WindowPadding`/`CellPadding`, tighter scrollbars) and the type scale
  raised via `io.FontGlobalScale = 1.10`; `kSidebarWidth` 232→200 and `kStatusBarHeight` 30→24. New
  reusable primitives were added to `AuraWidgets.h`: `nav_item` (custom navigation row with an accent
  selection bar driven by the real page index), `status_item`, `metric_block`, and `success_meter`
  (labelled 0–100% bar that renders `N/A` rather than a fake percentage). `tools/aura_gui.cpp` gained a
  measured, right-aligned header (brand/instrument left; `TF`/`SHADOW ONLY`/market-health/renderer right —
  no hard-coded pixel offset) and a slim real-value status bar. The Dashboard was recomposed in
  `GuiPanels.h`: a market information strip (real last closed price, close time, stream state, quality,
  bar count) above a full-width, viewport-adaptive chart (min 320 / max 660 px) that is now the dominant
  workspace element; a six-column aligned status strip (`SYSTEM / DATA STREAMS / SCORE / SIGNAL / RISK /
  MODE`); and a three-card success-metrics row.
- Real metrics surfaced, no fabricated probability (GUI-0023, `TESTED` 2026-10-03): the GUI now shows
  AURA's **real** deterministic `SCORE` (RT-0011) and `CONFIDENCE` (RT-0012) — computed by the pipeline
  and previously hidden behind a `NOT AVAILABLE (no calibrated probability is claimed)` placeholder — plus
  a **realized shadow success rate** (`wins / closed positions`). `ApplicationPipeline` exposes
  `last_score()`/`last_confidence()` and cumulative `EngineStatus.positions_closed`/`wins` (a win is a
  full close with net realised P&L > 0, derived incrementally by the shadow simulator — deterministic, no
  clock, no lookahead). `DesktopModel` copies them verbatim; the GUI renders score and confidence bounded
  to `[0,1]`, each explicitly labelled "not a probability" / "not calibrated", and shows the success rate
  only once a position has closed (otherwise `N/A`). This closes a real presentation gap **without**
  inventing a calibrated success probability: AURA does not compute one and the GUI still does not claim
  one. `DesktopTests.test_real_metrics_projection` proves the projection equals the pipeline values,
  bounds them, and marks them unavailable when absent.
- The manifest graph is now 223 file-level tasks (178 `APPROVED`, 30 `TESTED`, 2 `IMPLEMENTED`
  [TASK-MANIFEST-001, GUI-0001], 11 `DEFERRED`, 2 `BLOCKED`). The deferred Master capabilities
  remain visible.
- Remaining unproven items are environment/data-bound (real-Windows-desktop interactive GUI including
  the legacy-GPU path, real MetaEditor/MT5 run, historical dataset campaign, and wiring sources for the
  still-`NOT AVAILABLE` panels). No live trading is enabled; Phase 11 stays a gate/registry.

## Verification (2026-10-03, local; remote CI recorded after push)

- `DesktopTests` PASS under g++ `-std=c++17`/`c++20` `-Wall -Wextra -Werror -pedantic` (real code paths,
  no mocks): section/incident/single-owner tests, the five renderer-policy/fallback tests, and the three
  redesign tests (`test_state_category_mapping`, `test_navigation_catalog`, `test_freshness_projection`).
- Default (GUI OFF) CMake build + ctest: **18/18 passed**. GUI build (`AURA_BUILD_GUI=ON`) c++17: **19/19
  passed** (includes `GuiSelfTest`).
- `aura_gui --self-test` PASS (270 frames, 9/9 streams, shadow-only, ledger+version sourced, incidents
  wired, unsourced sections `NOT AVAILABLE`, checkpoint OK, recovery CLEAN_SHUTDOWN resumable).
- `aura_gui --gui --frames N` interactive smoke under Xvfb software OpenGL PASS: booted, cycled through
  all 19 V3-37 sections, clean shutdown checkpoint OK. The redesigned layout was additionally exercised
  in a **Debug** (assert-enabled) GUI build — ImGui's own Begin/End balance and ID-stack assertions are
  active there — cycling all 19 sections for 80 frames under both `MODERN_GL33` and `LEGACY_GL21` with a
  clean shutdown (exit 0). A captured frame of the running app (Xvfb + ImageMagick) confirms the new
  shell structure (top bar, grouped sidebar, four-card dashboard row, nine-timeframe table, incidents
  table, status bar), and the modern and legacy captures are structurally identical (same design on both
  backends).
- Renderer smokes under Xvfb (software Mesa): `--renderer modern` -> `renderer=MODERN_GL33`; `--renderer
  legacy` -> `renderer=LEGACY_GL21`; AUTO with `MESA_GL_VERSION_OVERRIDE=2.1` (forces the modern attempt
  to fail) -> `renderer=LEGACY_GL21` with `OpenGL 3.3 unavailable` and clean shutdown; `--renderer
  modern` with Mesa capped -> exit 3 with the actionable no-renderer error. Each rendered frames and shut
  down cleanly. Remote CI run 37128118600 (commit d1c42f0, redesign + MSVC min/max fix) is ALL GREEN
  across 6/6 jobs, including the extended `desktop-gui-linux` renderer smokes and the
  `windows-x64-release-package` job that builds and packages `aura_gui.exe`; the earlier run
  37125095527 (commit f3e1886) was also ALL GREEN.

## XAUUSD candlestick chart verification (2026-10-03, local)

- `DesktopTests` ALL PASS (g++ C++17 strict; GUI build) including the seven new chart tests:
  `test_timeframe_selection_default_and_switch` (M15 default, nine-timeframe identity switching, UNKNOWN
  refused), `test_candle_series_closed_bar_identity` (foreign-timeframe and non-closed bars excluded),
  `test_candle_series_rejects_non_finite` (NaN dropped, not fabricated), `test_chart_geometry_bullish_bearish`
  (wick/body ordering, 5-level grid, deterministic layout), `test_chart_series_from_real_runtime`
  (report bound to the selected stream over real runtime frames), `test_chart_empty_state_no_fake_candles`
  (empty -> unavailable, zero candles), `test_chart_utc_formatter` (deterministic epoch labels).
- Full CTest under the GUI build: **19/19 passed**. `aura_gui --self-test` PASS and additionally prints
  `chart nine timeframes OK, default M15, each stream has real candles`.
- Interactive chart render verified by Xvfb captures (ImageMagick) under **both** renderers
  (`MODERN_GL33` and `LEGACY_GL21`): with the all-bullish builtin frame set the chart region shows ~5.4k
  green candle pixels; with an alternating up/down M15 series it shows ~12k green + ~13k red candle
  pixels — i.e. real OHLC wicks and green/red bodies actually render, identically on both backends.
- NOT claimed: human aesthetic review on the target machine; real historical XAUUSD data (the frames are
  deterministic synthetic closed bars produced by the trusted encoder path, labelled as such); any
  profitability/calibration/broker/production claim.
- Remote CI run `37131254459` (commit `02d3d5b`) for this work is **ALL GREEN 6/6** (ubuntu c++17/c++20,
  windows msvc c++17/c++20, `desktop-gui-linux`, `windows-x64-release-package` including the chart GUI).
  CI proves build + tests + bounded software-GL smokes, not visual quality or real-Windows rendering.
- NOT claimed: interactive rendering on a real Windows desktop, and specifically the Intel HD Graphics
  3000 path (UNPROVEN); any profitability, calibration, broker-validation or production-safety claim.
  The visual quality of the redesign is verified structurally (layout, no assertions, both backends) and
  by a captured frame, not by a human aesthetic review on the target machine.

## Terminal UI reconstruction verification (2026-10-03, local)

- A deep redesign of the desktop GUI's visual composition, information hierarchy, layout, component
  system and interaction model — not a cosmetic theme pass. Six file-level tasks were added and set
  `TESTED`: `GUI-0025` (pure GUI-free layout contract), `GUI-0026` (terminal composition shell),
  `GUI-0027` (design system + icon system), `GUI-0028` (structured financial candle renderer),
  `GUI-0029` (shell/Dashboard integration + staged fonts), `GUI-0030` (GUI-free layout-contract test).
- New/changed outputs: `src/desktop/TerminalLayout.h` (new), `src/desktop/AuraTerminal.h` (new),
  `src/desktop/AuraIcons.h` (new), recreated `src/desktop/AuraTheme.h`, refreshed
  `src/desktop/AuraWidgets.h`, rewritten `src/desktop/CandleChartWidget.h`, additively extended
  `src/desktop/DesktopModel.h` (real signal closed-bar close time + risk inputs, no new data source),
  recomposed `src/desktop/GuiPanels.h` `draw_dashboard`, reworked `tools/aura_gui.cpp` shell, and
  `CMakeLists.txt` (font staging + `AURA_FONT_DIR`).
- Composition: top terminal bar (brand + instrument + descriptor on the left; RENDERER / DATA / TF and a
  SHADOW ONLY pill measured and right-aligned), an icon navigation sidebar driven by the real page
  index, an instrument/market header (XAUUSD + selected-timeframe badge + real last-closed price/time +
  stream state/quality/bars), a horizontal nine-timeframe tab strip with an accent underline, the
  candlestick workspace as the dominant element, a compact single-row metric strip
  (SCORE / CONFIDENCE / REALIZED SUCCESS / DATA / RISK / MODE) and a slim real-value status bar.
  The chart is a structured financial chart: inset canvas, subtle grid, integrated right-hand price
  gutter, integrated bottom time axis and a last-price marker.
- Determinism / no-fabrication preserved: every value is read from the pre-built `ControlCenterReport`;
  absent data renders as explicit `N/A` / `NOT AVAILABLE`; the chart draws only real closed bars.
  No runtime reach-through, no new data source, no order path; shadow-only invariant unchanged.
- Verification: standalone strict compiles of the new/changed headers
  (`g++ -std=c++17 -Wall -Wextra -Werror -pedantic`) clean; `DesktopTests` ALL PASS including the new
  `test_terminal_layout_contract`; full CTest **19/19** under the GUI build (`AURA_BUILD_GUI=ON`, c++17)
  and **18/18** under the default GUI-off build; `aura_gui --self-test` PASS.
- Interactive render verified by Xvfb captures (ImageMagick) + OCR (tesseract) at **1280x720**,
  **1366x768**, **1600x900** and **1920x1080** under **both** `MODERN_GL33` and `LEGACY_GL21`: OCR
  confirms the top bar (`AURA | XAUUSD | MARKET INTELLIGENCE ... RENDERER MODERN_GL33 DATA 9/9 ONLINE TF
  M15` + the SHADOW ONLY pill), the market header (`... MARKET STREAM QUALITY BARS LAST CLOSED / ONLINE
  VALID 30 131.800`), the nine-tab strip (`M1 M5 M15 M30 H1 H4 D1 W1 MN1`) and the metric strip
  (`SCORE 0.98 CONFIDENCE 0.98 REALIZED SUCCESS 0% DATA LAGGING RISK PROPOSED MODE SHADOW`). The
  SHADOW ONLY pill was additionally colour-confirmed in the top bar (orange, x≈1208–1284). Candle
  pixels were counted (Pillow) in the chart region: ~2.0k green on MODERN and ~1.9k green on LEGACY,
  i.e. real candles render on both backends. With no data the capture reads `NO CANDLE DATA` /
  `MARKET STREAM NOT AVAILABLE` / `N/A` throughout — no fabricated zeros.
- NOT claimed: human aesthetic review on the target machine; real historical XAUUSD data (the frames
  are deterministic synthetic closed bars from the trusted encoder path, labelled as such); any
  profitability/calibration/broker/production claim. CI proves build + tests + bounded software-GL
  smokes, not visual quality or real-Windows rendering. The specific Intel HD Graphics 3000 path remains
  UNPROVEN (`BLOCK-006`).

## Premium GUI overhaul verification (2026-10-03, local)

- `DesktopTests` ALL PASS (g++ C++17 strict; GUI build), now including `test_real_metrics_projection`
  (score/confidence absent when no signal, present and bounded to `[0,1]` and equal to the pipeline's
  own `last_score()`/`last_confidence()` after a real run, realized outcomes consistent with
  `wins <= positions_closed`, unavailable until a close, second capture identical, shadow-only invariant
  held).
- Full CTest: **19/19 passed** under the GUI build (`AURA_BUILD_GUI=ON`, c++17) and **18/18** under the
  default GUI-OFF build. `aura_gui --self-test` PASS (270 frames, 9/9 streams, chart nine timeframes OK,
  checkpoint OK, recovery CLEAN_SHUTDOWN resumable).
- Interactive render verified by Xvfb captures (ImageMagick) + OCR (tesseract) at **1600x900** and
  **1280x720** under **both** `MODERN_GL33` and `LEGACY_GL21`. OCR of the header reads
  `AURA XAUUSD MARKET INTELLIGENCE ... TF M15 SHADOW ONLY MARKET HEALTHY RENDERER MODERN_GL33` on one
  line on both backends; OCR of the market strip reads `LAST CLOSED 141.808 AT 01-12 15:43 STREAM ONLINE
  QUALITY VALID BARS 32`; OCR of the six-column status strip reads `SYSTEM HEALTHY | DATA STREAMS 9/9 |
  SCORE 0.97 | SIGNAL LONG | RISK PROPOSED | MODE SHADOW` aligned in a single row (a first cut that used
  manual `SameLine` offsets cascaded vertically and was replaced by a borderless table; the misalignment
  was caught by the OCR pass, not assumed away). The Dashboard success row reads `SIGNAL SCORE 97% |
  CONFIDENCE 97% | REALIZED SHADOW RATE` over the real `closed 37 / 8 up` counts. This is the real score
  and confidence the pipeline computes; no calibrated probability is shown or claimed.
- NOT claimed: human aesthetic review on the target machine; the values shown are from deterministic
  synthetic closed bars (produced by the trusted encoder path, labelled as such), not licensed historical
  XAUUSD data; any profitability/calibration/broker/production claim. CI proves build + tests + bounded
  software-GL smokes, not visual quality or real-Windows rendering.

## Next action

1. Re-run the updated `aura_gui.exe` (from the new Windows x64 Release package) on the Windows 10 /
   Intel HD Graphics 3000 machine to confirm the startup line reports `renderer=LEGACY_GL21` and that the
   reconstructed terminal — including the new XAUUSD candlestick workspace, timeframe tab strip and
   chart-first composition — renders and shuts down cleanly; open a window large enough to see several
   candles and try switching timeframes. Record the real result in `TEST_LOG.md` (closes BLOCK-006 and
   promotes the terminal from structurally-verified to visually VERIFIED on the target machine).
2. When a MetaEditor/MT5 environment becomes available, run Phase 11 controlled validation
   (demo/shadow only) behind the readiness gate; record real results in `TEST_LOG.md`.
3. Optionally have a human run `aura_gui --gui` on a real Windows desktop to promote the interactive
   GUI from UNPROVEN to VERIFIED; do not claim it verified from the CI smoke alone.
4. Optional hardening: a SIGKILL-during-write recovery test to prove the atomic temp+rename path
   leaves either the old or the complete new file, never a torn one.
5. Increase GUI data coverage: wire the remaining `NOT AVAILABLE` panels (prediction/observation,
   knowledge, research, candidates, validation evidence, approvals, evolution, schedule, operating-window
   checkpoints) to their existing read-only ledgers when the control center actually runs those planes.
   Do not fabricate their values.
6. Keep the control plane and manifest updated per wave; preserve state in Git.
7. Do not enable unattended live trading or make profitability/safety claims without evidence.

## Update rule

Update this file after each meaningful implementation session.
