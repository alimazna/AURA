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
- Phases 3–11 remain DEFERRED and visible.
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

## Current task

- Phases 0, 0.5, 1 and 2 are COMPLETE and `APPROVED`.
- Remaining phases (3..11) are `DEFERRED` and have no file-level decomposition. Promotion of a deferred
  phase requires an explicit `DECISIONS.md` entry (per `IMPLEMENTATION_SCOPE.md`).

## Next action

1. Promote the next phase (Phase 3) with a `DECISIONS.md` entry before implementing it.
2. Then execute it in dependency order, preserving implementation + state in Git after each wave.

## Update rule

Update this file after each meaningful implementation session.
