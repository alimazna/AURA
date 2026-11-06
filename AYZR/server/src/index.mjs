#!/usr/bin/env node
// AYZR entry point.

import { fileURLToPath } from 'node:url';
import { dirname, resolve } from 'node:path';
import { Runtime } from './runtime.mjs';
import { Store } from './store/db.mjs';
import { createApi } from './api/server.mjs';
import { RUNTIME, SYMBOL, DEFAULT_TIMEFRAME } from './config.mjs';

const here = dirname(fileURLToPath(import.meta.url));
const webRoot = resolve(here, '../../web/dist');
const dbPath = resolve(here, '..', RUNTIME.dbPath);

const store = new Store(dbPath);
const runtime = new Runtime(store);
runtime.start();

const operatorToken = process.env.AYZR_OPERATOR_TOKEN || '';
const server = createApi({ runtime, store, webRoot, operatorToken });

server.listen(RUNTIME.port, RUNTIME.host, () => {
  const banner = [
    '',
    '  AYZR — XAUUSD Gold Intelligence',
    `  mode            SHADOW ONLY (no live trading)`,
    `  symbol          ${SYMBOL}   default timeframe ${DEFAULT_TIMEFRAME}`,
    `  data source     BiQuote REST + SignalR tick hub`,
    `  listening       http://${RUNTIME.host}:${RUNTIME.port}`,
    `  operator mode   ${operatorToken ? 'ENABLED (token required for AI + controls)' : 'DISABLED (set AYZR_OPERATOR_TOKEN to enable)'}`,
    `  ai provider     ${RUNTIME.ai.provider}${RUNTIME.ai.provider === 'none' ? ' (AI returns NOT AVAILABLE until configured)' : ''}`,
    `  database        ${dbPath}`,
    '',
  ].join('\n');
  console.log(banner);
});

const shutdown = () => {
  console.log('\nAYZR shutting down: persisting state...');
  try { store.checkpoint(); } catch { /* best effort */ }
  runtime.stop();
  store.close();
  server.close(() => process.exit(0));
  setTimeout(() => process.exit(0), 3000);
};
process.on('SIGINT', shutdown);
process.on('SIGTERM', shutdown);
