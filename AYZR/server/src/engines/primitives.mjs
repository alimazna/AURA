// Shared analytical primitives for the AYZR engines.
//
// These operate on closed bars only. Nothing here looks ahead: every value at index i
// is computed from bars[0..i]. That is what keeps the engines honest against
// look-ahead bias (brief §4, §7).

/** True range of bar i (needs bar i-1). */
export function trueRange(bars, i) {
  const b = bars[i];
  if (i === 0) return b.high - b.low;
  const p = bars[i - 1].close;
  return Math.max(b.high - b.low, Math.abs(b.high - p), Math.abs(b.low - p));
}

/** Wilder's ATR series. atr[i] is undefined until there is enough history. */
export function atrSeries(bars, period = 14) {
  const out = new Array(bars.length).fill(undefined);
  if (bars.length < period + 1) return out;
  let sum = 0;
  for (let i = 1; i <= period; i++) sum += trueRange(bars, i);
  let atr = sum / period;
  out[period] = atr;
  for (let i = period + 1; i < bars.length; i++) {
    atr = (atr * (period - 1) + trueRange(bars, i)) / period;
    out[i] = atr;
  }
  return out;
}

export function lastDefined(series) {
  for (let i = series.length - 1; i >= 0; i--) if (series[i] !== undefined) return series[i];
  return undefined;
}

/**
 * Fractal swing points. A swing high at i requires `k` strictly lower highs on each
 * side. The point is only *confirmed* at i+k, which is the bar index callers must use
 * as the discovery time — never i itself.
 */
export function swings(bars, k = 2) {
  const highs = [];
  const lows = [];
  for (let i = k; i < bars.length - k; i++) {
    let isHigh = true;
    let isLow = true;
    for (let j = 1; j <= k; j++) {
      if (bars[i].high <= bars[i - j].high || bars[i].high <= bars[i + j].high) isHigh = false;
      if (bars[i].low >= bars[i - j].low || bars[i].low >= bars[i + j].low) isLow = false;
      if (!isHigh && !isLow) break;
    }
    if (isHigh) highs.push({ index: i, price: bars[i].high, time: bars[i].openTime, confirmedAt: i + k, confirmedTime: bars[i + k].openTime });
    if (isLow) lows.push({ index: i, price: bars[i].low, time: bars[i].openTime, confirmedAt: i + k, confirmedTime: bars[i + k].openTime });
  }
  return { highs, lows };
}

export function mean(values) {
  if (!values.length) return 0;
  return values.reduce((a, b) => a + b, 0) / values.length;
}

export function stdev(values) {
  if (values.length < 2) return 0;
  const m = mean(values);
  return Math.sqrt(values.reduce((a, b) => a + (b - m) ** 2, 0) / (values.length - 1));
}

export function clamp(v, lo, hi) {
  return Math.min(hi, Math.max(lo, v));
}

export function round(v, digits = 2) {
  const f = 10 ** digits;
  return Math.round(v * f) / f;
}

/** Deterministic id so the same evidence always produces the same region id. */
export function stableId(parts) {
  const s = parts.join('|');
  let h = 2166136261 >>> 0;
  for (let i = 0; i < s.length; i++) {
    h ^= s.charCodeAt(i);
    h = Math.imul(h, 16777619) >>> 0;
  }
  return h.toString(16).padStart(8, '0');
}

export function isoNow(d = new Date()) {
  return d.toISOString().replace(/\.\d{3}Z$/, 'Z');
}

export function isoFromMs(ms) {
  return new Date(ms).toISOString().replace(/\.\d{3}Z$/, 'Z');
}
