// AYZR HTTP API.
//
// Public read-only surface for visitors (no account, no login). The only write
// endpoints are operator-gated: AI analysis, signal decisions, and the safe controls
// REFRESH / CHECKPOINT / PAUSE / RESUME / STOP / RECOVERY. There is no BUY/SELL/ORDER
// endpoint anywhere in this server.

import { createServer } from 'node:http';
import { readFile } from 'node:fs/promises';
import { extname, join, normalize } from 'node:path';
import { RUNTIME, SYMBOL, TIMEFRAMES, DEFAULT_TIMEFRAME, DEFAULT_INDICATORS, INDICATOR_IDS } from '../config.mjs';
import { ENGINE_SPECS } from '../engines/index.mjs';
import { COST_MODEL, statistics } from '../engines/trades.mjs';
import { providerStatus } from '../ai/planner.mjs';

/**
 * What each indicator is and where its definition comes from. Sent with every response so the
 * UI can explain a number without hard-coding a second copy of the truth, and so a reader can
 * check the formula against the source rather than trusting the chart.
 */
export const INDICATOR_DEFINITIONS = {
  ema20: { name: 'EMA 20', kind: 'overlay', formula: 'EMA(close, 20), seeded with the SMA of the first 20 closes', source: 'Standard exponential average; seed follows common platform convention.' },
  ema50: { name: 'EMA 50', kind: 'overlay', formula: 'EMA(close, 50), SMA-seeded', source: 'As above.' },
  ema200: { name: 'EMA 200', kind: 'overlay', formula: 'EMA(close, 200), SMA-seeded', source: 'As above. Needs 200 bars, so it is absent on short windows.' },
  sma50: { name: 'SMA 50', kind: 'overlay', formula: 'mean(close, 50)', source: 'Arithmetic mean.' },
  sma200: { name: 'SMA 200', kind: 'overlay', formula: 'mean(close, 200)', source: 'Arithmetic mean.' },
  vwap: { name: 'VWAP 20', kind: 'overlay', formula: 'sum(typicalPrice x tickVolume) / sum(tickVolume) over 20 bars', source: 'Rolling VWAP. tickVolume is a venue proxy for traded volume, not true volume.' },
  bollinger: { name: 'Bollinger 20,2', kind: 'overlay', formula: 'SMA(20) +/- 2 x populationStdev(20)', source: 'John Bollinger. Population stdev (divide by n), matching his own definition.' },
  rsi: { name: 'RSI 14', kind: 'pane', formula: "100 - 100/(1+RS), RS = Wilder-smoothed avgGain/avgLoss, 14 periods", source: 'J. Welles Wilder Jr., "New Concepts in Technical Trading Systems" (1978).' },
  macd: { name: 'MACD 12,26,9', kind: 'pane', formula: 'line = EMA(12) - EMA(26); signal = EMA(9) of line; histogram = line - signal', source: 'Gerald Appel, as popularised by Thomas Aspray.' },
  stochastic: { name: 'Stochastic 14,3,3', kind: 'pane', formula: '%K = 100 x (close - lowestLow)/(highestHigh - lowestLow); %D = SMA(3) of SMA(3) of %K', source: 'George Lane.' },
  adx: { name: 'ADX 14', kind: 'pane', formula: 'Wilder-smoothed +DI/-DI, then ADX = Wilder average of DX', source: 'J. Welles Wilder Jr. (1978). Measures trend strength, not direction.' },
  volume: { name: 'Volume (tick)', kind: 'pane', formula: 'tick volume per bar, with a 20-bar mean', source: 'Venue tick volume. A proxy for traded volume and labelled as such in the UI.' },
};

const MIME = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.mjs': 'text/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.png': 'image/png',
  '.svg': 'image/svg+xml',
  '.woff2': 'font/woff2',
  '.ico': 'image/x-icon',
};

export function createApi({ runtime, store, webRoot, operatorToken }) {
  const paused = { value: false };

  const server = createServer(async (req, res) => {
    const url = new URL(req.url, `http://${req.headers.host || 'localhost'}`);
    const path = url.pathname;

    if (path.startsWith('/api/')) {
      return handleApi({ req, res, url, runtime, store, operatorToken, paused });
    }
    return serveStatic({ req, res, path, webRoot });
  });

  server.runtimePaused = () => paused.value;
  return server;
}

async function handleApi({ req, res, url, runtime, store, operatorToken, paused }) {
  const json = (code, body) => {
    const payload = JSON.stringify(body);
    res.writeHead(code, {
      'content-type': 'application/json; charset=utf-8',
      'cache-control': 'no-store',
      'access-control-allow-origin': '*',
    });
    res.end(payload);
  };

  const operatorOk = () => {
    if (!operatorToken) return { ok: false, reason: 'Operator mode is disabled: AYZR_OPERATOR_TOKEN is not set on the server.' };
    const provided = req.headers['x-operator-token'] || url.searchParams.get('operatorToken');
    if (provided !== operatorToken) return { ok: false, reason: 'Operator token missing or invalid.' };
    return { ok: true };
  };

  try {
    // ---- public read-only -------------------------------------------------
    if (url.pathname === '/api/health') {
      return json(200, {
        status: runtime.lastError ? 'DEGRADED' : 'HEALTHY',
        mode: 'SHADOW_ONLY',
        liveTrading: 'DISABLED',
        symbol: SYMBOL,
        ingestCycles: runtime.ingestCount,
        lastIngest: runtime.lastIngest,
        lastError: runtime.lastError,
        stream: runtime.streamState,
        ai: providerStatus(RUNTIME.ai),
        operatorMode: !!operatorToken,
        paused: paused.value,
        at: new Date().toISOString(),
      });
    }

    if (url.pathname === '/api/snapshot') {
      return json(200, runtime.snapshot(url.searchParams.get('timeframe')));
    }

    if (url.pathname === '/api/timeframes') {
      return json(200, { default: 'M15', timeframes: runtime.timeframeMatrix() });
    }

    // Indicator series for one timeframe. `only` is a comma-separated subset of the ids the
    // server knows how to compute; an unknown id is rejected rather than silently dropped, so
    // the chart can never believe it is showing something it is not.
    if (url.pathname === '/api/indicators') {
      const tf = url.searchParams.get('timeframe');
      const onlyParam = url.searchParams.get('only');
      let only = null;
      if (onlyParam) {
        only = onlyParam.split(',').map((s) => s.trim()).filter(Boolean);
        const unknown = only.filter((id) => !INDICATOR_IDS.includes(id));
        if (unknown.length) {
          return json(400, { error: `Unknown indicator(s): ${unknown.join(', ')}`, known: INDICATOR_IDS });
        }
      }
      // Report the timeframe that was actually used, not the one requested: echoing back
      // an unknown id made a rejected request look accepted.
      const resolved = TIMEFRAMES.some((t) => t.id === tf) ? tf : DEFAULT_TIMEFRAME;
      return json(200, {
        timeframe: resolved,
        requestedTimeframe: tf || null,
        indicators: runtime.indicators(resolved, only),
        definitions: INDICATOR_DEFINITIONS,
        default: DEFAULT_INDICATORS,
        available: INDICATOR_IDS,
        note: 'All indicators are computed on closed bars only and use their published definitions. Tick volume is a proxy for traded volume and is labelled as such.',
      });
    }

    if (url.pathname === '/api/engines') {
      return json(200, {
        engines: Object.entries(ENGINE_SPECS).map(([id, spec]) => ({ id, ...spec })),
        costModel: COST_MODEL,
        note: 'Every engine is defined by inputs, outputs, uncertainty and no-emit conditions. Statistics are computed per engine, timeframe and session so no engine can hide behind another.',
      });
    }

    if (url.pathname === '/api/regions') {
      const id = url.searchParams.get('id');
      if (id) {
        const region = store.getRegions({ limit: 2000 }).find((r) => r.id === id);
        if (!region) return json(404, { error: 'region not found' });
        return json(200, { region, events: store.getRegionEvents(id) });
      }
      return json(200, { regions: store.getRegions({ timeframe: url.searchParams.get('timeframe') || undefined, limit: 300 }) });
    }

    if (url.pathname === '/api/signals') {
      return json(200, { signals: store.getSignals({ timeframe: url.searchParams.get('timeframe') || undefined, limit: 100 }) });
    }

    if (url.pathname === '/api/virtual-trades') {
      return json(200, { trades: store.getVirtualTrades({ timeframe: url.searchParams.get('timeframe') || undefined, limit: 300 }) });
    }

    if (url.pathname === '/api/performance') {
      const tf = url.searchParams.get('timeframe') || undefined;
      const trades = store.getVirtualTrades({ timeframe: tf, limit: 2000 });
      return json(200, { statistics: statistics(trades), costsModelled: true });
    }

    if (url.pathname === '/api/audit') {
      return json(200, { entries: store.getAudit(Number(url.searchParams.get('limit') || 100)) });
    }

    if (url.pathname === '/api/ai/requests') {
      return json(200, { requests: store.getAiRequests(30) });
    }

    if (url.pathname === '/api/ai/status') {
      return json(200, providerStatus(RUNTIME.ai));
    }

    // ---- operator-gated writes --------------------------------------------
    if (url.pathname === '/api/ai/analyze' && req.method === 'POST') {
      const auth = operatorOk();
      if (!auth.ok) return json(403, { error: auth.reason });
      const body = await readJson(req);
      if (!body || typeof body.prompt !== 'string' || !body.prompt.trim()) {
        return json(400, { error: 'a non-empty "prompt" is required' });
      }
      const result = await runtime.requestAnalysis({
        prompt: body.prompt.trim().slice(0, 4000),
        operator: body.operator || 'operator',
        timeframe: body.timeframe,
      });
      return json(result.plan ? 200 : 202, result);
    }

    if (url.pathname === '/api/signals/decision' && req.method === 'POST') {
      const auth = operatorOk();
      if (!auth.ok) return json(403, { error: auth.reason });
      const body = await readJson(req);
      if (!body?.signalId || !['ACCEPTED', 'IGNORED'].includes(body.decision)) {
        return json(400, { error: 'signalId and decision (ACCEPTED|IGNORED) are required' });
      }
      return json(200, runtime.setOperatorDecision(body.signalId, body.decision));
    }

    if (url.pathname === '/api/virtual-trades/manual' && req.method === 'POST') {
      const auth = operatorOk();
      if (!auth.ok) return json(403, { error: auth.reason });
      const body = await readJson(req);
      if (!body) return json(400, { error: 'a JSON body is required' });

      const asNum = (v) => (typeof v === 'number' ? v : Number(v));
      const result = runtime.createManualTrade({
        direction: body.direction,
        entry: asNum(body.entry),
        stop: asNum(body.stop),
        target: asNum(body.target),
        timeframe: body.timeframe,
        operator: (body.operator || 'operator').toString().slice(0, 60),
        note: (body.note || '').toString().slice(0, 280),
      });
      if (!result.ok) return json(400, { error: result.errors.join('; '), errors: result.errors });
      return json(201, { ok: true, trade: result.trade });
    }

    if (url.pathname === '/api/virtual-trades/close' && req.method === 'POST') {
      const auth = operatorOk();
      if (!auth.ok) return json(403, { error: auth.reason });
      const body = await readJson(req);
      if (!body?.id) return json(400, { error: '"id" is required' });
      const result = runtime.closeManualTrade({
        id: body.id,
        exit: typeof body.exit === 'number' ? body.exit : Number(body.exit),
        reason: (body.reason || '').toString().slice(0, 200),
        operator: (body.operator || 'operator').toString().slice(0, 60),
      });
      if (!result.ok) return json(400, { error: result.errors.join('; '), errors: result.errors });
      return json(200, { ok: true, trade: result.trade });
    }

    if (url.pathname === '/api/control' && req.method === 'POST') {
      const auth = operatorOk();
      if (!auth.ok) return json(403, { error: auth.reason });
      const body = await readJson(req);
      const action = body?.action;
      const allowed = ['REFRESH', 'CHECKPOINT', 'PAUSE', 'RESUME', 'STOP', 'RECOVERY'];
      if (!allowed.includes(action)) {
        return json(400, { error: `action must be one of ${allowed.join(', ')}` });
      }
      store.audit('control', action, 'operator control action', 'operator');
      switch (action) {
        case 'REFRESH':
          runtime.ingest().catch(() => {});
          return json(200, { ok: true, action, detail: 'ingest triggered' });
        case 'CHECKPOINT':
          return json(200, { ok: true, action, detail: `checkpoint written at ${store.checkpoint()}` });
        case 'PAUSE':
          paused.value = true;
          return json(200, { ok: true, action, detail: 'ingest paused' });
        case 'RESUME':
          paused.value = false;
          return json(200, { ok: true, action, detail: 'ingest resumed' });
        case 'STOP':
          paused.value = true;
          runtime.stop();
          return json(200, { ok: true, action, detail: 'runtime stopped cleanly; state persisted' });
        case 'RECOVERY':
          runtime.restore();
          return json(200, { ok: true, action, detail: `recovery completed; ${runtime.regions.size} open regions` });
        default:
          return json(400, { error: 'unreachable' });
      }
    }

    return json(404, { error: `no route for ${url.pathname}` });
  } catch (err) {
    return json(500, { error: err.message });
  }
}

async function readJson(req) {
  const chunks = [];
  for await (const c of req) {
    chunks.push(c);
    if (chunks.reduce((a, b) => a + b.length, 0) > 1_000_000) throw new Error('request body too large');
  }
  if (!chunks.length) return null;
  try { return JSON.parse(Buffer.concat(chunks).toString('utf8')); } catch { return null; }
}

async function serveStatic({ req, res, path, webRoot }) {
  try {
    const safe = normalize(path).replace(/^(\.\.[/\\])+/, '');
    let filePath = join(webRoot, safe === '/' ? 'index.html' : safe);
    let data;
    try {
      data = await readFile(filePath);
    } catch {
      filePath = join(webRoot, 'index.html');
      data = await readFile(filePath);
    }
    res.writeHead(200, { 'content-type': MIME[extname(filePath)] || 'application/octet-stream', 'cache-control': 'no-cache' });
    res.end(data);
  } catch (err) {
    res.writeHead(404, { 'content-type': 'text/plain; charset=utf-8' });
    res.end(`not found: ${err.message}`);
  }
}

export { TIMEFRAMES };
