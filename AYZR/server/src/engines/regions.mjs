// Region lifecycle.
//
// A region is a zone the engines discovered, tracked through the canonical states from
// the brief (§3). Transitions are driven by closed-bar price action only, and every
// transition is appended to the audit ledger with source, time and reason.

export const REGION_STATES = [
  'DISCOVERED',
  'WAITING_APPROACH',
  'APPROACH',
  'UNDER_VALIDATION',
  'CONFIRMED_RETEST',
  'CONFIRMED_BREAKOUT',
  'FAKE_BREAKOUT',
  'EXPIRED',
];

export const TERMINAL_STATES = new Set(['CONFIRMED_RETEST', 'CONFIRMED_BREAKOUT', 'FAKE_BREAKOUT', 'EXPIRED']);

// States that represent a price reaction which can be evaluated as a signal.
export const ACTIONABLE_STATES = new Set(['CONFIRMED_RETEST', 'CONFIRMED_BREAKOUT', 'FAKE_BREAKOUT']);

const ENGINE_TO_KIND = {
  liquidity: 'liquidity',
  levels: 'level',
  supplyDemand: 'supplyDemand',
  structure: 'structure',
  imbalance: 'imbalance',
  breakout: 'breakout',
};

export function kindFor(engine) {
  return ENGINE_TO_KIND[engine] || engine;
}

/**
 * Advance one region by one closed bar.
 * Returns { state, transitioned, reason } — never mutates history silently.
 */
export function advanceRegion(region, bar, atr) {
  if (TERMINAL_STATES.has(region.state)) return { state: region.state, transitioned: false, reason: 'terminal' };

  const { low, high } = region.zone;
  const tol = Math.max(atr * 0.25, 0.3);
  const touched = bar.high >= low - tol && bar.low <= high + tol;
  const inside = bar.close >= low && bar.close <= high;
  const beyondHigh = bar.close > high + tol;
  const beyondLow = bar.close < low - tol;

  switch (region.state) {
    case 'DISCOVERED':
      if (touched) return { state: 'APPROACH', transitioned: true, reason: 'price entered the zone tolerance band' };
      return { state: 'WAITING_APPROACH', transitioned: true, reason: 'region published, price has not arrived' };
    case 'WAITING_APPROACH':
      if (touched) return { state: 'APPROACH', transitioned: true, reason: 'price arrived within the zone tolerance band' };
      return { state: region.state, transitioned: false, reason: 'still waiting for price' };
    case 'APPROACH':
      if (inside) return { state: 'UNDER_VALIDATION', transitioned: true, reason: 'price is trading inside the zone' };
      if (beyondHigh || beyondLow) return { state: 'UNDER_VALIDATION', transitioned: true, reason: 'price pushed through the zone' };
      return { state: region.state, transitioned: false, reason: 'price still approaching' };
    case 'UNDER_VALIDATION': {
      // A reaction is confirmed only by a close back inside/away from the zone.
      const bullishReaction = bar.close > bar.open && bar.close > region.zone.low && region.direction === 'LONG';
      const bearishReaction = bar.close < bar.open && bar.close < region.zone.high && region.direction === 'SHORT';
      if (bullishReaction || bearishReaction) {
        const type = region.type || '';
        if (type.includes('FAKE')) return { state: 'FAKE_BREAKOUT', transitioned: true, reason: 'rejection close confirmed after the sweep' };
        if (type.includes('BREAKOUT')) return { state: 'CONFIRMED_BREAKOUT', transitioned: true, reason: 'close confirmed beyond the level' };
        return { state: 'CONFIRMED_RETEST', transitioned: true, reason: 'reaction close confirmed inside the zone' };
      }
      if (region.touches >= 8) return { state: 'EXPIRED', transitioned: true, reason: 'zone consumed without a confirming reaction' };
      return { state: region.state, transitioned: false, reason: 'awaiting a confirming close' };
    }
    default:
      return { state: region.state, transitioned: false, reason: 'no rule for state' };
  }
}

/** Regions expire when they age out, regardless of state. */
export function expireStale(region, nowMs, maxAgeMs) {
  if (TERMINAL_STATES.has(region.state)) return null;
  if (nowMs - region.discoveredAtMs <= maxAgeMs) return null;
  return { state: 'EXPIRED', transitioned: true, reason: `region older than ${Math.round(maxAgeMs / 3600000)}h without resolution` };
}

export function regionKey(ev) {
  return `${ev.engine}:${ev.type}:${ev.zone ? ev.zone.low.toFixed(2) : ''}:${ev.zone ? ev.zone.high.toFixed(2) : ''}`;
}
