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
- Two Phase 0 contracts are blocked on human architectural decisions:
  `FND-0015 ErrorCode.h` (OPEN DECISION) and `FND-0016 ErrorRecord.h` (BLOCKED).

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
- All of the above are recorded as `IMPLEMENTED` (not `APPROVED`/`INTEGRATED`) pending independent
  review. `FND-0001` remains `TESTED`.
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
- Status is recorded as `REVIEW_PENDING` for the reviewed files — NOT `APPROVED`/`INTEGRATED`.
  Independent approval has not been granted, and compilation does not prove correctness.
- No Phase 0.5 work was started. `FND-0015` remains OPEN DECISION and `FND-0016` remains BLOCKED.

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
- Status: `FND-0015` and `FND-0016` are `REVIEW_PENDING` — implemented and reviewed, NOT yet
  `APPROVED`/`INTEGRATED`. `BLOCK-002` is RESOLVED; the manifest has no `BLOCKED` tasks.
- No Phase 0.5 work was started.

## Current task

- `FND-0001` — `src/foundation/EntityId.h` — status: `TESTED`.
- All 36 Phase 0 foundation files (incl. `ErrorCode.h`, `ErrorRecord.h`) — status: `REVIEW_PENDING`.
- No task is currently READY: the `PHASE-0` milestone requires all Phase 0 tasks `APPROVED` (a
  human/integration action), and Phase 0.5 depends on `PHASE-0`.

## Next action

1. Independently APPROVE/INTEGRATE the `REVIEW_PENDING` Phase 0 files (human/integration action) to
   satisfy the `PHASE-0` milestone acceptance.
2. Only then may Phase 0.5 be promoted; do not begin Phase 0.5 beforehand.
3. Preserve implementation + state in Git before ending the session.

## Update rule

Update this file after each meaningful implementation session.
