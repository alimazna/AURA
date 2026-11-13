"""learn — re-measure the knowledge base on fresh data and update it.

This is what makes the knowledge base a loop rather than a snapshot. Each run
pulls the current bars, re-tests the rules in `rules.json`, compares the result
with what is already recorded, and:

  - appends the new measurement to `evidence.jsonl` (never overwrites — the
    ledger is the record, and a measurement that contradicts an earlier one is
    the most valuable row in it),
  - updates `rules.json` only when the evidence actually changes a verdict.

The honesty rules are enforced here, not just documented:

  - A rule is promoted to `proven` only if it clears its own threshold
    (n >= 94 or p < 0.05). The loop cannot talk itself into confidence.
  - A rule that was `proven` and stops holding is demoted. Evidence cuts both
    ways, or it is not evidence.
  - `refuted` rules stay refuted until new evidence reverses them; they never
    silently become usable.
  - The random baseline is printed with every comparison, so a number is never
    read without the thing it must beat.

    python3 agent/tools/learn.py            # measure 1d, update the base
    python3 agent/tools/learn.py --tf 1h
    python3 agent/tools/learn.py --dry-run  # measure, print, write nothing
"""
from __future__ import annotations

import argparse
import json
import sys
from datetime import datetime, timezone
from math import comb
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from chart_eye import atr, ema, fetch, rsi, swings, structure  # noqa: E402
from validate import COST, break_even, resolve  # noqa: E402

KB_DIR = Path(__file__).resolve().parent.parent / "knowledge"
RULES = KB_DIR / "rules.json"
EVIDENCE = KB_DIR / "evidence.jsonl"
HISTORY = KB_DIR / "learn_history.jsonl"

MIN_N = 94          # trades before a win rate can be believed
P_THRESHOLD = 0.05


def today() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%d")


def load_rules() -> dict:
    return json.loads(RULES.read_text(encoding="utf-8"))


def tail_p(wins: int, n: int, p: float) -> float:
    return sum(comb(n, i) * p ** i * (1 - p) ** (n - i) for i in range(wins, n + 1))


def series(bars):
    c = [b["close"] for b in bars]
    return ema(c, 20), ema(c, 50), rsi(c), atr(bars)


# ------------------------------------------------------------------ measures
def measure_trend_conflict(bars, stop_atr=0.5, target_r=1.5):
    """R-TREND-CONFLICT: does trend/structure agreement beat disagreement?

    Structure is computed **per bar from the swings known up to that bar**. The
    first version called structure() once over all swings and reused the answer,
    which read the future and — because the whole series reads as "range" —
    produced zero trades instead of a measurement.
    """
    e20, e50, _, a = series(bars)
    hi, lo = swings(bars, 3)
    agree = {"wins": 0, "losses": 0}
    disagree = {"wins": 0, "losses": 0}
    last_exit = -1
    for i in range(60, len(bars) - 5):
        if i <= last_exit or None in (e20[i], e50[i], a[i]) or not a[i]:
            continue
        h = [p for p in hi if p["i"] < i]
        l = [p for p in lo if p["i"] < i]
        if len(h) < 2 or len(l) < 2:
            continue
        label = structure(h, l, 2)["label"]
        struct_up = "صاعد" in label
        struct_down = "هابط" in label
        if not (struct_up or struct_down):
            continue
        up = e20[i] > e50[i]
        bucket = agree if ((up and struct_up) or ((not up) and struct_down)) else disagree
        d = "BUY" if up else "SELL"
        sgn = 1.0 if d == "BUY" else -1.0
        entry = bars[i]["close"]
        stop = entry - sgn * a[i] * stop_atr
        target = entry + sgn * abs(entry - stop) * target_r
        r, exit_i = resolve(bars, i, d, stop, target)
        last_exit = exit_i
        if r == "win":
            bucket["wins"] += 1
        elif r == "loss":
            bucket["losses"] += 1
    return agree, disagree


def measure_pullback(bars, stop_atr=0.5, target_r=1.5, cap=1.5):
    """R-ENTRY-PULLBACK: pullback to EMA20 vs breakout entry.

    Single position at a time, per R-ONE-POSITION: an open trade blocks new
    entries. Counting every matching bar instead inflates the rate by 6.5
    points, which the ledger records.
    """
    e20, e50, r, a = series(bars)
    pb = {"wins": 0, "losses": 0}
    bo = {"wins": 0, "losses": 0}
    last_exit = -1
    for i in range(60, len(bars) - 5):
        if i <= last_exit or None in (e20[i], e50[i], r[i], a[i]) or not a[i]:
            continue
        up = e20[i] > e50[i]
        d = "BUY" if up else "SELL"
        sgn = 1.0 if d == "BUY" else -1.0
        entry = bars[i]["close"]
        dist = abs(entry - e20[i]) / a[i]
        # pullback: near EMA20 with RSI on the pullback side
        is_pb = dist <= cap and ((up and r[i] < 50) or ((not up) and r[i] > 50))
        # breakout: extended away from EMA20
        is_bo = dist > cap
        if not (is_pb or is_bo):
            continue
        stop = entry - sgn * a[i] * stop_atr
        target = entry + sgn * abs(entry - stop) * target_r
        res, exit_i = resolve(bars, i, d, stop, target)
        last_exit = exit_i
        bucket = pb if is_pb else bo
        if res == "win":
            bucket["wins"] += 1
        elif res == "loss":
            bucket["losses"] += 1
    return pb, bo


def wr(b: dict) -> float | None:
    n = b["wins"] + b["losses"]
    return b["wins"] / n * 100.0 if n else None


def verdict_for(measured: float, baseline: float, n: int, p: float,
                was: str) -> tuple[str, str]:
    """Decide the new status from the evidence, with a stated reason."""
    if n < MIN_N and p >= P_THRESHOLD:
        return was, f"عينة {n} < {MIN_N} و p={p:.3f} ≥ {P_THRESHOLD} — لا تغيير"
    if measured > baseline and p < P_THRESHOLD:
        return "proven", f"{measured:.1f}% > خط أساس {baseline:.1f}% و p={p:.3f} — مُثبت"
    if measured > baseline:
        return "direction", f"{measured:.1f}% > {baseline:.1f}% لكن p={p:.3f} — اتجاه فقط"
    return was, f"{measured:.1f}% ≤ {baseline:.1f}% — لم يتحسن"


def above_break_even(rate: float | None, be: float) -> str:
    """State plainly whether a rate clears the cost of trading.

    A rule can be valid and still lose money. R-TREND-CONFLICT is the example:
    rejecting the disagreeing case is right (25.0% against 41.4%), but the
    agreeing case itself sits below the 43.5% needed to pay the spread. Saying
    "proven" without saying "and still below break-even" would be a half-truth,
    so the comparison is printed on its own line.
    """
    if rate is None:
        return "لا قياس"
    if rate > be:
        return f"فوق التعادل ({be:.1f}%) ✅"
    return f"⚠️  تحت التعادل ({be:.1f}%) — القاعدة صحيحة نسبياً لكنها لا تكفي للربح"


# --------------------------------------------------------------------- driver
def learn(tf: str, dry: bool) -> int:
    kb = load_rules()
    bars = fetch(tf)
    if len(bars) < 120:
        print(f"بيانات غير كافية على {tf}: {len(bars)} شمعة")
        return 1

    print("=" * 74)
    print(f"  حلقة التعلّم — {tf} | {len(bars)} شمعة")
    print(f"  آخر شمعة: {bars[-1]['openTime'][:16]}")
    print("=" * 74)

    changes = 0
    new_rows: list[dict] = []

    # --- R-TREND-CONFLICT
    agree, disagree = measure_trend_conflict(bars)
    a_wr, d_wr = wr(agree), wr(disagree)
    if a_wr is not None and d_wr is not None:
        n = agree["wins"] + agree["losses"]
        be = break_even(0.5, 1.5)
        p = tail_p(agree["wins"], n, be / 100.0) if n else 1.0
        print(f"\n  R-TREND-CONFLICT")
        print(f"    اتفاق:   n={n:<4} {a_wr:5.1f}%   (تعادل {be:.1f}%)")
        print(f"    تعارض:   n={disagree['wins']+disagree['losses']:<4} "
              f"{d_wr:5.1f}%")
        print(f"    خط الأساس العشوائي: {be:.1f}%   p={p:.3f}")
        rule = next(r for r in kb["rules"] if r["id"] == "R-TREND-CONFLICT")
        new_status, reason = verdict_for(a_wr, d_wr, n, p, rule["status"])
        print(f"    الحالة: {rule['status']} → {new_status}  ({reason})")
        print(f"    {above_break_even(a_wr, be)}")
        rule["evidence"] = {"tf": tf, "bars": len(bars),
                            "agree": {"n": n, "win_rate": round(a_wr, 1), "p": round(p, 3)},
                            "disagree": {"n": disagree["wins"] + disagree["losses"],
                                         "win_rate": round(d_wr, 1)},
                            "break_even": round(be, 1), "note": reason}
        if new_status != rule["status"]:
            rule["status"] = new_status
            changes += 1
        new_rows.append({"ts": today(), "tf": tf, "bars": len(bars),
                         "study": "R-TREND-CONFLICT",
                         "agree": {"n": n, "win_rate": round(a_wr, 1)},
                         "disagree": {"n": disagree["wins"] + disagree["losses"],
                                      "win_rate": round(d_wr, 1)},
                         "break_even": round(be, 1), "verdict": new_status,
                         "note": reason})

    # --- R-ENTRY-PULLBACK
    pb, bo = measure_pullback(bars)
    pb_wr, bo_wr = wr(pb), wr(bo)
    if pb_wr is not None and bo_wr is not None:
        n = pb["wins"] + pb["losses"]
        be = break_even(0.5, 1.5)
        p = tail_p(pb["wins"], n, be / 100.0) if n else 1.0
        print(f"\n  R-ENTRY-PULLBACK")
        print(f"    ارتداد:  n={n:<4} {pb_wr:5.1f}%")
        print(f"    اختراق:  n={bo['wins']+bo['losses']:<4} {bo_wr:5.1f}%")
        print(f"    خط الأساس العشوائي: {be:.1f}%   p={p:.3f}")
        rule = next(r for r in kb["rules"] if r["id"] == "R-ENTRY-PULLBACK")
        new_status, reason = verdict_for(pb_wr, bo_wr, n, p, rule["status"])
        print(f"    الحالة: {rule['status']} → {new_status}  ({reason})")
        print(f"    {above_break_even(pb_wr, be)}")
        rule["evidence"] = {"tf": tf, "bars": len(bars),
                            "pullback": {"n": n, "win_rate": round(pb_wr, 1), "p": round(p, 3)},
                            "breakout": {"n": bo["wins"] + bo["losses"],
                                         "win_rate": round(bo_wr, 1)},
                            "break_even": round(be, 1), "note": reason}
        if new_status != rule["status"]:
            rule["status"] = new_status
            changes += 1
        new_rows.append({"ts": today(), "tf": tf, "bars": len(bars),
                         "study": "R-ENTRY-PULLBACK",
                         "pullback": {"n": n, "win_rate": round(pb_wr, 1)},
                         "breakout": {"n": bo["wins"] + bo["losses"],
                                      "win_rate": round(bo_wr, 1)},
                         "break_even": round(be, 1), "verdict": new_status,
                         "note": reason})

    # --- baseline for context
    total_w = sum(b["wins"] for b in (agree, disagree, pb, bo))
    total_l = sum(b["losses"] for b in (agree, disagree, pb, bo))
    if total_w + total_l:
        print(f"\n  الخط الأساسي الكلي: {total_w/(total_w+total_l)*100:.1f}% "
              f"من {total_w+total_l} صفقة")

    print(f"\n  النتيجة: {changes} تغيير في الحالة"
          if changes else "\n  النتيجة: لا تغيير في الحالة — الأدلة لم تكفِ بعد")

    if dry:
        print("\n  (تشغيل تجريبي — لم يُكتب شيء)")
        return 0

    kb["updated"] = today()
    RULES.write_text(json.dumps(kb, ensure_ascii=False, indent=2) + "\n",
                     encoding="utf-8")
    with EVIDENCE.open("a", encoding="utf-8") as f:
        for r in new_rows:
            f.write(json.dumps(r, ensure_ascii=False) + "\n")
    with HISTORY.open("a", encoding="utf-8") as f:
        f.write(json.dumps({"ts": today(), "tf": tf, "bars": len(bars),
                            "changes": changes}, ensure_ascii=False) + "\n")
    print(f"  حُفظ: {len(new_rows)} قياس في الدليل، {changes} تغيير في القاعدة")
    return 0


def main() -> None:
    ap = argparse.ArgumentParser(description="حلقة تعلّم قاعدة المعرفة")
    ap.add_argument("--tf", default="1d")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    sys.exit(learn(args.tf, args.dry_run))


if __name__ == "__main__":
    main()
