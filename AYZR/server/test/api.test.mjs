// Integration tests: the real HTTP API against a real SQLite store.
//
// The market runtime is stubbed here so the test is deterministic and offline. The
// store, routing, operator gate and static serving are all the real implementations.

import { test } from 'node:test';
import assert from 'node:assert/strict';
import { mkdtempSync, rmSync, writeFileSync, mkdirSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { Store } from '../src/store/db.mjs';
import { createApi } from '../src/api/server.mjs';

function makeHarness() {
  const dir = mkdtempSync(join(tmpdir(), 'ayzr-test-'));
  const store = new Store(join(dir, 'test.sqlite'));
  const webRoot = join(dir, 'web');
  mkdirSync(webRoot, { recursive: true });
  writeFileSync(join(webRoot, 'index.html'), '<html><body>AYZR</body></html>');

  const calls = { control: [] };
  const runtime = {
    lastError: null,
    lastIngest: '2026-01-05T12:00:00Z',
    ingestCount: 3,
    streamState: { transport: 'signalr', state: 'subscribed', detail: '' },
    regions: new Map(),
    snapshot: () => ({ symbol: 'XAUUSD', timeframe: 'M15', mode: 'SHADOW_ONLY', quote: null, data: {}, candles: [], analysis: { engines: [] }, regions: [], signals: [], virtualTrades: [], statistics: { counts: {} } }),
    timeframeMatrix: () => [{ id: 'M15', label: 'M15', status: 'OK' }],
    ingest: async () => {},
    stop: () => {},
    restore: () => {},
    setOperatorDecision: (id, d) => { store.setSignalDecision(id, d); return { ok: true }; },
    // Mirrors Runtime.requestAnalysis bookkeeping so the API wiring is what is under test.
    requestAnalysis: async ({ prompt, operator, timeframe }) => {
      const rec = { id: 'ai1', prompt, operator, timeframe, verdict: 'NOT_AVAILABLE', reason: 'no provider', plan: null, provider: 'none', createdAt: new Date().toISOString() };
      store.saveAiRequest(rec);
      store.audit('ai', 'ANALYSIS_REQUESTED', { id: rec.id, operator, verdict: rec.verdict }, 'operator');
      return rec;
    },
  };

  const server = createApi({ runtime, store, webRoot, operatorToken: 'secret-token' });
  return new Promise((resolve) => {
    server.listen(0, '127.0.0.1', () => {
      const { port } = server.address();
      resolve({ server, store, dir, base: `http://127.0.0.1:${port}`, calls });
    });
  });
}

async function withHarness(fn) {
  const h = await makeHarness();
  try { await fn(h); } finally {
    await new Promise((r) => h.server.close(r));
    h.store.close();
    rmSync(h.dir, { recursive: true, force: true });
  }
}

test('health reports shadow-only mode and that live trading is disabled', async () => {
  await withHarness(async ({ base }) => {
    const r = await fetch(`${base}/api/health`);
    assert.equal(r.status, 200);
    const b = await r.json();
    assert.equal(b.mode, 'SHADOW_ONLY');
    assert.equal(b.liveTrading, 'DISABLED');
    assert.equal(b.status, 'HEALTHY');
  });
});

test('there is no order-placement route anywhere on the server', async () => {
  await withHarness(async ({ base }) => {
    for (const p of ['/api/order', '/api/trade', '/api/buy', '/api/sell', '/api/execute', '/api/positions/open']) {
      const r = await fetch(`${base}${p}`, { method: 'POST', body: '{}' });
      assert.equal(r.status, 404, `${p} must not exist`);
    }
  });
});

test('read-only endpoints are public', async () => {
  await withHarness(async ({ base }) => {
    for (const p of ['/api/snapshot', '/api/timeframes', '/api/engines', '/api/regions', '/api/signals', '/api/virtual-trades', '/api/performance', '/api/audit']) {
      const r = await fetch(`${base}${p}`);
      assert.equal(r.status, 200, `${p} should be publicly readable`);
    }
  });
});

test('AI analysis is refused without the operator token', async () => {
  await withHarness(async ({ base }) => {
    const r = await fetch(`${base}/api/ai/analyze`, {
      method: 'POST',
      headers: { 'content-type': 'application/json' },
      body: JSON.stringify({ prompt: 'analyse gold' }),
    });
    assert.equal(r.status, 403);
    const b = await r.json();
    assert.match(b.error, /token/i);
  });
});

test('AI analysis is refused with the wrong operator token', async () => {
  await withHarness(async ({ base }) => {
    const r = await fetch(`${base}/api/ai/analyze`, {
      method: 'POST',
      headers: { 'content-type': 'application/json', 'x-operator-token': 'wrong' },
      body: JSON.stringify({ prompt: 'analyse gold' }),
    });
    assert.equal(r.status, 403);
  });
});

test('AI analysis with the correct token records the request and returns its verdict', async () => {
  await withHarness(async ({ base, store }) => {
    const r = await fetch(`${base}/api/ai/analyze`, {
      method: 'POST',
      headers: { 'content-type': 'application/json', 'x-operator-token': 'secret-token' },
      body: JSON.stringify({ prompt: 'analyse gold', timeframe: 'M15' }),
    });
    assert.ok([200, 202].includes(r.status));
    const b = await r.json();
    assert.equal(b.verdict, 'NOT_AVAILABLE');
    const audits = store.getAudit(50).filter((a) => a.category === 'ai');
    assert.ok(audits.length > 0, 'the request must be audited');
  });
});

test('an empty AI prompt is rejected before it reaches the provider', async () => {
  await withHarness(async ({ base }) => {
    const r = await fetch(`${base}/api/ai/analyze`, {
      method: 'POST',
      headers: { 'content-type': 'application/json', 'x-operator-token': 'secret-token' },
      body: JSON.stringify({ prompt: '   ' }),
    });
    assert.equal(r.status, 400);
  });
});

test('operator controls require the token and only allow the safe set', async () => {
  await withHarness(async ({ base }) => {
    const noToken = await fetch(`${base}/api/control`, {
      method: 'POST', headers: { 'content-type': 'application/json' }, body: JSON.stringify({ action: 'REFRESH' }),
    });
    assert.equal(noToken.status, 403);

    const badAction = await fetch(`${base}/api/control`, {
      method: 'POST',
      headers: { 'content-type': 'application/json', 'x-operator-token': 'secret-token' },
      body: JSON.stringify({ action: 'LIQUIDATE' }),
    });
    assert.equal(badAction.status, 400);

    const ok = await fetch(`${base}/api/control`, {
      method: 'POST',
      headers: { 'content-type': 'application/json', 'x-operator-token': 'secret-token' },
      body: JSON.stringify({ action: 'CHECKPOINT' }),
    });
    assert.equal(ok.status, 200);
    const b = await ok.json();
    assert.equal(b.action, 'CHECKPOINT');
  });
});

test('signal decisions require the operator token', async () => {
  await withHarness(async ({ base, store }) => {
    store.upsertSignal({
      id: 's1', regionId: 'r1', engine: 'liquidity', timeframe: 'M15', direction: 'LONG',
      score: 0.6, state: 'CONFIRMED_RETEST', entry: 4000, stop: 3990, target: 4020,
      conditions: [], reason: 'test', sourceTime: null, createdAt: new Date().toISOString(),
    });

    const refused = await fetch(`${base}/api/signals/decision`, {
      method: 'POST', headers: { 'content-type': 'application/json' }, body: JSON.stringify({ signalId: 's1', decision: 'ACCEPTED' }),
    });
    assert.equal(refused.status, 403);

    const accepted = await fetch(`${base}/api/signals/decision`, {
      method: 'POST',
      headers: { 'content-type': 'application/json', 'x-operator-token': 'secret-token' },
      body: JSON.stringify({ signalId: 's1', decision: 'ACCEPTED' }),
    });
    assert.equal(accepted.status, 200);

    const signals = store.getSignals({});
    assert.equal(signals.find((s) => s.id === 's1').operatorDecision, 'ACCEPTED');
  });
});

test('an invalid signal decision value is rejected', async () => {
  await withHarness(async ({ base }) => {
    const r = await fetch(`${base}/api/signals/decision`, {
      method: 'POST',
      headers: { 'content-type': 'application/json', 'x-operator-token': 'secret-token' },
      body: JSON.stringify({ signalId: 's1', decision: 'MAYBE' }),
    });
    assert.equal(r.status, 400);
  });
});

test('static files are served and unknown paths fall back to the app shell', async () => {
  await withHarness(async ({ base }) => {
    const root = await fetch(`${base}/`);
    assert.equal(root.status, 200);
    assert.match(await root.text(), /AYZR/);
    const deep = await fetch(`${base}/some/client/route`);
    assert.equal(deep.status, 200, 'client-side routes must return the shell');
  });
});

/* ------------------------------------------------------------- persistence */

test('bars round-trip through the store in time order', async () => {
  await withHarness(async ({ store }) => {
    store.upsertBars('M15', [
      { openTime: '2026-01-05T12:15:00Z', open: 2, high: 3, low: 1, close: 2, tickVolume: 5, isOpen: false },
      { openTime: '2026-01-05T12:00:00Z', open: 1, high: 2, low: 1, close: 2, tickVolume: 5, isOpen: false },
    ]);
    const bars = store.getBars('M15');
    assert.equal(bars.length, 2);
    assert.ok(bars[0].time < bars[1].time, 'bars must come back oldest-first');
  });
});

test('regions and their transitions are recorded for audit', async () => {
  await withHarness(async ({ store }) => {
    const region = {
      id: 'r9', engine: 'levels', type: 'SUPPORT_LEVEL', timeframe: 'M15', direction: 'LONG',
      zone: { low: 3990, high: 3995 }, state: 'DISCOVERED', score: 0.5, conditions: ['3 touches'],
      reason: 'support', sourceTime: '2026-01-05T12:00:00Z', discoveredAtMs: Date.now(),
    };
    store.upsertRegion(region);
    store.recordRegionEvent('r9', null, 'DISCOVERED', 'support', region.sourceTime);
    store.recordRegionEvent('r9', 'DISCOVERED', 'APPROACH', 'price arrived', region.sourceTime);

    const events = store.getRegionEvents('r9');
    assert.equal(events.length, 2);
    assert.equal(events[0].to, 'APPROACH', 'newest transition first');
    assert.equal(store.getRegions({ timeframe: 'M15' }).length, 1);
  });
});

test('virtual trades round-trip with their cost model intact', async () => {
  await withHarness(async ({ store }) => {
    store.upsertVirtualTrade({
      id: 'v1', regionId: 'r1', signalId: 's1', engine: 'imbalance', timeframe: 'M15', direction: 'SHORT',
      state: 'OPEN', entry: 4010, entryRaw: 4009.5, stop: 4020, target: 3990, lot: 0.01,
      costs: { spreadAtEntry: 0.17, slippagePerSide: 0.15, commission: 0, modelled: true },
      conditions: ['fvg'], reason: 'imbalance', openedAt: '2026-01-05T12:00:00Z', openedAtMs: 1,
      triggerBarTime: '2026-01-05T12:00:00Z', closedAt: null, closedAtMs: null, exit: null,
      result: null, rMultiple: null, pnlUsd: null, exitReason: null,
    });
    const t = store.getVirtualTrades({ state: 'OPEN' })[0];
    assert.equal(t.costs.modelled, true);
    assert.equal(t.costs.spreadAtEntry, 0.17);
    assert.equal(t.triggerBarTime, '2026-01-05T12:00:00Z');
  });
});

test('the audit log records who acted', async () => {
  await withHarness(async ({ store }) => {
    store.audit('control', 'PAUSE', 'operator paused ingest', 'operator');
    store.audit('data', 'INGEST', 'cycle 1 ok', 'runtime');
    const entries = store.getAudit(10);
    assert.equal(entries.length, 2);
    assert.ok(entries.some((e) => e.actor === 'operator'));
    assert.ok(entries.some((e) => e.actor === 'runtime'));
  });
});
