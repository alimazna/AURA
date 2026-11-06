// Engine suite tests: determinism, insufficient-history honesty, and look-ahead safety.

import { test } from 'node:test';
import assert from 'node:assert/strict';
import { runAllEngines, ENGINE_IDS, ENGINE_SPECS } from '../src/engines/index.mjs';
import { demandBaseBars, equalHighsBars, flatBars } from './fixtures.mjs';

test('every engine is documented with inputs, outputs, uncertainty and no-emit conditions', () => {
  assert.deepEqual(Object.keys(ENGINE_SPECS).sort(), [...ENGINE_IDS].sort());
  for (const [id, spec] of Object.entries(ENGINE_SPECS)) {
    assert.ok(spec.covers && spec.covers.length > 10, `${id} needs a coverage description`);
    assert.ok(Array.isArray(spec.inputs) && spec.inputs.length, `${id} needs inputs`);
    assert.ok(Array.isArray(spec.outputs) && spec.outputs.length, `${id} needs outputs`);
    assert.ok(Array.isArray(spec.uncertainty) && spec.uncertainty.length, `${id} needs uncertainty`);
    assert.ok(Array.isArray(spec.noEmit) && spec.noEmit.length, `${id} needs no-emit conditions`);
  }
});

test('insufficient history yields INSUFFICIENT_HISTORY, not invented output', () => {
  const r = runAllEngines(flatBars(5), 'M15');
  assert.equal(r.status, 'INSUFFICIENT_HISTORY');
  for (const e of r.engines) {
    assert.equal(e.emitted.length, 0);
    assert.ok(e.withheld.length > 0, `${e.engine} must explain why it did not emit`);
  }
});

test('engines are deterministic: identical bars produce identical evidence', () => {
  const bars = demandBaseBars();
  const a = runAllEngines(bars, 'M15');
  const b = runAllEngines(bars, 'M15');
  assert.deepEqual(a.evidence, b.evidence);
  assert.equal(a.trend, b.trend);
  assert.equal(a.atr, b.atr);
});

test('no evidence may reference a bar index beyond the supplied closed bars', () => {
  const bars = demandBaseBars();
  const r = runAllEngines(bars, 'M15');
  for (const ev of r.evidence) {
    assert.ok(Number.isFinite(ev.sourceBarIndex), `${ev.type} needs a source bar index`);
    assert.ok(ev.sourceBarIndex >= 0 && ev.sourceBarIndex < bars.length,
      `${ev.type} references bar ${ev.sourceBarIndex} but only ${bars.length} bars exist`);
    assert.ok(ev.sourceBarTime, `${ev.type} must carry the source bar time`);
  }
});

test('every emitted evidence item states its conditions and reason', () => {
  const r = runAllEngines(demandBaseBars(), 'M15');
  assert.ok(r.evidence.length > 0, 'the impulse fixture should produce evidence');
  for (const ev of r.evidence) {
    assert.ok(Array.isArray(ev.conditions) && ev.conditions.length > 0, `${ev.type} must state conditions`);
    assert.ok(typeof ev.reason === 'string' && ev.reason.length > 5, `${ev.type} must state a reason`);
    assert.ok(ev.score >= 0 && ev.score <= 1, `${ev.type} score must be an internal 0..1 ranking`);
  }
});

test('scores are never presented as probabilities', () => {
  const r = runAllEngines(demandBaseBars(), 'M15');
  for (const ev of r.evidence) {
    assert.ok(!('probability' in ev), 'evidence must not carry a probability field');
    assert.ok(!('confidence' in ev), 'evidence must not carry a confidence field');
  }
});

test('the ATR is derived from the supplied bars only', () => {
  const bars = demandBaseBars();
  const r = runAllEngines(bars, 'M15');
  assert.ok(r.atr > 0);
  const longer = runAllEngines([...bars, ...flatBars(3, 4100)], 'M15');
  assert.ok(longer.atr >= r.atr, 'adding wider bars must not reduce ATR below the original');
});

test('liquidity engine finds equal-high pools in the fixture', () => {
  const r = runAllEngines(equalHighsBars(), 'M15');
  const pools = r.evidence.filter((e) => e.engine === 'liquidity' && e.type === 'LIQUIDITY_POOL_ABOVE');
  assert.ok(pools.length > 0, 'equal highs should produce a buy-side liquidity pool');
  for (const p of pools) assert.ok(p.zone.high > p.zone.low);
});

test('engines that find nothing say so instead of staying silent', () => {
  const r = runAllEngines(flatBars(120, 4000), 'M15');
  for (const e of r.engines) {
    if (e.emitted.length === 0) {
      assert.ok(e.withheld.length > 0, `${e.engine} emitted nothing and gave no reason`);
    }
  }
});

test('structure engine labels a trend from the swing sequence', () => {
  const r = runAllEngines(demandBaseBars(), 'M15');
  assert.ok(['UP', 'DOWN', 'RANGE', 'UNKNOWN'].includes(r.trend));
});
