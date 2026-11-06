// API client. Read-only calls are public; operator calls carry the operator token.

const TOKEN_KEY = 'ayzr.operatorToken';

export function getOperatorToken() {
  try { return sessionStorage.getItem(TOKEN_KEY) || ''; } catch { return ''; }
}
export function setOperatorToken(t) {
  try { t ? sessionStorage.setItem(TOKEN_KEY, t) : sessionStorage.removeItem(TOKEN_KEY); } catch { /* storage blocked */ }
}
export function isOperator() {
  return getOperatorToken().length > 0;
}

async function get(path) {
  const res = await fetch(path, { headers: { accept: 'application/json' } });
  const body = await res.json().catch(() => ({}));
  if (!res.ok) throw new Error(body.error || `request failed (${res.status})`);
  return body;
}

async function post(path, payload) {
  const res = await fetch(path, {
    method: 'POST',
    headers: { 'content-type': 'application/json', 'x-operator-token': getOperatorToken() },
    body: JSON.stringify(payload),
  });
  const body = await res.json().catch(() => ({}));
  return { ok: res.ok, status: res.status, body };
}

export const api = {
  health: () => get('/api/health'),
  snapshot: (timeframe) => get(`/api/snapshot?timeframe=${encodeURIComponent(timeframe)}`),
  timeframes: () => get('/api/timeframes'),
  engines: () => get('/api/engines'),
  regions: (timeframe) => get(`/api/regions${timeframe ? `?timeframe=${timeframe}` : ''}`),
  region: (id) => get(`/api/regions?id=${encodeURIComponent(id)}`),
  signals: (timeframe) => get(`/api/signals${timeframe ? `?timeframe=${timeframe}` : ''}`),
  virtualTrades: (timeframe) => get(`/api/virtual-trades${timeframe ? `?timeframe=${timeframe}` : ''}`),
  performance: (timeframe) => get(`/api/performance${timeframe ? `?timeframe=${timeframe}` : ''}`),
  audit: (limit = 100) => get(`/api/audit?limit=${limit}`),
  aiStatus: () => get('/api/ai/status'),
  aiRequests: () => get('/api/ai/requests'),

  analyze: (prompt, timeframe) => post('/api/ai/analyze', { prompt, timeframe }),
  decide: (signalId, decision) => post('/api/signals/decision', { signalId, decision }),
  control: (action) => post('/api/control', { action }),
};
