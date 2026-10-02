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

- Current GitHub HEAD: `048a95f` ("Add files via upload").
- The current repository contains only `README.md`, `docs/`, and `project-control/`.
- No C++ source tree, CMake build system, tests, or MQL5 adapters are present in the current GitHub repository.
- The earlier "baseline repository audit" (reported tag `v1.0-fixed`, `175/175` build, `40/40` CTest)
  is NOT reproducible from the current repository content and is treated as unverified external
  material, not as evidence about this repository.
- No build or test could be run against application source because no application source exists here yet.

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

## Current task

- `TASK-MANIFEST-001` — status: IMPLEMENTED (pending REVIEW / INTEGRATION / TEST).
- Next READY task: `FND-0001` — `src/foundation/EntityId.h` (no code dependencies).

## Next action

1. Review/integrate/test `TASK-MANIFEST-001` (structural validation recorded in `TEST_LOG.md`).
2. Obtain the human `ErrorCode` taxonomy decision needed for `FND-0015`/`FND-0016`, or leave them BLOCKED.
3. Start `FND-0001` and continue Phase 0 in dependency-wave order.
4. Preserve implementation + state in Git before ending the session.

## Update rule

Update this file after each meaningful implementation session.
