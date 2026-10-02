# AURA — AI Handoff

## Current handoff

Date: 2026-10-02

### Where we are

- Repository layout is confirmed and normalized: the project lives under the nested
  `AURA/` directory inside `alimazna/AURA`; the repository root contains only `AURA/`.
- Phase 0 immutable foundations, configuration, audit, integrity, persistence and guardian
  contracts now exist under `AURA/src/foundation/` (34 source tasks implemented).
- There is still NO application build system, no test framework, and no MQL5 adapters.
- The earlier baseline build/test audit is not reproducible from the current repository and is
  treated as unverified historical material (see `TEST_LOG.md`).
- `TASK-MANIFEST-001` produced the detailed file-level task graph
  (`project-control/TASK_MANIFEST.yaml`, `manifest_version: 2`, 96 tasks) for the active scope.

### Task graph summary

- Active now: Phase 0 (36 tasks), Phase 0.5 (20), Phase 1 (21), Phase 2 (4), plus 4 phase milestones.
- Deferred and still visible: Phases 3–11 (`DEFERRED`).
- `FND-0001` (`src/foundation/EntityId.h`) is `TESTED`.
- Phase 0 source tasks `FND-0002..FND-0014`, `FND-0017`, `FND-0010/0011/0013`,
  `CFG-0001..0005`, `AUD-0001..0003`, `INT-0001..0003`, `PER-0001..0004`, `GDN-0001..0004`
  are `IMPLEMENTED` and verified by an ad-hoc compile harness (see `TEST_LOG.md`).
  They are NOT yet independently `REVIEWED`/`INTEGRATED`/`APPROVED`.
- Blocked on human architectural decision: `FND-0015 ErrorCode.h` (OPEN DECISION) and
  `FND-0016 ErrorRecord.h` (BLOCKED), per V3-45. These require an `ErrorCode` taxonomy decision
  recorded in `DECISIONS.md`; do not invent it. `current_task` is set to `FND-0015`.
- Authority note: Phase 0 filenames come from V3-42. The `src/` layout, the `EntityId`
  representation, the `Version` grammar, and the Phase 0.5/1/2 filenames are PROPOSED
  decomposition and require integration review; they are not canonical architecture.

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

### Next AI action

1. Read `project-control/AI_BOOTSTRAP.md`.
2. Read `project-control/IMPLEMENTATION_SCOPE.md`.
3. Read `project-control/PROJECT_STATE.md`.
4. Read `project-control/TASK_MANIFEST.yaml` and pick the next actionable task.
5. Inspect the repository; Phase 0 `src/foundation/` headers now exist.
6. The next real blocker is the human `ErrorCode` taxonomy decision (`FND-0015`/`FND-0016`).
   Do not invent it. Otherwise begin the REVIEW/INTEGRATION stage for the `IMPLEMENTED` Phase 0 files,
   one task = one source file.
7. Run appropriate deterministic checks; record real results in `TEST_LOG.md`.
8. Preserve implementation + state in Git before ending the session.

### Session end state (2026-10-02)

- The latest local commit on `main` (message: "Implement Phase 0 foundation contracts (Waves 0A-0D)
  and reconcile manifest") contains all Phase 0 contract files and the control-plane updates
  described above.
- PUSH STATUS: NOT pushed. The session `GITHUB_TOKEN` is read-only for this repository and no
  write credential was supplied, so `git push origin main` was rejected with HTTP 403.
  `origin/main` is therefore still `1fedbbc`. The work exists locally only and MUST be pushed by
  the next session that has write access.
- The remote URL is token-free (`https://github.com/alimazna/AURA.git`); no credential was
  persisted to the repository, config, or logs.

### Open blockers

- `FND-0015 ErrorCode.h` / `FND-0016 ErrorRecord.h`: need a human `ErrorCode` taxonomy decision.
- `src/` layout and Phase 0.5/1/2 filenames are PROPOSED and need integration review before they
  are treated as stable contracts.

### Do not

- begin by blindly repairing existing code (there is none here)
- treat current source as automatically authoritative
- implement the whole Master in one pass
- silently promote deferred phases
- flatten the repository again
- rely on previous AI conversation memory
- claim live trading/profitability readiness without evidence
