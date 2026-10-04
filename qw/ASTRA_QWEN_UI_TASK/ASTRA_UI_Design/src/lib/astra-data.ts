// PROTOTYPE / SAMPLE DATA ONLY — deterministic mock values for visual design.
// Not connected to the ASTRA runtime; no business decisions live here.

export type Timeframe = "M1" | "M5" | "M15" | "M30" | "H1" | "H4" | "D1" | "W1" | "MN1";
export const TIMEFRAMES: Timeframe[] = ["M1", "M5", "M15", "M30", "H1", "H4", "D1", "W1", "MN1"];

export interface Candle { time: number; open: number; high: number; low: number; close: number }
export interface MarketView { symbol: "XAUUSD"; timeframe: Timeframe; candles: Candle[] }

export type State = "HEALTHY" | "WARNING" | "CRITICAL" | "UNKNOWN" | "NOT AVAILABLE" | "EMPTY";

const TF_MINUTES: Record<Timeframe, number> = {
  M1: 1, M5: 5, M15: 15, M30: 30, H1: 60, H4: 240, D1: 1440, W1: 10080, MN1: 43200,
};
const TF_VOL: Record<Timeframe, number> = {
  M1: 0.6, M5: 1.3, M15: 2.4, M30: 3.4, H1: 4.8, H4: 9.5, D1: 22, W1: 48, MN1: 110,
};

export const TF_ROLE: Partial<Record<Timeframe, string>> = {
  M15: "Operational", H4: "Structural", H1: "Context", M30: "Context",
  M5: "Execution ctx", M1: "Execution ctx", D1: "Long horizon", W1: "Long horizon", MN1: "Long horizon",
};

function rng(seed: number) {
  let s = seed >>> 0;
  return () => ((s = (s * 1664525 + 1013904223) >>> 0) / 4294967296);
}

const END = Date.UTC(2026, 9, 4, 11, 0);
const LAST = 4958.42;

export function mockMarket(tf: Timeframe, count = 140): MarketView {
  const r = rng(TIMEFRAMES.indexOf(tf) * 7919 + 17);
  const vol = TF_VOL[tf];
  const step = TF_MINUTES[tf] * 60000;
  const raw: Candle[] = [];
  let price = LAST;
  // build backwards from the last price
  for (let i = 0; i < count; i++) {
    const close = price;
    const drift = Math.sin(i / 17) * vol * 0.35 + (r() - 0.5) * vol * 2;
    const open = close - drift;
    const high = Math.max(open, close) + r() * vol * 0.9;
    const low = Math.min(open, close) - r() * vol * 0.9;
    raw.push({ time: END - i * step, open, high, low, close });
    price = open + (r() - 0.5) * vol * 0.2;
  }
  return { symbol: "XAUUSD", timeframe: tf, candles: raw.reverse() };
}

export const MATRIX: Record<Timeframe, { health: State; quality: number; seq: string; fresh: string; last: string; signal: string }> = {
  M1: { health: "HEALTHY", quality: 99.2, seq: "OK", fresh: "0.8s", last: "10:59", signal: "—" },
  M5: { health: "HEALTHY", quality: 99.6, seq: "OK", fresh: "1.1s", last: "10:55", signal: "NEUTRAL" },
  M15: { health: "HEALTHY", quality: 99.8, seq: "OK", fresh: "1.2s", last: "10:45", signal: "LONG SETUP" },
  M30: { health: "HEALTHY", quality: 99.7, seq: "OK", fresh: "1.4s", last: "10:30", signal: "LONG" },
  H1: { health: "HEALTHY", quality: 99.9, seq: "OK", fresh: "1.6s", last: "10:00", signal: "LONG" },
  H4: { health: "HEALTHY", quality: 100, seq: "OK", fresh: "2.0s", last: "08:00", signal: "STRUCTURE ↑" },
  D1: { health: "WARNING", quality: 97.1, seq: "GAP 1", fresh: "3.4s", last: "03 OCT", signal: "NEUTRAL" },
  W1: { health: "HEALTHY", quality: 99.4, seq: "OK", fresh: "4.1s", last: "28 SEP", signal: "—" },
  MN1: { health: "UNKNOWN", quality: 0, seq: "—", fresh: "—", last: "SEP 26", signal: "—" },
};

export const SIGNALS = [
  { tf: "M15", dir: "LONG", score: 72, conf: 0.68, time: "10:45", state: "OBSERVING" },
  { tf: "H1", dir: "LONG", score: 64, conf: 0.61, time: "10:00", state: "OBSERVING" },
  { tf: "M5", dir: "NEUTRAL", score: 48, conf: 0.44, time: "10:55", state: "WATCH" },
  { tf: "H4", dir: "LONG", score: 70, conf: 0.66, time: "08:00", state: "CONTEXT" },
  { tf: "D1", dir: "SHORT", score: 38, conf: 0.35, time: "03 OCT", state: "STALE" },
] as const;
