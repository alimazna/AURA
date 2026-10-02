# AURA — Blocked Items

Use this file only for real blockers.

## Current blockers

### BLOCK-001 — Detailed file-level task graph

Status: RESOLVED (2026-10-02)

Resolution:
`TASK-MANIFEST-001` produced `project-control/TASK_MANIFEST.yaml`
(`manifest_version: 2`, 96 tasks) covering the active scope, with deferred Master capabilities
marked `DEFERRED`. No missing contract was invented.

### BLOCK-002 — ErrorCode taxonomy decision (FND-0015 / FND-0016)

Status: OPEN

Task IDs:
- `FND-0015` — `src/foundation/ErrorCode.h` — status `BLOCKED` (authority OPEN DECISION)
- `FND-0016` — `src/foundation/ErrorRecord.h` — status `BLOCKED`

Exact blocker:
The Master V3 section 113 lists structured error categories
(`DATA_ERROR`, `SCHEMA_ERROR`, `CLOCK_ERROR`, `CONNECTION_ERROR`, `BROKER_ERROR`, `RISK_ERROR`,
`EXECUTION_ERROR`, `RECONCILIATION_ERROR`, `PERSISTENCE_ERROR`, `MODEL_ERROR`, `CONFIG_ERROR`,
`TELEGRAM_ERROR`, `RECOVERY_ERROR`) but does not freeze them as a closed, numbered `ErrorCode`
enumeration, nor define the code ranges, extensibility rule, or mapping to `ErrorSeverity`.

Why it matters:
`FND-0015` defines the canonical error identity used by `FND-0016 ErrorRecord` and by later
persistence, audit and Guardian behaviour. Freezing the wrong taxonomy would silently constrain
every downstream error contract and violate V3-11 (BLOCKED is a valid success state) and V3-45
(do not silently freeze OPEN DECISION items).

Smallest required human decision:
Record in `project-control/DECISIONS.md` a canonical `ErrorCode` taxonomy: the closed value set
(or explicit extensibility rule), stable numeric assignments, and the severity mapping.

Affected tasks:
`FND-0016 ErrorRecord.h`; later error/audit/persistence/Guardian contracts that consume error identity.

Rule:
Do not invent missing contracts or silently freeze OPEN DECISION items.

## Blocked-state rule

For every blocked task record:

- task ID
- exact blocker
- why it matters
- smallest required human decision/dependency
- affected tasks
