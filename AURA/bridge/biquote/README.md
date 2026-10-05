# AURA — Biquote Bridge (real XAUUSD candles)

A small Python bridge that pulls **real XAUUSD candles** from
[`biquote`](https://pypi.org/project/biquote/) — a market-data client serving an
MT5 feed — into JSON that AURA's desktop app can read.

This bridge is **independent of the AURA C++ core**. It modifies nothing in
`AURA/src/`, `AURA/tools/`, CMake, CI, or the MQL5 EA, and it places no orders.

---

## ⚠️ Two real limitations you must know

Both were found by probing the live API (biquote 0.3.0), not assumed:

### 1. Weekly and monthly candles are not available

The server accepts exactly these intervals:

```
1m   5m   15m   30m   1h   4h   1d
```

- `1w` → HTTP 400 `Invalid interval '1w'`
- `1M` → **silently returns ONE MINUTE** of data. Verified byte-identical to the
  `1m` series. Writing that as "MN1" would be fabricated monthly data.

So `W1` and `MN1` are written with `count: 0` and an explicit
`unavailable_reason`. They are never approximated, resampled, or relabelled.
Seven of nine timeframes work; **M15, the operational timeframe, is covered.**

### 2. The feed serves MT5 "Broker 1", not your broker

`source` is `MetaTrader 5 (Broker 1)`. This is *not* your Valetax account. Prices
are real market data from a different feed, with different liquidity and spread.

### 3. The feed carries one defective daily bar

The D1 series contains a duplicated bar identity (`2026-05-22`) carrying two
*different* bodies. AURA identifies a bar by `(symbol, timeframe, close_time)`,
so both conflicting bars are dropped and the count is reported, rather than
arbitrarily picking a winner. This costs 1 bar out of 499.

---

## Requirements

- Python 3.9+ (`biquote` declares `requires_python >=3.9`)
- Internet access to the biquote feed
- **No MetaTrader terminal and no broker login required** — unlike the MT5
  bridge in `../mt5_python/`, this one talks to a hosted feed over HTTPS and
  therefore runs on Linux, macOS or Windows.

```sh
pip install -r requirements.txt
```

---

## Run it

```sh
python connect_test.py    # live quote + 5 closed M15 candles
python read_candles.py    # 9 timeframes -> out/candles_<LABEL>.json + index.json
python read_news.py       # -> out/news.json
python read_calendar.py   # -> out/calendar.json
python run_all.py         # everything, with a summary
```

Every script exits non-zero with a specific code on failure:

| Script | Codes |
|---|---|
| `connect_test.py` | `0` ok · `1` library missing · `2` no quote · `3` no closed candles |
| `read_candles.py` | `0` ok · `1` library missing · `2` no quote · `3` no M15 |
| `read_news.py` | `0` always — news is auxiliary |
| `read_calendar.py` | `0` always — the calendar is auxiliary |
| `run_all.py` | `0` candles produced · `1` library missing · `2` no candles |

News and calendar failures **never** abort the run.

---

## Symbol detection

The symbol is never hardcoded. `resolve_symbol()` calls `bq.symbols()`, filters
names containing `XAU` (case-insensitive), prefers an exact `XAUUSD` match, and
otherwise takes the first match — logging every candidate. On the Valetax feed
this list includes `BTCXAU, XAUUSDT, XAUUSD_HRM, XAUUSD_HKN, XAUGBP, XAUEUR,
XAUAUD, XAUUSD, XAUJPY, XAUEUR_HKN`, and the exact match `XAUUSD` wins.

---

## Data guarantees

- **Closed bars only.** Any candle with `isOpen == true` is discarded.
- **Timestamps** are ISO 8601 UTC converted to epoch seconds, otherwise verbatim.
  No resampling, interpolation, or timezone shifting.
- **No fabrication.** An unavailable timeframe is `count: 0` with a reason, and
  `null` timestamps/prices — never a placeholder.
- **Interval aliasing is detected.** A median bar-spacing check refuses to write
  a series whose bars are systematically closer together than the label implies.
- **Conflicting bar identities are dropped**, and reported.

---

## Output

```
out/candles_M1.json  candles_M5.json  candles_M15.json
out/candles_M30.json candles_H1.json  candles_H4.json
out/candles_D1.json  candles_W1.json  candles_MN1.json
out/index.json  out/news.json  out/calendar.json
```

`out/candles_M15.json`:

```json
{
  "symbol": "XAUUSD",
  "timeframe": "M15",
  "source": "biquote (MetaTrader 5 (Broker 1))",
  "interval": "15m",
  "generated_at_utc": 1791207100,
  "count": 192,
  "unavailable_reason": "",
  "candles": [
    { "time": 1791201600, "open": 4156.415, "high": 4159.287,
      "low": 4154.06, "close": 4157.094, "tick_volume": 2054 }
  ]
}
```

`out/index.json` adds `status` (`OK` / `NOT_AVAILABLE`) and `note` per timeframe.

---

## How AURA consumes this

`src/desktop/BiquoteCandles.h` reads these files when they exist and reports
`using_real_data = true`; when they are absent AURA falls back to its existing
synthetic feed, unchanged. The chart then shows a small
`LIVE (Biquote)` / `SYNTHETIC (no Biquote)` label near the instrument header so
an operator always knows which source produced the candles on screen.

The app searches, in order:

1. `$AURA_BRIDGE_DIR/out/`
2. `<exe_dir>/../../../bridge/biquote/out/`
3. `./bridge/biquote/out/`
