#!/usr/bin/env python3
"""AURA MT5 Python bridge - run the full candle export and print a summary.

Calls read_candles.main() (which writes ./out/candles_<TF>.json and ./out/index.json),
then prints a per-timeframe summary table. Exit code 0 when the candle export
succeeded. Read-only: no order is ever placed.
"""

import json
import os
import sys

import read_candles

INDEX_PATH = os.path.join(read_candles.OUT_DIR, "index.json")


def print_summary():
    if not os.path.exists(INDEX_PATH):
        print("SUMMARY: index.json not found - no summary available.")
        return
    with open(INDEX_PATH, "r", encoding="utf-8") as fh:
        index = json.load(fh)

    print("")
    print("AURA MT5 bridge - candle summary")
    print("  symbol : %s" % index.get("symbol"))
    print("  broker : %s" % index.get("broker"))
    print("  login  : %s" % index.get("account_login"))
    print("  +-----------+-------+")
    print("  | timeframe | count |")
    print("  +-----------+-------+")
    total = 0
    for row in index.get("timeframes", []):
        count = int(row.get("count", 0))
        total += count
        print("  | %-9s | %5d |" % (row.get("timeframe"), count))
    print("  +-----------+-------+")
    print("  | %-9s | %5d |" % ("TOTAL", total))
    print("  +-----------+-------+")
    print("  output dir: %s" % read_candles.OUT_DIR)


def main():
    result = read_candles.main()
    print_summary()
    if result == 0:
        print("RUN ALL: PASS")
    else:
        print("RUN ALL: FAIL")
    return result


if __name__ == "__main__":
    sys.exit(main())
