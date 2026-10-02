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

## 2026-10-02 — MT5 one-EA / nine logical timeframe stream deployment decision

- **Date:** 2026-10-02
- **Decision:** The physical MT5 deployment model is ONE MQL5 Expert Advisor attached to ONE
  chart using ONE transport connection, carrying NINE logically independent timeframe streams
  (M1, M5, M15, M30, H1, H4, D1, W1, MN1). The nine logical streams keep independent identity,
  symbol identity, closed-bar identity, bar timestamp, capture/receive timestamp, sequence,
  data-quality state, stream health, provenance, last successful update, last error, and
  independent closed-bar tracking. One stream failing must not cause the other streams to be
  reported as unhealthy; one stream recovering must not overwrite another stream's state. Each
  message/event must carry explicit timeframe identity; positional multiplexing (e.g. "first
  message = M1") is forbidden unless the protocol contract makes identity explicit and stable.
  The canonical timeframe authority hierarchy (V3-29 / §4) is unchanged: H4 = primary
  structural authority, M15 = primary operational setup/trigger, M5/M1/TICK = execution /
  microstructure context, D1/W1/MN1 = long-horizon context.
- **Rationale:** The earlier design implied nine physical `.mq5` files (one per timeframe). The
  human project owner approved a one-physical-EA deployment that preserves the nine canonical
  logical stream responsibilities. This is an implementation/deployment decomposition decision,
  not an architectural change: the nine logical adapters/streams remain canonical (V3-29); only
  the physical packaging changes. The existing task graph already used a single adapter boundary
  (`RT-0001 AdapterManager.h`) and did not enumerate nine physical `.mq5` tasks, so no task
  replacement was required; the affected task contracts were refined to encode the nine-stream
  identity requirements.
- **Affected tasks:** `RT-0001 AdapterManager.h` (adapter boundary must carry nine explicit stream
  identities with independent health and closed-bar state), `RT-0021 Mt5Boundary.md` (boundary
  contract must require explicit per-stream timeframe identity and forbid positional
  multiplexing). No new source tasks are created by this decision.
- **Scope-change flag:** No. This is a deployment/decomposition decision within the
  already-authorized scope; it promotes no deferred phase and does not modify Master V3.
- **Master V3 status:** No modification required. V3-29 ("Nine MT5/MQL5 timeframe adapters are
  canonical") is satisfied by nine logical stream responsibilities under one physical EA.

## Decision format

For each future material decision record:

- date
- decision
- rationale
- affected phases/tasks
- whether it changes scope or architecture
