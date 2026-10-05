#!/usr/bin/env python3
"""Step 1 - verify that this machine can talk to MetaTrader 5 and that the
XAUUSD variant offered by the broker can actually return closed candles.

This script is READ-ONLY with respect to the broker account: it never places,
modifies or cancels an order, and it never touches AURA internals.

Windows only. MetaTrader5 (the official MQL5 Python connector) has no Linux or
macOS build, so this script can only run on a Windows host with the MetaTrader 5
terminal installed, running, and already logged in.

Exit codes
    0  connection OK, symbol resolved, 3 closed M15 candles retrieved
    1  MetaTrader5 library not installed
    2  mt5.initialize() failed (terminal not running / not logged in)
    3  no XAU-containing symbol could be resolved
    4  symbol resolved but returned no closed M15 candles

Usage (Windows, from this folder)
    pip install -r requirements.txt
    python connect_test.py
"""

import sys

# --------------------------------------------------------------------------
# Import guard. MetaTrader5 is Windows-only; on any other OS the import fails
# and we say so explicitly rather than letting a bare traceback surface.
# --------------------------------------------------------------------------
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


def _xau_symbols():
    """Every symbol name the terminal knows that contains "XAU" (any case)."""
    groups = mt5.symbols_get()
    if groups is None:
        sys.stderr.write("ERROR: mt5.symbols_get() returned None. last_error=%r\n"
                         % (mt5.last_error(),))
        sys.exit(2)
    names = [g.name for g in groups if SYMBOL_FILTER in g.name.upper()]
    return names


def resolve_symbol():
    """Prefer the exact name "XAUUSD"; otherwise take the first XAU match.

    Returns the chosen symbol name. Exits 3 when nothing matches, printing the
    full candidate list so the operator can see what the broker really offers.
    """
    names = _xau_symbols()
    if not names:
        sys.stderr.write(
            "ERROR: no symbol containing %r was found in the terminal.\n"
            "Total symbols known to the terminal: %d\n"
            "Fix: open Market Watch, confirm XAUUSD is listed (right-click > "
            "Show All), then re-run.\n"
            % (SYMBOL_FILTER, len(mt5.symbols_get() or ()))
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


def main():
    # -- connect ----------------------------------------------------------
    if not mt5.initialize():
        sys.stderr.write("ERROR: mt5.initialize() failed. last_error=%r\n"
                         "Fix: start MetaTrader 5 and log in to the account.\n"
                         % (mt5.last_error(),))
        sys.exit(2)

    try:
        print("MetaTrader5 python package version: %s" % (mt5.version(),))

        account = mt5.account_info()
        terminal = mt5.terminal_info()
        if account is None or terminal is None:
            sys.stderr.write("ERROR: could not read account/terminal info. "
                             "last_error=%r\n" % (mt5.last_error(),))
            sys.exit(2)

        print("Account login : %s" % account.login)
        print("Account name  : %s" % account.name)
        print("Broker        : %s" % account.company)
        print("Server        : %s" % account.server)
        print("Terminal path : %s" % terminal.path)
        print("Terminal build: %s" % terminal.build)

        # -- symbol --------------------------------------------------------
        symbol = resolve_symbol()
        if not mt5.symbol_select(symbol, True):
            # Not fatal: the symbol can still be readable even if it cannot be
            # added to Market Watch. Report it and try the read anyway.
            sys.stderr.write("WARN: mt5.symbol_select(%r, True) returned False. "
                             "last_error=%r\n"
                             % (symbol, mt5.last_error(),))

        # -- candles -------------------------------------------------------
        # start_pos=1 excludes the currently-forming bar, so every returned
        # candle is closed. Timestamps are left exactly as MT5 reports them
        # (UTC epoch seconds); nothing is resampled or interpolated.
        rates = mt5.copy_rates_from_pos(symbol, mt5.TIMEFRAME_M15, 1, 3)
        if rates is None or len(rates) == 0:
            sys.stderr.write(
                "WARN: no closed M15 candles for %r. last_error=%r\n"
                "This is reported, never substituted with placeholder values.\n"
                % (symbol, mt5.last_error(),))
            sys.exit(4)

        print("")
        print("Last 3 closed M15 candles for %s:" % symbol)
        for r in rates:
            print("  time=%s open=%s high=%s low=%s close=%s tick_volume=%s spread=%s real_volume=%s"
                  % (int(r["time"]), float(r["open"]), float(r["high"]),
                     float(r["low"]), float(r["close"]), int(r["tick_volume"]),
                     int(r["spread"]), int(r["real_volume"])))

        print("")
        print("CONNECT TEST PASS")
        return 0
    finally:
        mt5.shutdown()
        print("MetaTrader5 connection closed cleanly.")


if __name__ == "__main__":
    sys.exit(main())
