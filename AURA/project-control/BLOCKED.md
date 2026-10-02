# AURA — Blocked Items

Use this file only for real blockers.

## Current blockers

No open blockers as of 2026-10-02. (Both historical entries below are resolved and retained for
provenance.)

### BLOCK-001 — Detailed file-level task graph

Status: RESOLVED (2026-10-02)

Resolution:
`TASK-MANIFEST-001` produced `project-control/TASK_MANIFEST.yaml`
(`manifest_version: 2`, 96 tasks) covering the active scope, with deferred Master capabilities
marked `DEFERRED`. No missing contract was invented.

### BLOCK-002 — ErrorCode taxonomy decision (FND-0015 / FND-0016)

Status: RESOLVED (2026-10-02)

Task IDs:
- `FND-0015` — `src/foundation/ErrorCode.h` — status `REVIEW_PENDING` (was `BLOCKED` / OPEN DECISION)
- `FND-0016` — `src/foundation/ErrorRecord.h` — status `REVIEW_PENDING` (was `BLOCKED`)

Resolution:
The required human architectural decision was recorded in `project-control/DECISIONS.md`
(2026-10-02, "Phase 0 ErrorCode taxonomy decision", which explicitly resolves the V3-45 OPEN
DECISION). It freezes `ErrorCode` as a strongly typed `enum class` over the 13 V3 section 113 ERROR
categories, with explicit stable integral identities independent of compiler ordering, a closed set
(no `UNKNOWN`/`OTHER`), and severity kept as an independent record-level field.

`FND-0015 ErrorCode.h` and `FND-0016 ErrorRecord.h` were then implemented and verified
(self-contained standalone TUs and the full Phase 0 harness pass under `g++ -std=c++17` and
`-std=c++20` with `-Wall -Wextra -Werror -pedantic`; the include graph remains acyclic and
lower-layer only). No taxonomy beyond the recorded decision was invented.

Remaining state: both files are `REVIEW_PENDING` (implemented and reviewed, not yet independently
`APPROVED`/`INTEGRATED`). This is a normal lifecycle state, not a blocker.

Affected tasks:
`FND-0016 ErrorRecord.h`; later error/audit/persistence/Guardian contracts that consume error identity.

## Blocked-state rule

For every blocked task record:

- task ID
- exact blocker
- why it matters
- smallest required human decision/dependency
- affected tasks
