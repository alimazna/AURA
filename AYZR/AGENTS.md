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

## Matching the reference dashboard

`aa/AYZR_Reference_Dashboard.png` (1672x941) is the visual spec. Every colour token in
`styles/app.css` is sampled from it with PIL, not eyeballed — re-measure before changing
one. Measured surfaces: top bar `#010917`, sidebar `#030d1e`, chart `#020b1a`, right panel
`#020e20`, hairline `#010a19`. Accents: blue `#064dcf`, cyan `#13b8fa`, gold `#fbbf1d`,
green `#01cd75`, red `#e93d53`, text `#ffffff` / muted `#9abbe5`. The chart's `UP`/`DOWN`/
`GRID`/`AXIS_TEXT` constants mirror the same values so candles and UI never drift apart.

Layout, measured the same way: the reference is one full-width chart panel with the market
strip **inside it** as a header row, then one row of three cards. The opening screen must
show all of that without scrolling — chart panel + card row + gaps has to total the main
area height. Today: `height={357}` on `CandlestickChart` and `max-height: 288px` on
`.grid-3 > .panel` sum to 855px. Change one and you must re-tune the other, then re-measure
with `python3 /tmp/fit.py`.

Density is the honest check, not eyeballing: compare bright-pixel share per 60px band
against the reference. The reference's extra brightness is decorative sample data (fake
prices, extra cards) that must NOT be copied — only the layout and colour are real
specifications.

## Chart label collisions

Three things print numbers in the same right-hand column: the price scale, the pane guide
values (RSI 30/70, etc.) and the last-price badge. Any two landing within ~14px of each
other read as one garbled number. `paneGuides` decides placement in a single pass over all
panes top-to-bottom, keeping a label only when it clears the previous one; price ticks are
then dropped when they would collide with either the guides or the badge. Verify with
`/tmp/verify3.py` — it reports the text-overlap count and must be 0.

## Dashboard panel height

`.grid-3` stretches every panel to the tallest sibling. With 12 signal rows that pushed the
row to 770px and the first screen showed one panel plus empty space. Panels are capped
(`max-height`) and scroll internally instead.
