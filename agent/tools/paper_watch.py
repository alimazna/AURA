"""paper_watch — take shadow trades and report the result.

This is the machine behind "watch and take trades, tell me the result in 12
hours". It is built to be honest about three things:

1. The rule it follows is the one measured in `lessons/03`, not an invented
   one: trade only when the EMA trend and the swing structure agree, enter on
   a pullback to EMA20, require RSI on the pullback side, stop at 0.5xATR.
2. The trades are **paper** trades. Nothing is sent to a broker. The word
   "صفقة" here means a recorded hypothetical, priced from real biquote bars.
3. The sample will be tiny. A 12-hour run on 15m bars might produce one or two
   trades. Two trades decide nothing. The report says so rather than dressing
   them up as a result.

Design notes that matter:

- **Resumable, not a long loop.** `--once` processes any bar that closed since
  the last checkpoint and exits. Running it again never double-counts. A
  sandbox restart cannot lose the run, because the state lives in files.
- **No look-ahead.** A trade opened on the close of bar T is judged only on
  bars that close strictly after T.
- **Pessimistic resolution.** A bar touching both stop and target counts as a
  loss.
- **One position at a time.** The overlapping-count error that inflated an
  earlier 73% figure cannot recur here.

    python3 agent/tools/paper_watch.py --once        # one cycle
    python3 agent/tools/paper_watch.py --report      # summary so far
    python3 agent/tools/paper_watch.py --loop --hours 12 --interval 300
"""
from __future__ import annotations

import argparse
import json
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from chart_eye import atr, adx, ema, fetch, rsi, swings, structure  # noqa: E402

PAPER = Path(__file__).resolve().parent.parent / "paper"
JOURNAL = PAPER / "journal.jsonl"
STATE = PAPER / "state.json"
LOG = PAPER / "watch.log"

TIMEFRAME = "15m"
STOP_ATR = 0.5          # stop distance, in ATR
TP1_R = 1.0             # first target, in R
TP2_R = 1.5             # second target, in R
COST = 0.37             # round-trip cost in USD per unit, from the bot's model
PULLBACK_ATR = 1.5      # how close to EMA20 counts as "at" the level


def now() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def log(msg: str) -> None:
    PAPER.mkdir(parents=True, exist_ok=True)
    line = f"[{now()}] {msg}"
    print(line, flush=True)
    with LOG.open("a", encoding="utf-8") as f:
        f.write(line + "\n")


def load_state() -> dict:
    if STATE.exists():
        return json.loads(STATE.read_text(encoding="utf-8"))
    return {"last_bar": None, "position": None}


def save_state(s: dict) -> None:
    PAPER.mkdir(parents=True, exist_ok=True)
    STATE.write_text(json.dumps(s, ensure_ascii=False, indent=2), encoding="utf-8")


def append(rec: dict) -> None:
    PAPER.mkdir(parents=True, exist_ok=True)
    with JOURNAL.open("a", encoding="utf-8") as f:
        f.write(json.dumps(rec, ensure_ascii=False) + "\n")


# --- The live journal is only ever written by a live run.
#
# A replay must not touch it. The first version of this file wrote to the same
# journal from every code path, so a verification replay (backfilling 131 bars
# of history) appended four trades that never happened live — each stamped
# "at 19:25" but carrying an entry time ten hours earlier. They then showed up
# in the report as real results. That is exactly the kind of fabricated record
# this section exists to prevent, so the sink is now explicit and a replay
# must pass its own.
_LIVE_SINK: list = []


def emit(rec: dict, sink: list | None = None) -> None:
    """Write a record to `sink` if given, otherwise to the live journal."""
    if sink is None:
        append(rec)
    else:
        sink.append(rec)


# ------------------------------------------------------------------ the rule
def read_context(bars):
    """Everything the entry rule needs, computed from bars up to the last one."""
    c = [b["close"] for b in bars]
    e20, e50 = ema(c, 20), ema(c, 50)
    r, a = rsi(c), atr(bars)
    dx = adx(bars)
    hi, lo = swings(bars, 3)
    i = len(bars) - 1
    if None in (e20[i], e50[i], r[i], a[i]):
        return None
    st = structure(hi, lo, 2)
    up = e20[i] > e50[i]
    struct_up = "صاعد" in st["label"]
    struct_down = "هابط" in st["label"]
    agree = (up and struct_up) or ((not up) and struct_down)
    dist = abs(c[i] - e20[i])
    return {
        "i": i, "price": c[i], "ema20": e20[i], "ema50": e50[i],
        "rsi": r[i], "atr": a[i], "adx": dx[i], "structure": st["label"],
        "agree": agree, "dist_atr": dist / a[i] if a[i] else None,
        "up": up,
    }


def entry_signal(ctx) -> tuple[str | None, str]:
    """Return (direction, reason) or (None, why not)."""
    if ctx is None:
        return None, "مؤشرات غير مكتملة"
    if not ctx["agree"]:
        return None, f"لا اتفاق: اتجاه {'صاعد' if ctx['up'] else 'هابط'} والبنية {ctx['structure']}"
    if ctx["dist_atr"] is None or ctx["dist_atr"] > PULLBACK_ATR:
        return None, (f"السعر بعيد عن EMA20 ({ctx['dist_atr']:.2f}×ATR) — "
                      f"الدخول يحتاج ارتداداً لا مطاردة")
    if ctx["up"] and ctx["rsi"] >= 50:
        return None, f"اتجاه صاعد لكن RSI {ctx['rsi']:.1f} ≥ 50 — لا ارتداد"
    if (not ctx["up"]) and ctx["rsi"] <= 50:
        return None, f"اتجاه هابط لكن RSI {ctx['rsi']:.1f} ≤ 50 — لا ارتداد"
    d = "BUY" if ctx["up"] else "SELL"
    reason = (f"اتجاه {'صاعد' if ctx['up'] else 'هابط'} والبنية موافقة؛ "
              f"السعر ارتد إلى EMA20 ({ctx['ema20']:.2f}) على بعد "
              f"{ctx['dist_atr']:.2f}×ATR؛ RSI {ctx['rsi']:.1f} على جانب الارتداد")
    return d, reason


def levels(direction: str, entry: float, atr_v: float) -> dict:
    sgn = 1.0 if direction == "BUY" else -1.0
    stop = entry - sgn * atr_v * STOP_ATR
    risk = abs(entry - stop)
    return {
        "entry": round(entry, 3),
        "stop": round(stop, 3),
        "tp1": round(entry + sgn * risk * TP1_R, 3),
        "tp2": round(entry + sgn * risk * TP2_R, 3),
        "risk": round(risk, 3),
    }


# --------------------------------------------------------------- resolution
def judge(bars, pos, from_index, sink=None):
    """Walk bars after entry. Returns an update dict or None.

    Pessimistic: a bar touching stop and target together is a loss.
    """
    sgn = 1.0 if pos["direction"] == "BUY" else -1.0
    for j in range(from_index, len(bars)):
        b = bars[j]
        if b["openTime"] <= pos["entry_time"]:
            continue
        hit_stop = b["low"] <= pos["stop"] if sgn > 0 else b["high"] >= pos["stop"]
        hit_tp1 = b["high"] >= pos["tp1"] if sgn > 0 else b["low"] <= pos["tp1"]
        hit_tp2 = b["high"] >= pos["tp2"] if sgn > 0 else b["low"] <= pos["tp2"]
        if hit_stop and (hit_tp1 or hit_tp2):
            return {"event": "stop", "bar": b["openTime"], "price": pos["stop"]}
        if hit_stop:
            return {"event": "stop", "bar": b["openTime"], "price": pos["stop"]}
        if hit_tp2:
            return {"event": "tp2", "bar": b["openTime"], "price": pos["tp2"]}
        if hit_tp1 and not pos.get("tp1_hit"):
            pos["tp1_hit"] = True
            emit({"kind": "tp1_hit", "id": pos["id"], "bar": b["openTime"],
                  "price": pos["tp1"], "at": now()}, sink)
    return None


def close_position(pos, ev):
    pos["status"] = "closed"
    pos["outcome"] = ev["event"]
    pos["exit_time"] = ev["bar"]
    pos["exit_price"] = ev["price"]
    if ev["event"] == "tp2":
        pos["result"] = round(pos["risk"] * TP2_R - COST, 3)
    elif ev["event"] == "tp1_then_stop" or (ev["event"] == "stop" and pos.get("tp1_hit")):
        pos["outcome"] = "tp1_then_stop"
        pos["result"] = round(pos["risk"] * TP1_R - COST - pos["risk"] - COST, 3)
    else:
        pos["result"] = round(-pos["risk"] - COST, 3)


# --------------------------------------------------------------------- cycle
def process_range(bars, start_index, state, sink=None):
    """Advance the state machine over bars[start_index:], one bar at a time.

    This is the gap-safe core. The earlier version looked only at the latest
    bar, so any bar that closed while the sandbox was paused was skipped and an
    entry on it was lost. Here every bar after the checkpoint is evaluated in
    order, so a 12-hour run survives the sandbox pausing and resuming: the
    checkpoint is a bar timestamp, not a promise that the process stayed alive.

    `sink` routes records away from the live journal. A replay of history must
    pass one; without it, a verification run writes trades into the journal that
    never happened live.

    Returns a list of human-readable events.
    """
    events: list[str] = []
    for i in range(start_index, len(bars)):
        bar = bars[i]
        if state.get("position"):
            pos = state["position"]
            ev = judge([bar], pos, 0, sink)    # this single bar only
            if ev:
                close_position(pos, ev)
                emit({"kind": "close", "at": now(), **pos}, sink)
                events.append(f"أُغلقت #{pos['id']}: {pos['outcome']} "
                              f"({pos['result']:+.2f}$)")
                state["position"] = None
        if state.get("position") is None:
            ctx = read_context(bars[:i + 1])
            direction, reason = entry_signal(ctx)
            if direction is not None:
                lv = levels(direction, ctx["price"], ctx["atr"])
                pos = {
                    "id": state.get("next_id", 1),
                    "direction": direction, "timeframe": TIMEFRAME,
                    "entry_time": bar["openTime"], "opened_at": now(),
                    "atr": round(ctx["atr"], 3), "rsi": round(ctx["rsi"], 1),
                    "adx": round(ctx["adx"], 1) if ctx["adx"] else None,
                    "reason": reason,
                    "invalidation": (f"إغلاق "
                                     f"{'تحت' if direction == 'BUY' else 'فوق'} "
                                     f"{lv['stop']:.2f} يُبطل"),
                    "status": "open", "tp1_hit": False, **lv,
                }
                state["position"] = pos
                state["next_id"] = pos["id"] + 1
                emit({"kind": "open", "at": now(), **pos}, sink)
                events.append(f"فتحت #{pos['id']} {direction} @ {lv['entry']} "
                              f"وقف {lv['stop']} هدف2 {lv['tp2']}")
        state["last_bar"] = bar["openTime"]
    return events


def once() -> str:
    state = load_state()
    bars = fetch(TIMEFRAME)
    if len(bars) < 60:
        return "بيانات غير كافية"
    latest = bars[-1]["openTime"]

    last = state.get("last_bar")
    if last is None:
        # First run: only consider the latest bar, so a fresh watcher does not
        # backfill a week of history as if it had traded it live.
        start = len(bars) - 1
    else:
        after = [j for j, b in enumerate(bars) if b["openTime"] > last]
        if not after:
            return "لا شمعة جديدة"
        start = after[0]

    events = process_range(bars, start, state)
    save_state(state)
    if not events:
        return f"لا دخول حتى {latest[:16]}"
    return " | ".join(events)


# -------------------------------------------------------------------- report
def replay(bars_back: int = 0) -> str:
    """Backfill a checkpoint over history into a throwaway sink.

    This exists so the entry rule can be exercised on bars that already closed,
    which is the only way to prove the loop takes trades during a quiet market.
    It writes **nowhere near the live journal** — the records go to an in-memory
    list and are printed. The first version of this ran as an ad-hoc script that
    called the same functions, which silently appended four historical trades to
    the live journal; the report then presented them as live results.
    """
    bars = fetch(TIMEFRAME)
    if len(bars) < 80:
        return "بيانات غير كافية"
    start = max(61, len(bars) - bars_back) if bars_back else 61
    state = {"last_bar": bars[start - 1]["openTime"], "position": None}
    sink: list = []
    events = process_range(bars, start, state, sink)
    lines = [f"إعادة تشغيل على {len(bars) - start} شمعة — {len(events)} حدث:"]
    lines += [f"  • {e}" for e in events]
    closes = [r for r in sink if r["kind"] == "close"]
    if closes:
        wins = sum(1 for c in closes if c["result"] > 0)
        net = sum(c["result"] for c in closes)
        lines.append(f"  مغلقة: {len(closes)} | رابحة: {wins} | "
                     f"صافي: {net:+.2f}$")
    lines.append("  (هذه إعادة تشغيل — لم تُكتب في الدفتر الحي)")
    return "\n".join(lines)


def report() -> str:
    if not JOURNAL.exists():
        return "لا سجل بعد."
    opens, closes = [], []
    for line in JOURNAL.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        r = json.loads(line)
        if r["kind"] == "open":
            opens.append(r)
        elif r["kind"] == "close":
            closes.append(r)

    out = []
    out.append("=" * 70)
    out.append("  تقرير صفقات الظل — AYZR")
    out.append("=" * 70)
    out.append(f"  الصفقات المفتوحة: {len(opens)} | المغلقة: {len(closes)}")

    if not closes:
        out.append("\n  لا صفقات مغلقة بعد — لا نتيجة لأي شيء.")
        if opens:
            out.append("  المفتوحة حالياً:")
            for p in opens:
                out.append(f"    #{p['id']} {p['direction']} @ {p['entry']} "
                           f"وقف {p['stop']}")
        return "\n".join(out)

    wins = [c for c in closes if c["result"] > 0]
    losses = [c for c in closes if c["result"] <= 0]
    total = sum(c["result"] for c in closes)
    out.append("")
    out.append(f"  {'#':>3} {'اتجاه':>6} {'دخول':>10} {'وقف':>10} "
               f"{'هدف2':>10} {'نتيجة':>9} {'النتيجة':>10}")
    out.append("  " + "-" * 64)
    for c in closes:
        out.append(f"  {c['id']:>3} {c['direction']:>6} {c['entry']:>10.2f} "
                   f"{c['stop']:>10.2f} {c['tp2']:>10.2f} "
                   f"{c['result']:>+9.2f} {c['outcome']:>10}")
    out.append("  " + "-" * 64)
    out.append(f"  الرابحة: {len(wins)} | الخاسرة: {len(losses)} | "
               f"نسبة النجاح: {len(wins)/len(closes)*100:.1f}%")
    out.append(f"  صافي النتيجة: {total:+.2f}$")
    out.append("")
    out.append("  ⚠️  تحذير: العينة صغيرة جداً. لا استنتاج من عدد قليل من الصفقات.")
    out.append("      القاعدة تحتاج ~94 صفقة مغلقة قبل أي حكم. هذه ليست نتيجة.")
    return "\n".join(out)


def main() -> None:
    ap = argparse.ArgumentParser(description="صفقات ظل حقيقية + تقرير")
    ap.add_argument("--once", action="store_true")
    ap.add_argument("--report", action="store_true")
    ap.add_argument("--replay", type=int, nargs="?", const=0, default=None,
                    metavar="N",
                    help="أعد تشغيل على آخر N شمعة (لا يكتب في الدفتر الحي)")
    ap.add_argument("--loop", action="store_true")
    ap.add_argument("--hours", type=float, default=12.0)
    ap.add_argument("--interval", type=int, default=300)
    args = ap.parse_args()

    if args.replay is not None:
        print(replay(args.replay))
        return
    if args.report:
        print(report())
        return
    if args.once:
        log(once())
        return
    if args.loop:
        end = time.time() + args.hours * 3600
        cycles = 0
        while time.time() < end:
            try:
                log(once())
            except Exception as e:  # noqa: BLE001
                log(f"خطأ في الدورة: {e}")
            cycles += 1
            if time.time() >= end:
                break
            time.sleep(args.interval)
        log(f"انتهت المراقبة بعد {cycles} دورة.")
        print(report())
        return
    ap.print_help()


if __name__ == "__main__":
    main()
