# AURA — MT5 Runtime Boundary Contract

Task: `RT-0021` (Phase 1, deterministic runtime)
Status: implemented as a contract document (not an executable implementation)
Architecture reference: Master V3, V3-23 (identity), V3-24 (clock/causality),
V3-29 (timeframe authority), V3-32 (shadow), V3-33 (broker reality), V3-25 (data quality).
Approved deployment decision: one physical MT5 EA / one transport carrying nine
logically independent timeframe streams (see `project-control/DECISIONS.md`).

## 1. Purpose and scope

This document defines the boundary between the MetaTrader 5 (MQL5) side and the
C++ AURA runtime. It is a contract only. It does not prove a real connection and
does not define broker behaviour. Real connection and broker behaviour remain
unproven until measured (V3-33); no broker-specific value may be assumed here.

## 2. Physical deployment model (approved decision)

- One physical MT5 Expert Advisor runs on one chart.
- One transport carries the data for all nine canonical timeframe streams.
- The nine streams are logically independent. A single transport does not merge
  them: each message identifies its timeframe explicitly (section 4).
- There is no positional multiplexing: a message is never interpreted by the
  position of its fields or its arrival order. Timeframe identity is explicit.

The nine canonical timeframe streams, in V3-29 order, are:

```text
M1, M5, M15, M30, H1, H4, D1, W1, MN1
```

Analytical authority (V3-29) is preserved and is not a transport concern:

```text
M15      = primary operational / setup timeframe
H4       = primary structural authority
H1/M30   = intermediate context
M5/M1    = execution / microstructure context
D1/W1/MN1 = long-horizon context
```

## 3. Direction of flow

```text
MT5 EA (MQL5)  --market data / health-->  AURA C++ runtime
AURA C++ runtime --(shadow only, no live orders)--> ledger
```

In the initial operating target the C++ runtime is in SHADOW mode (V3-32). The
boundary carries no live order instruction. Any future order path is out of scope
for this phase and would require explicit governance approval (V3-17).

## 4. Message envelope (V3-23)

Every message across the boundary carries, at minimum:

```text
protocol_version
schema_version
message_id
timestamp
source
destination
timeframe        # explicit; one of the nine canonical labels; never positional
payload
checksum/hash    # where appropriate
```

The `timeframe` field is mandatory on every market-data message. A receiver must
reject or quarantine a market-data message whose timeframe is missing or is not
one of the nine canonical labels. This is what guarantees no positional
multiplexing.

## 5. Event metadata (V3-23, V3-24)

Every market event carries:

```text
event_id
event_type
symbol
source
source_instance
event_time
receive_time
sequence_id
schema_version
timeframe        # explicit identity of the stream that produced the event
```

`event_time` and `receive_time` are distinct and preserved. A receiver must
quarantine an event whose `event_time` is in the future relative to
`receive_time` (no lookahead, V3-24). `sequence_id` is per stream, not global.

## 6. Closed-bar identity and no-repaint (V3-24)

- Only closed bars cross into decision-making. A forming bar may be transported
  for display but must be marked not-closed and must not enter decisions.
- Closed-bar identity is deterministic: `symbol + timeframe + close_time`.
- A bar whose `close_time` is not strictly later than the last finalized bar for
  that timeframe is rejected. A bar is never repainted, re-finalized or replaced.

## 7. Independent per-stream state (MT5 one-EA decision, V3-13)

The receiver maintains independent state per timeframe:

```text
connection/health state
data-quality state
last processed closed bar
per-stream sequence
last error
```

- A failure or recovery of one stream must not change another stream's state.
- A message for one timeframe must never overwrite another timeframe's state.
- On failure, no data is fabricated: the affected stream is marked degraded or
  offline and downstream gating treats it as not usable (V3-25).

## 8. Data quality (V3-25)

The receiver assigns a canonical data-quality state to each bar
(`VALID, DEGRADED, INVALID, UNKNOWN, STALE, MISSING, OUT_OF_ORDER, DUPLICATE,
INCOMPLETE`). `UNKNOWN` is not a neutral success state. Quality propagates into
eligibility and decision gating, not merely into logs.

## 9. Rejection and quarantine

A receiver rejects or quarantines when metadata is unknown protocol, invalid
schema, duplicate, expired, future-dated, clock-anomalous, invalid payload, or
from an unknown source (V3-23). Rejection is recorded; it is not silently
dropped.

## 10. Idempotency (V3-23)

Delivery is assumed at-least-once. Duplicate `message_id` or duplicate
closed-bar identity must be idempotent: a re-delivered message must not create a
second bar, signal or ledger record.

## 11. Reconciliation (V3-33)

Assumed behaviour (cost model, slippage, fill assumptions) and observed broker
behaviour are kept distinct. Reconciliation precedes trust; a divergence is
surfaced, not assumed away. Broker profile values (spread, tick freshness, volume
constraints, margin, sessions, cost, latency) must be measured before they are
relied upon.

## 12. Explicit non-claims

This contract does not claim: a working real connection; broker validation;
execution equivalence; profitability; calibrated probability; or production
safety. Those require measured evidence (V3-55) and are out of scope for this
phase.

## 13. Implementation mapping

| Boundary concern | C++ implementation |
| --- | --- |
| Nine logical streams, independent per-stream state | `runtime/AdapterManager.h` (`RT-0001`) |
| Deterministic ordered market-event bus | `runtime/DataBus.h` (`RT-0002`) |
| Canonical data-quality assignment | `runtime/DataValidator.h` (`RT-0003`) |
| Closed-bar identity, no repaint | `runtime/BarFinalizer.h` (`RT-0004`) |
| Last-processed-bar persistence | `runtime/TimeframeStateStore.h` (`RT-0005`) |
| Shadow execution (no live orders) | `runtime/ShadowExecutionEngine.h` (`RT-0016`) |
| Reconciliation assumed vs observed | `runtime/ReconciliationEngine.h` (`RT-0018`) |
| Append-only ledger | `runtime/ShadowLedger.h` (`RT-0019`) |
| Deterministic replay | `runtime/ReplayEngine.h` (`RT-0020`) |

The MQL5 side of this boundary is not implemented in Phase 1 and is not owned by
any Phase 1 task. No `.mq5` file is invented here.
