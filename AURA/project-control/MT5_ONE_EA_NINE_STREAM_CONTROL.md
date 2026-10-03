# AURA — MT5 One-EA / Nine-Logical-Stream Control & Ownership

Status: ACTIVE
Date: 2026-10-02
Authority: `project-control/DECISIONS.md` (2026-10-02 "MT5 one-EA / nine logical timeframe stream
deployment decision" and "MT5 one-EA / nine-stream boundary implementation authorization").
Contract of record: `src/runtime/Mt5Boundary.md` (RT-0021).

## 1. Mandatory architecture

```text
PHYSICAL EA COUNT     = 1
LOGICAL STREAM COUNT  = 9
TRANSPORT COUNT       = 1
```

Logical streams (explicit identity, never positional):

```text
M1  M5  M15  M30  H1  H4  D1  W1  MN1
```

Authority hierarchy (V3-29), preserved:

```text
H4        = primary structural authority
M15       = primary operational / setup trigger
H1 / M30  = intermediate context
M5 / M1   = execution / microstructure context
D1/W1/MN1 = long-horizon context
TICK      = broker / execution health context only (not a directional timeframe)
```

There are **no** nine physical `.mq5` EAs. Packet position never defines stream identity; every
stream message carries an explicit timeframe label.

## 2. Module decomposition (one task = one primary output file)

### MQL5 side (`src/mt5/mql5/`) — single physical EA

| Task | File | Responsibility |
| --- | --- | --- |
| MT5-0001 | `src/mt5/mql5/Common.mqh` | Shared value types: explicit timeframe enum + `ENUM_TIMEFRAMES` mapping + label parse/format, message-type enum, closed-bar struct, stream health/quality enums, symbol/timeframe identity helpers. No logic beyond pure mapping. |
| MT5-0002 | `src/mt5/mql5/Protocol.mqh` | Deterministic message framing/encoding. Builds one frame per stream message carrying explicit protocol/schema version, message id, symbol, timeframe, sequence, event_time, receive_time, type, payload and a SHA-256 checksum (MQL5 `CryptEncode`). Identity is explicit, never positional. |
| MT5-0003 | `src/mt5/mql5/SocketClient.mqh` | The single shared transport: connect, send, receive, health, reconnect. No per-stream socket. Transport failure must not erase stream state. |
| MT5-0004 | `src/mt5/mql5/TimeframeStream.mqh` | One logical stream's independent state and closed-bar logic: last closed bar time/identity, per-stream sequence, initialization/recovery state, data-quality, health, provenance, last error, last emission; closed-bar detection; duplicate finalized-bar rejection; no forming bar emitted; no future timestamps. |
| MT5-0005 | `src/mt5/mql5/StreamManager.mqh` | Owns exactly nine `TimeframeStream` objects and the single `SocketClient`. Multiplexes all nine over the one transport; keeps streams isolated (one failing/recovering never mutates another); heartbeat and per-stream health. |
| MT5-0006 | `src/mt5/mql5/ShadowOrderGateway.mqh` | Shadow-only execution gateway. Produces only shadow/paper execution intents. Any live/broker execution request is explicitly **blocked/unavailable**. Contains no `OrderSend`, no position modification. |
| MT5-0007 | `src/mt5/mql5/AURA_MT5_EA.mq5` | The one physical EA entry point (`OnInit`/`OnTick`/`OnTimer`/`OnDeinit`). Wires one `StreamManager` (nine streams) and one `ShadowOrderGateway`. No second EA exists. |

### C++ side (`src/mt5/`) — receiver / codec

Paths follow the existing repository convention (`src/<layer>/*.h`, included as `"<layer>/..."`),
so C++ headers live directly in `src/mt5/` next to the `mql5/` sources.

| Task | File | Responsibility |
| --- | --- | --- |
| MT5-0008 | `src/mt5/ProtocolCodec.h` | Deterministic decoder/encoder counterpart of `Protocol.mqh`. Parses a frame into `foundation::MessageMetadata` + `foundation::EventMetadata` + `runtime::MarketBar`; validates protocol/schema, explicit timeframe, checksum, and rejects malformed/missing-timeframe/future-dated frames (no lookahead). Pure, no I/O. |
| MT5-0009 | `src/mt5/Mt5StreamManager.h` | Receiver stream manager. Routes decoded frames to the correct stream by explicit timeframe, updates per-stream state / `runtime::TimeframeStateStore`, rejects duplicates and regressions, tracks per-stream sequence/health. Stream-independent; no fabricated state. Depends only on `foundation`/`runtime`. |
| MT5-0010 | `src/mt5/Mt5ReceiverTests.cpp` | Receiver/codec behavioural tests: decode round-trip, nine-stream routing, malformed/missing-timeframe/wrong-timeframe/duplicate/future rejection, per-stream isolation, no-lookahead, determinism. |

`src/mt5/` layer direction: `src/mt5/*` may include `foundation`, `runtime`, `resilience`; no
lower layer includes `src/mt5/`. The MQL5 `.mqh`/`.mq5` files are not part of the C++ compile graph.

## 3. Message contract across the one transport

Every stream message frame carries, minimally and explicitly:

```text
protocol_version
schema_version
message_id
message_type          (BAR_CLOSED | HEARTBEAT | SYMBOL_SPEC | HEALTH | QUOTE | ERROR)
symbol
timeframe             (explicit; one of M1..MN1; never positional)
sequence_id           (per stream)
event_time            (bar close time; authoritative time)
receive_time          (separate; never substituted for event_time)
payload               (OHLCV for BAR_CLOSED)
checksum              (SHA-256 hex of the canonical pre-checksum frame)
```

Rules:
- A market-data frame with a missing/unknown timeframe is rejected/quarantined.
- A bar whose `event_time` (close time) is not strictly later than that stream's last finalized
  closed bar is rejected (duplicate / no repaint).
- A `receive_time` before `event_time` is a clock anomaly and is rejected.
- Delivery is at-least-once; duplicates are idempotent.

## 4. Shadow-only invariant

`ShadowOrderGateway.mqh` must expose a live-execution request that returns an explicit
blocked/unavailable result and performs no broker call. Static audit must find **zero**
`OrderSend`, `OrderSendAsync`, `PositionModify`, `PositionClose`, or trade-request constructors.

## 5. Verification level (to be recorded honestly)

MetaEditor / MetaTrader 5 is **not available** in the current Linux environment, so the MQL5 side
cannot be compiled here. Verification is therefore: (a) structural/static MQL5 checks, (b) contract
checks, (c) the C++ codec/receiver tests (which compile and run under c++17/c++20 strict).
Real terminal connectivity remains **UNPROVEN** until compiled in MetaEditor and run against a
terminal on Windows.
