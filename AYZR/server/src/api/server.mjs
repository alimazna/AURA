// AYZR HTTP API.
//
// Public read-only surface for visitors (no account, no login). The only write
// endpoints are operator-gated: AI analysis, signal decisions, and the safe controls
// REFRESH / CHECKPOINT / PAUSE / RESUME / STOP / RECOVERY. There is no BUY/SELL/ORDER
// endpoint anywhere in this server.

import { createServer } from 'node:http';
import { readFile } from 'node:fs/promises';
import { extname, join, normalize } from 'node:path';
import { RUNTIME, SYMBOL, TIMEFRAMES } from '../config.mjs';
import { ENGINE_SPECS } from '../engines/index.mjs';
import { COST_MODEL, statistics } from '../engines/trades.mjs';
import { providerStatus } from '../ai/planner.mjs';

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
