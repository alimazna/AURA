#!/usr/bin/env python3
"""Step 2 - read closed XAUUSD candles for all nine canonical AURA timeframes
and write one JSON file per timeframe plus a combined index.

This script is READ-ONLY with respect to the broker account and to the AURA
project: it places no orders, writes only inside ./out/, and touches no file
outside this folder.

Windows only. MetaTrader5 (the official MQL5 Python connector) has no Linux or
macOS build, so this script can only run on a Windows host with the MetaTrader 5
terminal installed, running, and already logged in.

Data rules
    * nine timeframes: M1 M5 M15 M30 H1 H4 D1 W1 MN1 (M15 is the operational
      default used by AURA)
    * copy_rates_from_pos(symbol, tf, 1, 500) - start_pos=1 excludes the
      currently-forming bar, so every candle written is closed
    * timestamps are preserved exactly as MT5 returns them (UTC epoch seconds);
      no resampling, aggregation or interpolation
    * nothing is ever fabricated: an empty timeframe is reported as count 0,
      never filled with placeholder candles

Exit codes
    0  ran to completion and M15 (the operational timeframe) returned data
    1  MetaTrader5 library not installed
    2  mt5.initialize() failed, or the output folder is not writable
    3  no XAU-containing symbol could be resolved

Usage (Windows, from this folder)
    pip install -r requirements.txt
    python read_candles.py
"""

import json
import os
import sys
import time

try:
    import MetaTrader5 as mt5
except ImportError:
    sys.stderr.write(
        "MetaTrader5 library not found. Run: pip install MetaTrader5\n"
        "(MetaTrader5 is Windows-only; on Linux/macOS use the Wine-hosted "
        "Windows Python if you must, but a real Windows host is preferred.)\n"
    )
    sys.exit(1)


# The broker's exact XAUUSD name is not guaranteed: Valetax may expose
# "XAUUSD", "XAUUSD.vx", "XAUUSD.vcn", or another suffix. Resolve it, never
# hardcode it.
PREFERRED_SYMBOL = "XAUUSD"
SYMBOL_FILTER = "XAU"

# The nine canonical AURA timeframes, in canonical order.
TIMEFRAMES = (
    ("M1",  "TIMEFRAME_M1"),
    ("M5",  "TIMEFRAME_M5"),
    ("M15", "TIMEFRAME_M15"),
    ("M30", "TIMEFRAME_M30"),
    ("H1",  "TIMEFRAME_H1"),
    ("H4",  "TIMEFRAME_H4"),
    ("D1",  "TIMEFRAME_D1"),
    ("W1",  "TIMEFRAME_W1"),
    ("MN1", "TIMEFRAME_MN1"),
)

DEFAULT_TF = "M15"
CANDLE_COUNT = 500
OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "out")


def _xau_symbols():
    """Every symbol name the terminal knows that contains "XAU" (any case)."""
    groups = mt5.symbols_get()
    if groups is None:
        sys.stderr.write("ERROR: mt5.symbols_get() returned None. last_error=%r\n"
                         % (mt5.last_error(),))
        sys.exit(2)
    return [g.name for g in groups if SYMBOL_FILTER in g.name.upper()]


def resolve_symbol():
    """Prefer the exact name "XAUUSD"; otherwise take the first XAU match.

    Returns the chosen symbol name. Exits 3 when nothing matches, printing the
    full candidate list so the operator can see what the broker really offers.
    """
    names = _xau_symbols()
    if not names:
        all_names = [g.name for g in (mt5.symbols_get() or ())]
        sys.stderr.write(
            "ERROR: no symbol containing %r was found in the terminal.\n"
            "Total symbols known to the terminal: %d\n"
            "Fix: open Market Watch, confirm XAUUSD is listed (right-click > "
            "Show All), then re-run.\n"
            % (SYMBOL_FILTER, len(all_names))
        )
        sys.exit(3)

    exact = [n for n in names if n == PREFERRED_SYMBOL]
    chosen = exact[0] if exact else names[0]

    print("XAU symbols found (%d): %s" % (len(names), ", ".join(names)))
    print("Chosen symbol: %s (%s)"
          % (chosen,
             "exact %s match" % PREFERRED_SYMBOL if exact
             else "first XAU match; no exact %s symbol exists" % PREFERRED_SYMBOL))
    return chosen


def write_json(path, payload):
    """Write payload as JSON. Any filesystem problem is fatal and explicit."""
    try:
        with open(path, "w", encoding="utf-8") as handle:
            json.dump(payload, handle, indent=2)
            handle.write("\n")
    except OSError as exc:
        sys.stderr.write("ERROR: could not write %s (%s)\n" % (path, exc))
        sys.exit(2)


def read_timeframe(symbol, label, tf):
    """Return the candle list for one timeframe, or [] if MT5 returned nothing."""
    rates = mt5.copy_rates_from_pos(symbol, tf, 1, CANDLE_COUNT)
    if rates is None or len(rates) == 0:
        sys.stderr.write("WARN: no candles for %s (last_error=%r)\n"
                         % (label, mt5.last_error(),))
        return []
    return [
        {
            "time": int(r["time"]),
            "open": float(r["open"]),
            "high": float(r["high"]),
            "low": float(r["low"]),
            "close": float(r["close"]),
            "tick_volume": int(r["tick_volume"]),
            "spread": int(r["spread"]),
            "real_volume": int(r["real_volume"]),
        }
        for r in rates
    ]


def main():
    # -- connect ----------------------------------------------------------
    if not mt5.initialize():
        sys.stderr.write("ERROR: mt5.initialize() failed. last_error=%r\n"
                         "Fix: start MetaTrader 5 and log in to the account.\n"
                         % (mt5.last_error(),))
        sys.exit(2)

    try:
        account = mt5.account_info()
        if account is None:
            sys.stderr.write("ERROR: mt5.account_info() returned None. last_error=%r\n"
                             % (mt5.last_error(),))
            sys.exit(2)

        symbol = resolve_symbol()
        if not mt5.symbol_select(symbol, True):
            # Not fatal: a symbol can be readable even when it cannot be added
            # to Market Watch. Report it and continue with the read.
            sys.stderr.write("WARN: mt5.symbol_select(%r, True) returned False. "
                             "last_error=%r\n"
                             % (symbol, mt5.last_error(),))

        # -- output folder -------------------------------------------------
        try:
            os.makedirs(OUT_DIR, exist_ok=True)
        except OSError as exc:
            sys.stderr.write("ERROR: could not create output directory %s (%s)\n"
                             % (OUT_DIR, exc))
            sys.exit(2)

        broker = account.company
        account_login = int(account.login)
        generated_at = int(time.time())

        print("Broker      : %s" % broker)
        print("Account     : %s" % account_login)
        print("Output      : %s" % OUT_DIR)
        print("")

        # -- read every timeframe ------------------------------------------
        # A failure on one timeframe must never abort the whole run, so each
        # timeframe is read independently and reported honestly.
        summary = {}
        for label, const_name in TIMEFRAMES:
            tf = getattr(mt5, const_name)
            candles = read_timeframe(symbol, label, tf)

            write_json(
                os.path.join(OUT_DIR, "candles_%s.json" % label),
                {
                    "symbol": symbol,
                    "timeframe": label,
                    "broker": broker,
                    "account_login": account_login,
                    "generated_at_utc": generated_at,
                    "count": len(candles),
                    "candles": candles,
                },
            )

            if candles:
                last = candles[-1]
                summary[label] = {
                    "count": len(candles),
                    "last_time": last["time"],
                    "last_close": last["close"],
                }
            else:
                # No candles is recorded as an explicit empty result. No
                # placeholder timestamp or price is ever substituted.
                summary[label] = {
                    "count": 0,
                    "last_time": None,
                    "last_close": None,
                }
            print("Wrote candles_%s.json (%d closed candles)" % (label, len(candles)))

        write_json(
            os.path.join(OUT_DIR, "index.json"),
            {
                "generated_at_utc": generated_at,
                "symbol": symbol,
                "broker": broker,
                "timeframes": summary,
            },
        )
        print("Wrote index.json")

        # -- summary table --------------------------------------------------
        print("")
        print("Timeframe summary for %s (%s):" % (symbol, broker))
        print("%-8s %8s %14s %12s" % ("TF", "COUNT", "LAST_TIME(UTC)", "LAST_CLOSE"))
        for label, _ in TIMEFRAMES:
            row = summary[label]
            print("%-8s %8d %14s %12s"
                  % (label, row["count"],
                     row["last_time"] if row["last_time"] is not None else "-",
                     row["last_close"] if row["last_close"] is not None else "-"))

        if summary[DEFAULT_TF]["count"] == 0:
            sys.stderr.write(
                "\nERROR: the operational timeframe %s returned no candles. "
                "Output is incomplete.\n" % DEFAULT_TF)
            return 1

        print("\nREAD CANDLES OK")
        return 0
    finally:
        mt5.shutdown()
        print("MetaTrader5 connection closed cleanly.")


if __name__ == "__main__":
    sys.exit(main())
