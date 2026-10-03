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

## Open blockers (2026-10-02, from `GAP_AUDIT_2026-10-02`)

These are real blockers: the required environment is not available in this workspace, so the task is
recorded rather than implemented with an invented substitute.

### BLOCK-003 — GUI-0001 real desktop control center (RESOLVED 2026-10-03)

Status: RESOLVED with one UNPROVEN remainder. See the entry above (moved here from the open-blocker
list). The GUI now builds and runs; real-Windows-desktop interactive visual verification remains
UNPROVEN (the Xvfb software-OpenGL smoke proves the lifecycle only).

### BLOCK-004 — MT5-REAL-0001 MetaEditor compile + real terminal run

- Status: BLOCKED
- Blocker: MetaEditor/MetaTrader 5 and a broker terminal are not available on Linux, so the MQL5 EA
  cannot be compiled or attached, and no real terminal connection can be established.
- Why it matters: the one-EA/nine-stream transport and the C++ receiver are statically proven and now
  end-to-end tested over a real socket, but a real MT5 terminal round-trip is UNPROVEN.
- Smallest required dependency: a Windows host with MetaTrader 5 + MetaEditor, and a demo account.
- Affected tasks: `MT5-REAL-0001`; Phase 11 real-world validation.

### BLOCK-005 — VAL-EVID-0001 historical XAUUSD validation campaign

- Status: BLOCKED
- Blocker: no licensed historical XAUUSD dataset (and no broker cost/latency measurements) is present
  in this environment.
- Why it matters: the validation firewall is implemented and tested, but a real validation campaign
  requires real data and realistic cost/slippage/latency inputs.
- Smallest required dependency: a permitted historical dataset with documented provenance and broker
  cost/latency observations.
- Affected tasks: `VAL-EVID-0001`; Phase 6 evidence; Phase 11.

### BLOCK-006 — Legacy-GPU (Intel HD Graphics 3000) GUI compatibility verification

- Status: BLOCKED (hardware-specific verification only). The legacy-renderer FALLBACK CODE is
  implemented and TESTED on software GL; the specific Intel HD Graphics 3000 hardware is UNPROVEN.
- Observed on a real Windows 10 machine (Intel HD Graphics 3000, driver 9.17.10.4459):
  - `aura_gui.exe --self-test` = PASS (core/runtime is healthy).
  - `aura_gui.exe --gui` failed with
    `GLFW error 65543: WGL: OpenGL profile requested but WGL_ARB_create_context_profile is unavailable`
    then `failed to create window / OpenGL 3.3 context`. The old build requested a core-profile 3.3
    context unconditionally and terminated.
- Blocker: no Intel HD 3000 / OpenGL 2.1-only GPU is available in CI (Linux Xvfb uses software Mesa),
  so the fallback cannot be executed against that exact hardware here.
- Why it matters: the fallback code path is what makes the GUI usable on that class of hardware, but
  "the code path runs" (proven on software GL with `--renderer legacy` and with Mesa capped at GL 2.1
  to force the AUTO fallback) is not the same as "it renders correctly on Intel HD 3000".
- Smallest required dependency: a re-run of the new `aura_gui.exe` (from the updated Windows release
  package) on that Windows 10 / Intel HD 3000 machine, confirming the startup line reports
  `renderer=LEGACY_GL21` and the control center renders and shuts down cleanly.
- Affected tasks: `GUI-0007` (fallback implementation — TESTED), `GUI-0008` (tests — TESTED);
  Intel HD 3000 hardware compatibility remains UNPROVEN.

## Blocked-state rule

For every blocked task record:

- task ID
- exact blocker
- why it matters
- smallest required human decision/dependency
- affected tasks
