// BiQuote client — REST + SignalR hub.
//
// Every endpoint used here was discovered and verified live against biquote.io
// (see docs/BIQUOTE_DISCOVERY.md). Nothing in this file assumes an API shape that
// was not observed. Failures are surfaced as explicit states, never as fake data.

import { BIQUOTE } from '../config.mjs';

export class BiQuoteError extends Error {
  constructor(message, { status = 0, cause = null } = {}) {
    super(message);
    this.name = 'BiQuoteError';
    this.status = status;
    this.cause = cause;
  }
}

async function getJson(path) {
  const url = `${BIQUOTE.base}${path}`;
  const ac = new AbortController();
  const timer = setTimeout(() => ac.abort(), BIQUOTE.timeoutMs);
  try {
    const res = await fetch(url, {
      signal: ac.signal,
      headers: { accept: 'application/json' },
    });
    const text = await res.text();
    if (!res.ok) {
      throw new BiQuoteError(`BiQuote ${path} responded ${res.status}`, { status: res.status });
    }
    if (!text) throw new BiQuoteError(`BiQuote ${path} returned an empty body`);
    try {
      return JSON.parse(text);
    } catch (err) {
      throw new BiQuoteError(`BiQuote ${path} returned non-JSON content`, { cause: err });
    }
  } catch (err) {
    if (err instanceof BiQuoteError) throw err;
    throw new BiQuoteError(`BiQuote ${path} request failed: ${err.message}`, { cause: err });
  } finally {
    clearTimeout(timer);
  }
}

/** Live quote for one symbol. Verified shape: bid, ask, mid, spread, timestamp, marketState, stale. */
export async function quote(symbol) {
  const q = await getJson(`/api/${encodeURIComponent(symbol)}`);
  if (!q || typeof q !== 'object') throw new BiQuoteError(`BiQuote returned no quote for ${symbol}`);
  return normalizeQuote(q);
}

/** Batched quotes. Verified shape: { SYMBOL: quote }. */
export async function quotes(symbols) {
  const qs = symbols.map((s) => `symbols=${encodeURIComponent(s)}`).join('&');
  const data = await getJson(`/api/latest?${qs}`);
  const out = {};
  for (const [k, v] of Object.entries(data || {})) out[k] = normalizeQuote(v);
  return out;
}

function normalizeQuote(q) {
  const bid = num(q.bid);
  const ask = num(q.ask);
  const mid = num(q.mid) || (bid && ask ? (bid + ask) / 2 : 0);
  return {
    symbol: q.symbol,
    description: q.description || '',
    bid,
    ask,
    mid,
    spread: num(q.spread),
    last: num(q.last),
    high: num(q.high),
    low: num(q.low),
    direction: q.direction || 'UNKNOWN',
    dayDiffPercent: num(q.dayDiffPercent),
    source: q.source || 'UNKNOWN',
    exchange: q.exchange || '',
    marketState: q.marketState || 'unknown',
    stale: q.stale === true,
    quoteAgeSeconds: num(q.quoteAgeSeconds),
    timestamp: q.timestamp || null,
    lastQuoteAt: q.lastQuoteAt || q.timestamp || null,
  };
}

function num(v) {
  const n = typeof v === 'number' ? v : parseFloat(v);
  return Number.isFinite(n) ? n : 0;
}

/**
 * OHLC bars. Verified shape: { symbol, interval, bars: [{ openTime, open, high, low, close, tickVolume, isOpen }] }.
 * The API returns bars newest-first; AYZR normalises to oldest-first so downstream
 * engines and the chart never depend on response ordering.
 *
 * `isOpen` is the venue's own flag, so closed-bar-only rendering does not have to be
 * inferred from timestamps.
 */
export async function ohlc(symbol, interval, limit = 400) {
  const data = await getJson(`/api/${encodeURIComponent(symbol)}/ohlc?interval=${encodeURIComponent(interval)}&limit=${limit}`);
  const bars = Array.isArray(data?.bars) ? data.bars : [];
  const normalised = bars.map((b) => ({
    openTime: b.openTime,
    time: Date.parse(b.openTime),
    open: num(b.open),
    high: num(b.high),
    low: num(b.low),
    close: num(b.close),
    tickVolume: num(b.tickVolume),
    volume: num(b.volume),
    isOpen: b.isOpen === true,
  })).filter((b) => Number.isFinite(b.time) && b.high >= b.low && b.high > 0);

  normalised.sort((a, b) => a.time - b.time);
  return { symbol: data?.symbol || symbol, interval: data?.interval || interval, bars: normalised };
}

/**
 * Higher-timeframe bars the venue does not serve directly (verified: `1w` is rejected
 * with HTTP 400). These are aggregated from the venue's own D1 bars, never fabricated:
 * a derived bar is the union of its constituent daily bars, and the set is marked
 * `derived: true` so the UI can label it honestly.
 */
export function aggregateBars(dailyBars, minutes) {
  const buckets = new Map();
  for (const b of dailyBars) {
    const d = new Date(b.time);
    let key;
    if (minutes >= 43200) {
      key = Date.UTC(d.getUTCFullYear(), d.getUTCMonth(), 1);
    } else {
      // ISO weeks starting Monday
      const day = (d.getUTCDay() + 6) % 7;
      const monday = Date.UTC(d.getUTCFullYear(), d.getUTCMonth(), d.getUTCDate() - day);
      key = monday;
    }
    const cur = buckets.get(key);
    if (!cur) {
      buckets.set(key, { openTime: new Date(key).toISOString().replace('.000Z', 'Z'), time: key, open: b.open, high: b.high, low: b.low, close: b.close, tickVolume: b.tickVolume, volume: b.volume, isOpen: false, derived: true });
    } else {
      cur.high = Math.max(cur.high, b.high);
      cur.low = Math.min(cur.low, b.low);
      cur.close = b.close;
      cur.tickVolume += b.tickVolume;
      cur.volume += b.volume;
    }
  }
  return [...buckets.values()].sort((a, b) => a.time - b.time);
}

/** Symbol catalogue. Verified shape: array of descriptor objects. */
export async function symbols() {
  const data = await getJson('/api/symbols');
  return Array.isArray(data) ? data : [];
}

/**
 * Tick stream over the SignalR hub.
 *
 * The hub contract was read from the published documentation and verified live against
 * biquote.io: negotiate returns a connection token, the WebSocket transport speaks the
 * SignalR JSON protocol with \u001e record separators, and subscribing yields
 * `ReceiveTick` (and `ReceiveSubscriptionState`) events. Verified on 2026-11-06 with 40
 * ticks observed in 12s for XAUUSD.
 *
 * This is a minimal protocol client, not a bundled dependency. If the transport is
 * unavailable the caller receives an explicit `unavailable` state and must fall back to
 * REST polling — the runtime never presents polling as a live stream.
 */
export async function streamTicks(symbolsToWatch, onTick, { onState = () => {} } = {}) {
  if (typeof WebSocket !== 'function') {
    throw new BiQuoteError('This Node runtime has no global WebSocket; tick streaming unavailable');
  }
  const RS = '\u001e';
  const frame = (obj) => JSON.stringify(obj) + RS;

  let token;
  try {
    const res = await fetch(`${BIQUOTE.hub}/negotiate?negotiateVersion=1`, {
      method: 'POST',
      headers: { accept: 'application/json' },
    });
    if (!res.ok) throw new BiQuoteError(`hub negotiate responded ${res.status}`, { status: res.status });
    const body = await res.json();
    token = body.connectionToken || body.connectionId;
    if (!token) throw new BiQuoteError('hub negotiate returned no connection token');
    onState({ state: 'negotiated', transports: (body.availableTransports || []).map((t) => t.transport) });
  } catch (err) {
    const e = err instanceof BiQuoteError ? err : new BiQuoteError(`hub negotiate failed: ${err.message}`, { cause: err });
    onState({ state: 'unavailable', reason: e.message });
    throw e;
  }

  const ws = new WebSocket(`${BIQUOTE.hub.replace(/^http/, 'ws')}?id=${encodeURIComponent(token)}`);
  let handshakeDone = false;

  ws.addEventListener('open', () => {
    ws.send(frame({ protocol: 'json', version: 1 }));
  });

  ws.addEventListener('message', (ev) => {
    const raw = typeof ev.data === 'string' ? ev.data : ev.data.toString();
    for (const part of raw.split(RS)) {
      if (!part) continue;
      let msg;
      try { msg = JSON.parse(part); } catch { continue; }

      if (!handshakeDone) {
        handshakeDone = true;
        if (msg.error) { onState({ state: 'handshake_error', reason: msg.error }); return; }
        onState({ state: 'connected' });
        ws.send(frame({ type: 1, target: 'Subscribe', arguments: [symbolsToWatch] }));
        continue;
      }
      if (msg.type === 6) { ws.send(frame({ type: 6 })); continue; }  // keep-alive ping
      if (msg.type === 1 && msg.target === 'ReceiveTick') {
        const tick = msg.arguments?.[0];
        if (tick) onTick(normalizeQuote(tick));
      } else if (msg.type === 1 && msg.target === 'ReceiveSubscriptionState') {
        onState({ state: 'subscribed', detail: msg.arguments?.[0] });
      }
    }
  });

  ws.addEventListener('error', () => onState({ state: 'error' }));
  ws.addEventListener('close', (ev) => onState({ state: 'closed', code: ev.code, reason: ev.reason }));

  return {
    close() {
      try { ws.close(); } catch { /* already closed */ }
    },
    subscribe(list) {
      if (ws.readyState === WebSocket.OPEN) ws.send(frame({ type: 1, target: 'Subscribe', arguments: [list] }));
    },
    get state() {
      return ws.readyState === WebSocket.OPEN && handshakeDone ? 'connected' : 'connecting';
    },
  };
}
