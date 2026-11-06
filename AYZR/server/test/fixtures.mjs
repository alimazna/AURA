// Deterministic bar fixtures.
//
// Synthetic bars are legitimate in tests: they let a specific condition be asserted
// exactly. They are never used by the runtime, which only ever reads live venue data.

export function flatBars(n, price = 4000, step = 0) {
  const out = [];
  const t0 = Date.UTC(2026, 0, 5, 0, 0, 0);
  for (let i = 0; i < n; i++) {
    const p = price + step * i;
    out.push({
      openTime: new Date(t0 + i * 15 * 60000).toISOString().replace('.000Z', 'Z'),
      time: t0 + i * 15 * 60000,
      open: p, high: p + 1, low: p - 1, close: p,
      tickVolume: 100, volume: 0, isOpen: false,
    });
  }
  return out;
}

/** Bars that form a clean demand base then an impulsive advance. */
export function demandBaseBars({ base = 40, impulse = 6 } = {}) {
  const bars = flatBars(base, 4000);
  let p = 4000;
  for (let i = 0; i < impulse; i++) {
    const o = p;
    const c = p + 12;              // 12 points per bar, well above 1.5x ATR
    const t = Date.UTC(2026, 0, 5, 0, 0, 0) + (base + i) * 15 * 60000;
    bars.push({
      openTime: new Date(t).toISOString().replace('.000Z', 'Z'), time: t,
      open: o, high: c + 2, low: o - 1, close: c, tickVolume: 500, volume: 0, isOpen: false,
    });
    p = c;
  }
  return bars;
}

/** Two swing highs at the same price, forming a liquidity pool. */
export function equalHighsBars() {
  const bars = flatBars(60, 4000);
  const setHigh = (i, h) => {
    bars[i] = { ...bars[i], high: h, open: h - 4, close: h - 2, low: h - 8 };
    bars[i - 1] = { ...bars[i - 1], high: h - 6, low: h - 12, open: h - 10, close: h - 8 };
    bars[i + 1] = { ...bars[i + 1], high: h - 6, low: h - 12, open: h - 8, close: h - 10 };
  };
  setHigh(20, 4050);
  setHigh(40, 4050.4);
  return bars;
}

/** A bar that trades beyond a level then closes back inside it. */
export function sweepBar(price = 4050) {
  return {
    openTime: '2026-01-05T12:00:00Z', time: Date.parse('2026-01-05T12:00:00Z'),
    open: price - 3, high: price + 5, low: price - 6, close: price - 2,
    tickVolume: 900, volume: 0, isOpen: false,
  };
}

export function barsWithOpenLast(n = 30) {
  const bars = flatBars(n, 4000, 1);
  const last = bars[bars.length - 1];
  bars[bars.length - 1] = { ...last, isOpen: true };
  return bars;
}
