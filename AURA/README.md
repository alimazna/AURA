# AURA — Project Control Plane

This package adds persistent project memory and execution control files around the AURA architecture reference.

The goal is to let different AI agents continue the same project from Git history without relying on chat memory.

Start here:

1. `project-control/AI_BOOTSTRAP.md`
2. `project-control/IMPLEMENTATION_SCOPE.md`
3. `project-control/PROJECT_STATE.md`
4. `project-control/TASK_MANIFEST.yaml`

Architecture reference:

`docs/AURA_MASTER_UNIFIED_PROJECT_v3.0.md`

## Build and run

AURA is header-only C++17/20 with a Foundation translation unit. An authoritative CMake build is
provided (see the build system and `project-control/TEST_LOG.md`):

```sh
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure   # 18 behavioural suites incl. end-to-end + persistence
```

The build produces the `aura` host executable (console host, shadow mode only):

```sh
aura --replay <frames-file> [--store <path>]  # run the pipeline over recorded canonical frames
aura --serve  <port>        [--store <path>]  # serve the single MT5 transport; Ctrl-C to stop
aura --self-test            [--store <path>]  # bounded offline persistence + recovery smoke (CI)
aura --recover <store-path>                   # load + verify a persisted store, report the decision
```

Derived state (per-timeframe progress and the append-only shadow ledger) is persisted to a
file-backed, append-only, checksummed store and restored across restarts through the V2-36 crash
recovery path. A corrupted or version-incompatible store is refused, never silently resumed.
CI (`.github/workflows/ci.yml`) builds and tests on Linux under C++17 and C++20 and runs the smoke
test; an MSVC Windows job is included as a non-blocking portability check.

Status: AURA is an advanced engineered foundation/prototype, not a finished product. Live trading is
not enabled; no profitability, calibration, broker-validation, or production-safety claim is made.
See `project-control/PROJECT_STATE.md` and `project-control/BLOCKED.md` for the current state.
