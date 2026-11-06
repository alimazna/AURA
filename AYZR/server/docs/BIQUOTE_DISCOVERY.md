# BiQuote integration discovery report

Generated: 2026-11-06T15:46:08.796Z
Symbol under test: `XAUUSD`

This report is produced by `node src/biquote/discover.mjs`, which exercises the live
service. It records what was actually observed, including failures.

## 1. Endpoint probes

- `GET /api/XAUUSD` — OK
- `GET /api/symbols` — OK
- `GET /api/latest?symbols=XAUUSD` — OK

## 2. OHLC interval coverage (closed-bar flag observed)

| Timeframe | interval | bars | newest bar | isOpen | first bar | last bar |
|---|---|---|---|---|---|---|
| M1 | `1m` | 301 | 2026-11-06T15:46:00Z | true | 2026-11-06T10:46:00Z | 2026-11-06T15:46:00Z |
| M5 | `5m` | 289 | 2026-11-06T15:45:00Z | true | 2026-11-05T14:45:00Z | 2026-11-06T15:45:00Z |
| M15 | `15m` | 193 | 2026-11-06T15:45:00Z | true | 2026-11-04T13:45:00Z | 2026-11-06T15:45:00Z |
| M30 | `30m` | 193 | 2026-11-06T15:30:00Z | true | 2026-11-02T11:30:00Z | 2026-11-06T15:30:00Z |
| H1 | `1h` | 169 | 2026-11-06T15:00:00Z | true | 2026-10-28T07:00:00Z | 2026-11-06T15:00:00Z |
| H4 | `4h` | 181 | 2026-11-06T12:00:00Z | true | 2026-09-28T08:00:00Z | 2026-11-06T12:00:00Z |
| D1 | `1d` | 401 | 2026-11-06T00:00:00Z | true | 2025-07-24T00:00:00Z | 2026-11-06T00:00:00Z |
| W1 | `1d` | 401 | 2026-11-06T00:00:00Z | true | 2025-07-24T00:00:00Z | 2026-11-06T00:00:00Z |
| MN1 | `1d` | 401 | 2026-11-06T00:00:00Z | true | 2025-07-24T00:00:00Z | 2026-11-06T00:00:00Z |

## 3. Tick stream (SignalR hub)

- hub state: `negotiated` transports=WebSockets,ServerSentEvents,LongPolling
- hub state: `connected`
- hub state: `subscribed`
- ticks received in 12s: 47

## 4. Observed quote payload

```json
{
  "symbol": "XAUUSD",
  "description": "Gold vs US Dollar",
  "bid": 4050.002,
  "ask": 4050.17,
  "mid": 4050.086,
  "spread": 0.168,
  "last": 0,
  "high": 4106.978,
  "low": 4026.791,
  "direction": "DOWN",
  "dayDiffPercent": -0.3805,
  "source": "MetaTrader 5 (Broker 1)",
  "exchange": "OTC",
  "marketState": "open",
  "stale": false,
  "quoteAgeSeconds": 1,
  "timestamp": "2026-11-06T15:46:08Z",
  "lastQuoteAt": "2026-11-06T15:46:08Z"
}
```

## 5. Verdict

- REST quotes: VERIFIED
- Symbol catalogue: VERIFIED (2079 symbols)
- Batched latest: VERIFIED
- OHLC per timeframe: see table above (failures are marked NOT AVAILABLE, never filled with synthetic data)
- Tick stream: VERIFIED (47 ticks observed)

No API key was required for any of the above. No undocumented behaviour is assumed by the AYZR runtime.
