// AI gate tests, plus data-integrity tests for the BiQuote client.
//
// The AI tests assert the hard constraint from the brief: no provider configured means
// an explicit NOT AVAILABLE verdict, never a fabricated analysis.

import { test } from 'node:test';
import assert from 'node:assert/strict';
import { providerStatus, validatePlan, buildContext, requestPlan, SYSTEM_PROMPT } from '../src/ai/planner.mjs';
import { aggregateBars, ohlc } from '../src/biquote/client.mjs';
import { flatBars } from './fixtures.mjs';

const goodPlan = {
  direction: 'LONG',
  scenarios: [{ name: 'Retest', condition: 'close above 4000', expectation: 'continuation', supportingEvidence: ['2026-01-05T12:00:00Z'] }],
  invalidationConditions: ['close below 3990'],
  timeHorizon: 'intraday',
  keyLevels: [{ label: 'support', price: 3990, source: 'swing low' }],
  risks: ['thin liquidity'],
};

test('with no provider configured the status says so plainly', () => {
  const s = providerStatus({ provider: 'none', model: '', apiKey: '' });
  assert.equal(s.available, false);
  assert.match(s.reason, /No AI provider is configured/);
});

test('a provider selected without a key is reported as unavailable, not attempted', () => {
  const s = providerStatus({ provider: 'openai', model: 'gpt-x', apiKey: '' });
  assert.equal(s.available, false);
  assert.match(s.reason, /AYZR_AI_API_KEY is empty/);
});

test('requesting a plan with no provider returns NOT_AVAILABLE and no plan', async () => {
  const r = await requestPlan({ config: { provider: 'none', model: '', apiKey: '' }, context: { symbol: 'XAUUSD' } });
  assert.equal(r.verdict, 'NOT_AVAILABLE');
  assert.equal(r.plan, null);
  assert.ok(r.reason.length > 10);
});

test('a provider that returns no content is reported as an error, not silently accepted', async () => {
  const fakeFetch = async () => ({ ok: true, json: async () => ({ choices: [] }) });
  const r = await requestPlan({
    config: { provider: 'openai', model: 'm', apiKey: 'k', baseUrl: 'https://example.invalid/v1' },
    context: {},
    fetchImpl: fakeFetch,
  });
  assert.equal(r.verdict, 'PROVIDER_ERROR');
  assert.equal(r.plan, null);
});

test('a provider that returns prose instead of JSON is rejected', async () => {
  const fakeFetch = async () => ({ ok: true, json: async () => ({ choices: [{ message: { content: 'Gold looks bullish today.' } }] }) });
  const r = await requestPlan({
    config: { provider: 'openai', model: 'm', apiKey: 'k', baseUrl: 'https://example.invalid/v1' },
    context: {}, fetchImpl: fakeFetch,
  });
  assert.equal(r.verdict, 'REJECTED');
});

test('a well-formed plan is accepted', async () => {
  const fakeFetch = async () => ({ ok: true, json: async () => ({ choices: [{ message: { content: JSON.stringify(goodPlan) } }] }) });
  const r = await requestPlan({
    config: { provider: 'openai', model: 'm', apiKey: 'k', baseUrl: 'https://example.invalid/v1' },
    context: {}, fetchImpl: fakeFetch,
  });
  assert.equal(r.verdict, 'ACCEPTED');
  assert.equal(r.plan.direction, 'LONG');
});

test('plan validation rejects missing required fields', () => {
  const check = validatePlan({ direction: 'LONG' });
  assert.equal(check.ok, false);
  assert.ok(check.errors.some((e) => e.includes('scenarios')));
});

test('plan validation rejects an invalid direction', () => {
  const check = validatePlan({ ...goodPlan, direction: 'MAYBE' });
  assert.equal(check.ok, false);
  assert.ok(check.errors.some((e) => e.includes('direction must be one of')));
});

test('plan validation rejects certainty claims', () => {
  const check = validatePlan({ ...goodPlan, risks: ['this trade is guaranteed'] });
  assert.equal(check.ok, false);
  assert.ok(check.errors.some((e) => e.includes('certainty claim')));
});

test('plan validation rejects a scenario missing its expectation', () => {
  const check = validatePlan({ ...goodPlan, scenarios: [{ name: 'a', condition: 'b' }] });
  assert.equal(check.ok, false);
  assert.ok(check.errors.some((e) => e.includes('missing expectation')));
});

test('the system prompt forbids invention and probabilities', () => {
  assert.match(SYSTEM_PROMPT, /Never invent prices/);
  assert.match(SYSTEM_PROMPT, /Never claim certainty or probabilities/);
});

test('the AI context contains only closed bars', () => {
  const bars = [...flatBars(30), { ...flatBars(1)[0], isOpen: true }];
  const ctx = buildContext({ symbol: 'XAUUSD', timeframe: 'M15', quote: null, bars, analysis: { engines: [] }, regions: [] });
  assert.equal(ctx.closedBars.length, 30);
  for (const b of ctx.closedBars) assert.ok(b.t && Number.isFinite(b.c));
});

test('the AI context carries the shadow-only constraint to the model', () => {
  const ctx = buildContext({ symbol: 'XAUUSD', timeframe: 'M15', quote: null, bars: [], analysis: { engines: [] }, regions: [] });
  assert.ok(ctx.constraints.some((c) => /No real trading/.test(c)));
  assert.ok(ctx.constraints.some((c) => /Do not state probabilities/.test(c)));
});

/* ---------------------------------------------------- BiQuote data integrity */

test('OHLC bars are normalised oldest-first regardless of response order', async (t) => {
  const newestFirst = [
    { openTime: '2026-01-05T12:00:00Z', open: 2, high: 3, low: 1, close: 2.5, tickVolume: 1, isOpen: false },
    { openTime: '2026-01-05T11:45:00Z', open: 1, high: 2, low: 0.5, close: 1.5, tickVolume: 1, isOpen: false },
  ];
  const original = globalThis.fetch;
  globalThis.fetch = async () => ({ ok: true, text: async () => JSON.stringify({ symbol: 'XAUUSD', interval: '15m', bars: newestFirst }) });
  t.after(() => { globalThis.fetch = original; });

  const r = await ohlc('XAUUSD', '15m', 2);
  assert.equal(r.bars.length, 2);
  assert.ok(Date.parse(r.bars[0].openTime) < Date.parse(r.bars[1].openTime), 'bars must be oldest-first');
});

test('malformed OHLC bars are dropped rather than plotted', async (t) => {
  const bad = [
    { openTime: '2026-01-05T12:00:00Z', open: 2, high: 1, low: 3, close: 2, tickVolume: 1, isOpen: false },
    { openTime: 'not-a-date', open: 1, high: 2, low: 1, close: 1, tickVolume: 1, isOpen: false },
    { openTime: '2026-01-05T11:00:00Z', open: 1, high: 2, low: 1, close: 1, tickVolume: 1, isOpen: false },
  ];
  const original = globalThis.fetch;
  globalThis.fetch = async () => ({ ok: true, text: async () => JSON.stringify({ symbol: 'XAUUSD', interval: '15m', bars: bad }) });
  t.after(() => { globalThis.fetch = original; });

  const r = await ohlc('XAUUSD', '15m', 3);
  assert.equal(r.bars.length, 1, 'only the valid bar survives');
  assert.equal(r.bars[0].openTime, '2026-01-05T11:00:00Z');
});

test('weekly aggregation unions the constituent daily bars', () => {
  const days = [
    { openTime: '2026-01-05T00:00:00Z', time: Date.parse('2026-01-05T00:00:00Z'), open: 100, high: 110, low: 95, close: 105, tickVolume: 10, volume: 0 },
    { openTime: '2026-01-06T00:00:00Z', time: Date.parse('2026-01-06T00:00:00Z'), open: 105, high: 120, low: 104, close: 118, tickVolume: 12, volume: 0 },
    { openTime: '2026-01-12T00:00:00Z', time: Date.parse('2026-01-12T00:00:00Z'), open: 118, high: 119, low: 90, close: 92, tickVolume: 9, volume: 0 },
  ];
  const weeks = aggregateBars(days, 10080);
  assert.equal(weeks.length, 2, 'two ISO weeks');
  assert.equal(weeks[0].open, 100);
  assert.equal(weeks[0].high, 120);
  assert.equal(weeks[0].low, 95);
  assert.equal(weeks[0].close, 118);
  assert.equal(weeks[0].tickVolume, 22);
  assert.equal(weeks[0].derived, true, 'aggregated bars must be flagged as derived');
});

test('monthly aggregation groups by calendar month', () => {
  const days = [
    { openTime: '2026-01-05T00:00:00Z', time: Date.parse('2026-01-05T00:00:00Z'), open: 100, high: 110, low: 95, close: 105, tickVolume: 1, volume: 0 },
    { openTime: '2026-01-20T00:00:00Z', time: Date.parse('2026-01-20T00:00:00Z'), open: 105, high: 130, low: 100, close: 128, tickVolume: 1, volume: 0 },
    { openTime: '2026-02-02T00:00:00Z', time: Date.parse('2026-02-02T00:00:00Z'), open: 128, high: 129, low: 120, close: 121, tickVolume: 1, volume: 0 },
  ];
  const months = aggregateBars(days, 43200);
  assert.equal(months.length, 2);
  assert.equal(months[0].high, 130);
  assert.equal(months[0].low, 95);
  assert.equal(months[1].open, 128);
});
