#!/usr/bin/env python3
"""Step 2 - read closed XAUUSD candles for the nine canonical AURA timeframes.

Writes out/candles_<LABEL>.json for each timeframe plus out/index.json.

Closed bars only: any candle with isOpen == True is discarded, because AURA
never consumes a still-forming bar. Timestamps are ISO 8601 UTC converted to
epoch seconds and otherwise preserved verbatim - no resampling, no
interpolation, no timezone shifting.

Nothing is fabricated. biquote's feed only offers 1m 5m 15m 30m 1h 4h 1d:
weekly and monthly candles are not obtainable, so W1 and MN1 are written with
count 0 and an explicit reason rather than approximated.

Exit codes
    0  ran to completion and M15 (the operational timeframe) returned data
    1  biquote library not installed
    2  a quote could not be read
    3  the operational timeframe M15 returned no candles

Usage
    pip install -r requirements.txt
    python read_candles.py
"""

import sys
import time

from biquote_common import (DEFAULT_SOURCE, OPERATIONAL_TF, TIMEFRAMES, check_spacing,
                            closed_candles, import_biquote, resolve_symbol, write_json)


def read_timeframe(bq, symbol, label, interval):
    """Return (candles, note). `note` is non-empty when the TF is unavailable."""
    if interval is None:
        return [], "unsupported by the biquote feed (no such interval)"

    try:
        raw = bq.ohlc(symbol, interval=interval, limit=500)
    except Exception as exc:  # noqa: BLE001 - reported, never swallowed
        return [], "%s: %s" % (type(exc).__name__, exc)

    if not raw:
        return [], "API returned no candles"

    candles, dropped = closed_candles(raw)
    if dropped:
        sys.stderr.write("WARN: %s - dropped %d bar(s) with a duplicated bar identity "
                         "and conflicting values.\n" % (label, dropped))
    if not candles:
        return [], "all returned bars were still forming or conflicted"

    ok, detail = check_spacing(candles, label)
    if not ok:
        return [], detail
    return candles, ""


def main():
    biquote = import_biquote()
    bq = biquote.Biquote()
    symbol = resolve_symbol(bq)

    try:
        tick = bq.tick(symbol)
        source = DEFAULT_SOURCE
        if isinstance(tick, dict) and tick.get("source"):
            source = "biquote (%s)" % tick["source"]
    except Exception as exc:  # noqa: BLE001 - provenance is nice-to-have only
        sys.stderr.write("WARN: could not read a quote (%s). "
                         "Provenance falls back to %r.\n" % (type(exc).__name__, DEFAULT_SOURCE))
        source = DEFAULT_SOURCE

    generated_at = int(time.time())
    summary = {}

    print("")
    print("Source: %s" % source)
    print("")

    for label, interval in TIMEFRAMES:
        candles, note = read_timeframe(bq, symbol, label, interval)

        write_json("candles_%s.json" % label, {
            "symbol": symbol,
            "timeframe": label,
            "source": source,
            "interval": interval or "",
            "generated_at_utc": generated_at,
            "count": len(candles),
            "unavailable_reason": note,
            "candles": candles,
        })

        if candles:
            last = candles[-1]
            summary[label] = {
                "count": len(candles),
                "last_time": last["time"],
                "last_close": last["close"],
                "status": "OK",
                "note": "",
            }
            print("Wrote candles_%s.json (%d closed candles)" % (label, len(candles)))
        else:
            # Explicitly unavailable: nulls, never a placeholder timestamp/price.
            summary[label] = {
                "count": 0,
                "last_time": None,
                "last_close": None,
                "status": "NOT_AVAILABLE",
                "note": note,
            }
            sys.stderr.write("WARN: no candles for %s (%s)\n" % (label, note))
            print("Wrote candles_%s.json (0 candles - NOT AVAILABLE)" % label)

    write_json("index.json", {
        "generated_at_utc": generated_at,
        "symbol": symbol,
        "source": source,
        "timeframes": summary,
    })
    print("Wrote index.json")

    print("")
    print("Timeframe summary for %s (%s):" % (symbol, source))
    print("%-6s %7s %14s %12s  %s" % ("TF", "COUNT", "LAST_TIME(UTC)", "LAST_CLOSE", "STATUS"))
    for label, _ in TIMEFRAMES:
        row = summary[label]
        print("%-6s %7d %14s %12s  %s"
              % (label, row["count"],
                 row["last_time"] if row["last_time"] is not None else "-",
                 row["last_close"] if row["last_close"] is not None else "-",
                 row["status"]))

    if summary[OPERATIONAL_TF]["status"] != "OK":
        sys.stderr.write("\nERROR: the operational timeframe %s returned no candles.\n"
                         % OPERATIONAL_TF)
        return 3

    print("\nREAD CANDLES OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
