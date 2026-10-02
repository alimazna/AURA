# AURA — Blocked Items

Use this file only for real blockers.

## Current blockers

### BLOCK-001 — Detailed file-level task graph

Status: OPEN

Reason:
The Master V3 contains architecture and a Phase 0 decomposition, but the active implementation scope is intentionally narrower than the complete end-state system.

Required action:
`TASK-MANIFEST-001` must inspect the repository plus the Master and create the detailed active-scope task graph.

Rule:
Do not invent missing contracts or silently freeze OPEN DECISION items.

## Blocked-state rule

For every blocked task record:

- task ID
- exact blocker
- why it matters
- smallest required human decision/dependency
- affected tasks
