#!/usr/bin/env python3
"""Step 4 - read economic-calendar events from biquote into out/calendar.json.

Tries `calendar_upcoming` first (the forward-looking view an operator wants)
and falls back to `calendar`. Both signatures are discovered at runtime with
inspect.signature rather than assumed, and every event keeps the API's own
fields verbatim - no event value, importance or forecast is ever synthesised.

If every call fails, an empty calendar.json is written with an explicit `error`
field and this script exits 0. The calendar is auxiliary: it must never abort
the rest of the bridge.

Exit codes
    0  always (the calendar is optional)
    1  biquote library not installed
"""

import inspect
import sys
import time

from biquote_common import import_biquote, write_json

# Tried in order; the first one that yields events wins.
METHODS = ("calendar_upcoming", "calendar")


def discover_signature(biquote, method_name):
    method = getattr(biquote.Biquote, method_name, None)
    if method is None:
        return None
    try:
        return str(inspect.signature(method))
    except (TypeError, ValueError) as exc:
        sys.stderr.write("WARN: could not inspect %s(): %s: %s\n"
                         % (method_name, type(exc).__name__, exc))
        return None


def main():
    biquote = import_biquote()
    generated_at = int(time.time())

    signatures = {name: discover_signature(biquote, name) for name in METHODS}
    for name, sig in signatures.items():
        if sig is None:
            sys.stderr.write("WARN: %s is absent or its signature is undiscoverable.\n" % name)
        else:
            print("Discovered signature: Biquote.%s%s" % (name, sig))

    if all(sig is None for sig in signatures.values()):
        write_json("calendar.json", {
            "generated_at_utc": generated_at,
            "source": "biquote",
            "signatures": {k: None for k in METHODS},
            "method_used": None,
            "count": 0,
            "events": [],
            "error": "UNVERIFIED - discover it on your machine with: "
                     "python -c \"import biquote, inspect; "
                     "print(inspect.signature(biquote.Biquote.calendar_upcoming))\"",
        })
        print("No calendar method could be discovered; wrote an empty calendar.json.")
        return 0

    bq = biquote.Biquote()
    errors = []
    for name in METHODS:
        if getattr(biquote.Biquote, name, None) is None:
            continue
        try:
            raw = getattr(bq, name)()
        except Exception as exc:  # noqa: BLE001 - try the next method
            errors.append("%s: %s: %s" % (name, type(exc).__name__, exc))
            sys.stderr.write("WARN: %s() failed: %s: %s\n" % (name, type(exc).__name__, exc))
            continue

        events = [e for e in (raw or []) if isinstance(e, dict)]
        if events:
            write_json("calendar.json", {
                "generated_at_utc": generated_at,
                "source": "biquote",
                "signatures": signatures,
                "method_used": name,
                "count": len(events),
                "events": events,
                "error": "",
            })
            print("Wrote calendar.json (%d events via %s)" % (len(events), name))
            return 0
        errors.append("%s returned no events" % name)

    write_json("calendar.json", {
        "generated_at_utc": generated_at,
        "source": "biquote",
        "signatures": signatures,
        "method_used": None,
        "count": 0,
        "events": [],
        "error": "; ".join(errors) if errors else "no events returned",
    })
    print("Calendar unavailable; wrote an empty calendar.json with the error recorded.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
