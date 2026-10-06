# AURA — MT5 Python Bridge

Read-only bridge that exports real XAUUSD candles from a local MetaTrader 5
terminal into JSON files that the AURA desktop app can load (opt-in). It never
places, modifies or closes an order. The system remains **shadow-only**.

## Requirements

- **Windows** with MetaTrader 5 installed and **logged in** (a running terminal).
- Python **3.11** (tested against 3.11.7) with the `MetaTrader5` package:

  ```powershell
  pip install -r requirements.txt
  ```

  `requirements.txt` pins `MetaTrader5>=5.0.6231`.

## Scripts

| Script | Purpose |
| --- | --- |
| `connect_test.py` | Smoke test: initialise MT5, print version + account (login/company/balance), list every `XAU` symbol, auto-pick the best XAUUSD candidate, print the last 3 closed M15 candles and the current bid/ask/spread. Exit 0 on success. |
| `read_candles.py` | For all nine timeframes (M1..MN1) read the most recent 500 **closed** bars and write `out/candles_<TF>.json` plus `out/index.json`. Exit 0 when M15 succeeded. |
| `run_all.py` | Runs `read_candles.main()` and prints a per-timeframe summary table. |

## Symbol auto-detection

Brokers often suffix the symbol (`XAUUSD.vx`, `XAUUSD.vcn`, `XAUUSDs`, ...).
The bridge prefers, in order:

1. an exact `XAUUSD`,
2. a `XAUUSD` with a `.vx` / `.vcn` suffix,
3. the first symbol containing `XAU`.

## Closed bars only

Every read uses `mt5.copy_rates_from_pos(symbol, tf, 1, N)`. `start=1` skips the
currently-forming bar, so only **closed** bars are exported. AURA's chart never
shows a forming bar and never repaints.

## Output format

`out/candles_<TF>.json`:

```json
{
  "symbol": "XAUUSD",
  "timeframe": "M15",
  "broker": "<account company>",
  "account_login": 12345678,
  "generated_at_utc": 1700000000,
  "count": 500,
  "candles": [
    {"time": 1700000000, "open": 2000.1, "high": 2001.2, "low": 1999.8,
     "close": 2000.9, "tick_volume": 1234, "spread": 20, "real_volume": 0}
  ]
}
```

`out/index.json` summarises all nine timeframes (symbol, broker, login,
`generated_at_utc`, and a `timeframes` list of `{timeframe, count, file}`).

`out/*.json` is git-ignored.

## Usage

```powershell
cd AURA\bridge\mt5_python
python connect_test.py     # verify the terminal link
python read_candles.py     # export candles for all nine timeframes
python run_all.py          # export + print the summary table
```

## How AURA consumes it

The desktop app (`aura_gui`) opts in via `ControlCenterState::enable_mt5_bridge(exe_dir)`.
It looks for the bridge output directory in this order:

1. `<exe_dir>/../../bridge/mt5_python/out`
2. the `AURA_BRIDGE_DIR` environment variable (point it at the `out` directory)
3. `./bridge/mt5_python/out` (relative to the working directory)

When candle JSON is present, the chart uses the real data; otherwise AURA falls
back to its existing synthetic source unchanged. The UI is identical either way
(no badge is added).

## Non-claims

This bridge does not claim a working MT5 round-trip on any host where it has not
been run, and it makes no profitability, calibration, broker-validation or
production-safety claim. It is a read-only data export.
