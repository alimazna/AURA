#!/usr/bin/env node
// BiQuote integration discovery + verification.
//
// The brief requires the agent to *discover and test* the real access path rather
// than assume an API or protocol. This tool performs that discovery against the
// live service and writes the observed evidence to docs/BIQUOTE_DISCOVERY.md.
//
// Run: node src/biquote/discover.mjs [symbol]

import { writeFileSync, mkdirSync } from 'node:fs';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import * as bq from './client.mjs';
import { TIMEFRAMES, SYMBOL } from '../config.mjs';

const here = dirname(fileURLToPath(import.meta.url));
const outPath = resolve(here, '../../docs/BIQUOTE_DISCOVERY.md');
const symbol = process.argv[2] || SYMBOL;

const lines = [];
const rec = (s = '') => { lines.push(s); console.log(s); };
const probe = async (label, fn) => {
  try {
    const value = await fn();
    rec(`- \`${label}\` — OK`);
    return { ok: true, value };
  } catch (err) {
    rec(`- \`${label}\` — FAILED: ${err.message}`);
    return { ok: false, error: err.message };
  }
};

rec(`# BiQuote integration discovery report`);
rec();
rec(`Generated: ${new Date().toISOString()}`);
rec(`Symbol under test: \`${symbol}\``);
rec();
rec('This report is produced by `node src/biquote/discover.mjs`, which exercises the live');
rec('service. It records what was actually observed, including failures.');
rec();
rec('## 1. Endpoint probes');
rec();

const q = await probe(`GET /api/${symbol}`, () => bq.quote(symbol));
const sym = await probe('GET /api/symbols', async () => (await bq.symbols()).length);
const lat = await probe(`GET /api/latest?symbols=${symbol}`, async () => (await bq.quotes([symbol]))[symbol]?.mid);

rec();
rec('## 2. OHLC interval coverage (closed-bar flag observed)');
rec();
rec('| Timeframe | interval | bars | newest bar | isOpen | first bar | last bar |');
rec('|---|---|---|---|---|---|---|');
for (const tf of TIMEFRAMES) {
  try {
    const r = await bq.ohlc(symbol, tf.stream, 400);
    const newest = r.bars[r.bars.length - 1];
    rec(`| ${tf.id} | \`${tf.stream}\` | ${r.bars.length} | ${newest?.openTime ?? 'n/a'} | ${newest ? newest.isOpen : 'n/a'} | ${r.bars[0]?.openTime ?? 'n/a'} | ${newest?.openTime ?? 'n/a'} |`);
  } catch (err) {
    rec(`| ${tf.id} | \`${tf.stream}\` | NOT AVAILABLE | — | — | — | — |`);
    rec(`<!-- ${tf.id}: ${err.message} -->`);
  }
}

rec();
rec('## 3. Tick stream (SignalR hub)');
rec();
const hubStates = [];
let ticks = 0;
let tickSample = null;
try {
  const stream = await bq.streamTicks([symbol], (t) => { ticks += 1; tickSample = tickSample || t; }, {
    onState: (s) => { hubStates.push(s); rec(`- hub state: \`${s.state}\`${s.reason ? ` (${s.reason})` : ''}${s.transports ? ` transports=${s.transports.join(',')}` : ''}`); },
  });
  await new Promise((r) => setTimeout(r, 12000));
  stream.close();
  rec(`- ticks received in 12s: ${ticks}`);
} catch (err) {
  rec(`- tick stream NOT AVAILABLE: ${err.message}`);
}

rec();
rec('## 4. Observed quote payload');
rec();
if (q.ok) {
  rec('```json');
  rec(JSON.stringify(q.value, null, 2));
  rec('```');
} else {
  rec('Quote unavailable at discovery time.');
}

rec();
rec('## 5. Verdict');
rec();
rec(`- REST quotes: ${q.ok ? 'VERIFIED' : 'NOT AVAILABLE'}`);
rec(`- Symbol catalogue: ${sym.ok ? `VERIFIED (${sym.value} symbols)` : 'NOT AVAILABLE'}`);
rec(`- Batched latest: ${lat.ok ? 'VERIFIED' : 'NOT AVAILABLE'}`);
rec(`- OHLC per timeframe: see table above (failures are marked NOT AVAILABLE, never filled with synthetic data)`);
rec(`- Tick stream: ${ticks > 0 ? `VERIFIED (${ticks} ticks observed)` : 'NOT VERIFIED in this run'}`);
rec();
rec('No API key was required for any of the above. No undocumented behaviour is assumed by the AYZR runtime.');

mkdirSync(dirname(outPath), { recursive: true });
writeFileSync(outPath, lines.join('\n') + '\n');
console.log(`\nWrote ${outPath}`);
