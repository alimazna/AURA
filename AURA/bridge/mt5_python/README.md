# AURA — MT5 Python Bridge (candle reader)

A standalone, read-only bridge that reads **closed XAUUSD candles** straight from
a running MetaTrader 5 terminal through the official MetaTrader5 Python library
and writes them as JSON that AURA can consume later.

This bridge is **independent of the AURA C++ core**. It modifies nothing in
`AURA/src/`, `AURA/tools/`, the MQL5 EA, CMake, or CI. It places no orders and
has no trading logic of any kind.

---

## ⚠️ Windows only

The `MetaTrader5` package is published **for Windows only** (wheels are
`win_amd64` for CPython 3.6–3.11). It also needs a **running MetaTrader 5
terminal that is already logged in**. Therefore:

- ✅ Run these scripts on your **Windows 10/11** PC with MT5 open.
- ❌ They cannot run on Linux or macOS — `pip install MetaTrader5` there fails
  with `No matching distribution found`. This is expected, not a bug.

A demo account is fine. This bridge only reads.

---

## Requirements

- Windows 10/11
- MetaTrader 5 terminal installed, running, and logged in
- `XAUUSD` (or a suffixed variant) visible in Market Watch
- Python 3.8+ with pip

If any of these is missing, `connect_test.py` tells you exactly which one.

---

## Run it

From this folder:

```bat
pip install -r requirements.txt
python connect_test.py
python read_candles.py
```

### Step 1 — `connect_test.py`

Prints the library version, account, terminal build, every symbol containing
`XAU`, the chosen symbol, and the last 3 closed M15 candles. Exits non-zero with
a specific code if anything is missing:

| Code | Meaning |
|---|---|
| 0 | OK |
| 1 | `MetaTrader5` library not installed |
| 2 | `mt5.initialize()` failed (terminal not running / not logged in) |
| 3 | no XAU-containing symbol found |
| 4 | symbol resolved but returned no closed M15 candles |

### Step 2 — `read_candles.py`

Reads all nine canonical timeframes and writes to `out/`:

```
out/candles_M1.json   candles_M5.json   candles_M15.json
out/candles_M30.json  candles_H1.json   candles_H4.json
out/candles_D1.json   candles_W1.json   candles_MN1.json
out/index.json
```

Exit 0 when the operational timeframe `M15` returned data; non-zero otherwise.

---

## The symbol problem

Valetax may expose XAUUSD as `XAUUSD`, `XAUUSD.vx`, `XAUUSD.vcn`, or another
suffix. **Neither script hardcodes a symbol name.** Both call
`mt5.symbols_get()`, filter names containing `XAU` (case-insensitive), prefer
an exact `XAUUSD` match, and otherwise take the first match — logging every
candidate and the one that was chosen.

---

## Data guarantees

- **Closed bars only.** Every read uses `copy_rates_from_pos(symbol, tf, 1, 500)`;
  `start_pos=1` excludes the currently-forming bar.
- **Last 500 closed candles** per timeframe, requested. If MT5 returns fewer
  (history depth limit, weekend gaps, fresh account), the **actual** count is
  reported — nothing is back-filled.
- **Timestamps are preserved exactly as MT5 returns them** (UTC epoch seconds).
  No resampling, no aggregation, no interpolation, no timezone shifting.
- **Nothing is ever fabricated.** A timeframe with no data is written with
  `count: 0` and `null` timestamps/prices, plus a `WARN: no candles for <TF>`
  line. One empty timeframe never aborts the run.

---

## JSON format

`out/candles_M15.json`:

```json
{
  "symbol": "XAUUSD",
  "timeframe": "M15",
  "broker": "Valetax Global Limited",
  "account_login": 12345678,
  "generated_at_utc": 1759680000,
  "count": 500,
  "candles": [
    {
      "time": 1759679100,
      "open": 3420.50,
      "high": 3428.30,
      "low": 3418.20,
      "close": 3425.80,
      "tick_volume": 52213,
      "spread": 20,
      "real_volume": 0
    }
  ]
}
```

`out/index.json`:

```json
{
  "generated_at_utc": 1759680000,
  "symbol": "XAUUSD",
  "broker": "Valetax Global Limited",
  "timeframes": {
    "M15": { "count": 500, "last_time": 1759679100, "last_close": 3425.80 }
  }
}
```

---

## Verify the output

```bat
python -c "import json,glob; [print(f, len(json.load(open(f))['candles'])) for f in sorted(glob.glob('out/candles_*.json'))]"
dir out
```

Expect 9 `candles_*.json` files plus `index.json` — 10 files total.

---

## Status

Syntax-checked only. The bridge has **never been executed against a live
MetaTrader 5 terminal**, because its only buildable platform (Windows) is not
available in the Linux environment where it was authored. Real symbol detection,
real candle counts and real broker metadata remain unverified until you run
step 1 and step 2 on your own Windows machine.
