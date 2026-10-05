#!/usr/bin/env python3
"""Step 1 - verify that this machine can reach biquote and read XAUUSD.

Prints the real live quote and the last 5 closed M15 candles. Read-only: this
places no order and touches nothing in the AURA C++ core.

Exit codes
    0  quote and candles retrieved
    1  biquote library not installed
    2  quote could not be read
    3  no closed M15 candles returned

Usage
    pip install -r requirements.txt
    python connect_test.py
"""

import sys

from biquote_common import (closed_candles, import_biquote, resolve_symbol)


def main():
    biquote = import_biquote()
    bq = biquote.Biquote()
    symbol = resolve_symbol(bq)

    print("biquote version: %s" % getattr(biquote, "__version__", "unknown"))

    # ---- live quote ------------------------------------------------------
    try:
        tick = bq.tick(symbol)
    except Exception as exc:  # noqa: BLE001 - reported explicitly
        sys.stderr.write("ERROR: could not read a quote for %s.\n"
                         "  %s: %s\n" % (symbol, type(exc).__name__, exc))
        return 2

    if not isinstance(tick, dict) or tick.get("bid") in (None, 0.0):
        sys.stderr.write("ERROR: quote for %s carried no usable bid. Raw: %r\n"
                         % (symbol, tick))
        return 2

    print("")
    print("Live quote for %s:" % symbol)
    for field in ("bid", "ask", "spread", "mid", "source", "marketState",
                  "stale", "timestamp", "exchange", "description"):
        print("  %-12s %s" % (field, tick.get(field)))

    # ---- closed candles --------------------------------------------------
    print("")
    try:
        raw = bq.ohlc(symbol, interval="15m", limit=5)
    except Exception as exc:  # noqa: BLE001 - reported explicitly
        sys.stderr.write("ERROR: could not read candles for %s.\n"
                         "  %s: %s\n" % (symbol, type(exc).__name__, exc))
        return 3

    candles, dropped = closed_candles(raw)
    if dropped:
        sys.stderr.write("WARN: dropped %d bar(s) with a duplicated bar identity "
                         "and conflicting values.\n" % dropped)
    if not candles:
        sys.stderr.write("ERROR: no CLOSED M15 candles for %s. "
                         "Reported, never substituted.\n" % symbol)
        return 3

    print("Last %d closed M15 candles for %s:" % (len(candles), symbol))
    for candle in candles:
        print("  time=%s open=%s high=%s low=%s close=%s tick_volume=%s"
              % (candle["time"], candle["open"], candle["high"], candle["low"],
             candle["close"], candle["tick_volume"]))

    print("")
    print("CONNECT TEST PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
