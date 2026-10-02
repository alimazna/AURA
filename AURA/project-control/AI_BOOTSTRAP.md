# AURA — AI Bootstrap

## Purpose

This file is the first entry point for any AI agent working on the repository.

The AI must not rely on previous chat memory. GitHub files are the persistent project memory.

## Read order

1. `project-control/IMPLEMENTATION_SCOPE.md`
2. `project-control/PROJECT_STATE.md`
3. `project-control/TASK_MANIFEST.yaml`
4. `project-control/DECISIONS.md`
5. `project-control/BLOCKED.md`
6. `project-control/HANDOFF.md`
7. Current task and required dependencies
8. `docs/XAUUSD_SOVEREIGN_MASTER_UNIFIED_PROJECT_v3.0.md` when architectural context is needed

## Authority

The Master V3 document is the architectural reference.

`IMPLEMENTATION_SCOPE.md` controls what portion of that architecture is in scope now.

Do not silently interpret every Master requirement as an immediate implementation requirement.

## Working rules

- Build the system from the Master architecture incrementally.
- Reuse existing code only when it conforms to the current scope and contracts.
- Existing code is implementation material, not architectural authority.
- Do not silently redesign architecture.
- Do not invent missing contracts, policies, types, or dependencies.
- If a required dependency or architectural decision is missing, mark the task `BLOCKED`.
- Keep changes scoped to the current task.
- Run deterministic build/tests appropriate to the task.
- Never claim completion from compilation alone.
- Preserve no-lookahead, no-repaint, determinism, auditability, provenance, versioning, and idempotency where applicable.
- Live trading is not the initial objective.
- Do not claim profitability, probability calibration, broker validation, or live safety without evidence.

## Task lifecycle

TASK READY
→ IMPLEMENT
→ BUILD
→ REVIEW
→ INTEGRATE
→ TEST
→ APPROVED / REWORK / BLOCKED

## Handoff

Before ending a meaningful work session, update:

- `PROJECT_STATE.md`
- `TASK_MANIFEST.yaml`
- `TEST_LOG.md`
- `HANDOFF.md`

Commit the implementation and state together so the next AI can resume from Git history.
