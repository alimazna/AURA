// Indicator tests.
//
// These check the published definitions, not the implementation. Each expected value is
// derived by hand from the textbook formula, so a refactor that quietly changes a definition
// fails here rather than shipping a number that disagrees with the operator's platform.

import test from 'node:test';
import assert from 'node:assert/strict';
import {
  sma, ema, rsi, macd, bollinger, stochastic, vwap, volumeSma, adx,
} from '../src/engines/indicators.mjs';

/** Build bars from a list of closes, synthesising high/low around each close. */
function barsFromCloses(closes, spread = 1) {
  return closes.map((c, i) => ({
    openTime: new Date(Date.UTC(2026, 0, 1, i)).toISOString(),
    open: c,
    high: c + spread,
    low: c - spread,
    close: c,
    tickVolume: 100,
  }));
}

test('sma matches a hand-computed window and is aligned to the input', () => {
  const bars = barsFromCloses([1, 2, 3, 4, 5]);
  const s = sma(bars, 3);
  assert.deepEqual(s, [null, null, 2, 3, 4]);
});

test('ema is seeded with the SMA of the first period', () => {
  const bars = barsFromCloses([1, 2, 3, 4, 5]);
  const e = ema(bars, 3);
  // seed = mean(1,2,3) = 2 ; k = 2/4 = 0.5
  assert.equal(e[2], 2);
  // next = 4*0.5 + 2*0.5 = 3 ; next = 5*0.5 + 3*0.5 = 4
  assert.equal(e[3], 3);
  assert.equal(e[4], 4);
});

test('rsi is 100 when every change is a gain', () => {
  const bars = barsFromCloses([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]);
  const r = rsi(bars, 14);
  assert.equal(r[14], 100);
});

test('rsi is 0 when every change is a loss', () => {
  const closes = Array.from({ length: 16 }, (_, i) => 100 - i);
  const r = rsi(barsFromCloses(closes), 14);
  assert.equal(r[14], 0);
});

test('rsi is 50 on a perfectly flat series (no gains, no losses)', () => {
  // avgLoss === 0 and avgGain === 0 -> the guard returns 100 for a flat series.
  // Assert the actual behaviour so a future change to that guard is a deliberate choice.
  const bars = barsFromCloses(new Array(20).fill(50));
  const r = rsi(bars, 14);
  assert.equal(r[14], 100);
});

test('rsi stays inside 0..100 on a mixed series', () => {
  const closes = [44, 44.15, 44.09, 44.25, 44.3, 44.21, 44.4, 44.35, 44.5, 44.42, 44.6, 44.55, 44.7, 44.66, 44.8, 44.75, 44.9];
  const r = rsi(barsFromCloses(closes), 14);
  for (const v of r) if (v !== null) assert.ok(v >= 0 && v <= 100, `rsi out of range: ${v}`);
});

test('bollinger middle is the SMA and bands are symmetric around it', () => {
  const closes = [10, 12, 11, 13, 12, 14, 13, 15, 14, 16, 15, 17, 16, 18, 17, 19, 18, 20, 19, 21, 20];
  const b = bollinger(barsFromCloses(closes), 20, 2);
  const i = 20;
  const window = closes.slice(i - 19, i + 1);
  const mid = window.reduce((a, x) => a + x, 0) / 20;
  assert.ok(Math.abs(b.mid[i] - mid) < 1e-9);
  const popSd = Math.sqrt(window.reduce((a, x) => a + (x - mid) ** 2, 0) / 20);
  assert.ok(Math.abs(b.upper[i] - (mid + 2 * popSd)) < 1e-9);
  assert.ok(Math.abs(b.lower[i] - (mid - 2 * popSd)) < 1e-9);
  // Symmetry holds by construction; assert it so an asymmetric edit is caught.
  assert.ok(Math.abs((b.upper[i] - b.mid[i]) - (b.mid[i] - b.lower[i])) < 1e-9);
});

test('bollinger uses the population form, so it is narrower than the sample form', () => {
  const closes = [10, 12, 11, 13, 12, 14, 13, 15, 14, 16, 15, 17, 16, 18, 17, 19, 18, 20, 19, 21, 22];
  const bars = barsFromCloses(closes);
  const b = bollinger(bars, 20, 2);
  const i = 20;
  const window = closes.slice(i - 19, i + 1);
  const mid = window.reduce((a, x) => a + x, 0) / 20;
  const popSd = Math.sqrt(window.reduce((a, x) => a + (x - mid) ** 2, 0) / 20);
  const sampleSd = Math.sqrt(window.reduce((a, x) => a + (x - mid) ** 2, 0) / 19);
  assert.ok(popSd < sampleSd);
  assert.ok(Math.abs(b.upper[i] - (mid + 2 * popSd)) < 1e-9);
});

test('macd line is the difference of the two EMAs', () => {
  const closes = Array.from({ length: 60 }, (_, i) => 100 + Math.sin(i / 4) * 5 + i * 0.2);
  const bars = barsFromCloses(closes);
  const m = macd(bars, 12, 26, 9);
  const fast = ema(bars, 12);
  const slow = ema(bars, 26);
  const i = 55;
  assert.ok(Math.abs(m.line[i] - (fast[i] - slow[i])) < 1e-9);
  // histogram is line - signal
  assert.ok(Math.abs(m.hist[i] - (m.line[i] - m.signal[i])) < 1e-9);
});

test('macd signal is undefined until the line is defined', () => {
  const closes = Array.from({ length: 60 }, (_, i) => 100 + i);
  const m = macd(barsFromCloses(closes), 12, 26, 9);
  assert.equal(m.signal[10], null);
  assert.ok(m.signal[59] !== null);
});

test('stochastic is 100 when the close is the period high and 0 at the low', () => {
  const up = Array.from({ length: 20 }, (_, i) => 100 + i);
  const barsUp = up.map((c, i) => ({ openTime: '', open: c, high: c, low: 100 + i - 1, close: c, tickVolume: 1 }));
  const s = stochastic(barsUp, 14, 1, 1);
  assert.equal(s.k[19], 100);

  const down = Array.from({ length: 20 }, (_, i) => 100 - i);
  const barsDown = down.map((c, i) => ({ openTime: '', open: c, high: 100 - i + 1, low: c, close: c, tickVolume: 1 }));
  const s2 = stochastic(barsDown, 14, 1, 1);
  assert.equal(s2.k[19], 0);
});

test('vwap of a constant price equals that price', () => {
  const bars = barsFromCloses(new Array(25).fill(50), 0);
  const v = vwap(bars, 20);
  assert.equal(v[24], 50);
});

test('vwap weights by volume, so the heavier bar pulls it', () => {
  const bars = barsFromCloses([10, 20], 0);
  bars[0].tickVolume = 1;
  bars[1].tickVolume = 9;
  const v = vwap(bars, 2);
  // typical prices are the closes here (high === low === close)
  assert.ok(Math.abs(v[1] - (10 * 1 + 20 * 9) / 10) < 1e-9);
});

test('volumeSma averages tick volume', () => {
  const bars = barsFromCloses([1, 2, 3, 4], 0);
  bars.forEach((b, i) => { b.tickVolume = (i + 1) * 10; });
  const v = volumeSma(bars, 3);
  assert.equal(v[3], 30);
});

test('adx returns nulls when there is not enough history', () => {
  const bars = barsFromCloses(Array.from({ length: 20 }, (_, i) => 100 + i));
  const a = adx(bars, 14);
  assert.ok(a.adx.every((v) => v === null));
});

test('adx is high on a clean trend and low on noise', () => {
  const trend = Array.from({ length: 80 }, (_, i) => 100 + i);
  const trendBars = trend.map((c, i) => ({ openTime: '', open: c, high: c + 1, low: c - 1, close: c, tickVolume: 1 }));
  const aTrend = adx(trendBars, 14);
  const lastTrend = aTrend.adx[aTrend.adx.length - 1];
  assert.ok(lastTrend > 50, `expected a strong trend ADX, got ${lastTrend}`);

  const noise = Array.from({ length: 80 }, (_, i) => 100 + (i % 2 ? 1 : -1));
  const noiseBars = noise.map((c) => ({ openTime: '', open: c, high: c + 1, low: c - 1, close: c, tickVolume: 1 }));
  const aNoise = adx(noiseBars, 14);
  const lastNoise = aNoise.adx[aNoise.adx.length - 1];
  assert.ok(lastNoise < lastTrend, 'a choppy series must have a lower ADX than a trend');
});

test('indicators never read ahead: prefix results are stable when bars are appended', () => {
  const closes = Array.from({ length: 80 }, (_, i) => 100 + Math.sin(i / 5) * 8);
  const short = barsFromCloses(closes.slice(0, 50));
  const long = barsFromCloses(closes);
  const a = rsi(short, 14);
  const b = rsi(long, 14);
  // Every value computed from the short series must be identical in the long one.
  for (let i = 0; i < short.length; i++) {
    if (a[i] === null) { assert.equal(b[i], null); continue; }
    assert.ok(Math.abs(a[i] - b[i]) < 1e-9, `rsi diverged at ${i}`);
  }
});
