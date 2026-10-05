#!/usr/bin/env python3
"""Step 3 - read news items from the biquote feed into out/news.json.

The `news` signature is discovered at runtime with inspect.signature rather
than assumed, and every item is stored with the API's own fields verbatim.
No headline, summary or timestamp is ever synthesised.

If the call fails - no network, plan restriction, changed signature - an empty
news.json is written with an explicit `error` field and this script exits 0.
News is auxiliary: it must never abort the rest of the bridge.

Exit codes
    0  always (news is optional)
    1  biquote library not installed
"""

import inspect
import sys
import time

from biquote_common import import_biquote, write_json


def discover_signature(biquote, method_name):
    """Return the signature string for a Biquote method, or None if unknowable."""
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

    signature = discover_signature(biquote, "news")
    if signature is None:
        write_json("news.json", {
            "generated_at_utc": generated_at,
            "source": "biquote",
            "signature": None,
            "count": 0,
            "items": [],
            "error": "UNVERIFIED - discover it on your machine with: "
                     "python -c \"import biquote, inspect; "
                     "print(inspect.signature(biquote.Biquote.news))\"",
        })
        print("news signature could not be discovered; wrote an empty news.json "
              "with an explicit marker.")
        return 0

    print("Discovered signature: Biquote.news%s" % signature)

    bq = biquote.Biquote()
    try:
        items = bq.news()
    except Exception as exc:  # noqa: BLE001 - news is optional, never fatal
        sys.stderr.write("WARN: biquote.news() failed: %s: %s\n"
                         % (type(exc).__name__, exc))
        write_json("news.json", {
            "generated_at_utc": generated_at,
            "source": "biquote",
            "signature": signature,
            "count": 0,
            "items": [],
            "error": "%s: %s" % (type(exc).__name__, exc),
        })
        print("News unavailable; wrote an empty news.json with the error recorded.")
        return 0

    # Keep the API's fields verbatim - no reshaping, no invented fields.
    records = [i for i in (items or []) if isinstance(i, dict)]
    write_json("news.json", {
        "generated_at_utc": generated_at,
        "source": "biquote",
        "signature": signature,
        "count": len(records),
        "items": records,
        "error": "",
    })
    print("Wrote news.json (%d items)" % len(records))
    return 0


if __name__ == "__main__":
    sys.exit(main())
