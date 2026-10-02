# AURA — Human Architectural Decisions

Only explicit human decisions belong here.

## 2026-10-02 — Initial control-plane decision

- The Master V3 document is the architectural reference.
- The Master is important but is not interpreted as “implement every line immediately”.
- Implementation is incremental and scope-controlled.
- GitHub is the persistent memory of the project.
- Different AI agents may be used during the project.
- Any AI must resume from repository state, not chat history.
- The current priority is to build the Master-described system incrementally.
- Existing repository code is reusable implementation material only when compatible.
- Initial active scope follows the Master MVP guidance rather than the full end-state architecture.
- Live automation is deferred during the initial build.
- MT5 is initially a market/data boundary; later execution capabilities require validation and explicit promotion.
- Deferred capabilities require explicit human promotion into active scope.

## 2026-10-02 — Phase 0 ErrorCode taxonomy decision (resolves V3-45 OPEN DECISION)

- **Date:** 2026-10-02
- **Decision:** The canonical Phase 0 `ErrorCode` taxonomy is the 13 ERROR categories listed in
  Master V3 section 113, represented as a strongly typed `enum class`:
  `DATA_ERROR`, `SCHEMA_ERROR`, `CLOCK_ERROR`, `CONNECTION_ERROR`, `BROKER_ERROR`, `RISK_ERROR`,
  `EXECUTION_ERROR`, `RECONCILIATION_ERROR`, `PERSISTENCE_ERROR`, `MODEL_ERROR`, `CONFIG_ERROR`,
  `TELEGRAM_ERROR`, `RECOVERY_ERROR`.
  Each code has an explicit, documented, stable integral identity (explicit numeric assignment in
  the canonical order above; identity does not depend on compiler enum ordering). The taxonomy is a
  closed canonical set: no `UNKNOWN`, `OTHER`, or additional categories are added, and adding or
  changing codes in future requires a new explicit architectural/project decision. `ErrorSeverity`
  remains an independent record-level field; `ErrorCode` does not embed or derive severity. The
  stable `ErrorCode` identity is the explicit numeric value; no serialization, persistence, network,
  JSON or database code is implied.
- **Rationale:** Master V3 section 113 names the categories but does not freeze them as a closed,
  numbered enumeration (V3-45 marks this an OPEN DECISION). V3-02 makes an explicit human
  architectural decision the highest source of truth, and V3-03/V3-11 forbid an implementation agent
  from silently freezing an OPEN DECISION. This decision supplies the missing authority so that
  `FND-0015`/`FND-0016` can proceed without inventing a taxonomy.
- **Affected tasks:** `FND-0015 ErrorCode.h` (unblocked), `FND-0016 ErrorRecord.h` (unblocked once
  its remaining dependency conditions are met); later error/audit/persistence/Guardian contracts that
  consume error identity.
- **Scope-change flag:** No. This freezes an existing OPEN DECISION and does not expand the active
  implementation scope; the Master V3 document is not modified.
- **V3-45 status:** This decision resolves the V3-45 OPEN DECISION for `FND-0015 ErrorCode.h`.

## Decision format

For each future material decision record:

- date
- decision
- rationale
- affected phases/tasks
- whether it changes scope or architecture
