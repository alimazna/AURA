# AYZR — working notes

Gold (XAUUSD) intelligence platform. Arabic RTL UI. Shadow-only: it never sends an order.

## Hard constraints (from the brief — do not violate)

- **Shadow only.** No order/broker/execution path anywhere. Do not add one.
- **Never fabricate a value.** Show "غير متاح" / "لا توجد بيانات" instead of guessing.
- **Separate operator stats from engine stats.** A discretionary trade must not be able to
  flatter or hide an engine's record. Use `statistics().byOrigin`.
- **Audit-grade.** Every analytical output and operator action is written to `audit_log`
  with actor and timestamp.
- Visitors are read-only. Operator actions require `x-operator-token`.

## Layout

```
AYZR/
  server/   Node, .mjs, no build step. node:sqlite (no native deps).
    src/runtime.mjs        ingest loop, region lifecycle, trade evaluation
    src/engines/trades.mjs signals, virtual trades, cost model, look-ahead guard
    src/store/db.mjs       schema + migrations
    src/api/server.mjs     HTTP API
    test/                  node:test, 81 tests
  web/      React + Vite
    src/components/CandlestickChart.jsx
    src/components/Panels.jsx
    src/styles/app.css     single token-based design system
```

## Commands

```bash
cd AYZR/server && npm test          # 81 tests
cd AYZR/web && npm run build        # vite build
AYZR_OPERATOR_TOKEN=... node src/index.mjs   # serve (PORT, default 8787)
```

## Design system

`web/src/styles/app.css` is the single source of truth. Use tokens, never literal
values: type (`--t-micro` … `--t-display`), spacing (`--s1` … `--s4`), color
(`--fg`, `--fg-mid`, `--fg-dim`, `--fg-faint`, `--line`, `--line-strong`, `--blue`).

Two rules that matter for Arabic:

- **11px is the floor.** Arabic is unreadable below it. Verified in-browser: no
  rendered text is smaller.
- **Body text must clear WCAG AA** on its own background. `--fg-faint` is #647f9b
  (4.8:1). `--blue` is for fills and borders only — as text it is 4.24:1 and fails.

Blue is the accent, not a text color. Candle green/red (`UP`/`DOWN` in the chart)
are separate from the semantic `--up`/`--down` used in the UI.

## The look-ahead guard

The rule: a trade created on the close of bar T may only be judged on bars that close
strictly after T. It lives in `engines/trades.mjs` and is **enforced twice** — once in
`eligibleBarsFor()` and again inside `evaluateVirtualTrade()` — so a new call site that
forgets to filter cannot reintroduce the bug. There is a regression test for it; it was
a real bug (four spurious immediate stop-outs).

Any test that resolves a trade must pass a bar timestamped *after* the trade's
creation. `regions-trades.test.mjs` has an `AFTER` constant for this.

## Manual trades

`manualTradeFromDrawing()` builds the *same object shape* as an engine trade — same cost
model, same lot size, same evaluation loop — differing only in `origin: 'manual'` and the
operator's name. That is what keeps them comparable while still separable.

Validation (`validateDrawing()`) rejects: stop on the wrong side of entry, risk < 0.05,
risk above `RUNTIME.manualMaxRiskPoints`, and an entry more than 5% from the live price
(the signature of a mis-drag). A trade with no valid creation time is refused at
construction, because without one it could never be evaluated safely.

The store gains `origin`/`operator`/`note` through an additive migration in
`migrateAdditive()` — existing databases keep their rows and results.

## Gotchas

- **Do not use `e.currentTarget.getBoundingClientRect()` in drag handlers.** During a
  drag the events come from a `window` listener, whose `currentTarget` is not an element.
  Measure a ref instead (see `pointerPrice()` measuring `plotRef` in
  `CandlestickChart.jsx`).
- Chart drags listen on `window`, not on the element, so the drag survives crossing the
  SVG, the readout panel and the drag handle.
- The chart SVG is not the only `<svg>` on the page — the shell has small icon SVGs.
  When targeting it from a script, pick the largest by area.
- Commits push to a branch connected to Lovable. Never force-push or rewrite pushed
  history.
