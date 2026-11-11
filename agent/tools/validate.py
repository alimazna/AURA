"""validate — does what I believe actually hold on real data?

Every rule in `lessons/` is a claim. This file turns each claim into a
measurement so the claim can be falsified instead of repeated. A rule that
stops testing true gets moved to `lessons/05-failure-memory.md`.

The rules under test:

  R1  trend agreement (EMA stack agrees with swing structure) selects better
      bars than either reading alone.
  R2  a level with more touches holds more often than one with fewer.
  R3  entering on a pullback to a level beats entering on a breakout.
  R4  a stop below 0.5xATR is taken out by noise more often than a wider one.
  R5  the London/New York overlap produces more range than the quiet hours.

Method, held constant for all of them:
  - real biquote bars, closed only
  - one position at a time (no overlapping counts — the error that inflated
    an earlier 73% figure)
  - pessimistic resolution: a bar touching stop and target counts as a loss
  - costs charged on every trade
  - a baseline is printed next to every result

    python3 agent/tools/validate.py
    python3 agent/tools/validate.py --tf 1d
"""
from __future__ import annotations

import argparse
import sys
from math import comb

sys.path.insert(0, "agent/tools")
from chart_eye import (adx, atr, ema, fetch, rsi, structure, swings,  # noqa: E402
                       SESSIONS)

COST = 0.37
LOOKBACK = 60


def ema_series(bars, n):
    return ema([b["close"] for b in bars], n)


def resolve(bars, i, direction, stop, target, hold=60):
    """Walk forward. A bar touching both levels counts as a loss."""
    sgn = 1.0 if direction == "BUY" else -1.0
    for j in range(i + 1, min(i + 1 + hold, len(bars))):
        b = bars[j]
        hit_stop = b["low"] <= stop if sgn > 0 else b["high"] >= stop
        hit_target = b["high"] >= target if sgn > 0 else b["low"] <= target
        if hit_stop and hit_target:
            return "loss", j
        if hit_stop:
            return "loss", j
        if hit_target:
            return "win", j
    return "open", min(i + hold, len(bars) - 1)


def break_even(stop, target):
    w, l = target - COST, stop + COST
    return l / (w + l) * 100.0


def tail_p(wins, n, p):
    """P(X >= wins | p). The probability of luck alone producing this."""
    return sum(comb(n, i) * p ** i * (1 - p) ** (n - i) for i in range(wins, n + 1))


def stat(wins, losses):
    n = wins + losses
    return {"wins": wins, "losses": losses, "n": n,
            "wr": wins / n * 100.0 if n else None}


def show(label, s, be, p=None):
    if not s["n"]:
        print(f"  {label:<44} لا صفقات")
        return
    extra = f"  p={p:.3f}" if p is not None else ""
    verdict = "✅" if s["wr"] > be else "❌"
    print(f"  {label:<44} n={s['n']:<4} {s['wr']:5.1f}%  "
          f"(تعادل {be:.1f}%) {verdict}{extra}")


# --------------------------------------------------------------------- R1
def r1_trend_agreement(bars, stop, target):
    print("\n  R1: هل اتفاق الاتجاهين يختار شمعات أفضل؟")
    e20, e50 = ema_series(bars, 20), ema_series(bars, 50)
    r = rsi([b["close"] for b in bars])
    a = atr(bars)
    highs, lows = swings(bars, 3)
    be = break_even(stop, target)

    # structure at each bar, from swings known up to that bar
    struct = [None] * len(bars)
    for i in range(LOOKBACK, len(bars)):
        h = [p for p in highs if p["i"] < i]
        l = [p for p in lows if p["i"] < i]
        if len(h) >= 2 and len(l) >= 2:
            st = structure(h, l, 2)
            struct[i] = ("up" if "صاعد" in st["label"]
                         else "down" if "هابط" in st["label"] else "range")

    buckets = {"agree": [0, 0], "disagree": [0, 0], "range": [0, 0]}
    last_exit = -1
    for i in range(LOOKBACK, len(bars) - 1):
        if i <= last_exit or None in (e20[i], e50[i], r[i], a[i]):
            continue
        ema_up = e20[i] > e50[i]
        d = "BUY" if ema_up else "SELL"
        key = ("range" if struct[i] in (None, "range")
               else "agree" if ((struct[i] == "up") == ema_up) else "disagree")
        sgn = 1.0 if d == "BUY" else -1.0
        e = bars[i]["close"]
        out, exit_i = resolve(bars, i, d, e - sgn * stop, e + sgn * target)
        last_exit = exit_i
        if out == "win":
            buckets[key][0] += 1
        elif out == "loss":
            buckets[key][1] += 1
    for k, (w, l) in buckets.items():
        s = stat(w, l)
        p = tail_p(s["wins"], s["n"], be / 100) if s["n"] else None
        show(f"  {k}", s, be, p)
    print("  → R1 يثبت إذا كان 'agree' أعلى بوضوح من 'disagree'.")


# --------------------------------------------------------------------- R2
def r2_level_touches(bars, stop, target):
    print("\n  R2: هل المستوى الأكثر لمسات يصمد أكثر؟")
    a = atr(bars)
    highs, lows = swings(bars, 3)
    be = break_even(stop, target)
    buckets = {1: [0, 0], 2: [0, 0], "3+": [0, 0]}
    last_exit = -1
    for i in range(LOOKBACK, len(bars) - 1):
        if i <= last_exit or a[i] is None:
            continue
        price = bars[i]["close"]
        tol = a[i] * 0.25
        # how many confirmed swing points sit within tol of price?
        near = [p for p in highs + lows if p["i"] < i and abs(p["price"] - price) <= tol]
        if not near:
            continue
        n_touch = len(near)
        key = 1 if n_touch == 1 else 2 if n_touch == 2 else "3+"
        # trade away from the level, in the direction of the last swing trend
        h = [p for p in highs if p["i"] < i][-2:]
        l = [p for p in lows if p["i"] < i][-2:]
        if len(h) < 2 or len(l) < 2:
            continue
        up = h[-1]["price"] > h[-2]["price"] and l[-1]["price"] > l[-2]["price"]
        d = "BUY" if up else "SELL"
        sgn = 1.0 if d == "BUY" else -1.0
        out, exit_i = resolve(bars, i, d, price - sgn * stop, price + sgn * target)
        last_exit = exit_i
        if out == "win":
            buckets[key][0] += 1
        elif out == "loss":
            buckets[key][1] += 1
    for k, (w, l) in buckets.items():
        s = stat(w, l)
        p = tail_p(s["wins"], s["n"], be / 100) if s["n"] else None
        show(f"  لمسات = {k}", s, be, p)
    print("  → R2 يثبت إذا زادت النسبة مع اللمسات.")


# --------------------------------------------------------------------- R3
def r3_pullback_vs_breakout(bars, stop, target, pb=1.0):
    print("\n  R3: ارتداد إلى المستوى مقابل اختراق")
    e20, e50 = ema_series(bars, 20), ema_series(bars, 50)
    r = rsi([b["close"] for b in bars])
    a = atr(bars)
    be = break_even(stop, target)
    res = {"pullback": [0, 0], "breakout": [0, 0]}
    last_exit = -1
    for i in range(LOOKBACK, len(bars) - 1):
        if i <= last_exit or None in (e20[i], e50[i], r[i], a[i]):
            continue
        price = bars[i]["close"]
        up = e20[i] > e50[i]
        near = abs(price - e20[i]) <= a[i] * 1.5
        beyond = abs(price - e20[i]) >= a[i] * pb
        key = None
        if up and near and r[i] < 50:
            key, d = "pullback", "BUY"
        elif (not up) and near and r[i] > 50:
            key, d = "pullback", "SELL"
        elif up and beyond and price > e20[i]:
            key, d = "breakout", "BUY"
        elif (not up) and beyond and price < e20[i]:
            key, d = "breakout", "SELL"
        if not key:
            continue
        sgn = 1.0 if d == "BUY" else -1.0
        out, exit_i = resolve(bars, i, d, price - sgn * stop, price + sgn * target)
        last_exit = exit_i
        if out == "win":
            res[key][0] += 1
        elif out == "loss":
            res[key][1] += 1
    for k, (w, l) in res.items():
        s = stat(w, l)
        p = tail_p(s["wins"], s["n"], be / 100) if s["n"] else None
        show(f"  {k}", s, be, p)
    print("  → R3 يثبت إذا تفوّق الارتداد على الاختراق.")


# --------------------------------------------------------------------- R4
def r4_stop_size(bars, target_ratio=1.5):
    print("\n  R4: هل الوقف الأضيق من 0.5×ATR يُضرب بالضجيج؟")
    e20, e50 = ema_series(bars, 20), ema_series(bars, 50)
    r = rsi([b["close"] for b in bars])
    a = atr(bars)
    buckets = {"<0.3": [0, 0], "0.3-0.5": [0, 0], "0.5-1.0": [0, 0], ">1.0": [0, 0]}
    last_exit = -1
    for i in range(LOOKBACK, len(bars) - 1):
        if i <= last_exit or None in (e20[i], e50[i], r[i], a[i]):
            continue
        price = bars[i]["close"]
        up = e20[i] > e50[i]
        near = abs(price - e20[i]) <= a[i] * 1.5
        if up and near and r[i] < 50:
            d = "BUY"
        elif (not up) and near and r[i] > 50:
            d = "SELL"
        else:
            continue
        stop = a[i] * 0.5                       # stop is always 0.5xATR
        target = stop * target_ratio
        ratio = stop / a[i]                     # = 0.5 by construction
        key = "<0.3" if ratio < 0.3 else "0.3-0.5" if ratio < 0.5 else \
              "0.5-1.0" if ratio < 1.0 else ">1.0"
        sgn = 1.0 if d == "BUY" else -1.0
        out, exit_i = resolve(bars, i, d, price - sgn * stop, price + sgn * target)
        last_exit = exit_i
        if out == "win":
            buckets[key][0] += 1
        elif out == "loss":
            buckets[key][1] += 1
    print("  (الوقف مثبّت عند 0.5×ATR، فكل الصفقات في شريحة واحدة — "
          "هذا يكشف أن الشرط لا يُختبر بهذه الطريقة)")
    for k, (w, l) in buckets.items():
        s = stat(w, l)
        if s["n"]:
            show(f"  نسبة الوقف/ATR = {k}", s, break_even(a[len(bars)//2]*0.5,
                                                          a[len(bars)//2]*0.5*target_ratio))
    print("  → اختبار بديل: قارن وقفاً ثابتاً 0.5×ATR مقابل 1.5×ATR على نفس الدخول.")


def r4b_stop_compare(bars, target_ratio=1.5):
    """The honest version of R4: same entries, two stop widths.

    The comparison has to be in R, not in win rate. A wider stop lowers the win
    rate the strategy *needs* (break-even falls), so a raw win-rate table shows
    every column failing while saying nothing. What matters is expectancy per
    unit risk. The earlier version of this test compared win rates against a
    single break-even that was identical for every row, which made the table
    unreadable — the break-even depends on the ratio, and the ratio is held
    fixed here on purpose.
    """
    print("\n  R4b: نفس الدخول، وقفان مختلفان — المقارنة بالـR لا بنسبة النجاح")
    e20, e50 = ema_series(bars, 20), ema_series(bars, 50)
    r = rsi([b["close"] for b in bars])
    a = atr(bars)
    print(f"  {'وقف':>10}{'هدف':>9}{'n':>5}{'نجاح%':>8}{'تعادل%':>9}"
          f"{'إجمالي R':>10}{'متوسط R':>10}")
    print("  " + "-" * 60)
    for mult in (0.25, 0.5, 1.0, 1.5, 2.0):
        wins = losses = 0
        total_r = 0.0
        last_exit = -1
        for i in range(LOOKBACK, len(bars) - 1):
            if i <= last_exit or None in (e20[i], e50[i], r[i], a[i]):
                continue
            price = bars[i]["close"]
            up = e20[i] > e50[i]
            near = abs(price - e20[i]) <= a[i] * 1.5
            if up and near and r[i] < 50:
                d = "BUY"
            elif (not up) and near and r[i] > 50:
                d = "SELL"
            else:
                continue
            stop = a[i] * mult
            target = stop * target_ratio
            sgn = 1.0 if d == "BUY" else -1.0
            out, exit_i = resolve(bars, i, d, price - sgn * stop, price + sgn * target)
            last_exit = exit_i
            if out == "win":
                wins += 1
                total_r += (target - COST) / stop
            elif out == "loss":
                losses += 1
                total_r -= (stop + COST) / stop
        n = wins + losses
        if not n:
            continue
        wr = wins / n * 100
        be = break_even(stop, target)
        avg = total_r / n
        mark = "✅" if avg > 0 else "❌"
        print(f"  {mult:>9.2f}×{target:>9.2f}{n:>5}{wr:>7.1f}%{be:>8.1f}%"
              f"{total_r:>+10.2f}{avg:>+10.3f} {mark}")
    print("  → R4 يثبت إذا تحسّن متوسط R مع اتساع الوقف ثم توقف.")


# --------------------------------------------------------------------- R5
def r5_sessions(bars):
    print("\n  R5: هل تختلف حركة السوق بين الجلسات؟")
    # Session analysis needs intraday bars. A daily bar opens at 00:00 UTC, so
    # every daily bar lands in the same session bucket and the test measures
    # nothing. It was run on daily data first and reported four identical rows
    # of 66.24$ — a result that looks like data but is an artifact of the
    # timeframe. The guard below refuses instead of printing it.
    hours = set()
    for b in bars[:50]:
        try:
            from datetime import datetime
            hours.add(datetime.fromisoformat(b["openTime"].replace("Z", "+00:00")).hour)
        except Exception:
            continue
    if len(hours) <= 1:
        print("  ⚠️  لا يمكن الاختبار على هذا الإطار: كل الشمعات تفتح في نفس الساعة.")
        print("      يحتاج تحليلاً داخل اليوم (1m/5m/15m/1h).")
        return
    ranges = {name: [] for name, _, _ in SESSIONS}
    for b in bars:
        try:
            from datetime import datetime
            h = datetime.fromisoformat(b["openTime"].replace("Z", "+00:00")).hour
        except Exception:
            continue
        rng = b["high"] - b["low"]
        for name, a, z in SESSIONS:
            inside = (a <= h < z) if a < z else (h >= a or h < z)
            if inside:
                ranges[name].append(rng)
    med_all = sorted(r for rs in ranges.values() for r in rs)
    med_all = med_all[len(med_all) // 2] if med_all else 0
    for name, rs in ranges.items():
        if rs:
            m = sorted(rs)[len(rs) // 2]
            print(f"  {name:<8} نطاق وسيط {m:7.2f}$  "
                  f"({m/med_all*100:5.1f}% من الوسيط العام)  n={len(rs)}")
    print("  → R5 يثبت إذا كان نطاق لندن/نيويورك أعلى بوضوح.")


def main() -> None:
    ap = argparse.ArgumentParser(description="تحقق من قواعدي على بيانات حقيقية")
    ap.add_argument("--tf", default="1d")
    args = ap.parse_args()
    bars = fetch(args.tf)
    a = atr(bars)
    vals = [x for x in a if x is not None]
    med = sorted(vals)[len(vals) // 2]
    stop = round(med * 0.6, 2)
    target = round(stop * 1.5, 2)
    print("=" * 74)
    print(f"  التحقق — {args.tf} | {len(bars)} شمعة | "
          f"{bars[0]['openTime'][:10]} → {bars[-1]['openTime'][:10]}")
    print(f"  ATR وسيط {med:.2f}$ | وقف {stop}$ هدف {target}$")
    print("=" * 74)
    r1_trend_agreement(bars, stop, target)
    r2_level_touches(bars, stop, target)
    r3_pullback_vs_breakout(bars, stop, target)
    r4_stop_size(bars, target / stop)
    r4b_stop_compare(bars, target / stop)
    r5_sessions(bars)
    print("\n  ملاحظة: النتائج على عينة واحدة. الأرقام الصغيرة ضوضاء.")
    print()


if __name__ == "__main__":
    main()
