#!/usr/bin/env python3
"""AURA MT5 Python bridge - connection smoke test.

Verifies that the MetaTrader 5 terminal is reachable from Python, prints the
terminal build and account identity, lists every symbol containing "XAU", picks
the best XAUUSD candidate (broker suffix aware), and reads the last three closed
M15 candles plus the current bid/ask/spread.

Read-only: it never places, modifies or closes an order. Exit code 0 on success.

Requires: Windows + a running MetaTrader 5 terminal + `pip install -r requirements.txt`.
"""

import sys

try:
    import MetaTrader5 as mt5
except ImportError:  # pragma: no cover - only reachable off the target host
    print("ERROR: the MetaTrader5 package is not installed.")
    print("Install it with: pip install -r requirements.txt")
    sys.exit(1)

# One closed bar is excluded so we only ever look at CLOSED bars.
START_POS = 1
CANDLE_COUNT = 3


def pick_xau_symbol(symbols):
    """Pick the best XAUUSD candidate from the terminal's symbol list.

    Preference order: exact "XAUUSD" > a suffixed "XAUUSD.<suffix>" (e.g. .vx /
    .vcn) > the first symbol containing "XAU". Returns None when none match.
    """
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


def main():
    if not mt5.initialize():
        print("ERROR: mt5.initialize() failed:", mt5.last_error())
        return 1

    try:
        version = mt5.version()
        account = mt5.account_info()
        print("MetaTrader5 version:", version)
        if account is None:
            print("ERROR: mt5.account_info() returned None:", mt5.last_error())
            return 1
        print("account login  :", account.login)
        print("account company:", account.company)
        print("account balance:", account.balance)

        symbols = mt5.symbols_get() or []
        xau = [s.name for s in symbols if "XAU" in s.name.upper()]
        print("XAU symbols (%d): %s" % (len(xau), ", ".join(xau) if xau else "(none)"))

        symbol = pick_xau_symbol(symbols)
        if not symbol:
            print("ERROR: no XAU symbol available on this terminal.")
            return 1
        print("selected symbol:", symbol)

        if not mt5.symbol_select(symbol, True):
            print("ERROR: mt5.symbol_select(%s, True) failed: %s" % (symbol, mt5.last_error()))
            return 1

        # Last three CLOSED M15 bars (start=1 skips the forming bar).
        rates = mt5.copy_rates_from_pos(symbol, mt5.TIMEFRAME_M15, START_POS, CANDLE_COUNT)
        if rates is None or len(rates) == 0:
            print("WARN: no M15 candles returned:", mt5.last_error())
        else:
            print("last %d M15 candles (time, open, high, low, close, tick_volume, spread, real_volume):"
                  % len(rates))
            for r in rates:
                print("  ", int(r["time"]), float(r["open"]), float(r["high"]), float(r["low"]),
                      float(r["close"]), int(r["tick_volume"]), int(r["spread"]),
                      int(r["real_volume"]))

        tick = mt5.symbol_info_tick(symbol)
        if tick is None:
            print("WARN: symbol_info_tick returned None:", mt5.last_error())
        else:
            spread = tick.ask - tick.bid
            print("tick bid=%.5f ask=%.5f spread=%.5f" % (tick.bid, tick.ask, spread))

        print("CONNECT TEST: PASS")
        return 0
    finally:
        mt5.shutdown()


if __name__ == "__main__":
    sys.exit(main())
