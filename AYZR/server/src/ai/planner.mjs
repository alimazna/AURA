// AI analysis planner.
//
// Hard rules from the brief (§1, §4, §6, §9):
//   * The AI runs only when the operator asks. There is no timer, no scheduled analysis.
//   * Visitors cannot reach it. Requests must carry the operator token.
//   * The AI never publishes free text as a plan. Output is a structured object that is
//     validated field-by-field before it is stored or drawn.
//   * If no provider is configured the request is still recorded and answered with an
//     explicit NOT AVAILABLE verdict. Nothing is fabricated to look functional.

import { stableId, isoNow } from '../engines/primitives.mjs';

const REQUIRED_PLAN_FIELDS = ['direction', 'scenarios', 'invalidationConditions', 'timeHorizon'];

export function providerStatus(config) {
  if (!config.provider || config.provider === 'none') {
    return { available: false, reason: 'No AI provider is configured (set AYZR_AI_PROVIDER, AYZR_AI_MODEL, AYZR_AI_API_KEY).' };
  }
  if (!config.apiKey) {
    return { available: false, reason: `Provider "${config.provider}" is selected but AYZR_AI_API_KEY is empty.` };
  }
  return { available: true, provider: config.provider, model: config.model };
}

/**
 * Validate a model-produced plan against the structured contract. Anything missing or
 * of the wrong shape is rejected rather than partially accepted.
 */
export function validatePlan(raw) {
  const errors = [];
  if (!raw || typeof raw !== 'object') return { ok: false, errors: ['plan is not an object'] };
  for (const f of REQUIRED_PLAN_FIELDS) {
    if (raw[f] === undefined || raw[f] === null) errors.push(`missing field: ${f}`);
  }
  if (raw.direction && !['LONG', 'SHORT', 'NEUTRAL', 'WAIT'].includes(raw.direction)) {
    errors.push(`direction must be one of LONG, SHORT, NEUTRAL, WAIT (got ${raw.direction})`);
  }
  if (raw.scenarios && !Array.isArray(raw.scenarios)) errors.push('scenarios must be an array');
  if (Array.isArray(raw.scenarios)) {
    raw.scenarios.forEach((s, i) => {
      if (!s || typeof s !== 'object') { errors.push(`scenario ${i} is not an object`); return; }
      for (const f of ['name', 'condition', 'expectation']) {
        if (!s[f]) errors.push(`scenario ${i} missing ${f}`);
      }
    });
  }
  if (raw.invalidationConditions && !Array.isArray(raw.invalidationConditions)) errors.push('invalidationConditions must be an array');
  if (raw.timeHorizon && typeof raw.timeHorizon !== 'string') errors.push('timeHorizon must be a string');
  // Reject anything that looks like an uncalibrated probability presented as certainty.
  const blob = JSON.stringify(raw).toLowerCase();
  for (const banned of ['guaranteed', '100%', 'risk-free', 'certain profit']) {
    if (blob.includes(banned)) errors.push(`plan contains a forbidden certainty claim: "${banned}"`);
  }
  return { ok: errors.length === 0, errors };
}

/** The evidence bundle handed to the model. Only real, sourced values. */
export function buildContext({ symbol, timeframe, quote, bars, analysis, regions }) {
  const closed = (bars || []).filter((b) => !b.isOpen).slice(-120);
  return {
    symbol,
    timeframe,
    capturedAt: isoNow(),
    quote: quote ? {
      bid: quote.bid, ask: quote.ask, mid: quote.mid, spread: quote.spread,
      source: quote.source, timestamp: quote.timestamp, marketState: quote.marketState, stale: quote.stale,
    } : null,
    closedBars: closed.map((b) => ({ t: b.openTime, o: b.open, h: b.high, l: b.low, c: b.close })),
    engineSummary: (analysis?.engines || []).map((e) => ({
      engine: e.engine,
      emitted: e.emitted.map((x) => ({ type: x.type, zone: x.zone, direction: x.direction, reason: x.reason })),
      withheld: e.withheld,
    })),
    activeRegions: (regions || []).slice(0, 20).map((r) => ({
      engine: r.engine, type: r.type, state: r.state, zone: r.zone, direction: r.direction, reason: r.reason,
    })),
    constraints: [
      'No real trading. Shadow analysis only.',
      'Every price you cite must come from the supplied closed bars.',
      'Do not state probabilities; rank conviction qualitatively.',
      'Return only the structured JSON plan described in the system prompt.',
    ],
  };
}

export const SYSTEM_PROMPT = `You are AYZR's analysis planner for XAUUSD.
You receive only real, sourced market data supplied in the user message.
Produce a structured plan. Never invent prices. Never claim certainty or probabilities.
Return strict JSON with this shape:
{
  "direction": "LONG" | "SHORT" | "NEUTRAL" | "WAIT",
  "scenarios": [{ "name": string, "condition": string, "expectation": string, "supportingEvidence": string[] }],
  "invalidationConditions": string[],
  "timeHorizon": string,
  "keyLevels": [{ "label": string, "price": number, "source": string }],
  "risks": string[]
}
Cite the bar timestamps you used in supportingEvidence.`;

/**
 * Ask the configured provider for a plan. Returns a verdict object; never throws on
 * provider failure, so the API can report the failure truthfully.
 */
export async function requestPlan({ config, context, fetchImpl = fetch }) {
  const status = providerStatus(config);
  if (!status.available) {
    return { verdict: 'NOT_AVAILABLE', reason: status.reason, plan: null, provider: config.provider || 'none', model: config.model || '' };
  }

  const base = config.baseUrl || defaultBaseUrl(config.provider);
  if (!base) {
    return { verdict: 'NOT_AVAILABLE', reason: `No base URL known for provider "${config.provider}".`, plan: null, provider: config.provider, model: config.model };
  }

  const ac = new AbortController();
  const timer = setTimeout(() => ac.abort(), 60000);
  try {
    const res = await fetchImpl(`${base.replace(/\/$/, '')}/chat/completions`, {
      method: 'POST',
      signal: ac.signal,
      headers: { 'content-type': 'application/json', authorization: `Bearer ${config.apiKey}` },
      body: JSON.stringify({
        model: config.model,
        temperature: 0.2,
        response_format: { type: 'json_object' },
        messages: [
          { role: 'system', content: SYSTEM_PROMPT },
          { role: 'user', content: JSON.stringify(context) },
        ],
      }),
    });
    if (!res.ok) {
      return { verdict: 'PROVIDER_ERROR', reason: `provider responded ${res.status}`, plan: null, provider: config.provider, model: config.model };
    }
    const body = await res.json();
    const text = body?.choices?.[0]?.message?.content;
    if (!text) return { verdict: 'PROVIDER_ERROR', reason: 'provider returned no content', plan: null, provider: config.provider, model: config.model };

    let parsed;
    try { parsed = JSON.parse(text); } catch { return { verdict: 'REJECTED', reason: 'provider output was not valid JSON', plan: null, provider: config.provider, model: config.model }; }

    const check = validatePlan(parsed);
    if (!check.ok) return { verdict: 'REJECTED', reason: check.errors.join('; '), plan: null, provider: config.provider, model: config.model };

    return { verdict: 'ACCEPTED', reason: 'plan validated against the structured contract', plan: parsed, provider: config.provider, model: config.model };
  } catch (err) {
    return { verdict: 'PROVIDER_ERROR', reason: err.message, plan: null, provider: config.provider, model: config.model };
  } finally {
    clearTimeout(timer);
  }
}

function defaultBaseUrl(provider) {
  switch (provider) {
    case 'openai': return 'https://api.openai.com/v1';
    case 'openrouter': return 'https://openrouter.ai/api/v1';
    case 'deepseek': return 'https://api.deepseek.com/v1';
    case 'openai-compatible': return process.env.AYZR_AI_BASE_URL || '';
    default: return process.env.AYZR_AI_BASE_URL || '';
  }
}

export function newRequestId(prompt, operator) {
  return stableId(['ai', operator, prompt, Date.now()]);
}
