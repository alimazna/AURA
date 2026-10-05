#!/usr/bin/env python3
"""Step 5 - run the whole bridge and print one summary.

Runs connect_test -> read_candles -> read_news -> read_calendar. Candles are
required; news and the calendar are auxiliary, so a failure in either is
reported and does NOT abort the run.

Exit codes
    0  candles were produced (news/calendar may still have failed)
    1  biquote library not installed
    2  no candles were produced
"""

import json
import os
import sys

import read_calendar
import read_candles
import read_news
from biquote_common import OUT_DIR, TIMEFRAMES, import_biquote


def run(label, module):
    """Run one stage, returning (exit_code, detail). Never raises."""
    print("=" * 72)
    print("STAGE: %s" % label)
    print("=" * 72)
    try:
        code = module.main()
        return code, ""
    except SystemExit as exc:
        return int(exc.code or 0), "SystemExit"
    except Exception as exc:  # noqa: BLE001 - isolated per stage
        import traceback
        traceback.print_exc()
        return 1, "%s: %s" % (type(exc).__name__, exc)


def main():
    import_biquote()

    results = []
    results.append(("connect_test",) + run("connect_test", __import__("connect_test")))
    results.append(("read_candles",) + run("read_candles", read_candles))
    results.append(("read_news",) + run("read_news", read_news))
    results.append(("read_calendar",) + run("read_calendar", read_calendar))

    # ---- summary ---------------------------------------------------------
    candles_ok = 0
    total_bars = 0
    unavailable = []
    for label, _ in TIMEFRAMES:
        path = os.path.join(OUT_DIR, "candles_%s.json" % label)
        if not os.path.exists(path):
            unavailable.append("%s (no file)" % label)
            continue
        try:
            with open(path, encoding="utf-8") as handle:
                data = json.load(handle)
        except (OSError, ValueError):
            unavailable.append("%s (unreadable)" % label)
            continue
        count = int(data.get("count", 0))
        if count > 0:
            candles_ok += 1
            total_bars += count
        else:
            unavailable.append("%s (%s)" % (label, data.get("unavailable_reason") or "no data"))

    news_items = 0
    news_path = os.path.join(OUT_DIR, "news.json")
    if os.path.exists(news_path):
        try:
            with open(news_path, encoding="utf-8") as handle:
                news_items = int(json.load(handle).get("count", 0))
        except (OSError, ValueError):
            news_items = 0

    cal_events = 0
    cal_path = os.path.join(OUT_DIR, "calendar.json")
    if os.path.exists(cal_path):
        try:
            with open(cal_path, encoding="utf-8") as handle:
                cal_events = int(json.load(handle).get("count", 0))
        except (OSError, ValueError):
            cal_events = 0

    print("=" * 72)
    print("SUMMARY")
    print("=" * 72)
    print("Candles  : %d/%d timeframes OK, %d closed bars total" %
          (candles_ok, len(TIMEFRAMES), total_bars))
    if unavailable:
        print("           NOT AVAILABLE -> %s" % "; ".join(unavailable))
    print("News     : %d items" % news_items)
    print("Calendar : %d events" % cal_events)
    print("Output   : %s" % OUT_DIR)
    print("")
    print("Stage exit codes:")
    for name, code, detail in results:
        print("  %-16s %s%s" % (name, code, (" (%s)" % detail) if detail else ""))

    if candles_ok == 0:
        sys.stderr.write("\nRUN ALL FAILED: no candles were produced.\n")
        return 2
    print("\nRUN ALL OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
