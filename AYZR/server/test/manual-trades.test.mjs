// Manual (operator-drawn) shadow trades.
//
// These tests pin the guarantees that make a hand-drawn trade trustworthy: it pays the
// same costs as an engine trade, it is judged only against bars that closed after it was
// drawn, it can never be mistaken for a real order, and its statistics stay separate
// from the engines'. A drawing that makes no sense must be refused rather than stored.

import { test } from 'node:test';
import assert from 'node:assert/strict';
import {
  manualTradeFromDrawing, validateDrawing, eligibleBarsFor,
  evaluateVirtualTrade, closeVirtualTrade, statistics, COST_MODEL,
} from '../src/engines/trades.mjs';
import { Store } from '../src/store/db.mjs';
import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';

const NOW = Date.parse('2026-01-05T12:00:00Z');
const TRIGGER = '2026-01-05T11:45:00Z';

const drawing = (over = {}) => ({
  direction: 'LONG', entry: 4000, stop: 3990, target: 4020,
  timeframe: 'M15', operator: 'ali', nowMs: NOW, triggerBarTime: TRIGGER,
  spread: 0.16, ...over,
});

function bar(o, h, l, c, time) {
  return { openTime: time, time: Date.parse(time), open: o, high: h, low: l, close: c, tickVolume: 1, isOpen: false };
}

/* ------------------------------------------------------------- creation ---- */

test('a manual trade is marked manual and records who drew it', () => {
  const t = manualTradeFromDrawing(drawing());
  assert.equal(t.origin, 'manual');
  assert.equal(t.operator, 'ali');
  assert.equal(t.engine, 'MANUAL');
  assert.equal(t.state, 'OPEN');
  assert.equal(t.regionId, null);
  assert.equal(t.signalId, null);
});

test('a manual trade pays the same entry costs as an engine trade', () => {
  const t = manualTradeFromDrawing(drawing());
  // Long entries are filled worse than the drawn level: half the spread plus slippage.
  assert.equal(t.entry, 4000 + 0.16 / 2 + COST_MODEL.slippagePerSide);
  assert.equal(t.costs.slippagePerSide, COST_MODEL.slippagePerSide);
  assert.equal(t.costs.modelled, true);
  assert.equal(t.lot, COST_MODEL.lotSize);
});

test('a short drawing is filled below the drawn level, symmetrically to a long', () => {
  const t = manualTradeFromDrawing(drawing({ direction: 'SHORT', entry: 4000, stop: 4010, target: 3980 }));
  assert.equal(t.entry, 4000 - 0.16 / 2 - COST_MODEL.slippagePerSide);
  assert.equal(t.stop, 4010);
  assert.equal(t.target, 3980);
});

test('risk-to-reward is derived from the drawn levels', () => {
  const t = manualTradeFromDrawing(drawing());     // drawn risk 10, drawn reward 20
  // The fill moves the entry by the modelled costs, so the realised ratio is just
  // under 2. It must never round up to a flattering number.
  assert.ok(t.riskReward < 2 && t.riskReward > 1.8, `expected ~1.9, got ${t.riskReward}`);
  assert.equal(Math.abs(t.target - t.entry) / Math.abs(t.entry - t.stop) > 0, true);
});

test('a manual trade carries no order path', () => {
  const t = manualTradeFromDrawing(drawing());
  const keys = Object.keys(t).join(',').toLowerCase();
  for (const forbidden of ['orderid', 'broker', 'ticket', 'executionid', 'positionid']) {
    assert.ok(!keys.includes(forbidden), `manual trade must not carry "${forbidden}"`);
  }
});

/* --------------------------------------------------------- look-ahead ---- */

test('a manual trade is judged only against bars that closed after the drawing', () => {
  const t = manualTradeFromDrawing(drawing());
  const bars = [
    bar(3995, 3999, 3980, 3981, '2026-01-05T11:30:00Z'),  // before the drawing
    bar(4000, 4002, 3999, 4001, '2026-01-05T11:45:00Z'),  // the bar it was drawn on
    bar(4001, 4025, 4000, 4024, '2026-01-05T12:00:00Z'),  // after
  ];
  const eligible = eligibleBarsFor(t, bars);
  assert.equal(eligible.length, 1);
  assert.equal(eligible[0].openTime, '2026-01-05T12:00:00Z');
});

test('a bar that dipped below the stop before the drawing cannot close it out', () => {
  const t = manualTradeFromDrawing(drawing());          // stop 3990
  const before = bar(4000, 4001, 3950, 3960, '2026-01-05T11:30:00Z');
  assert.equal(eligibleBarsFor(t, [before]).length, 0);
  assert.equal(evaluateVirtualTrade(t, before).closed, false);
});

test('a manual trade with no usable creation time cannot be built at all', () => {
  // Without a timestamp the trade could never be evaluated safely, so it is refused
  // at construction rather than stored in a state that can never resolve.
  assert.throws(() => manualTradeFromDrawing(drawing({ nowMs: NaN })), /creation time/);
});

test('a manual trade with no trigger bar falls back to its open time for the look-ahead guard', () => {
  const t = manualTradeFromDrawing(drawing({ triggerBarTime: null }));
  assert.equal(t.triggerBarTime, null);
  const before = [bar(4000, 4050, 3950, 4000, '2026-01-05T11:59:00Z')];
  const after = [bar(4000, 4050, 3950, 4000, '2026-01-05T12:01:00Z')];
  assert.equal(eligibleBarsFor(t, before).length, 0);
  assert.equal(eligibleBarsFor(t, after).length, 1);
});

/* --------------------------------------------------------- evaluation ---- */

test('a manual trade closes on target and reports a positive R multiple', () => {
  const t = manualTradeFromDrawing(drawing());
  const r = evaluateVirtualTrade(t, bar(4001, 4030, 4000, 4029, '2026-01-05T12:15:00Z'));
  assert.equal(r.closed, true);
  assert.equal(r.result, 'WIN');
  const closed = closeVirtualTrade(t, { ...r, exit: r.exit, atMs: r.atMs });
  assert.ok(closed.rMultiple > 0);
  assert.ok(closed.pnlUsd > 0);
});

test('a manual trade closes on stop with a negative R multiple', () => {
  const t = manualTradeFromDrawing(drawing());
  const r = evaluateVirtualTrade(t, bar(3999, 4000, 3985, 3986, '2026-01-05T12:15:00Z'));
  assert.equal(r.closed, true);
  assert.equal(r.result, 'LOSS');
  const closed = closeVirtualTrade(t, { ...r, exit: r.exit, atMs: r.atMs });
  assert.ok(closed.rMultiple < 0);
});

test('stop is taken before target when one bar spans both, as for engine trades', () => {
  const t = manualTradeFromDrawing(drawing());
  const r = evaluateVirtualTrade(t, bar(4000, 4030, 3980, 4000, '2026-01-05T12:15:00Z'));
  assert.equal(r.result, 'LOSS');
});

/* --------------------------------------------------------- validation ---- */

test('a drawing that is not a valid trade is rejected', () => {
  const cases = [
    [{ direction: 'LONG', entry: 4000, stop: 4010, target: 4020 }, 'long stop above entry'],
    [{ direction: 'LONG', entry: 4000, stop: 3990, target: 3980 }, 'long target below entry'],
    [{ direction: 'SHORT', entry: 4000, stop: 3990, target: 3980 }, 'short stop below entry'],
    [{ direction: 'SHORT', entry: 4000, stop: 4010, target: 4020 }, 'short target above entry'],
    [{ direction: 'SIDEWAYS', entry: 4000, stop: 3990, target: 4020 }, 'unknown direction'],
    [{ direction: 'LONG', entry: 0, stop: 3990, target: 4020 }, 'non-positive entry'],
    [{ direction: 'LONG', entry: NaN, stop: 3990, target: 4020 }, 'NaN entry'],
  ];
  for (const [d, label] of cases) {
    const errors = validateDrawing({ ...d, quote: { mid: 4000 }, maxRiskPoints: 60 });
    assert.ok(errors.length > 0, `${label} should be rejected`);
  }
});

test('a valid drawing passes validation', () => {
  assert.deepEqual(
    validateDrawing({ direction: 'LONG', entry: 4000, stop: 3990, target: 4020, quote: { mid: 4000 }, maxRiskPoints: 60 }),
    [],
  );
});

test('a drawing far from the live price is refused as a likely mis-drag', () => {
  const errors = validateDrawing({ direction: 'LONG', entry: 5000, stop: 4990, target: 5020, quote: { mid: 4000 }, maxRiskPoints: 60 });
  assert.ok(errors.some((e) => e.includes('away from the live price')));
});

test('an oversized risk is refused', () => {
  const errors = validateDrawing({ direction: 'LONG', entry: 4000, stop: 3800, target: 4400, quote: { mid: 4000 }, maxRiskPoints: 60 });
  assert.ok(errors.some((e) => e.includes('exceeds the 60 point limit')));
});

test('a stop a hair away from the entry is refused', () => {
  const errors = validateDrawing({ direction: 'LONG', entry: 4000, stop: 3999.99, target: 4020, quote: { mid: 4000 }, maxRiskPoints: 60 });
  assert.ok(errors.length > 0);
});

/* ------------------------------------------------------- persistence ---- */

test('a manual trade round-trips through the store with its origin intact', () => {
  const dir = mkdtempSync(join(tmpdir(), 'ayzr-'));
  try {
    const store = new Store(join(dir, 'test.sqlite'));
    const t = manualTradeFromDrawing(drawing());
    store.upsertVirtualTrade(t);
    const back = store.getVirtualTrades({ limit: 10 });
    assert.equal(back.length, 1);
    assert.equal(back[0].origin, 'manual');
    assert.equal(back[0].operator, 'ali');
    assert.equal(back[0].engine, 'MANUAL');
    assert.equal(back[0].entry, t.entry);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test('engine trades default to origin "engine" and can be filtered apart from manual ones', () => {
  const dir = mkdtempSync(join(tmpdir(), 'ayzr-'));
  try {
    const store = new Store(join(dir, 'test.sqlite'));
    const manual = manualTradeFromDrawing(drawing());
    // An engine-shaped trade written without an origin field, as older code did.
    store.upsertVirtualTrade({ ...manual, id: 'engine-1', origin: undefined, operator: undefined, engine: 'liquidity' });
    store.upsertVirtualTrade(manual);

    assert.equal(store.getVirtualTrades({ origin: 'manual', limit: 10 }).length, 1);
    assert.equal(store.getVirtualTrades({ origin: 'engine', limit: 10 }).length, 1);
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
});

test('statistics keep manual and engine results separate', () => {
  const manual = manualTradeFromDrawing(drawing());
  const engine = { ...manual, id: 'e1', origin: 'engine', engine: 'liquidity' };
  const stats = statistics([{ ...manual, state: 'CLOSED', result: 'WIN', pnlUsd: 10, rMultiple: 2 },
    { ...engine, state: 'CLOSED', result: 'LOSS', pnlUsd: -5, rMultiple: -1 }]);
  assert.equal(stats.byOrigin.manual.count, 1);
  assert.equal(stats.byOrigin.manual.wins, 1);
  assert.equal(stats.byOrigin.engine.count, 1);
  assert.equal(stats.byOrigin.engine.losses, 1);
});
