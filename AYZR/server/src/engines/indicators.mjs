// Standard technical indicators for the AYZR chart.
//
// Every function here follows its published definition rather than an approximation, because
// a chart that disagrees with the operator's platform is worse than no chart. Where a
// definition has a well-known "fast" variant (an SMA where the original was a Wilder average),
// the original is implemented and the choice is named in the comment, so the number on screen
// can always be traced to a source.
//
// All functions take the same bar shape the engines use: { openTime, open, high, low, close }
// and return a plain array aligned to `bars`, with `null` wherever there is not yet enough
// history. No function reads ahead of index i.

import { mean, stdev } from '../engines/primitives.mjs';

const nulls = (n) => new Array(n).fill(null);

/** Simple moving average of closes. */
export function sma(bars, period = 20) {
  const out = nulls(bars.length);
  if (period <= 0 || bars.length < period) return out;
  let sum = 0;
  for (let i = 0; i < bars.length; i++) {
    sum += bars[i].close;
    if (i >= period) sum -= bars[i - period].close;
    if (i >= period - 1) out[i] = sum / period;
  }
  return out;
}

/**
 * Exponential moving average, seeded with the SMA of the first `period` closes.
 * Seed choice matters: seeding with the first close instead makes the first few dozen
 * values drift from every charting platform, which is exactly the disagreement to avoid.
 */
export function ema(bars, period = 20) {
  const out = nulls(bars.length);
  if (period <= 0 || bars.length < period) return out;
  const k = 2 / (period + 1);
  let sum = 0;
  for (let i = 0; i < period; i++) sum += bars[i].close;
  let prev = sum / period;
  out[period - 1] = prev;
  for (let i = period; i < bars.length; i++) {
    prev = bars[i].close * k + prev * (1 - k);
    out[i] = prev;
  }
  return out;
}

/**
 * Wilder's RSI (J. Welles Wilder Jr., "New Concepts in Technical Trading Systems", 1978).
 *
 * Uses Wilder's smoothing, not a simple average of gains and losses, and is seeded with the
 * mean of the first `period` changes. This is the definition every platform labels "RSI".
 * Returns 0..100, or null until `period` changes exist.
 */
export function rsi(bars, period = 14) {
  const out = nulls(bars.length);
  if (bars.length < period + 1) return out;
  let gain = 0;
  let loss = 0;
  for (let i = 1; i <= period; i++) {
    const d = bars[i].close - bars[i - 1].close;
    if (d >= 0) gain += d; else loss -= d;
  }
  let avgGain = gain / period;
  let avgLoss = loss / period;
  out[period] = avgLoss === 0 ? 100 : 100 - 100 / (1 + avgGain / avgLoss);
  for (let i = period + 1; i < bars.length; i++) {
    const d = bars[i].close - bars[i - 1].close;
    const g = d > 0 ? d : 0;
    const l = d < 0 ? -d : 0;
    avgGain = (avgGain * (period - 1) + g) / period;
    avgLoss = (avgLoss * (period - 1) + l) / period;
    out[i] = avgLoss === 0 ? 100 : 100 - 100 / (1 + avgGain / avgLoss);
  }
  return out;
}

/**
 * MACD (Gerald Appel). Fast EMA 12, slow EMA 26, signal = EMA 9 of the MACD line.
 * Returned as three series so the chart can plot the line and histogram without recomputing.
 */
export function macd(bars, fast = 12, slow = 26, signalPeriod = 9) {
  const fastEma = ema(bars, fast);
  const slowEma = ema(bars, slow);
  const line = nulls(bars.length);
  for (let i = 0; i < bars.length; i++) {
    if (fastEma[i] !== null && slowEma[i] !== null) line[i] = fastEma[i] - slowEma[i];
  }
  // Signal is an EMA of the MACD line, so it is computed over the defined tail only.
  const firstIdx = line.findIndex((v) => v !== null);
  const signal = nulls(bars.length);
  const hist = nulls(bars.length);
  if (firstIdx !== -1) {
    const tail = line.slice(firstIdx).map((v) => ({ close: v }));
    const sig = ema(tail, signalPeriod);
    for (let i = 0; i < sig.length; i++) {
      if (sig[i] === null) continue;
      const idx = firstIdx + i;
      signal[idx] = sig[i];
      hist[idx] = line[idx] - sig[i];
    }
  }
  return { line, signal, hist };
}

/**
 * Bollinger Bands (John Bollinger). Middle = SMA(period). Bands = middle +/- k * population
 * standard deviation of the same window. The population (÷n) form is used because that is
 * Bollinger's own definition; the sample form (÷n-1) widens the bands by ~2.5% at n=20.
 */
export function bollinger(bars, period = 20, k = 2) {
  const mid = sma(bars, period);
  const upper = nulls(bars.length);
  const lower = nulls(bars.length);
  for (let i = period - 1; i < bars.length; i++) {
    if (mid[i] === null) continue;
    const closes = bars.slice(i - period + 1, i + 1).map((b) => b.close);
    const sd = populationStdev(closes);
    upper[i] = mid[i] + k * sd;
    lower[i] = mid[i] - k * sd;
  }
  return { mid, upper, lower };
}

/** Population standard deviation (÷n). Bollinger's definition, not the sample form. */
function populationStdev(values) {
  if (values.length < 1) return 0;
  const m = mean(values);
  return Math.sqrt(values.reduce((a, b) => a + (b - m) ** 2, 0) / values.length);
}

/** Exported for tests: sample stdev, kept distinct from the population form used above. */
export { stdev as sampleStdev };

/**
 * Stochastic oscillator (George Lane). %K is the close's position within the period's range,
 * smoothed by `kSmooth`; %D is the SMA of %K over `dSmooth`.
 */
export function stochastic(bars, period = 14, kSmooth = 3, dSmooth = 3) {
  const raw = nulls(bars.length);
  for (let i = period - 1; i < bars.length; i++) {
    let hi = -Infinity;
    let lo = Infinity;
    for (let j = i - period + 1; j <= i; j++) {
      if (bars[j].high > hi) hi = bars[j].high;
      if (bars[j].low < lo) lo = bars[j].low;
    }
    raw[i] = hi === lo ? 50 : ((bars[i].close - lo) / (hi - lo)) * 100;
  }
  const k = smaOfSeries(raw, kSmooth);
  const d = smaOfSeries(k, dSmooth);
  return { k, d };
}

/** SMA over a series that may contain leading nulls, preserving alignment. */
function smaOfSeries(series, period) {
  const out = nulls(series.length);
  for (let i = period - 1; i < series.length; i++) {
    let sum = 0;
    let ok = true;
    for (let j = i - period + 1; j <= i; j++) {
      if (series[j] === null) { ok = false; break; }
      sum += series[j];
    }
    if (ok) out[i] = sum / period;
  }
  return out;
}

/**
 * Volume-weighted average price over a rolling window. Uses tick volume, which is what the
 * venue reports for gold; it is a proxy for traded volume, and is labelled as such in the UI
 * rather than presented as true volume.
 */
export function vwap(bars, period = 20) {
  const out = nulls(bars.length);
  for (let i = period - 1; i < bars.length; i++) {
    let pv = 0;
    let v = 0;
    for (let j = i - period + 1; j <= i; j++) {
      const tp = (bars[j].high + bars[j].low + bars[j].close) / 3;
      const vol = bars[j].tickVolume || 0;
      pv += tp * vol;
      v += vol;
    }
    if (v > 0) out[i] = pv / v;
  }
  return out;
}

/** Simple moving average of tick volume, for the volume pane. */
export function volumeSma(bars, period = 20) {
  const out = nulls(bars.length);
  for (let i = period - 1; i < bars.length; i++) {
    let sum = 0;
    for (let j = i - period + 1; j <= i; j++) sum += bars[j].tickVolume || 0;
    out[i] = sum / period;
  }
  return out;
}

/**
 * Average Directional Index (Wilder). Returns +DI, -DI and ADX, all Wilder-smoothed.
 * ADX measures trend strength without direction; the DI pair supplies the direction.
 */
export function adx(bars, period = 14) {
  const n = bars.length;
  const plusDI = nulls(n);
  const minusDI = nulls(n);
  const adxOut = nulls(n);
  if (n < period * 2 + 1) return { plusDI, minusDI, adx: adxOut };

  const tr = nulls(n);
  const plusDM = nulls(n);
  const minusDM = nulls(n);
  for (let i = 1; i < n; i++) {
    const up = bars[i].high - bars[i - 1].high;
    const dn = bars[i - 1].low - bars[i].low;
    plusDM[i] = up > dn && up > 0 ? up : 0;
    minusDM[i] = dn > up && dn > 0 ? dn : 0;
    const p = bars[i - 1].close;
    tr[i] = Math.max(bars[i].high - bars[i].low, Math.abs(bars[i].high - p), Math.abs(bars[i].low - p));
  }
  // Wilder smoothing of TR, +DM, -DM.
  const smooth = (src) => {
    const out = nulls(n);
    let sum = 0;
    for (let i = 1; i <= period; i++) sum += src[i] || 0;
    out[period] = sum;
    for (let i = period + 1; i < n; i++) out[i] = out[i - 1] - out[i - 1] / period + (src[i] || 0);
    return out;
  };
  const trS = smooth(tr);
  const pS = smooth(plusDM);
  const mS = smooth(minusDM);
  const dx = nulls(n);
  for (let i = period; i < n; i++) {
    if (!trS[i]) continue;
    plusDI[i] = (pS[i] / trS[i]) * 100;
    minusDI[i] = (mS[i] / trS[i]) * 100;
    const sum = plusDI[i] + minusDI[i];
    dx[i] = sum === 0 ? 0 : (Math.abs(plusDI[i] - minusDI[i]) / sum) * 100;
  }
  // ADX is the Wilder average of DX, itself seeded by the mean of the first `period` DX values.
  const first = period * 2 - 1;
  if (first >= n) return { plusDI, minusDI, adx: adxOut };
  let sum = 0;
  let count = 0;
  for (let i = period; i <= first; i++) { if (dx[i] !== null) { sum += dx[i]; count++; } }
  if (!count) return { plusDI, minusDI, adx: adxOut };
  let prev = sum / count;
  adxOut[first] = prev;
  for (let i = first + 1; i < n; i++) {
    if (dx[i] === null) continue;
    prev = (prev * (period - 1) + dx[i]) / period;
    adxOut[i] = prev;
  }
  return { plusDI, minusDI, adx: adxOut };
}

/** Arithmetic mean of a window, exported for reuse by tests. */
export const windowMean = (values) => mean(values);
