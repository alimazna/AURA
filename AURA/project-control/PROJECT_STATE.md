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

## Current task

- `FND-0001` — `src/foundation/EntityId.h` — status: `TESTED`.
- Phase 0 `src/foundation/` contracts — status: `IMPLEMENTED`, pending review.
- Next blocker: `FND-0015` — `src/foundation/ErrorCode.h` — status: `BLOCKED` (OPEN DECISION).

## Next action

1. Obtain the human `ErrorCode` taxonomy decision needed for `FND-0015`/`FND-0016`, or leave them BLOCKED.
2. Begin REVIEW/INTEGRATION of the `IMPLEMENTED` Phase 0 files, one task = one source file.
3. Preserve implementation + state in Git before ending the session.

## Update rule

Update this file after each meaningful implementation session.
