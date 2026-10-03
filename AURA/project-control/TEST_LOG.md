# AURA — Test Log

## 2026-10-03 — Phase 9 desktop productization: real GUI (GUI-0001/0002/0003)

Scope: build and verify a real desktop control-center GUI (Dear ImGui + GLFW + OpenGL 3.3) over the
existing runtime. Shadow-only; no live-order path.

Environment: Linux (g++ 14.2.0, CMake 4.4.3, libgl1-mesa-dev + xorg-dev + xvfb + mesa software GL
installed this session), plus the existing Windows CI runner.

Build commands verified locally:
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17` then `cmake --build build -j`
  (GUI OFF, default) -> BUILD PASS.
- `cmake -S . -B build -DAURA_BUILD_GUI=ON ... -DCMAKE_CXX_STANDARD=17` and `...=20` -> BUILD PASS
  (`aura_gui` links; pinned FetchContent glfw 3.4 + imgui v1.90.9).

Test commands and results (locally reproduced):
- `ctest --test-dir <GUI OFF build>` -> **18/18 PASS** (core matrix unchanged).
- `ctest --test-dir <GUI ON build>` -> **19/19 PASS** (adds `GuiSelfTest`).
- `DesktopTests` (direct, g++ -std=c++17 -Wall -Wextra -Wpedantic) -> `DesktopTests: ALL PASS`; also
  compiles clean under C++20. New tests: `test_desktop_model_empty_state`,
  `test_desktop_model_nine_timeframes`, `test_control_center_state_smoke`.
- `aura_gui --self-test --store /tmp/g20.aura` -> PASS: `fed 270 frames`, `snapshot rows=9 streams=9/9
  shadow_only=yes`, `checkpoint -> OK`, `recovery -> CLEAN_SHUTDOWN (resumable=yes)`, `SELF-TEST PASS`
  (exit 0). The empty-state run also printed `rows=9 shadow_only=yes` with no fabrication.
- **Interactive runtime smoke (genuine window + render + shutdown)**:
  `xvfb-run -a -s "-screen 0 1360x860x24" aura_gui --gui --frames 120 --store /tmp/gui_interactive.aura`
  -> `boot recovery -> UNKNOWN_STATE (resumable=no): no prior persisted state; clean fresh start`,
  `rendered 120 frames (bounded smoke); requesting close`, `shutdown checkpoint -> OK`, exit 0.
  This proves the GLFW window creation, ImGui GL3 render loop, bounded stop and clean-shutdown
  checkpoint actually run — not merely that the target links.

Behavioural checks exercised (real code paths, no mocks):
- Nine timeframes presented individually by identity (M1..MN1), in canonical order; absent streams
  shown `NOT AVAILABLE`, never fabricated as healthy.
- Fed deterministic frames -> 9/9 streams present, accepted > 0.
- `shadow_only` invariant true; no live-order path exists in the GUI.
- Checkpoint -> OK; fresh process cross-process recovery -> `CLEAN_SHUTDOWN`/resumable.
- Corrupted store -> `CORRUPTED_STATE`, refused, not auto-resumed just because the UI is open.

Files changed: `tools/aura_gui.cpp`, `src/desktop/DesktopModel.h`, `src/desktop/ControlCenterState.h`,
`src/desktop/GuiPanels.h`, `src/desktop/DesktopTests.cpp`, `CMakeLists.txt`, `.github/workflows/ci.yml`,
`README.md`, and the control-plane files.

Known failures / unproven: none observed locally. **UNPROVEN:** interactive rendering on a real Windows
desktop (no human `aura_gui --gui` run on Windows yet); the Xvfb smoke is software-rendered on Linux.
**UNPROVEN:** MT5/MetaEditor round-trip and any historical XAUUSD campaign.

Interpretation: Phase 9 now delivers a real desktop control center that builds and runs, with its
integration path and interactive lifecycle verified by reproducible local runs. This is NOT a claim of
Windows visual verification, correctness of unverified sections, profitability, calibration, broker
validation, production safety, or live-trading readiness. Shadow only. (Remote CI for this commit is
recorded in the handoff once the run completes; the local results above are the reproducible evidence.)

## Future test entry format

## 2026-10-02 — Baseline repository audit (unverified external material)

Source:
OpenHands read-only audit supplied by the project owner.

Reported:
- CMake configure: PASS with UI disabled
- Build: PASS, 175/175
- CTest: PASS, 40/40
- Repository status after audit: clean
- No commit/push from the audit
- MT5 C++: buildable
- MQL5 adapters: statically checked, not compiled/executed in MetaEditor
- Real C++↔MQL5 runtime connection: not proven
- Host application MT5 wiring: reported missing

Qualification:
These findings are NOT reproducible from the current GitHub repository content
(`alimazna/AURA` at `048a95f`), which contains only `README.md`, `docs/`, and
`project-control/`. They are retained as historical context only and must not be
treated as evidence about the current repository. They do not prove Master V3
implementation, live trading readiness, profitability, broker validation, or production safety.

## 2026-10-02 — Bootstrap normalization verification

Task/Phase: BOOTSTRAP normalization (repository layout + control-plane paths)

Verification performed (read-only inspection; no application source exists yet):
- `git --no-pager log --oneline` -> HEAD `048a95f`; working tree returned to the committed
  nested layout after an incorrect flattening attempt was reverted.
- `git reset` then re-created `AURA/`; `git status --short` shows only the three intended
  control-plane modifications (AI_BOOTSTRAP.md, PROJECT_STATE.md, TASK_MANIFEST.yaml).
- `find . -path ./.git -prune -o -type f -print` -> all 10 original files present under `AURA/`.
- `ls AURA/docs/AURA_MASTER_UNIFIED_PROJECT_v3.0.md` -> exists (354624 bytes).
- `grep -rn XAUUSD_SOVEREIGN_MASTER_UNIFIED_PROJECT AURA/project-control/` -> no matches (exit 1).
- `.git` was not modified.

Result: PASS (structural). No file lost. No stale control-plane reference remains.

Interpretation:
This proves the repository layout and control-plane path consistency. It does NOT prove any
runtime, trading, or architectural correctness. No application source code was built or executed.

## 2026-10-02 — TASK-MANIFEST-001 structural validation

Task/Phase: TASK-MANIFEST-001 (project-control)

Command: `python3` with `yaml.safe_load` on `AURA/project-control/TASK_MANIFEST.yaml`,
followed by structural checks.

Result (recorded values):
- Manifest parses as valid YAML.
- `manifest_version: 2`; 96 tasks; all task IDs unique.
- Status counts: APPROVED 1, IMPLEMENTED 1, READY 12, PLANNED 71, BLOCKED 2, DEFERRED 9.
- Authority counts: CANONICAL 24, PROPOSED 65, PROJECT DECISION 6, OPEN DECISION 1.
- Dependency integrity: zero unresolved task-ID dependencies (excluding file/document references).
- Source output paths: 81, all unique (one output path per source task).
- `FND-0015` (OPEN DECISION) and `FND-0016` are BLOCKED; Phases 3–11 are DEFERRED.

Result: PASS (structural only).

Interpretation:
Structural validation only. It proves the task graph is internally consistent and complete for
the active scope. It does NOT prove that any planned source file is correct, buildable, or that
the architecture has been implemented. No application source code was written.

## 2026-10-02 — FND-0001 EntityId implementation verification

Task/Phase: FND-0001 (Phase 0, Immutable Foundations)

Files changed: `AURA/src/foundation/EntityId.h` (new)

Build command (ad-hoc; no application build system exists):
`g++ -std=c++17 -Wall -Wextra -Werror -pedantic -I AURA/src /tmp/fnd0001_check.cpp`

Build result: PASS (clean compile, c++17). Also PASS under `-std=c++20`.

Test command: `/tmp/fnd0001_check` (runtime structural assertions, no test framework added)

Test result: PASS. Assertions covered:
- default-constructed identity is invalid; `valid()` false and `operator bool` false
- valid identity carries exact bytes (`value()`, `view()`)
- deterministic equality / inequality
- deterministic hashing: equal ids hash equally; `std::hash<EntityId>` specialization resolves
- deterministic ordering (`operator<`)
- usable as a key in `std::unordered_set` (duplicate insert collapses)
- copy preserves identity and hash
- explicit construction from `std::string` works
- no implicit conversion from `const char*` (compile-time `static_assert`)

Additional check: standalone-include self-containment (`#include "foundation/EntityId.h"` only)
compiled and linked cleanly.

Known limitations:
- No application build system, no test framework, and no other source files exist; verification is
  an ad-hoc compile plus a single throwaway program kept outside the repository.
- `EntityId` uses the default `std::hash<std::string>`; this is deterministic within a program run
  but is NOT a stable cross-process/serialization hash. No serialization hash is provided (out of scope).

Interpretation:
FND-0001's header compiles cleanly under strict warnings and its documented structural properties
hold at runtime. This does NOT prove architectural correctness, and does NOT prove anything about
trading, profitability, or live safety. `EntityId` is an immutable value type only.

## 2026-10-02 — Phase 0 Waves 0A–0D + reconciliation verification

Task/Phase: Phase 0 foundation/config/audit/integrity/persistence/guardian tasks
(FND-0002..FND-0014, FND-0017, FND-0010/0011/0013, CFG-0001..0005, AUD-0001..0003,
INT-0001..0003, PER-0001..0004, GDN-0001..0004).

Files added: `AURA/src/foundation/` —
`Timestamp.h`, `Version.h`, `ServiceState.h`, `SystemMode.h`, `HashDigest.h`,
`DataQualityState.h`, `ProtocolVersion.h`, `SchemaVersion.h`, `EventType.h`,
`ErrorSeverity.h`, `RecoveryAction.h`, `EventId.h`, `MessageMetadata.h`, `EventMetadata.h`,
`ConfigurationKey.h`, `ConfigurationValue.h`, `ConfigurationScope.h`,
`ConfigurationSnapshot.h`, `ConfigurationChange.h`, `AuditAction.h`, `AuditOutcome.h`,
`AuditRecord.h`, `HashAlgorithm.h`, `IHasher.h`, `Hasher.cpp`, `PersistenceStatus.h`,
`PersistenceRecordMetadata.h`, `IPersistenceStore.h`, `PersistenceTransaction.h`,
`GuardianStatus.h`, `GuardianPolicy.h`, `IGuardian.h`, `Guardian.h`.

Build command (ad-hoc; no application build system exists):
`g++ -std=c++17 -Wall -Wextra -Werror -pedantic -I AURA/src /tmp/phase0_full_verify.cpp AURA/src/foundation/Hasher.cpp`
Also compiled under `-std=c++20` with the same flags.

Build result: PASS (clean, both standards).

Test result: PASS. Structural assertions covered:
- Version parse/round-trip (`1.2.3`, `2.10.4-beta.1`) and ordering (`1.9.0 < 1.10.0`).
- Timestamp explicit-unit conversions and exact ordering.
- `HashDigest::from_hex`/`to_hex` round-trip and byte-exact equality.
- Canonical enum values: ServiceState (V3-14), SystemMode (V3-14), DataQualityState (V3-25).
- `EventId::to_string()` stable round-trip; `MessageMetadata` carries a `HashDigest` checksum;
  `EventMetadata` keeps `event_time` and `receive_time` distinct with `sequence_id` ordering.
- Configuration typed equality (`integer(7) != decimal(7.0)`), versioned snapshot lookup, effective-change.
- Audit record attributability; failed/rejected outcomes not treated as success.
- Integrity: real FIPS 180-4 SHA-256 known-answer tests passed —
  `sha256("abc") = ba7816bf...15ad` and `sha256("") = e3b0c442...b855`; determinism re-checked;
  `create_hasher(UNKNOWN) == nullptr`.
- Persistence transaction rejects duplicate record identity; applicable predicate.
- Guardian severity mapping (FATAL→HALT, CRITICAL→SAFE_MODE, INFO→NONE); rollback policy requires
  a KNOWN-GOOD anchor (`well_formed()` false without one).

Reconciliation (before continuing beyond Wave 0A):
- Wave 0A canonical set (V3-44) is exactly 12 tasks. Verified all 12 files exist:
  EntityId, Timestamp, Version, ServiceState, SystemMode, HashDigest, DataQualityState,
  ProtocolVersion, SchemaVersion, EventType, ErrorSeverity, RecoveryAction. No task omitted.
  The earlier "11 files" note excluded FND-0001 (already implemented/tested).
- Contract inconsistency found and corrected (not silently frozen): an earlier draft of
  `SystemMode.h` used invented values (`NORMAL/DEGRADED/SAFE/HALT/SHADOW/LIVE`). FND-0005's
  acceptance requires the V3-14 values exactly. Rewritten to
  `STARTING/RECOVERY/NORMAL/DEGRADED/SHADOW/PAUSED/MANUAL/EMERGENCY/HALTED`.
- `RecoveryAction.h` updated to make `UNKNOWN` representable per its acceptance criterion.
- `MessageMetadata.h` updated to carry the V3-23 `HashDigest` checksum per FND-0010 acceptance.
- `EventId.h` updated with a stable `to_string()` serialization per FND-0011 acceptance.
- `Version.h` semantics reviewed against FND-0003: MAJOR.MINOR.PATCH + optional build tag,
  ordering and round-trip are supported by the manifest acceptance and by the Master's versioned
  configuration/provenance requirement (V3-26). The Master does not freeze a concrete version
  grammar, so the grammar is a PROPOSED implementation choice; the file is marked `IMPLEMENTED`,
  not `TESTED`, pending REVIEW.

Known limitations:
- No application build system, test framework, or REVIEW/INTEGRATION/APPROVAL stage yet; verification
  is ad-hoc compile plus a throwaway harness outside the repository.
- `HashDigest` comparison is not constant-time (not a security boundary in Phase 0).
- These checks do not prove architectural or trading correctness.

Interpretation:
The Phase 0 files compile cleanly under strict warnings (c++17/c++20) and their documented
structural properties hold at runtime, including genuine SHA-256 known-answer tests. Status is
`IMPLEMENTED` (not `APPROVED`/`INTEGRATED`) because independent review has not occurred.

## 2026-10-02 — Phase 0 REVIEW / INTEGRATION (Phase 0 only)

Scope: review/integration of the 34 implemented Phase 0 files. No Phase 0.5 work. `FND-0015`
remains OPEN DECISION and `FND-0016` remains BLOCKED; no substitute ErrorCode definitions were
created.

Review method (per file): checked against the exact `TASK_MANIFEST.yaml` acceptance criterion, the
relevant canonical Master V3 section, declared dependencies, namespace/include correctness,
supported C++ standard, determinism, ownership boundaries, dependency direction, accidental
future-phase behaviour, and unnecessary API surface.

Findings:
- Defect (REWORK) `GDN-0002 src/foundation/GuardianPolicy.h`: included `GuardianStatus.h` but never
  used it. Removed the unused include. Reworked and re-verified.
- Defect (REWORK) `PER-0004 src/foundation/PersistenceTransaction.h`: included `PersistenceStatus.h`
  but never used it. Removed the unused include. Reworked and re-verified.
- Benign: several files include foundation headers that are not in their scheduling `dependencies`
  list (e.g. `ProtocolVersion.h`/`SchemaVersion.h` -> `Version.h`; `ConfigurationSnapshot.h` ->
  `ConfigurationScope.h`). This is correct layering (lower-layer only), not a contract mismatch.
  The manifest dependency lists are scheduling dependencies, not the include contract.
- Include graph: acyclic; no self-includes; all includes are lower-layer
  (no hidden higher-layer dependency). Verified programmatically.
- Focused re-checks: `SystemMode.h` uses the exact V3-14 values; `MessageMetadata.h` carries the
  V3-23 `HashDigest` checksum; `EventId.h` provides stable `to_string()`; `RecoveryAction.h` exposes
  `UNKNOWN` and is not treated as safe; `Version.h` parse/round-trip/ordering all hold.

Build command (ad-hoc; no application build system exists):
`g++ -std=c++17 -Wall -Wextra -Werror -pedantic -I AURA/src /tmp/phase0_review_verify.cpp AURA/src/foundation/Hasher.cpp`
Also compiled under `-std=c++20` with the same flags.

Build result: PASS (clean, both standards).

Test result: PASS. Additionally, standalone self-containment was verified for all 33 headers:
each header was compiled alone as a translation unit under `-std=c++17` and `-std=c++20` with
`-Wall -Wextra -Werror -pedantic` (66 TUs) — ALL PASS. Runtime assertions re-confirmed SHA-256
known-answer vectors (`sha256("abc")`, `sha256("")`), enum values against V3-14/V3-25, and the
Guardian severity mapping.

Hygiene:
- 34 files exist under `AURA/src/foundation/`; no duplicate or accidental files; no stray build
  artifacts.
- Manifest `src/foundation/` outputs = 36 (34 present + `ErrorCode.h`/`ErrorRecord.h` still BLOCKED).
- Secret scan of `AURA/**` for token/key/private-key patterns: no matches.

Interpretation:
Every implemented Phase 0 file passed review and integration verification. Status is recorded as
`REVIEW_PENDING` (not `APPROVED`/`INTEGRATED`), because independent approval has not been granted;
compilation and structural checks do not prove architectural or trading correctness.

## 2026-10-02 — FND-0015 ErrorCode + FND-0016 ErrorRecord (implement / review / full Phase 0 verify)

Tasks/Phase: FND-0015 `src/foundation/ErrorCode.h`, FND-0016 `src/foundation/ErrorRecord.h`
(Phase 0). The V3-45 OPEN DECISION was resolved by a human decision recorded in `DECISIONS.md`
(2026-10-02); no taxonomy was invented by the agent.

Build command (ad-hoc; no application build system exists):
`g++ -std=c++17 -Wall -Wextra -Werror -pedantic -I src /tmp/phase0_full_verify.cpp src/foundation/Hasher.cpp`
Also compiled under `-std=c++20` with the same flags.

Build result: PASS (clean, both standards).

Test result: PASS.
- `FND-0015`: exact enum values asserted — `DATA_ERROR=1` … `RECOVERY_ERROR=13` in V3-113 order;
  `to_string()` returns the canonical labels; strongly-typed scoped enum, underlying `uint8_t`.
- `FND-0016`: all eight V3-27 fields round-trip; default record is not `valid()` and defaults
  `recovery_action` to `UNKNOWN` (representable, not safe); empty component ⇒ not valid.
- SHA-256 known-answer tests still pass: `sha256("abc")`, `sha256("")`, and the 448-bit FIPS vector.
- Existing contracts re-checked: `SystemMode` V3-14 values; `ServiceState` names; `is_critical`,
  `is_known`, `is_protective`.

Structural verification (programmatic):
- Self-containment: 35 headers × 2 standards = 70 standalone TUs — ALL PASS
  (`-Wall -Wextra -Werror -pedantic`).
- Include graph: acyclic; no self-includes; no unknown includes; all includes lower-layer.
- Files: 36 expected Phase 0 files present (including `ErrorCode.h`, `ErrorRecord.h`); no missing,
  no stray/duplicate files.
- Duplicate type check: exactly one `enum class ErrorCode` (ErrorCode.h) and one `class ErrorRecord`
  (ErrorRecord.h).
- Secret scan of the repository: no matches.

Review findings: no defect found in either file. `ErrorRecord` includes only `EntityId`, `ErrorCode`,
`ErrorSeverity`, `RecoveryAction`, `ServiceState`, `Timestamp` (all lower-layer); no circular or
hidden higher-layer dependency; API limited to the contract; no logging/recovery/retry/evaluation/
serialization/persistence/networking behaviour.

Interpretation:
Both files compile cleanly and satisfy their structural and contract checks. Status is recorded as
`REVIEW_PENDING` — NOT `APPROVED`/`INTEGRATED` — because independent approval has not been granted;
compilation and structural checks do not prove architectural correctness. `BLOCK-002` is RESOLVED;
the manifest now has no `BLOCKED` tasks. `PHASE-0` (requires all Phase 0 tasks APPROVED) and Phase 0.5
are not started; no task is currently READY.

## 2026-10-02 — Phase 0 final review / integration gate

Scope: all 36 Phase 0 file-level tasks (`src/foundation/`), reviewed against
`TASK_MANIFEST.yaml`, `DECISIONS.md`, `BLOCKED.md`, Master V3, and the files on disk.

Commands (ad-hoc; no application build system exists):
- `g++ -std=c++17 -Wall -Wextra -Werror -pedantic -I src <harness>.cpp src/foundation/Hasher.cpp`
- same with `-std=c++20`
- per-header standalone TUs for all 35 headers under both standards
- programmatic include-graph, duplicate-type, manifest-vs-filesystem, and secret scans

Result: PASS.
- 36/36 canonical Phase 0 outputs exist; no duplicate or stray files; manifest outputs match the
  filesystem.
- 35 headers self-contained: 70 standalone TUs (35 × c++17/c++20) — ALL PASS.
- Full Phase 0 runtime harness passes under both standards, including genuine SHA-256 known-answer
  tests (`abc`, empty, 448-bit FIPS vector) and the Guardian severity→recovery mapping.
- Include graph: acyclic; no self-includes; no unknown includes; all includes lower-layer; no
  duplicate foundation type.
- Canonical enum values re-verified against Master V3: V3-14 `ServiceState`/`SystemMode`, V3-25
  `DataQualityState`, section 113 `ErrorCode` (1..13) and the eight-field `ErrorRecord` shape.
- Secret scan: no matches.

Defect found and fixed:
- 10 Phase 0 tasks under-declared their dependencies in `TASK_MANIFEST.yaml` relative to the actual
  (correct) include contracts: `FND-0008`, `FND-0009`, `FND-0010`, `FND-0016`, `CFG-0003`, `CFG-0005`,
  `AUD-0003`, `PER-0003`, `PER-0004`, `GDN-0003`. The dependency lists were reconciled to the verified
  includes (e.g. `FND-0008`/`FND-0009` → `FND-0003`; `PER-0003` → `PER-0001`, `PER-0002`, `FND-0001`,
  `FND-0006`). No source file was changed; the include graph was already correct. After the fix, no
  genuine dependency omission remains, and the V3-44 Wave 0B dependency sets are all satisfied.

Interpretation:
Every Phase 0 file-level task passes independent review and verification and is classified
`APPROVED`. The `PHASE-0` milestone is `APPROVED` and Phase 0 is complete. No task is `BLOCKED`;
`BLOCK-002` stays RESOLVED. The next READY task is `RS-0001` (Phase 0.5) but it was NOT started.
Compilation and structural checks support — but do not by themselves prove — architectural
correctness; no profitability, calibration, broker-validation or production-safety claim is made.

## 2026-10-02 — RS-0001 ServiceDescriptor (Phase 0.5)

Scope: `RS-0001` → `src/resilience/ServiceDescriptor.h` only. One task, one output file.

Commands (ad-hoc; no application build system exists):
- `g++ -std=c++17 -Wall -Wextra -Werror -pedantic -I src /tmp/sc17.cpp` (standalone self-containment)
- same with `-std=c++20`
- `g++ -std=c++17 ... /tmp/rs0001_check.cpp` and `-std=c++20` (structural harness)
- combined all-headers TU incl. `Hasher.cpp` under both standards

Result: PASS.
- Self-contained: the header compiles alone (no other project header) under c++17 and c++20 strict.
- Structural harness passes under both standards: default descriptor invalid; non-empty name valid;
  empty name invalid; name/state preserved; deterministic equality/inequality on both fields;
  deterministic ordering (name then state); equal values hash equal; usable as `unordered_set` and
  `set` key (equal keys dedup); copy preserves value; all 8 canonical `ServiceState` values
  representable; `is_operational` remains the sole operational authority.
- Includes: only `foundation/ServiceState.h` (FND-0004) plus std headers. Preprocessor check confirms
  `foundation/EntityId.h` is NOT pulled in (the only "EntityId" occurrence is in a comment). No
  higher-layer include. Include graph acyclic.
- No duplicate type name (`ServiceDescriptor` declared once); the `hash` matches are distinct
  `std::hash<...>` specializations, not duplicate types.
- Combined all-headers TU compiles cleanly under c++17 and c++20 strict.

Interpretation:
`RS-0001` satisfies its acceptance ("describes identity and state vocabulary; consistent with
ServiceState; no runtime behaviour") and the V3-13/V3-15/V3-16/V3-40/V3-46/V3-49 checks. Classified
`APPROVED`. No source defect found; no dependency contract was changed. Compilation and structural
checks support — but do not by themselves prove — architectural correctness.

## 2026-10-02 — Phase 0.5 resilience (RS-0001..RS-0020)

Scope: Phase 0.5, `src/resilience/`. 20 file-level tasks; RS-0001 completed earlier, RS-0002..RS-0020
in this session.

Commands (ad-hoc; no application build system exists):
- per-header self-containment: `g++ -std={c++17,c++20} -Wall -Wextra -Werror -pedantic -I src <TU>`
- combined all-headers TU incl. `src/foundation/*.cpp` under both standards
- `g++ -std={c++17,c++20} ... src/resilience/DegradationTests.cpp src/foundation/Hasher.cpp -o t && ./t`
- static include-graph / duplicate-type / dependency-direction analysis (python)

Result: PASS.
- All 19 resilience headers are self-contained (standalone TU) under c++17 and c++20 strict; the
  combined all-headers TU compiles under both.
- `DegradationTests.cpp` passes under both standards, exercising real code paths (no mocks):
  duplicate capability ids rejected; self-edges rejected; cycle detected and topological order empty
  on a cyclic graph; deterministic topological order (dependencies before dependents); dependency
  failure degrades dependents; independent capability stays ONLINE; missing/unregistered health is
  BLOCKED (never fabricated ONLINE); isolation disables only dependents and keeps independent
  capabilities available; non-critical failure is tolerable while critical/unclassified requires
  protection; impact names the exact failed capability; recovery prefers known-valid state and
  escalates to SAFE_MODE when none exists; paused is not treated as failed; freshness distinguishes
  FRESH/STALE/EXPIRED and treats future/incoherent input as UNKNOWN; per-timeframe stale detection
  reports exactly the stale timeframe (M15) while the other eight stay VALID; pause requires a
  reason and an operating mode; the supervisor consults the Guardian and reports HALT/SAFE_MODE
  without escalating itself.
- Include graph: acyclic; resilience headers include only `foundation/` and `resilience/` (no hidden
  higher-layer dependency). No duplicate type names. No future-phase behaviour; no live-trading
  enablement. Secret scan: none.

Interpretation:
Phase 0.5 satisfies the V3-13/V3-15/V3-16 requirements and the `PHASE-0.5` milestone acceptance
("capability dependency graph encoded; degradation/isolation tests prove non-critical failure
isolation"). All 20 Phase 0.5 tasks and the milestone are classified `APPROVED`. Compilation and
structural checks support — but do not by themselves prove — architectural correctness; no
profitability, calibration, broker-validation or production-safety claim is made.

## 2026-10-02 — Phase 1 deterministic runtime (RT-0001..RT-0021)

Scope: Phase 1, `src/runtime/`. 21 file-level tasks.

Commands (ad-hoc; no application build system exists):
- per-header self-containment under c++17/c++20 strict
- combined all-headers TU across foundation/resilience/runtime under both standards
- `g++ -std={c++17,c++20} ... src/runtime/RuntimeTests.cpp src/foundation/Hasher.cpp -o t && ./t`
- static include-graph / duplicate-type / layer-direction analysis (python)

Result: PASS.
- All 20 runtime headers self-contained under c++17 and c++20 strict; combined all-headers TU (74
  headers) compiles under both.
- `RuntimeTests.cpp` passes under both standards (real code paths, no mocks): nine independent MT5
  streams where an M15 failure/recovery leaves H4 untouched and a D1 disconnect leaves M15 healthy;
  unknown timeframe rejected; deterministic data-bus ordering; future-dated event rejected (no
  lookahead); forming bar rejected (no repaint); OHLC validation (INVALID/DEGRADED/VALID); closed-bar
  identity deterministic and per-timeframe; re-finalizing the same close_time rejected; timeframe state
  store is monotonic (regression refused) and persists idempotently; feature computation refuses any
  still-forming bar; H4 structural authority enforced; eligibility gated by data quality (UNKNOWN/STALE
  -> INELIGIBLE); decision/signal identity deterministic and configuration-version dependent; score and
  confidence bounded ranking values (zero confidence on non-VALID quality); market quality UNKNOWN when
  streams unassessed and GOOD when all valid; risk proposal is not an order and requires usable market
  quality; shadow fill is never live and models commission/partial fill; position lifecycle open ->
  partial close -> full close; reconciliation MATCHED within tolerance and DIVERGED/not-trustworthy
  outside it; ledger entry identity deterministic and append idempotent; ledger persist idempotent;
  replay finalizes ten closed bars, rejects the future-dated and forming events, and produces identical
  fingerprints across runs (reproducible).
- Include graph: acyclic; runtime headers include only foundation/resilience/runtime. No duplicate types
  (std::hash specializations only). No live-order path; no future-phase behaviour. Secret scan: none.

Interpretation:
Phase 1 satisfies the `PHASE-1` milestone acceptance ("deterministic runtime proven by replay tests;
shadow ledger append-only and idempotent; no live orders"). All 21 tasks and the milestone are
`APPROVED`. Compilation and structural checks support but do not by themselves prove architectural
correctness; no profitability, calibration, broker-validation or production-safety claim is made, and
the real MT5 connection remains unproven until measured.

## 2026-10-02 — Phase 2 observation and outcomes (OB-0001..OB-0004)

Scope: Phase 2, `src/observation/`. 4 file-level tasks.

Commands (ad-hoc; no application build system exists):
- per-header self-containment under c++17/c++20 strict
- combined all-headers TU across foundation/resilience/runtime/observation under both standards
- `g++ -std={c++17,c++20} ... src/observation/ObservationTests.cpp src/foundation/Hasher.cpp -o t && ./t`
- static include-graph / duplicate-type / layer-direction analysis (python)

Result: PASS.
- All 4 observation headers self-contained under c++17 and c++20 strict; combined all-headers TU (78
  headers) compiles under both.
- `ObservationTests.cpp` passes under both standards (real code paths, no mocks): prediction identity
  deterministic and sensitive to changed fields; ledger append-only, auditable (find by identity),
  idempotent on duplicate and rejecting on conflict; persist idempotent; invalid prediction rejected.
  Outcome resolution: TARGET_HIT / STOP_HIT resolved, ambiguous bar resolves conservatively to
  STOP_HIT, a bar at/before decision time cannot resolve (no lookahead), empty bars -> OPEN and no
  trigger -> EXPIRED (no fabricated win/loss), and resolution is deterministic across runs.
  Failure detection: an ERROR stream yields a valid structured ErrorRecord with its own error id, a
  named component, a non-INFO severity and a known recovery action; a healthy set yields none.
  System health: unassessed -> STARTING and not healthy, all-online -> healthy, one offline stream ->
  unhealthy with the aggregate reflecting the worst stream; severity ordering is total.
- Include graph: acyclic; observation headers include only foundation/resilience/runtime/observation.
  No duplicate types beyond a per-translation-unit test helper class and std::hash specializations. No
  live-order path; no future-phase behaviour. Secret scan: none.

Interpretation:
Phase 2 satisfies the `PHASE-2` milestone acceptance ("prediction ledger and outcome engine
deterministic and auditable; failures structured"). All 4 tasks and the milestone are `APPROVED`.
Compilation and structural checks support but do not by themselves prove architectural correctness;
no profitability, calibration, broker-validation or production-safety claim is made.

## 2026-10-02 - Conceptual Phase 3/4/5 verification + MT5 one-EA/nine-stream boundary

Commits: see `HANDOFF.md` / `PROJECT_STATE.md` (conceptual P3/P4/P5 wave and MT5 wave).

Toolchain: `g++ (Debian 14.2.0-19) 14.2.0`, `-std=c++17` and `-std=c++20`, `-Wall -Wextra -Werror
-pedantic`. Python 3 static audits. No application build system; ad-hoc harness.

Build/test commands:
- per-header self-containment and combined all-headers TU (80 headers) under both standards
- `g++ -std=<std> -Wall -Wextra -Werror -pedantic -I src src/<artifact>.cpp src/foundation/Hasher.cpp -o t && ./t`
  for `RuntimeTests`, `DegradationTests`, `ObservationTests`, `TimeframeStateTests`,
  `AnalysisPipelineTests`, `SignalPipelineTests`, `Mt5ReceiverTests`
- MQL5 static audit (python) and repository static audit (python)

Result: PASS.
- 80 headers self-contained and combined TU OK under c++17/c++20 strict.
- All seven behavioural test artifacts PASS under both standards (real code paths, no mocks):
  - Conceptual P3 (`TimeframeStateTests`): nine explicit stream identities; M1 failure leaves M15
    unchanged; M15 recovery does not reset M1/H4; W1 outage does not alter H1; per-stream sequence;
    duplicate finalized bar and same-close_time repaint rejected; future-dated and forming bars
    rejected (no lookahead/no repaint); per-stream quality; deterministic progress; provenance.
  - Conceptual P4 (`AnalysisPipelineTests`): deterministic bounded features with `based_on`
    provenance; forming bar refuses computation; empty/unknown input -> explicit invalid; H4
    structural authority (M15/M1 refused); deterministic bounded regime (ranking, not probability);
    macro context only D1/W1/MN1; degraded/stale -> INELIGIBLE with reasons; market quality never
    fabricates GOOD.
  - Conceptual P5 (`SignalPipelineTests`): deterministic symbol-sensitive V3-23 identity; eligibility
    gates the signal; M15 trigger under H4 structure; score/confidence deterministic and zero on
    non-VALID data; risk gate refuses unusable quality / zero ATR; shadow fills/positions
    `is_live == false`; attempted live execution blocked (`SHADOW_ONLY`); append-only idempotent
    ledger with deterministic identity and preserved provenance.
  - MT5 (`Mt5ReceiverTests`): encode/decode round-trip; nine-stream routing by explicit timeframe;
    malformed / unknown-timeframe / checksum-mismatch / future-dated rejection; duplicate and
    regression rejection; M1-failure/H4-unaffected; M1-recovery/H4-not-reset; W1-outage/H1-unaffected;
    deterministic per-stream progress.
- MQL5 static audit PASS: exactly one physical EA; nine explicit timeframe labels in canonical order;
  `AURA_STREAM_COUNT == 9`; single shared transport; zero `OrderSend`/`PositionModify`/`CTrade`
  tokens; includes resolve; brackets balanced; `OnInit`/`OnTick`/`OnTimer`/`OnDeinit` present.
- Repository static audit: no include cycles; correct layer direction; no duplicate class
  definitions; no live-execution tokens in C++; no secrets.

Interpretation:
The conceptual Phase 3/4/5 gates PASSED (verified against manifest Phase 1, which they map to per
`PHASE_RECONCILIATION.md`). The MT5 one-EA/nine-stream boundary is implemented to the statically
verified level. MetaEditor/MT5 are NOT available, so MQL5 compilation and real terminal connectivity
are UNPROVEN. Live trading is NOT enabled. Compilation and structural checks support but do not prove
architectural correctness; no profitability, calibration, broker-validation or production-safety
claim is made.


## 2026-10-02 - Canonical Phase 3 (Self-Learning) + Phase 4 (Research Plane)

Toolchain: `g++ (Debian 14.2.0-19) 14.2.0`, `-std=c++17` and `-std=c++20`,
`-Wall -Wextra -Werror -pedantic`. Python 3 static audits.

Promotion: canonical Phases 3-11 were promoted into active scope by the `DECISIONS.md` entry
"2026-10-02 - Session authorization: promote canonical Phases 3-11 into active scope".

Phase 3 (Self-Learning, `src/learning/`): KnowledgeObject, KnowledgeStore, KnowledgeLifecycle,
ContextLearning, ContradictionEngine, KnowledgeDecay, FailureMemory, LearningTests.
- Build/test: `g++ -std=<std> -Wall -Wextra -Werror -pedantic -I src src/learning/LearningTests.cpp
  src/foundation/Hasher.cpp -o t && ./t` -> PASS c++17/c++20.
- Proven: point-in-time consistency (assessment cannot precede observation; future evidence
  rejected); versioned append-only store (no silent overwrite, no rewind, idempotent, conflicting
  revision rejected, history retained); explicit lifecycle only; context learning excludes
  observations whose outcome postdates the assessment and counts the exclusion (no future-label
  leakage); contradiction preserved as a first-class object with order-independent identity;
  decay/revalidation never erases; failure memory excludes not-yet-known outcomes and aggregates
  deterministically.

Phase 4 (Research Plane, `src/research/`): Hypothesis, Experiment, ExperimentFingerprint,
ResearchBudget, ExperimentLedger, ResearchPlanner, ResearchSandbox, ResearchTests.
- Build/test: `g++ -std=<std> ... src/research/ResearchTests.cpp src/foundation/Hasher.cpp` -> PASS
  c++17/c++20.
- Proven: falsifiability mandatory; fingerprint deterministic, ignores transient fields
  (timestamp/result), changes with identity-relevant fields; append-only ledger with idempotent
  re-record, duplicate-fingerprint detection, failed experiments retained; budget GREEN/YELLOW/RED/
  FROZEN with no overspend/partial spend; validation protocol fixed before evaluation and immutable
  within a family; sandbox never mutates runtime and admits only recorded hypotheses with a fixed plan.

Header self-containment + combined TU verified for all Phase 3/4 headers under both standards.
Interpretation: Phase 3 and 4 satisfy their V3-40 capability sets to the statically verified,
behaviourally tested level. No live path; no profitability/calibration claim; research cannot mutate
runtime.


## 2026-10-02 - Canonical Phase 5 (Evolution) / Phase 6 (Validation) / Phase 7 (Governance)

Toolchain: `g++ (Debian 14.2.0-19) 14.2.0`, `-std=c++17`/`-std=c++20`,
`-Wall -Wextra -Werror -pedantic`.

Phase 5 (`src/evolution/`): Candidate, CandidateRegistry, EvolutionGraph, CandidateComparison,
EvolutionTests -> PASS c++17/c++20. Proven: explicit candidate lifecycle with no hidden transitions
(cannot reach PROMOTED without human-approval path); append-only registry retains history and refuses
illegal jumps; matched comparison yields INSUFFICIENT_EVIDENCE when unmatched and VETOED on a failed
hard constraint; evolution graph acyclic (unknown parent rejected) with reproducible lineage.

Phase 6 (`src/validation/`): ValidationFirewall, EvidenceFirewall, EvaluatorFirewall,
RewardHackingDefense, StatisticalControls, ValidationTests -> PASS c++17/c++20. Proven: missing check
is NOT_RUN not PASS; any FAIL is NOT_CREDIBLE; holdout budget enforced, malformed request refused, no
contamination on refused query; contamination is monotone (no silent reset); evaluator-family mismatch
refused; hard constraints veto a high composite metric; missing statistical evidence is not passing.

Phase 7 (`src/governance/`): PolicyEngine, ForbiddenBehavior, PromotionGate, HumanDecision,
AuditLedger, GovernanceTests -> PASS c++17/c++20. Proven: policy by change type (risk/evaluator/
research-method need human approval); forbidden behaviors rejected (only a trusted maintainer may
request governed deployment actions, never the integrity anchors); promotion requires every gate incl.
human approval and a shadow-stage candidate, with no self-promotion; automated actors cannot record an
ACCEPTED human decision; audit ledger append-only with no mutation API.

Header self-containment + combined TU verified for all Phase 5/6/7 headers under both standards.
Interpretation: Phases 5-7 satisfy their V3-40 capability sets to the statically verified,
behaviourally tested level. No live path; no profitability/calibration claim; governance cannot
self-promote or mutate runtime.


## 2026-10-02 - Canonical Phases 8-11 (Operating Window, Desktop, Telegram, Real-World Validation)

Toolchain: g++ 14.2.0, -std=c++17/-std=c++20, -Wall -Wextra -Werror -pedantic.
Phase 8 (src/operatingwindow/): OperatingWindow, Checkpoint, ResourceBudget, WindowOrchestrator,
OperatingWindowTests -> PASS c++17/c++20. Proven: human-bounded 3-8h window with self-extension
refused; automated sub-minimum schedule refused; phase transitions ACTIVE/DRAINING/OFFLINE;
recovery prefers a valid compatible checkpoint else known-good and never guesses; corrupt/incompatible
checkpoints unusable; no partial resource spend; uncheckpointed resumable work refused, not discarded.
Phase 9 (src/desktop/): ControlCenterViewModels, DashboardProjector, DesktopTests -> PASS
c++17/c++20. Proven: read-only projections from trusted layers; source values unchanged after
projection; an UNKNOWN tile is surfaced as unknown, never fabricated as healthy; deterministic
explicit-order graph projection. Headless, no GUI toolkit.
Phase 10 (src/telegram/): TelegramConfig, TelegramMessage, ApprovalRequest, TelegramGateway,
NotificationPolicy, TelegramTests -> PASS c++17/c++20. Proven: config is secret-free (no embedded
token, not configured until a human sets it); a disabled gateway is a safe no-op and is never a core
dependency; informational messages only; the approval pipeline enforces all six verification steps
and any failure stops before promotion; ERROR/INCIDENT/ROLLBACK/APPROVAL_NEEDED notifications cannot
be suppressed.
Phase 11 (src/realworld/): RealWorldValidation, ValidationRegistry, RealWorldTests -> PASS
c++17/c++20. Proven: all readiness gates required (missing = unmet, never assumed); never automatic;
explicitly makes no profitability/production-safety claim; append-only validation-run registry.
Full-suite regression: all 16 test suites PASS under both c++17 and c++20.
Interpretation: Phase 11 is a gate/registry only. No live trading path is enabled; real MT5
execution readiness remains UNPROVEN in this environment (no MetaEditor/MT5 on Linux).

## 2026-10-02 - Phase 12 Integration / Application (gap-audit closure)
Toolchain: g++ 14.2.0; CMake (pip) configure/build; -std=c++17 and -std=c++20,
-Wall -Wextra -Werror -pedantic (direct) / -Wall -Wextra -Wpedantic (CMake).
New sources under test: src/runtime/ApplicationPipeline.h (APP-0001),
src/runtime/ApplicationShell.h (APP-0002), tools/run_pipeline.cpp (APP-0003),
src/runtime/EndToEndTests.cpp (E2E-0001), CMakeLists.txt (BUILD-0001).
Results:
- CMake configure + build: SUCCESS (static lib aura_foundation, 17 test targets, aura executable).
- CTest: 17/17 tests PASSED (including EndToEndTests).
- Direct strict compile of all 17 suites: ALL PASS under both c++17 and c++20.
- New headers self-contained (standalone TU) under c++17 and c++20.
- EndToEndTests (real loopback TCP, no mocks): proves nine-stream availability, explicit-timeframe
  routing, closed-bar acceptance, duplicate/malformed/future-dated rejection, per-stream isolation,
  deterministic decision/proposal identity and ledger size across identical runs, and a strictly
  shadow-only path (proposal.is_order == false).
- aura --replay of 360 real canonical frames: 9/9 streams HEALTHY, 360 accepted, 0 rejected,
  0 malformed, 38 signals, 38 risk proposals, 38 shadow fills, 151 ledger entries, aggregate ONLINE.
- aura --serve smoke test over a real socket: same counts; clean SIGTERM shutdown (no hang).
- Live-lifecycle fix verified: accept() is now interruptible (select with timeout), so the host loop
  observes shutdown without a client connected.
Interpretation: This closes the audit's integration P0 for the environment-independent parts. A real
end-to-end MT5 -> analysis -> shadow path now exists and is proven over a real socket, but a real MT5
terminal round-trip, a native Windows GUI, and a historical dataset campaign remain BLOCKED (no
toolchain/terminal/dataset here). No profitability, calibration, broker-validation, production-safety
or live-trading claim is made.

## 2026-10-03 — Phase 13: durable persistence + crash recovery + packaging/CI

Task/Phase: PERSIST-0001, PERSIST-0002, BUILD-0002 (Phase 13 Integration).

Build command:
`cmake -S . -B <build> -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17|20 && cmake --build <build> -j4`

Build result: PASS for both C++17 and C++20 (g++, `-Wall -Wextra -Wpedantic`), including the new
`aura`, `PersistenceTests`, and all pre-existing targets. Header-only code also passes
`-Wall -Wextra -Werror -pedantic -fsyntax-only` under both standards.

Test command: `ctest --test-dir <build> --output-on-failure`

Test result: `100% tests passed out of 18` under C++17 AND under C++20 (the count rose from 17 to 18
with the new `PersistenceTests`). `PersistenceTests` covers, over real code paths (no mocks):
- append idempotency (identical re-append = OK, conflicting payload = CONFLICT);
- atomic flush + reload round-trip preserving records, digests and payloads;
- byte-identical artifacts for identical append sequences (determinism);
- tampered byte -> `CORRUPT`; checksum-less/truncated file -> `CORRUPT` (never silently OK);
- V2-36 lifecycle classification (CLEAN_SHUTDOWN / EXPECTED_PAUSE / INTERRUPTED_WORK /
  CORRUPTED_STATE / UNKNOWN_STATE), version-incompatible -> refused, corrupted -> refused,
  uncheckpointed interruption -> refused;
- application restart over real encoded frames: a fresh shell restores nine timeframe states and the
  ledger, accepts strictly-newer bars (no repaint), and re-persist is idempotent.

Real host verification (not compilation alone):
- `aura --serve 12001 --store /tmp/serve.aura`: a Python client sent all 360 canonical frames over a
  real loopback socket; on Ctrl-C the process persisted and printed: transport connected, 9/9 streams
  healthy, 360 accepted, 0 rejected, 0 malformed, 38 signals / 38 risk proposals / 38 shadow fills,
  151 ledger entries, HEALTHY (ONLINE), SHADOW ONLY, persisted store 161 records. This matches the
  Phase 12 replay result exactly, so persistence did not perturb the pipeline.
- `aura --replay /tmp/frames.txt --store /tmp/replay.aura`: 360 frames, 9/9 streams, 360 accepted,
  38/38/38, 151 ledger entries, HEALTHY, persist OK (161 records).
- `aura --recover /tmp/replay.aura` (good store) -> lifecycle CLEAN_SHUTDOWN, resumable yes, exit 0.
  After flipping one payload byte, `--recover` -> CORRUPTED_STATE, resumable no, exit 1.
- `aura --self-test` -> PASS: 30 deterministic steps x 9 streams -> 270 accepted, 28/28/28, 111 ledger
  entries; persist OK (121 records); recovery CLEAN_SHUTDOWN resumable; restores 9 timeframes + 111
  ledger entries.
- `cmake --install <build> --prefix <dir>` -> installs `bin/aura`, `include/aura/**.h`, and the doc.

Files changed: `src/foundation/FilePersistenceStore.h` (new), `src/runtime/ApplicationRecovery.h` (new),
`src/runtime/PersistenceTests.cpp` (new), `src/mt5/Mt5StreamManager.h`, `src/runtime/ApplicationPipeline.h`,
`src/runtime/ApplicationShell.h`, `tools/run_pipeline.cpp`, `CMakeLists.txt`,
`.github/workflows/ci.yml` (new), `README.md`, and the control-plane files.

Known failures: none. Remote CI on PR #1 now has ALL FOUR jobs green: Linux gcc C++17 and C++20
(build + ctest + smoke + install) and Windows MSVC C++17 and C++20 (build + 18/18 ctest). This proves
the console host compiles, links (after the `ws2_32` fix) and passes its full suite under MSVC. Two
real portability defects were found by CI and fixed rather than hidden: (1) `ws2_32` (Winsock) was not
linked on Windows (`LNK2019 __imp_socket/...`); (2) `std::rename` does not overwrite an existing file
on Windows, so the atomic flush now clears the target before renaming (same-directory swap). Each fix
was re-verified locally (Linux 18/18 unaffected) before pushing. The Windows/MSVC CI job remains
non-blocking by policy, but it is currently passing.

Interpretation: durable persistence and the V2-36 crash-recovery decision now exist in the running
application and are verified by unit tests and a real socket run. A corrupted or version-incompatible
store is refused, never silently resumed; shadow-only behaviour is preserved (no proposal is ever an
order). This does NOT establish profitability, calibrated probability, broker validation, production
safety, or live-trading readiness.

## Phase 14 - canonical Windows x64 Release package (2026-10-03)

- Date: 2026-10-03
- Commit: 64ce035 (merged into the branch; CI run 37115746002)
- Task/Phase: BUILD-0003 / Phase 14
- Build command (CI, windows-latest): `cmake -S AURA -B AURA/build -A x64 -DCMAKE_BUILD_TYPE=Release
  -DCMAKE_CXX_STANDARD=17` then `cmake --build AURA/build --config Release -j 4`
- Build result: PASS. `aura.exe` produced (Release, x64, MSVC, static CRT) = 440,320 bytes.
- Test command: `ctest --test-dir AURA/build -C Release --output-on-failure` plus a PowerShell smoke:
  `aura.exe --self-test --store <p> --keep`; `aura.exe --recover <p>`; `aura.exe --dump-frames <f>`;
  `aura.exe --replay <f> --store <p>`; a bounded `--serve 12345` process check (single instance + TCP
  connect + forced stop); package + secret scan + ZIP verification (extract and re-run the packaged exe).
- Test result: PASS. Full suite 18/18; self-test PASS (9 timeframes, 111 ledger entries restored);
  cross-process recover => CLEAN_SHUTDOWN/resumable; replay 270 frames => HEALTHY, 111 ledger entries,
  last proposal is not an order; serve: started, stayed up, exactly 1 aura process, accepted a TCP
  connection, stopped cleanly (no immediate crash); secret scan OK; ZIP contains aura.exe and the
  extracted exe re-runs self-test PASS.
- Names/sizes: `aura.exe` = 440,320 bytes; `AURA_Windows_x64_Release.zip` = 220,103 bytes. Artifacts:
  `aura-windows-x64-exe` (217,039 bytes as uploaded) and `AURA_Windows_x64_Release` (220,131 bytes).
- Files changed: `tools/run_pipeline.cpp`, `CMakeLists.txt`, `.github/workflows/ci.yml`, and the
  control-plane files.
- Known failures: none.
- Interpretation: the canonical AURA executable now exists as a real, self-contained Windows x64
  Release `.exe` and is packaged and published as a CI artifact. Its startup/init/single-instance/
  clean-stop and its persistence/recovery/replay paths are verified on a real Windows runner by a
  bounded smoke. The native Windows GUI is NOT included and its runtime is UNPROVEN. MT5/MetaEditor is
  NOT exercised. No live order path; shadow only. No profitability/calibration/production-safety claim.

## Future test entry format

- Date
- Commit
- Task/Phase
- Build command
- Build result
- Test command
- Test result
- Files changed
- Known failures
- Interpretation
