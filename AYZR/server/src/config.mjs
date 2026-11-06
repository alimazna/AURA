// AYZR runtime configuration. Every value here is either a documented default or
// supplied by the environment; nothing is invented at call time.

export const SYMBOL = process.env.AYZR_SYMBOL || 'XAUUSD';
export const DEFAULT_TIMEFRAME = process.env.AYZR_DEFAULT_TIMEFRAME || 'M15';

// Canonical timeframe set. `stream` is the BiQuote OHLC interval that backs the
// timeframe, so timeframe identity is never derived from array position.
//
// Verified 2026-11-06: BiQuote serves 1m, 5m, 15m, 30m, 1h, 4h, 1d. It rejects `1w`
// (HTTP 400) and does not document a monthly interval. W1 and MN1 are therefore
// aggregated from the venue's own D1 bars — marked `derived: true`, never fabricated.
export const TIMEFRAMES = [
  { id: 'M1', label: 'M1', stream: '1m', minutes: 1, role: 'execution', derived: false },
  { id: 'M5', label: 'M5', stream: '5m', minutes: 5, role: 'execution', derived: false },
  { id: 'M15', label: 'M15', stream: '15m', minutes: 15, role: 'operational', derived: false },
  { id: 'M30', label: 'M30', stream: '30m', minutes: 30, role: 'context', derived: false },
  { id: 'H1', label: 'H1', stream: '1h', minutes: 60, role: 'context', derived: false },
  { id: 'H4', label: 'H4', stream: '4h', minutes: 240, role: 'structural', derived: false },
  { id: 'D1', label: 'D1', stream: '1d', minutes: 1440, role: 'horizon', derived: false },
  { id: 'W1', label: 'W1', stream: '1d', minutes: 10080, role: 'horizon', derived: true },
  { id: 'MN1', label: 'MN1', stream: '1d', minutes: 43200, role: 'horizon', derived: true },
];

export const TIMEFRAME_BY_ID = new Map(TIMEFRAMES.map((t) => [t.id, t]));

export const BIQUOTE = {
  base: process.env.BIQUOTE_BASE || 'https://biquote.io',
  hub: process.env.BIQUOTE_HUB || 'https://biquote.io/hubs/tick',
  timeoutMs: Number(process.env.BIQUOTE_TIMEOUT_MS || 20000),
};

export const RUNTIME = {
  port: Number(process.env.PORT || 8787),
  host: process.env.HOST || '0.0.0.0',
  pollMs: Number(process.env.AYZR_POLL_MS || 15000),
  barsPerTimeframe: Number(process.env.AYZR_BARS || 400),
  // Daily bars fetched to build W1/MN1. Must be deep enough that the deepest derived
  // timeframe still has usable history: ~2 years of days gives 24 monthly bars.
  derivedSourceBars: Number(process.env.AYZR_DERIVED_SOURCE_BARS || 730),
  dbPath: process.env.AYZR_DB || 'data/ayzr.sqlite',
  // AI is never called by a timer. It requires an explicit operator request.
  ai: {
    provider: process.env.AYZR_AI_PROVIDER || 'none',
    model: process.env.AYZR_AI_MODEL || '',
    apiKey: process.env.AYZR_AI_API_KEY || '',
    baseUrl: process.env.AYZR_AI_BASE_URL || '',
  },
};
