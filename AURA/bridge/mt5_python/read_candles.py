#!/usr/bin/env python3
"""AURA MT5 Python bridge - read closed candles for all nine timeframes.

For each canonical timeframe it reads the most recent CLOSED bars (start=1 skips
the still-forming bar), and writes one JSON file per timeframe under ./out plus a
summary ./out/index.json. Read-only: no order is ever placed.

Exit code 0 when at least the M15 timeframe succeeded.

Requires: Windows + a running MetaTrader 5 terminal + `pip install -r requirements.txt`.
"""

import json
import os
import sys
import time

try:
    import MetaTrader5 as mt5
except ImportError:  # pragma: no cover - only reachable off the target host
    print("ERROR: the MetaTrader5 package is not installed.")
    print("Install it with: pip install -r requirements.txt")
    sys.exit(1)

# start=1 excludes the currently-forming bar: CLOSED BARS ONLY (no repaint).
START_POS = 1
CANDLE_COUNT = 500

# Canonical nine timeframe streams, in V3-29 order (M1 .. MN1).
TIMEFRAMES = [
    ("M1", mt5.TIMEFRAME_M1),
    ("M5", mt5.TIMEFRAME_M5),
    ("M15", mt5.TIMEFRAME_M15),
    ("M30", mt5.TIMEFRAME_M30),
    ("H1", mt5.TIMEFRAME_H1),
    ("H4", mt5.TIMEFRAME_H4),
    ("D1", mt5.TIMEFRAME_D1),
    ("W1", mt5.TIMEFRAME_W1),
    ("MN1", mt5.TIMEFRAME_MN1),
]

OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "out")


def pick_xau_symbol(symbols):
    """Pick the best XAUUSD candidate: exact > suffixed (.vx/.vcn) > first XAU."""
    names = [s.name for s in symbols if "XAU" in s.name.upper()]
    if not names:
        return None
    for name in names:
        if name.upper() == "XAUUSD":
            return name
    for name in names:
        upper = name.upper()
        if upper.startswith("XAUUSD") and ("VX" in upper or "VCN" in upper):
            return name
    return names[0]


def _candle_dict(row):
    """Normalise one MT5 rate row (numpy structured record or tuple) to a dict."""
    try:  # structured record: field access
        t = row["time"]
    except (TypeError, ValueError, IndexError):  # plain tuple: positional
        return {
            "time": int(row[0]),
            "open": float(row[1]),
            "high": float(row[2]),
            "low": float(row[3]),
            "close": float(row[4]),
            "tick_volume": int(row[5]),
            "spread": int(row[6]),
            "real_volume": int(row[7]),
        }
    return {
        "time": int(row["time"]),
        "open": float(row["open"]),
        "high": float(row["high"]),
        "low": float(row["low"]),
        "close": float(row["close"]),
        "tick_volume": int(row["tick_volume"]),
        "spread": int(row["spread"]),
        "real_volume": int(row["real_volume"]),
    }


def _write_json(path, payload):
    with open(path, "w", encoding="utf-8") as fh:
        json.dump(payload, fh, separators=(",", ":"))


def main():
    if not mt5.initialize():
        print("ERROR: mt5.initialize() failed:", mt5.last_error())
        return 1

    try:
        account = mt5.account_info()
        if account is None:
            print("ERROR: mt5.account_info() returned None:", mt5.last_error())
            return 1
        broker = account.company
        login = int(account.login)

        symbols = mt5.symbols_get() or []
        symbol = pick_xau_symbol(symbols)
        if not symbol:
            print("ERROR: no XAU symbol available on this terminal.")
            return 1
        if not mt5.symbol_select(symbol, True):
            print("ERROR: mt5.symbol_select(%s, True) failed: %s" % (symbol, mt5.last_error()))
            return 1
        print("symbol=%s broker=%s login=%d" % (symbol, broker, login))

        os.makedirs(OUT_DIR, exist_ok=True)
        generated_at = int(time.time())

        index = {
            "symbol": symbol,
            "broker": broker,
            "account_login": login,
            "generated_at_utc": generated_at,
            "timeframes": [],
        }
        m15_ok = False

        for label, tf in TIMEFRAMES:
            rates = mt5.copy_rates_from_pos(symbol, tf, START_POS, CANDLE_COUNT)
            if rates is None or len(rates) == 0:
                print("WARN: %s returned no candles: %s" % (label, mt5.last_error()))
                index["timeframes"].append({"timeframe": label, "count": 0, "file": None})
                continue

            candles = [_candle_dict(r) for r in rates]
            payload = {
                "symbol": symbol,
                "timeframe": label,
                "broker": broker,
                "account_login": login,
                "generated_at_utc": generated_at,
                "count": len(candles),
                "candles": candles,
            }
            filename = "candles_%s.json" % label
            _write_json(os.path.join(OUT_DIR, filename), payload)
            index["timeframes"].append({"timeframe": label, "count": len(candles), "file": filename})
            print("wrote %s: %d closed candles" % (filename, len(candles)))
            if label == "M15":
                m15_ok = True

        _write_json(os.path.join(OUT_DIR, "index.json"), index)
        print("wrote index.json")

        if m15_ok:
            print("READ CANDLES: PASS")
            return 0
        print("READ CANDLES: FAIL (M15 unavailable)")
        return 1
    finally:
        mt5.shutdown()


if __name__ == "__main__":
    sys.exit(main())
