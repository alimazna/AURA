// Indicator API tests: the real HTTP route against the real indicator maths.
//
// The runtime is stubbed only where it touches the venue. `indicators()` is the real
// implementation, so a broken formula or a bad route still fails here.

import { test } from 'node:test';
import assert from 'node:assert/strict';
import { mkdtempSync, rmSync, writeFileSync, mkdirSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { Store } from '../src/store/db.mjs';
import { createApi } from '../src/api/server.mjs';
import { Runtime } from '../src/runtime.mjs';
import { DEFAULT_INDICATORS, INDICATOR_IDS } from '../src/config.mjs';

/** A runtime with real indicator maths and synthetic bars, but no network. */
function runtimeWithBars(count = 260) {
  const store = new Store(join(mkdtempSync(join(tmpdir(), 'ayzr-ind-')), 't.sqlite'));
  const rt = new Runtime(store);
  const bars = Array.from({ length: count }, (_, i) => {
    const c = 4000 + Math.sin(i / 7) * 12 + i * 0.15;
    return {
      openTime: new Date(Date.UTC(2026, 0, 1, 0, i)).toISOString(),
      open: c, high: c + 2, low: c - 2, close: c, tickVolume: 100 + (i % 9),
      isOpen: false,
    };
  });
  rt.bars.set('M15', bars);
  return { rt, store };
}

function withServer(rt, fn) {
  const dir = mkdtempSync(join(tmpdir(), 'ayzr-ind-web-'));
  const webRoot = join(dir, 'web');
  mkdirSync(webRoot, { recursive: true });
  writeFileSync(join(webRoot, 'index.html'), '<html></html>');
  const server = createApi({ runtime: rt, store: rt.store, webRoot, operatorToken: 'tok' });
  return new Promise((resolve, reject) => {
    server.listen(0, '127.0.0.1', async () => {
      const port = server.address().port;
      try {
        await fn(`http://127.0.0.1:${port}`);
        resolve();
      } catch (e) {
        reject(e);
      } finally {
        server.close();
      }
    });
  });
}

test('GET /api/indicators returns the default set with definitions and sources', async () => {
  const { rt } = runtimeWithBars();
  await withServer(rt, async (base) => {
    const res = await fetch(`${base}/api/indicators?timeframe=M15`);
    assert.equal(res.status, 200);
    const body = await res.json();
    assert.deepEqual(body.default, DEFAULT_INDICATORS);
    assert.deepEqual(body.available, INDICATOR_IDS);
    // Default set is computed and aligned to the bar count.
    for (const id of DEFAULT_INDICATORS) {
      const series = body.indicators[id];
      assert.ok(Array.isArray(series), `${id} should be a series`);
      assert.equal(series.length, 200, `${id} should align to the 200-bar window`);
    }
    // Every indicator that can be requested is documented, with a source.
    for (const id of INDICATOR_IDS) {
      assert.ok(body.definitions[id], `${id} needs a definition`);
      assert.ok(body.definitions[id].source, `${id} needs a source`);
      assert.ok(body.definitions[id].formula, `${id} needs a formula`);
    }
  });
});

test('GET /api/indicators honours an explicit subset', async () => {
  const { rt } = runtimeWithBars();
  await withServer(rt, async (base) => {
    const body = await (await fetch(`${base}/api/indicators?timeframe=M15&only=rsi,macd,bollinger`)).json();
    assert.ok(body.indicators.rsi);
    assert.ok(body.indicators.macd);
    assert.ok(body.indicators.bollinger);
    assert.equal(body.indicators.ema20, undefined);
    assert.equal(body.indicators.volume, undefined);
  });
});

test('GET /api/indicators rejects an unknown indicator instead of ignoring it', async () => {
  const { rt } = runtimeWithBars();
  await withServer(rt, async (base) => {
    const res = await fetch(`${base}/api/indicators?timeframe=M15&only=rsi,superSignal`);
    assert.equal(res.status, 400);
    const body = await res.json();
    assert.match(body.error, /superSignal/);
    assert.deepEqual(body.known, INDICATOR_IDS);
  });
});

test('indicator values are plausible for a rising synthetic series', async () => {
  // A monotonic series, because "short EMA leads long EMA" is only a valid claim when the
  // series actually trends. The oscillating default fixture would make it false.
  const { rt } = runtimeWithBars(260);
  const rising = Array.from({ length: 260 }, (_, i) => {
    const c = 4000 + i * 0.5;
    return { openTime: new Date(Date.UTC(2026, 0, 1, 0, i)).toISOString(), open: c, high: c + 2, low: c - 2, close: c, tickVolume: 100, isOpen: false };
  });
  rt.bars.set('M15', rising);
  await withServer(rt, async (base) => {
    const body = await (await fetch(`${base}/api/indicators?timeframe=M15&only=rsi,ema20,ema50,bollinger,adx`)).json();
    const { rsi, ema20, ema50, bollinger, adx } = body.indicators;
    const last = (s) => s[s.length - 1];
    assert.ok(last(rsi) >= 0 && last(rsi) <= 100);
    // A rising series puts the short EMA above the long EMA.
    assert.ok(last(ema20) > last(ema50), 'EMA20 should lead EMA50 on a rising series');
    // Bands must bracket the middle line.
    assert.ok(last(bollinger.upper) > last(bollinger.mid));
    assert.ok(last(bollinger.lower) < last(bollinger.mid));
    // adx is the {plusDI, minusDI, adx} bundle, so the series is adx.adx.
    assert.ok(last(adx.adx) >= 0 && last(adx.adx) <= 100);
    assert.ok(last(adx.plusDI) >= 0 && last(adx.plusDI) <= 100);
    assert.ok(last(adx.adx) > 25, 'a clean trend should read as a trend');
  });
});

test('indicators are computed on closed bars only', async () => {
  const { rt } = runtimeWithBars(260);
  // Append a forming bar with an absurd close. It must not affect any indicator.
  const before = rt.indicators('M15', ['rsi', 'ema20']).rsi.slice(-1)[0];
  rt.bars.get('M15').push({
    openTime: new Date(Date.UTC(2026, 0, 2)).toISOString(),
    open: 99999, high: 99999, low: 99999, close: 99999, tickVolume: 1, isOpen: true,
  });
  const after = rt.indicators('M15', ['rsi', 'ema20']).rsi.slice(-1)[0];
  assert.equal(before, after, 'a forming bar must not change closed-bar indicators');
});

test('an unknown timeframe falls back to the default rather than erroring', async () => {
  const { rt } = runtimeWithBars();
  await withServer(rt, async (base) => {
    const res = await fetch(`${base}/api/indicators?timeframe=NOPE`);
    assert.equal(res.status, 200);
    const body = await res.json();
    assert.equal(body.timeframe, 'M15');
  });
});
