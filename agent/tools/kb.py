"""kb — the knowledge base, applied.

This is the difference between a document and a knowledge base: `rules.json`
holds what I know and how strongly, and this file **uses** it. It reads the
rules, reads the live market, and produces a decision that names the rule that
justified each step.

The contract is deliberately narrow. The knowledge base is allowed to:

  - reject an entry (a `gate` rule fired),
  - require a condition (an `entry_condition` rule),
  - clamp a risk parameter (`risk_floor`, `target_selection`),
  - describe a situation (`description_only`),
  - forbid a claim (`claim_gate`).

It is **not** allowed to claim profitability, because `R-PROFITABILITY` is
`unproven` and says so. If a rule is `refuted`, it can never gate a decision —
it exists so the mistake is not repeated, not so it can be obeyed.

    python3 agent/tools/kb.py                 # analyse the market now
    python3 agent/tools/kb.py --tf 1h
    python3 agent/tools/kb.py --json
    python3 agent/tools/kb.py --why R-STOP-MIN   # show one rule's evidence
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from chart_eye import adx, atr, ema, fetch, rsi, swings, structure  # noqa: E402

KB_DIR = Path(__file__).resolve().parent.parent / "knowledge"
RULES = KB_DIR / "rules.json"
EVIDENCE = KB_DIR / "evidence.jsonl"


def load_rules() -> dict:
    return json.loads(RULES.read_text(encoding="utf-8"))


def rules_by_usage(kb: dict) -> dict[str, list[dict]]:
    out: dict[str, list[dict]] = {}
    for r in kb["rules"]:
        out.setdefault(r["usage"], []).append(r)
    return out


def load_evidence(rule_id: str | None = None) -> list[dict]:
    if not EVIDENCE.exists():
        return []
    rows = [json.loads(l) for l in EVIDENCE.read_text(encoding="utf-8").splitlines() if l.strip()]
    if rule_id:
        rows = [r for r in rows if r.get("study") == rule_id]
    return rows


# ---------------------------------------------------------------- market read
def market(tf: str) -> dict:
    bars = fetch(tf)
    c = [b["close"] for b in bars]
    e20, e50 = ema(c, 20), ema(c, 50)
    r, a = rsi(c), atr(bars)
    dx = adx(bars)
    hi, lo = swings(bars, 3)
    i = len(bars) - 1
    st = structure(hi, lo, 2)
    return {
        "tf": tf, "bars": len(bars), "price": c[i],
        "ema20": e20[i], "ema50": e50[i], "rsi": r[i], "atr": a[i],
        "adx": dx[i], "structure": st["label"],
        "up": (e20[i] is not None and e50[i] is not None and e20[i] > e50[i]),
        "dist_atr": abs(c[i] - e20[i]) / a[i] if a[i] else None,
        "last_bar": bars[-1]["openTime"],
    }


# ------------------------------------------------------------------ decisions
def apply(m: dict, kb: dict) -> dict:
    """Walk the rules and build the decision, recording which rule did what."""
    by_use = rules_by_usage(kb)
    log: list[dict] = []
    decision = "NEUTRAL"
    direction = None
    levels = None
    rejects: list[str] = []

    # --- gates: a `proven` gate that fires stops the analysis here
    for rule in by_use.get("gate", []):
        if rule["status"] != "proven":
            continue
        if rule["id"] == "R-TREND-CONFLICT":
            struct_up = "صاعد" in m["structure"]
            struct_down = "هابط" in m["structure"]
            agree = (m["up"] and struct_up) or ((not m["up"]) and struct_down)
            if not agree:
                rejects.append(rule["reason_ar"])
                log.append({"rule": rule["id"], "effect": "reject",
                            "because": rule["reason_ar"], "status": rule["status"]})
    if rejects:
        return {"decision": "NEUTRAL", "direction": None, "levels": None,
                "rejects": rejects, "log": log, "market": m}

    direction = "BUY" if m["up"] else "SELL"

    # --- entry condition: pullback
    for rule in by_use.get("entry_condition", []):
        if rule["id"] == "R-ENTRY-PULLBACK":
            cap = rule["params"]["max_distance_atr"]
            if m["dist_atr"] is None or m["dist_atr"] > cap:
                rejects.append(f"{rule['reason_ar']} (الآن {m['dist_atr']:.2f}×ATR)")
                log.append({"rule": rule["id"], "effect": "reject",
                            "because": f"يبعد {m['dist_atr']:.2f}×ATR عن EMA20",
                            "status": rule["status"]})
            else:
                log.append({"rule": rule["id"], "effect": "pass",
                            "because": f"ارتداد على {m['dist_atr']:.2f}×ATR",
                            "status": rule["status"]})
    # RSI must be on the pullback side
    if m["up"] and m["rsi"] is not None and m["rsi"] >= 50:
        rejects.append(f"اتجاه صاعد لكن RSI {m['rsi']:.1f} ≥ 50 — لا ارتداد")
    if (not m["up"]) and m["rsi"] is not None and m["rsi"] <= 50:
        rejects.append(f"اتجاه هابط لكن RSI {m['rsi']:.1f} ≤ 50 — لا ارتداد")

    if rejects:
        return {"decision": "NEUTRAL", "direction": None, "levels": None,
                "rejects": rejects, "log": log, "market": m}

    # --- risk floor + target selection
    stop_atr = 0.5
    for rule in by_use.get("risk_floor", []):
        if rule["id"] == "R-STOP-MIN":
            stop_atr = max(stop_atr, rule["params"]["min_stop_atr"])
            log.append({"rule": rule["id"], "effect": "clamp",
                        "because": rule["reason_ar"], "status": rule["status"]})
    tp1_r, tp2_r = 1.0, 1.5
    for rule in by_use.get("target_selection", []):
        if rule["id"] == "R-RR-LOW":
            tp1_r = rule["params"]["tp1_r"]
            tp2_r = rule["params"]["tp2_r"]
            log.append({"rule": rule["id"], "effect": "targets",
                        "because": rule["reason_ar"], "status": rule["status"]})

    sgn = 1.0 if direction == "BUY" else -1.0
    entry = m["price"]
    stop = entry - sgn * m["atr"] * stop_atr
    risk = abs(entry - stop)
    levels = {
        "entry": round(entry, 3), "stop": round(stop, 3),
        "tp1": round(entry + sgn * risk * tp1_r, 3),
        "tp2": round(entry + sgn * risk * tp2_r, 3),
        "risk": round(risk, 3), "stop_atr": stop_atr,
    }

    # --- descriptions: recorded, never decisive
    for rule in by_use.get("description_only", []):
        log.append({"rule": rule["id"], "effect": "describe_only",
                    "because": rule["reason_ar"], "status": rule["status"]})

    return {"decision": direction, "direction": direction, "levels": levels,
            "rejects": [], "log": log, "market": m}


def confidence(dec: dict) -> dict:
    """How much this decision is worth — from the evidence, not from feeling."""
    proven = [l for l in dec["log"] if l["status"] == "proven"]
    direction = [l for l in dec["log"] if l["status"] == "direction"]
    return {
        "supporting_proven_rules": [l["rule"] for l in proven],
        "supporting_direction_rules": [l["rule"] for l in direction],
        "caveat_ar": ("القواعد المؤيدة بالاتجاه فقط (لا دلالة إحصائية). "
                      "لا ادّعاء ربحية — العينة لا تكفي."),
        "trades_needed": 94,
    }


# --------------------------------------------------------------------- output
def show(dec: dict, kb: dict) -> None:
    m = dec["market"]
    print("=" * 74)
    print(f"  تحليل بقاعدة المعرفة — {m['tf']} | {m['bars']} شمعة")
    print(f"  آخر شمعة: {m['last_bar'][:16]}   السعر: {m['price']:.2f}")
    print("=" * 74)

    print(f"\n  السوق: اتجاه {'صاعد' if m['up'] else 'هابط'} | "
          f"البنية {m['structure']}")
    print(f"  RSI {m['rsi']:.1f} | ATR {m['atr']:.2f} | ADX "
          f"{m['adx']:.1f} | بُعد عن EMA20 {m['dist_atr']:.2f}×ATR")

    print("\n  القواعد المطبَّقة:")
    for l in dec["log"]:
        mark = {"pass": "✅", "reject": "⛔", "clamp": "🔒", "targets": "🎯",
                "describe_only": "ℹ️"}.get(l["effect"], "•")
        print(f"    {mark} {l['rule']:<22} [{l['status']:<10}] {l['because']}")

    if dec["decision"] == "NEUTRAL":
        print("\n  القرار: NEUTRAL — لا صفقة")
        for r in dec["rejects"]:
            print(f"    ⛔ {r}")
    else:
        lv = dec["levels"]
        print(f"\n  القرار: {dec['decision']}")
        print(f"    الدخول {lv['entry']}  |  الوقف {lv['stop']} "
              f"({lv['risk']}$ = {lv['stop_atr']}×ATR)")
        print(f"    الهدف1 {lv['tp1']}  |  الهدف2 {lv['tp2']}")
        conf = confidence(dec)
        print(f"\n  الثقة:")
        print(f"    قواعد مُثبتة: {', '.join(conf['supporting_proven_rules']) or 'لا شيء'}")
        print(f"    قواعد اتجاه: {', '.join(conf['supporting_direction_rules']) or 'لا شيء'}")
        print(f"    ⚠️  {conf['caveat_ar']}")
    print()


def why(rule_id: str, kb: dict) -> None:
    r = next((x for x in kb["rules"] if x["id"] == rule_id), None)
    if not r:
        print(f"لا قاعدة بالمعرّف {rule_id}")
        return
    print("=" * 74)
    print(f"  {r['id']}  [{r['status']}]")
    print("=" * 74)
    print(f"\n  الادّعاء: {r['statement']}")
    print(f"  الاستخدام: {r['usage']}  |  الفعل: {r['action']}")
    print(f"\n  الدليل:")
    print(json.dumps(r["evidence"], ensure_ascii=False, indent=4))
    ev = load_evidence(rule_id)
    if ev:
        print(f"\n  قياسات مسجّلة ({len(ev)}):")
        for e in ev:
            print(f"    {e['ts']} {e.get('tf','')} — {e.get('note','')}")
    print()


def main() -> None:
    ap = argparse.ArgumentParser(description="قاعدة المعرفة مطبَّقة")
    ap.add_argument("--tf", default="15m")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--why", metavar="RULE_ID")
    args = ap.parse_args()
    kb = load_rules()
    if args.why:
        why(args.why, kb)
        return
    m = market(args.tf)
    dec = apply(m, kb)
    if args.json:
        print(json.dumps({"decision": dec["decision"], "levels": dec["levels"],
                          "rejects": dec["rejects"], "log": dec["log"],
                          "confidence": confidence(dec)}, ensure_ascii=False,
                         indent=2))
    else:
        show(dec, kb)


if __name__ == "__main__":
    main()
