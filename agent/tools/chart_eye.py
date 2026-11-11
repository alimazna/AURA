"""chart_eye — how I read a gold chart.

This is my own instrument. It exists so that every statement I make about the
market can be traced to a bar index in real biquote data, instead of to a
feeling. If the data cannot support a claim, the claim is reported as
"غير متاح" — never guessed.

What it reports, and where each answer comes from:

  trend        EMA20/50/200 stack, plus the swing sequence. Two independent
               readings; when they disagree the report says so rather than
               picking the one that sounds better.
  structure    the sequence of swing highs and lows: higher-high/higher-low is
               an uptrend, lower-high/lower-low a downtrend, mixed is a range.
  levels       swing points clustered into zones within a tolerance of ATR, with
               a touch count. A zone touched once is a line, not a level.
  supply/demand  a small-bodied base followed by a large-bodied impulse. The zone
               is the base's range. That is the definition, applied mechanically.
  momentum     RSI (Wilder) and MACD.
  volatility   ATR and its percentile inside the sample.
  regime       trending vs ranging, from ADX and the EMA spread.
  session      which of Sydney/Tokyo/London/New York is open, in UTC.
  plan         where I would draw, where entry becomes valid, where it is void.

Nothing here reads ahead of the current bar. Every swing is confirmed only
after `k` bars have closed past it, so a level shown on the last bar is one I
could actually have seen at that moment.

    python3 agent/tools/chart_eye.py --tf 5m
    python3 agent/tools/chart_eye.py --tf 1d --json
"""
from __future__ import annotations

import argparse
import json
import sys
import urllib.request
from datetime import datetime, timezone

BIQUOTE = "https://biquote.io/api/XAUUSD/ohlc"
UA = "AYZR-AURIX/1.0"

# Sessions in UTC. London/New York overlap is the liquid window for gold.
SESSIONS = [
    ("سيدني", 21, 6), ("طوكيو", 0, 9), ("لندن", 7, 16), ("نيويورك", 12, 21),
]


# ------------------------------------------------------------------ fetching
def fetch(interval: str, limit: int = 5000) -> list[dict]:
    url = f"{BIQUOTE}?interval={interval}&limit={limit}"
    req = urllib.request.Request(url, headers={"User-Agent": UA})
    doc = json.loads(urllib.request.urlopen(req, timeout=45).read())
    bars = [b for b in doc.get("bars", []) if not b.get("isOpen")]
    bars.sort(key=lambda x: x["openTime"])
    return bars


# ---------------------------------------------------------------- indicators
def ema(vals: list[float], n: int) -> list[float | None]:
    out: list[float | None] = [None] * len(vals)
    if len(vals) < n:
        return out
    k = 2 / (n + 1)
    e = sum(vals[:n]) / n
    out[n - 1] = e
    for i in range(n, len(vals)):
        e = vals[i] * k + e * (1 - k)
        out[i] = e
    return out


def rsi(vals: list[float], n: int = 14) -> list[float | None]:
    out: list[float | None] = [None] * len(vals)
    if len(vals) < n + 1:
        return out
    g = l = 0.0
    for i in range(1, n + 1):
        d = vals[i] - vals[i - 1]
        g += max(d, 0.0)
        l += max(-d, 0.0)
    ag, al = g / n, l / n
    out[n] = 100.0 if al == 0 else 100 - 100 / (1 + ag / al)
    for i in range(n + 1, len(vals)):
        d = vals[i] - vals[i - 1]
        ag = (ag * (n - 1) + max(d, 0.0)) / n
        al = (al * (n - 1) + max(-d, 0.0)) / n
        out[i] = 100.0 if al == 0 else 100 - 100 / (1 + ag / al)
    return out


def atr(bars: list[dict], n: int = 14) -> list[float | None]:
    out: list[float | None] = [None] * len(bars)
    if len(bars) < n + 1:
        return out
    trs = [None]
    for i in range(1, len(bars)):
        trs.append(max(bars[i]["high"] - bars[i]["low"],
                       abs(bars[i]["high"] - bars[i - 1]["close"]),
                       abs(bars[i]["low"] - bars[i - 1]["close"])))
    for i in range(n, len(bars)):
        out[i] = sum(trs[i - n + 1:i + 1]) / n
    return out


def adx(bars: list[dict], n: int = 14) -> list[float | None]:
    """Wilder's ADX. Needs 2n bars before the first value, as the definition does."""
    m = len(bars)
    out: list[float | None] = [None] * m
    if m < 2 * n + 1:
        return out
    tr, pdm, ndm = [0.0] * m, [0.0] * m, [0.0] * m
    for i in range(1, m):
        up = bars[i]["high"] - bars[i - 1]["high"]
        dn = bars[i - 1]["low"] - bars[i]["low"]
        pdm[i] = up if (up > dn and up > 0) else 0.0
        ndm[i] = dn if (dn > up and dn > 0) else 0.0
        tr[i] = max(bars[i]["high"] - bars[i]["low"],
                    abs(bars[i]["high"] - bars[i - 1]["close"]),
                    abs(bars[i]["low"] - bars[i - 1]["close"]))
    str_, sp, sn = sum(tr[1:n + 1]), sum(pdm[1:n + 1]), sum(ndm[1:n + 1])
    dxs: list[float] = []
    for i in range(n + 1, m):
        str_ = str_ - str_ / n + tr[i]
        sp = sp - sp / n + pdm[i]
        sn = sn - sn / n + ndm[i]
        if str_ == 0:
            continue
        pdi, ndi = 100 * sp / str_, 100 * sn / str_
        s = pdi + ndi
        dxs.append(100 * abs(pdi - ndi) / s if s else 0.0)
        if len(dxs) >= n:
            out[i] = sum(dxs[-n:]) / n
    return out


def macd(vals: list[float], fast: int = 12, slow: int = 26, sig: int = 9):
    ef, es = ema(vals, fast), ema(vals, slow)
    line = [(ef[i] - es[i]) if (ef[i] is not None and es[i] is not None) else None
            for i in range(len(vals))]
    have = [v for v in line if v is not None]
    sigs = ema(have, sig) if len(have) >= sig else []
    offset = len(line) - len(have)
    signal: list[float | None] = [None] * len(vals)
    for j, v in enumerate(sigs):
        if v is not None:
            signal[offset + j] = v
    return line, signal


# -------------------------------------------------------------- market shape
def swings(bars: list[dict], k: int = 3) -> tuple[list[dict], list[dict]]:
    """Confirmed swing highs and lows.

    A bar is a swing high only when `k` bars on each side are lower. That means
    the most recent `k` bars can never be swings — which is correct: at that
    moment the turn is not yet visible.
    """
    highs, lows = [], []
    for i in range(k, len(bars) - k):
        window = bars[i - k:i + k + 1]
        if bars[i]["high"] == max(b["high"] for b in window):
            highs.append({"i": i, "price": bars[i]["high"], "time": bars[i]["openTime"]})
        if bars[i]["low"] == min(b["low"] for b in window):
            lows.append({"i": i, "price": bars[i]["low"], "time": bars[i]["openTime"]})
    return highs, lows


def structure(highs: list[dict], lows: list[dict], n: int = 2) -> dict:
    """Read the swing sequence as HH/HL (up), LH/LL (down), or mixed (range).

    `n` is how many swings must agree. The default is 2 — the standard
    market-structure test (is the newest high above the previous high, and the
    newest low above the previous low). A stricter `n` reads a trend far less
    often: on 499 daily bars, n=4 called only 3 bars "agreeing with the EMA
    trend" while n=2 called many more, because demanding four monotonic swings
    in a row is rare in a market that spends most of its time ranging. Both
    readings are available; the stricter one is the more conservative.
    """
    if len(highs) < n or len(lows) < n:
        return {"label": "غير متاح", "detail": "قمم وقيعان غير كافية"}
    hs, ls = highs[-n:], lows[-n:]
    hh = all(hs[j]["price"] > hs[j - 1]["price"] for j in range(1, len(hs)))
    hl = all(ls[j]["price"] > ls[j - 1]["price"] for j in range(1, len(ls)))
    lh = all(hs[j]["price"] < hs[j - 1]["price"] for j in range(1, len(hs)))
    ll = all(ls[j]["price"] < ls[j - 1]["price"] for j in range(1, len(ls)))
    if hh and hl:
        label = "صاعد (قمم وقيعان أعلى)"
    elif lh and ll:
        label = "هابط (قمم وقيعان أدنى)"
    else:
        label = "عرضي (بنية مختلطة)"
    return {"label": label, "higher_highs": hh, "higher_lows": hl,
            "lower_highs": lh, "lower_lows": ll,
            "detail": f"آخر {len(hs)} قمم و{len(ls)} قيعان"}


def zones(points: list[dict], tolerance: float, min_touches: int = 2,
          max_width: float | None = None) -> list[dict]:
    """Cluster swing prices into zones. One touch is a line; two or more is a zone.

    `max_width` refuses a cluster that has grown wider than a zone can usefully
    be. Without it, a loose tolerance merges unrelated swings into one band
    spanning hundreds of dollars — a "level" no one could draw or trade against.
    The first measurement on daily gold produced exactly that: a 180$ band
    labelled as one resistance with 10 touches. A level has to be narrow enough
    to place a stop behind, so the width is capped and the cap is stated.
    """
    if not points:
        return []
    pts = sorted(points, key=lambda p: p["price"])
    out: list[list[dict]] = []
    cluster = [pts[0]]
    for p in pts[1:]:
        span = p["price"] - cluster[0]["price"]
        if (p["price"] - cluster[-1]["price"] <= tolerance
                and (max_width is None or span <= max_width)):
            cluster.append(p)
        else:
            out.append(cluster)
            cluster = [p]
    out.append(cluster)
    res = []
    for c in out:
        prices = [p["price"] for p in c]
        res.append({
            "low": min(prices), "high": max(prices),
            "mid": sum(prices) / len(prices),
            "width": max(prices) - min(prices),
            "touches": len(c),
            "last_index": max(p["i"] for p in c),
        })
    res = [z for z in res if z["touches"] >= min_touches]
    res.sort(key=lambda z: -z["touches"])
    return res


def nearest(zs: list[dict], price: float, side: str) -> list[dict]:
    """Levels on one side of price, closest first, with distance in ATR.

    This is the answer to "where do I draw": the nearest level above and the
    nearest below, not a list sorted by touch count that mixes both sides.
    """
    picked = [z for z in zs
              if (z["mid"] > price if side == "above" else z["mid"] < price)]
    picked.sort(key=lambda z: abs(z["mid"] - price))
    return picked



def supply_demand(bars: list[dict], atr_v: float, lookback: int = 200) -> list[dict]:
    """Base-then-impulse zones: a small candle cluster followed by a large move.

    Mechanical definition, no discretion: the base candle's range is under
    1.0x ATR, the next candle's body is over 1.2x ATR. The zone is the base's
    high-low. A definition that can be argued about is a definition that will
    be bent, so this one is fixed and stated.
    """
    if atr_v <= 0:
        return []
    out: list[dict] = []
    start = max(1, len(bars) - lookback)
    for i in range(start, len(bars) - 1):
        base = bars[i]
        if base["high"] - base["low"] > atr_v * 1.0:
            continue
        nxt = bars[i + 1]
        body = abs(nxt["close"] - nxt["open"])
        if body < atr_v * 1.2:
            continue
        out.append({"low": base["low"], "high": base["high"],
                    "kind": "demand" if nxt["close"] > nxt["open"] else "supply",
                    "index": i, "time": base["openTime"],
                    "impulse_atr": round(body / atr_v, 2)})
    out.sort(key=lambda z: -z["index"])
    return out[:6]


def percentile(vals: list[float], v: float) -> float:
    if not vals:
        return 0.0
    return sum(1 for x in vals if x <= v) / len(vals) * 100.0


def open_sessions(now: datetime) -> list[str]:
    h = now.hour
    out = []
    for name, a, b in SESSIONS:
        inside = (a <= h < b) if a < b else (h >= a or h < b)
        if inside:
            out.append(name)
    return out


# -------------------------------------------------------------------- report
def read(bars: list[dict], tf: str) -> dict:
    closes = [b["close"] for b in bars]
    e20, e50, e200 = ema(closes, 20), ema(closes, 50), ema(closes, 200)
    r, a = rsi(closes), atr(bars)
    dx = adx(bars)
    ml, ms = macd(closes)
    highs, lows = swings(bars, k=3)
    last = len(bars) - 1
    price = closes[last]

    rep: dict = {"timeframe": tf, "bars": len(bars),
                 "from": bars[0]["openTime"], "to": bars[-1]["openTime"],
                 "price": round(price, 3)}

    ema_up = (e20[last] is not None and e50[last] is not None
              and e20[last] > e50[last])
    stack_up = (e200[last] is not None and e20[last] > e50[last] > e200[last])
    st = structure(highs, lows)
    trend_ema = ("صاعد" if ema_up else "هابط") if (e20[last] and e50[last]) else "غير متاح"
    agree = ((trend_ema == "صاعد" and "صاعد" in st["label"]) or
             (trend_ema == "هابط" and "هابط" in st["label"]))
    rep["trend"] = {
        "ema": trend_ema,
        "ema_stack_full": bool(stack_up),
        "structure": st["label"],
        "agree": bool(agree),
        "note": "" if agree else "تعارض: المتوسطات والبنية لا يتفقان — لا اتجاه واضح",
    }

    # Tolerance is a fraction of ATR, and the cluster width is capped at the same
    # figure so a level stays drawable. 0.25xATR on daily gold is ~19$ — wide
    # enough to catch a real level, narrow enough to place a stop behind.
    atr_now = a[last] or 0.0
    tol = atr_now * 0.25
    all_zones = zones(highs + lows, tol, min_touches=2, max_width=atr_now * 0.5)
    above = [z for z in nearest(all_zones, price, "above")][:4]
    below = [z for z in nearest(all_zones, price, "below")][:4]
    for z in above + below:
        z["dist_atr"] = round(abs(z["mid"] - price) / atr_now, 2) if atr_now else None
        z["side"] = "فوق" if z["mid"] > price else "تحت"
    rep["levels"] = {"above": above, "below": below, "tolerance": round(tol, 3),
                     "max_width": round(atr_now * 0.5, 3)}
    rep["supply_demand"] = supply_demand(bars, atr_now)

    rep["momentum"] = {
        "rsi": round(r[last], 1) if r[last] is not None else None,
        "macd_line": round(ml[last], 3) if ml[last] is not None else None,
        "macd_signal": round(ms[last], 3) if ms[last] is not None else None,
        "macd_hist": (round(ml[last] - ms[last], 3)
                      if ml[last] is not None and ms[last] is not None else None),
    }
    atr_vals = [x for x in a if x is not None]
    rep["volatility"] = {
        "atr": round(a[last], 3) if a[last] is not None else None,
        "atr_percentile": round(percentile(atr_vals, a[last]), 1) if a[last] else None,
        "atr_median": round(sorted(atr_vals)[len(atr_vals) // 2], 3) if atr_vals else None,
    }

    adx_v = dx[last]
    if adx_v is None:
        rep["regime"] = {"label": "غير متاح", "adx": None}
    elif adx_v >= 25 and rep["trend"]["agree"]:
        rep["regime"] = {"label": "اتجاه قوي", "adx": round(adx_v, 1)}
    elif adx_v >= 25:
        rep["regime"] = {"label": "اتجاه بلا اتفاق", "adx": round(adx_v, 1)}
    elif adx_v < 20:
        rep["regime"] = {"label": "عرضي", "adx": round(adx_v, 1)}
    else:
        rep["regime"] = {"label": "انتقالي", "adx": round(adx_v, 1)}

    now = datetime.now(timezone.utc)
    rep["session"] = {"utc": now.strftime("%H:%M"), "open": open_sessions(now)}
    return rep


def plan(rep: dict) -> list[str]:
    """Where I would draw, when entry is valid, and what voids it.

    Every line is derived from the report above. If the trend is unavailable or
    the two readings disagree, the plan says to stand aside — a plan that
    invents a direction is worse than no plan.
    """
    out: list[str] = []
    t = rep["trend"]
    if not t["agree"]:
        out.append("لا خطة: المتوسطات والبنية لا يتفقان.")
        return out
    up = t["ema"] == "صاعد"
    lv = rep["levels"]
    near = lv["below"] if up else lv["above"]
    far = lv["above"] if up else lv["below"]
    if near:
        z = near[0]
        out.append(f"ارسم {'دعماً' if up else 'مقاومة'} عند {z['mid']:.2f} "
                   f"({z['touches']} لمسات، عرض {z['width']:.2f}$، "
                   f"يبعد {z['dist_atr']}×ATR)")
    else:
        out.append("لا منطقة قريبة مؤكدة — لا أرسم مستوى من لمسة واحدة")
    if far:
        z = far[0]
        out.append(f"ارسم الهدف عند {z['mid']:.2f} ({z['touches']} لمسات، "
                   f"يبعد {z['dist_atr']}×ATR)")
    atr_v = rep["volatility"]["atr"]
    if atr_v:
        out.append(f"الوقف لا يقل عن {atr_v*0.5:.2f}$ (0.5×ATR) وإلا ضربه الضجيج")
    out.append("الدخول صحيح عند ارتداد السعر إلى المنطقة، لا عند مطاردته")
    out.append(f"يبطل إذا أُغلق {'تحت' if up else 'فوق'} أقرب منطقة، أو انقلبت البنية")
    return out


def show(rep: dict) -> None:
    print("=" * 74)
    print(f"  قراءة الشارت — {rep['timeframe']} | {rep['bars']} شمعة")
    print(f"  {rep['from'][:16]} → {rep['to'][:16]}")
    print("=" * 74)
    print(f"\n  السعر: {rep['price']}")
    t = rep["trend"]
    print(f"\n  الاتجاه (متوسطات): {t['ema']}  |  الترتيب الكامل: "
          f"{'نعم' if t['ema_stack_full'] else 'لا'}")
    print(f"  البنية: {t['structure']}")
    print(f"  اتفاق القراءتين: {'نعم ✅' if t['agree'] else 'لا ❌'}")
    if t["note"]:
        print(f"  {t['note']}")

    print(f"\n  النظام: {rep['regime']['label']} (ADX {rep['regime']['adx']})")
    m, v = rep["momentum"], rep["volatility"]
    print(f"  الزخم: RSI {m['rsi']} | MACD {m['macd_line']} "
          f"(إشارة {m['macd_signal']}, فرق {m['macd_hist']})")
    print(f"  التقلب: ATR {v['atr']} | شريحة {v['atr_percentile']}% من العينة "
          f"(وسيط {v['atr_median']})")
    print(f"  الجلسات المفتوحة: {'، '.join(rep['session']['open']) or 'لا شيء'} "
          f"({rep['session']['utc']} UTC)")

    print(f"\n  المستويات (تسامح {rep['levels']['tolerance']}$, "
          f"أقصى عرض {rep['levels']['max_width']}$):")
    print("    فوق السعر:")
    for z in rep["levels"]["above"]:
        print(f"      {z['low']:.2f} – {z['high']:.2f}  ({z['touches']} لمسات، "
              f"عرض {z['width']:.2f}$، يبعد {z['dist_atr']}×ATR)")
    if not rep["levels"]["above"]:
        print("      لا مستوى مؤكد فوق السعر")
    print("    تحت السعر:")
    for z in rep["levels"]["below"]:
        print(f"      {z['low']:.2f} – {z['high']:.2f}  ({z['touches']} لمسات، "
              f"عرض {z['width']:.2f}$، يبعد {z['dist_atr']}×ATR)")
    if not rep["levels"]["below"]:
        print("      لا مستوى مؤكد تحت السعر")

    print("\n  مناطق العرض/الطلب (قاعدة + اندفاع):")
    for z in rep["supply_demand"]:
        kind = "طلب" if z["kind"] == "demand" else "عرض"
        print(f"    {kind}: {z['low']:.2f} – {z['high']:.2f} "
              f"(اندفاع {z['impulse_atr']}×ATR) @ {z['time'][:16]}")
    if not rep["supply_demand"]:
        print("    لا مناطق مطابقة للتعريف")

    print("\n  الخطة:")
    for ln in plan(rep):
        print(f"    • {ln}")
    print()


def main() -> None:
    ap = argparse.ArgumentParser(description="اقرأ شارت الذهب بصدق")
    ap.add_argument("--tf", default="5m", help="الإطار الزمني")
    ap.add_argument("--json", action="store_true", help="أخرج JSON")
    args = ap.parse_args()
    try:
        bars = fetch(args.tf)
    except Exception as e:  # noqa: BLE001
        print(f"تعذّر جلب البيانات: {e}", file=sys.stderr)
        raise SystemExit(1)
    if len(bars) < 60:
        print(f"بيانات غير كافية: {len(bars)} شمعة فقط", file=sys.stderr)
        raise SystemExit(1)
    rep = read(bars, args.tf)
    if args.json:
        print(json.dumps(rep, ensure_ascii=False, indent=2))
    else:
        show(rep)


if __name__ == "__main__":
    main()
