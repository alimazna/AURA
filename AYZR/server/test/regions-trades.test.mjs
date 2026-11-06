// Region lifecycle, signal and virtual-trade tests.
//
// The look-ahead test here is the regression guard for a real bug found during
// development: trades were being evaluated against bars that had closed before the
// trade existed, which produced four spurious immediate stop-outs.

import { test } from 'node:test';
import assert from 'node:assert/strict';
import { advanceRegion, expireStale, REGION_STATES, TERMINAL_STATES } from '../src/engines/regions.mjs';
import {
  signalFromRegion, virtualTradeFromRegion, evaluateVirtualTrade,
  closeVirtualTrade, statistics, COST_MODEL, eligibleBarsFor,
} from '../src/engines/trades.mjs';

const ATR = 10;
const NOW = Date.parse('2026-01-05T12:00:00Z');

function region(over = {}) {
  return {
    id: 'r1', engine: 'supplyDemand', type: 'DEMAND_ZONE', timeframe: 'M15',
    direction: 'LONG', zone: { low: 3990, high: 4000 }, state: 'DISCOVERED',
    score: 0.6, conditions: ['base then impulse'], reason: 'demand base',
    sourceTime: '2026-01-05T11:45:00Z', touches: 0, ...over,
  };
}

function bar(o, h, l, c, time = '2026-01-05T12:00:00Z') {
  return { openTime: time, time: Date.parse(time), open: o, high: h, low: l, close: c, tickVolume: 1, isOpen: false };
}

test('region lifecycle states are exactly the documented set', () => {
  assert.deepEqual(REGION_STATES, [
    'DISCOVERED', 'WAITING_APPROACH', 'APPROACH', 'UNDER_VALIDATION',
    'CONFIRMED_RETEST', 'CONFIRMED_BREAKOUT', 'FAKE_BREAKOUT', 'EXPIRED',
  ]);
});

test('a region far from price waits rather than jumping to a confirmation', () => {
  const r = region();
  const first = advanceRegion(r, bar(3900, 3901, 3899, 3900), ATR);
  assert.equal(first.state, 'WAITING_APPROACH');
  const second = advanceRegion({ ...r, state: 'WAITING_APPROACH' }, bar(3900, 3901, 3899, 3900), ATR);
  assert.equal(second.state, 'WAITING_APPROACH');
  assert.equal(second.transitioned, false);
});

test('price entering the zone advances to APPROACH then UNDER_VALIDATION', () => {
  const r = region();
  const approach = advanceRegion(r, bar(4000, 4004, 3998, 4001), ATR);
  assert.equal(approach.state, 'APPROACH');
  const inside = advanceRegion({ ...r, state: 'APPROACH' }, bar(3998, 4002, 3994, 3996), ATR);
  assert.equal(inside.state, 'UNDER_VALIDATION');
});

test('a confirming reaction close is required before CONFIRMED_RETEST', () => {
  const r = { ...region(), state: 'UNDER_VALIDATION' };
  const undecided = advanceRegion(r, bar(3996, 3999, 3992, 3995), ATR);
  assert.equal(undecided.state, 'UNDER_VALIDATION', 'an indecisive close must not confirm');
  const decisive = advanceRegion(r, bar(3996, 4006, 3995, 4005), ATR);
  assert.equal(decisive.state, 'CONFIRMED_RETEST');
});

test('a fake breakout region resolves to FAKE_BREAKOUT, not a retest', () => {
  const r = { ...region({ type: 'FAKE_BREAKOUT', direction: 'SHORT' }), state: 'UNDER_VALIDATION' };
  const out = advanceRegion(r, bar(4005, 4006, 3990, 3992), ATR);
  assert.equal(out.state, 'FAKE_BREAKOUT');
});

test('a consumed zone expires instead of confirming forever', () => {
  const r = { ...region(), state: 'UNDER_VALIDATION', touches: 9 };
  const out = advanceRegion(r, bar(3996, 3999, 3993, 3995), ATR);
  assert.equal(out.state, 'EXPIRED');
});

test('terminal states are never re-entered', () => {
  for (const s of TERMINAL_STATES) {
    const out = advanceRegion({ ...region(), state: s }, bar(4000, 4005, 3995, 4004), ATR);
    assert.equal(out.transitioned, false);
    assert.equal(out.state, s);
  }
});

test('stale regions expire by age', () => {
  const r = { ...region(), discoveredAtMs: NOW - 100 * 3600 * 1000 };
  const out = expireStale(r, NOW, 72 * 3600 * 1000);
  assert.equal(out.state, 'EXPIRED');
  const fresh = expireStale({ ...region(), discoveredAtMs: NOW - 1000 }, NOW, 72 * 3600 * 1000);
  assert.equal(fresh, null);
});

test('a signal is generated with an internal ranking, never a probability', () => {
  const s = signalFromRegion(region(), { atr: ATR, spread: 0.2, timeframe: 'M15', nowMs: NOW });
  assert.equal(s.scoreLabel, 'internal ranking (uncalibrated)');
  assert.ok(!('probability' in s));
  assert.ok(s.score >= 0 && s.score <= 1);
  assert.ok(s.stop < s.entry && s.target > s.entry, 'a long signal must bracket entry');
  assert.equal(s.operatorDecision, null);
});

test('a virtual trade models its costs explicitly', () => {
  const t = virtualTradeFromRegion(region(), { atr: ATR, spread: 0.2, timeframe: 'M15', nowMs: NOW, triggerBarTime: '2026-01-05T12:00:00Z' });
  assert.equal(t.costs.modelled, true);
  assert.ok(t.costs.spreadAtEntry > 0);
  assert.ok(t.costs.slippagePerSide > 0);
  assert.equal(t.triggerBarTime, '2026-01-05T12:00:00Z');
  assert.ok(t.entry > t.entryRaw, 'a long entry must pay the spread and slippage');
});

test('stop is evaluated before target when a bar spans both (conservative)', () => {
  const t = virtualTradeFromRegion(region(), { atr: ATR, spread: 0.2, timeframe: 'M15', nowMs: NOW });
  const spanning = bar(t.entry, t.target + 10, t.stop - 10, t.entry);
  const r = evaluateVirtualTrade(t, spanning);
  assert.equal(r.closed, true);
  assert.equal(r.result, 'LOSS', 'ambiguity must resolve against the strategy');
});

test('a winning trade reports a positive R multiple and profit', () => {
  const t = virtualTradeFromRegion(region(), { atr: ATR, spread: 0.2, timeframe: 'M15', nowMs: NOW });
  const hit = evaluateVirtualTrade(t, bar(t.entry, t.target + 1, t.entry - 1, t.target));
  assert.equal(hit.result, 'WIN');
  const closed = closeVirtualTrade(t, { exit: hit.exit, result: hit.result, exitReason: hit.exitReason, atMs: NOW });
  assert.ok(closed.rMultiple > 0);
  assert.ok(closed.pnlUsd > 0);
});

test('a losing trade reports a negative R multiple', () => {
  const t = virtualTradeFromRegion(region(), { atr: ATR, spread: 0.2, timeframe: 'M15', nowMs: NOW });
  const hit = evaluateVirtualTrade(t, bar(t.entry, t.entry + 1, t.stop - 1, t.stop));
  const closed = closeVirtualTrade(t, { exit: hit.exit, result: hit.result, exitReason: hit.exitReason, atMs: NOW });
  assert.ok(closed.rMultiple < 0);
  assert.ok(closed.pnlUsd < 0);
});

test('statistics separate performance by engine and timeframe', () => {
  const mk = (engine, timeframe, result, pnl, r) => ({
    engine, timeframe, session: 'unknown', state: 'CLOSED', result, pnlUsd: pnl, rMultiple: r,
  });
  const stats = statistics([
    mk('liquidity', 'M15', 'WIN', 20, 2),
    mk('liquidity', 'M15', 'LOSS', -10, -1),
    mk('structure', 'H1', 'WIN', 30, 2),
  ]);
  assert.equal(stats.counts.closed, 3);
  assert.equal(stats.counts.wins, 2);
  assert.equal(stats.byEngine.liquidity.count, 2);
  assert.equal(stats.byEngine.liquidity.wins, 1);
  assert.equal(stats.byTimeframe.H1.count, 1);
  assert.equal(stats.profitFactor, 5);
  assert.equal(stats.costsModelled, true);
});

test('statistics report null rather than zero when there is no evidence', () => {
  const stats = statistics([]);
  assert.equal(stats.winRate, null);
  assert.equal(stats.expectancyR, null);
  assert.equal(stats.profitFactor, null);
  assert.equal(stats.counts.closed, 0);
});

test('regression: a trade is not evaluated against bars that closed before it existed', () => {
  const t = virtualTradeFromRegion(region(), {
    atr: ATR, spread: 0.2, timeframe: 'M15', nowMs: NOW,
    triggerBarTime: '2026-01-05T12:00:00Z',
  });
  const before = bar(t.entry, t.entry + 1, t.stop - 5, t.stop, '2026-01-05T11:45:00Z');
  const after = bar(t.entry, t.entry + 1, t.stop - 5, t.stop, '2026-01-05T12:15:00Z');

  const eligible = eligibleBarsFor(t, [before, after]);
  assert.equal(eligible.length, 1, 'only the bar after the trigger may be used');
  assert.equal(eligible[0].openTime, after.openTime);
  assert.equal(eligibleBarsFor(t, [before]).length, 0, 'a pre-trigger bar must never be eligible');

  // Applying the guard end-to-end: the stale bar would stop the trade out, the fresh one is used.
  const evaluated = eligible.map((b) => evaluateVirtualTrade(t, b));
  assert.equal(evaluated.length, 1);
  assert.equal(evaluated[0].closed, true);
  assert.equal(evaluated[0].exit, t.stop);
});

test('a trade with no usable trigger time is never evaluated at all', () => {
  const t = { ...virtualTradeFromRegion(region(), { atr: ATR, spread: 0.2, timeframe: 'M15', nowMs: NOW }), triggerBarTime: null, openedAt: 'garbage' };
  assert.deepEqual(eligibleBarsFor(t, [bar(1, 2, 0.5, 1)]), []);
});

test('the cost model is exposed so statistics can be audited', () => {
  assert.ok(COST_MODEL.riskReward >= 1);
  assert.ok(COST_MODEL.contractSize > 0);
  assert.ok(COST_MODEL.lotSize > 0);
});
