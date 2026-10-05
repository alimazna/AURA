#!/usr/bin/env python3
"""Shared helpers for the Biquote bridge.

Every script in this folder talks to the same source: the `biquote` client,
which serves real XAUUSD data from a MetaTrader 5 feed ("Broker 1"). Nothing
here fabricates a value; when the API cannot supply something, the caller
records an explicit empty/unavailable state instead of a placeholder.

Interval reality (probed against biquote 0.3.0, not assumed)
    The server accepts only: 1m 5m 15m 30m 1h 4h 1d
    * "1w"  -> HTTP 400 "Invalid interval '1w'"
    * "1M"  -> silently resolves to ONE MINUTE (byte-identical to 1m),
               so it is rejected here rather than written as monthly data.
    Weekly and monthly candles therefore cannot be obtained from this API.
    They are reported as unavailable, never approximated or resampled.
"""

import json
import os
import sys

# Where the bridge writes. Resolved from this file so the scripts work from any
# working directory.
OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "out")

# Provenance string reported with every artifact. This is NOT the user's own
# broker: biquote serves an MT5 feed identified as "Broker 1".
DEFAULT_SOURCE = "biquote (MT5 Broker 1)"

# Canonical AURA timeframes -> biquote interval.
# `None` means the API has no such interval; those timeframes are reported as
# unavailable rather than faked. See the module docstring for the probe result.
TIMEFRAMES = (
    ("M1",  "1m"),
    ("M5",  "5m"),
    ("M15", "15m"),
    ("M30", "30m"),
    ("H1",  "1h"),
    ("H4",  "4h"),
    ("D1",  "1d"),
    ("W1",  None),   # server rejects "1w"
    ("MN1", None),   # server maps "1M" to 1 minute, not 1 month
)

OPERATIONAL_TF = "M15"

# Minimum spacing (seconds) a candle series must show for a given label before we
# trust that the server honoured the interval. Used to catch the "1M" -> 1 minute
# aliasing instead of silently mislabelling minute bars as monthly bars.
MIN_SPACING_SECONDS = {
    "M1": 60, "M5": 300, "M15": 900, "M30": 1800,
    "H1": 3600, "H4": 14400, "D1": 86400, "W1": 604800, "MN1": 2592000,
}


def import_biquote():
    """Import biquote or exit(1) with the exact install command."""
    try:
        import biquote  # noqa: F401
    except ImportError:
        sys.stderr.write("biquote library not found. Run: pip install -r requirements.txt\n")
        sys.exit(1)
    return biquote


def resolve_symbol(bq, preferred="XAUUSD"):
    """Find the XAUUSD symbol id from the live symbol list.

    The symbol name is discovered, never hardcoded: an exact match is preferred,
    otherwise the first XAU-containing match wins. Falls back to `preferred`
    only when the symbol list itself cannot be read (the subsequent call would
    then fail loudly with the server's own error).
    """
    try:
        symbols = bq.symbols()
    except Exception as exc:  # noqa: BLE001 - reported, not swallowed
        sys.stderr.write("WARN: could not list symbols (%s). Trying %r directly.\n"
                         % (type(exc).__name__, preferred))
        return preferred

    names = []
    for entry in symbols or []:
        name = entry.get("name") if isinstance(entry, dict) else getattr(entry, "name", None)
        if name:
            names.append(name)

    xau = [n for n in names if "XAU" in n.upper()]
    if not xau:
        sys.stderr.write("ERROR: no XAU-containing symbol found in biquote's symbol list "
                         "(%d symbols).\n" % len(names))
        sys.exit(1)

    exact = [n for n in xau if n == preferred]
    chosen = exact[0] if exact else xau[0]
    print("XAU symbols found (%d): %s" % (len(xau), ", ".join(xau)))
    print("Chosen symbol: %s (%s)"
          % (chosen, "exact match" if exact else "first XAU match"))
    return chosen


def parse_iso_epoch(value):
    """ISO 8601 '...Z' -> int epoch seconds. Returns None when unparseable.

    No timezone conversion is applied: the string is UTC by contract and is
    interpreted as UTC, matching how AURA stores bar identity.
    """
    if not isinstance(value, str):
        return None
    text = value.strip()
    if text.endswith("Z"):
        text = text[:-1]
    try:
        from datetime import datetime, timezone
        return int(datetime.strptime(text, "%Y-%m-%dT%H:%M:%S")
                   .replace(tzinfo=timezone.utc).timestamp())
    except ValueError:
        return None


def closed_candles(raw):
    """Keep only closed bars from an ohlc() response, oldest first.

    A candle is closed when `isOpen` is False. The forming bar is excluded
    because AURA never draws or consumes an unclosed bar.

    Returns (candles, dropped) where `dropped` describes bars discarded because
    they duplicated an already-present bar identity with a DIFFERENT body. AURA
    identifies a bar by (symbol, timeframe, close_time), so two different bodies
    claiming one identity is an unrecoverable conflict. Both are dropped rather
    than arbitrarily picking a winner, and the count is reported to the caller.
    The biquote D1 feed really does contain such a pair.
    """
    out = []
    for candle in raw or []:
        if not isinstance(candle, dict):
            continue
        if candle.get("isOpen", False):
            continue
        epoch = parse_iso_epoch(candle.get("openTime"))
        if epoch is None:
            continue
        try:
            out.append({
                "time": epoch,
                "open": float(candle["open"]),
                "high": float(candle["high"]),
                "low": float(candle["low"]),
                "close": float(candle["close"]),
                "tick_volume": int(candle.get("tickVolume") or 0),
            })
        except (KeyError, TypeError, ValueError):
            continue
    out.sort(key=lambda c: c["time"])

    kept = []
    dropped = 0
    seen = {}
    for candle in out:
        prior = seen.get(candle["time"])
        if prior is None:
            seen[candle["time"]] = candle
            kept.append(candle)
            continue
        same_body = all(prior[f] == candle[f]
                        for f in ("open", "high", "low", "close"))
        if same_body:
            continue  # harmless retransmission of an identical bar
        dropped += 1  # conflicting duplicate: both are discarded
        kept = [c for c in kept if c["time"] != candle["time"]]
    return kept, dropped


def check_spacing(candles, label):
    """Reject a series whose bars are systematically closer than `label` allows.

    Guards against the server aliasing an interval (observed: "1M" returns 1
    minute bars). The MEDIAN gap is used rather than the minimum so that one
    irregular bar in an otherwise valid series - a weekend gap, a duplicated
    identity already removed above - cannot condemn the whole timeframe.
    Returns (ok, detail).
    """
    floor = MIN_SPACING_SECONDS.get(label)
    if not candles or not floor or len(candles) < 2:
        return True, ""
    gaps = sorted(candles[i + 1]["time"] - candles[i]["time"]
                  for i in range(len(candles) - 1))
    median = gaps[len(gaps) // 2]
    if median < floor:
        return False, ("median bar spacing %ds is below the %ds expected for %s; "
                       "the feed aliased the interval, refusing to mislabel it"
                       % (median, floor, label))
    return True, ""


def write_json(name, payload):
    """Write out/<name>. Any filesystem problem is fatal and explicit."""
    path = os.path.join(OUT_DIR, name)
    try:
        os.makedirs(OUT_DIR, exist_ok=True)
        with open(path, "w", encoding="utf-8") as handle:
            json.dump(payload, handle, indent=2)
            handle.write("\n")
    except OSError as exc:
        sys.stderr.write("ERROR: could not write %s (%s)\n" % (path, exc))
        sys.exit(1)
    return path
