// AYZR engine suite.
//
// Each engine is a pure function over closed bars plus shared context. Engines never
// look ahead, never invent data, and always record *why* they did or did not emit.
// A missing input yields an explicit `insufficient`/`unknown` result, not a guess.

import { atrSeries, lastDefined, mean, round, stableId, swings, isoFromMs } from './primitives.mjs';

export const ENGINE_IDS = [
  'liquidity',
  'levels',
  'supplyDemand',
  'structure',
  'imbalance',
  'breakout',
];

export const ENGINE_SPECS = {
  liquidity: {
    name: 'Liquidity',
    covers: 'Resting liquidity above equal highs / below equal lows, session extremes, and sweep events.',
    inputs: ['closed bars', 'ATR', 'swing points'],
    outputs: ['liquidity pools', 'sweep evidence'],
    uncertainty: ['Thin sessions can leave pools untested for long periods', 'Pools are not guarantees of a reaction'],
    noEmit: ['fewer than 40 closed bars', 'no qualifying swing clusters'],
  },
  levels: {
    name: 'Support & Resistance',
    covers: 'Horizontal levels built from clustered swing highs/lows with touch counts.',
    inputs: ['closed bars', 'swing points', 'ATR'],
    outputs: ['support levels', 'resistance levels', 'touch counts'],
    uncertainty: ['Levels widen as volatility expands', 'Round-number magnetism is not modelled'],
    noEmit: ['fewer than 3 touches within tolerance', 'insufficient swing history'],
  },
  supplyDemand: {
    name: 'Supply & Demand',
    covers: 'Base-then-impulse zones, distinguishing demand (base low) from supply (base high).',
    inputs: ['closed bars', 'ATR', 'body/range ratio'],
    outputs: ['demand zones', 'supply zones'],
    uncertainty: ['A zone is a probability band, not a precise line', 'Zones may be mitigated in one candle'],
    noEmit: ['no impulse leg of at least 1.5x ATR', 'base longer than the impulse leg'],
  },
  structure: {
    name: 'Market Structure',
    covers: 'Swing sequence classification, break of structure (BOS) and change of character (CHoCH).',
    inputs: ['closed bars', 'swing points'],
    outputs: ['trend label', 'BOS events', 'CHoCH events'],
    uncertainty: ['Range conditions produce conflicting sequences', 'Confirmation requires the closing bar'],
    noEmit: ['fewer than 4 confirmed swings'],
  },
  imbalance: {
    name: 'Imbalance (FVG & Order Blocks)',
    covers: 'Three-bar fair value gaps and order blocks derived from the impulse leg that created a BOS.',
    inputs: ['closed bars', 'BOS events', 'ATR'],
    outputs: ['fair value gaps', 'order blocks'],
    uncertainty: ['Gaps may be partially filled', 'Order-block validity depends on the originating impulse'],
    noEmit: ['no BOS', 'gap smaller than 0.1x ATR'],
  },
  breakout: {
    name: 'Breakout & Fake Breakout',
    covers: 'Breakout confirmation by close beyond a level, and fake-breakout (failed retest) detection.',
    inputs: ['closed bars', 'levels', 'ATR'],
    outputs: ['confirmed breakouts', 'fake breakout evidence'],
    uncertainty: ['A close beyond a level can still be reclaimed next bar', 'Low-volume breaks are weaker evidence'],
    noEmit: ['no level within 0.5x ATR', 'break occurred on the newest bar only'],
  },
};

function buildContext(bars, atrPeriod = 14, swingK = 2) {
  const atr = atrSeries(bars, atrPeriod);
  const currentAtr = lastDefined(atr) ?? 0;
  const sw = swings(bars, swingK);
  return { atr, atrValue: currentAtr, swings: sw, lastIndex: bars.length - 1, last: bars[bars.length - 1] };
}

function evidence({ engine, type, timeframe, zone, direction, score, conditions, reason, bars, atIndex, sourceTime }) {
  const id = stableId([engine, type, timeframe, zone ? zone.low.toFixed(2) : '', zone ? zone.high.toFixed(2) : '', sourceTime || '']);
  return {
    id,
    engine,
    type,
    timeframe,
    direction,
    zone: zone ? { low: round(zone.low), high: round(zone.high) } : null,
    // `score` is an internal, uncalibrated ranking value in [0,1]. It is NOT a probability.
    score: round(score, 3),
    conditions,
    reason,
    sourceBarIndex: atIndex,
    sourceBarTime: sourceTime || bars[atIndex]?.openTime || null,
  };
}

/* ------------------------------------------------------------------ liquidity */

export function runLiquidity(bars, tfId, ctx) {
  const notes = { engine: 'liquidity', timeframe: tfId, emitted: [], withheld: [] };
  if (bars.length < 40) {
    notes.withheld.push('fewer than 40 closed bars');
    return notes;
  }
  const tolerance = Math.max(ctx.atrValue * 0.25, 0.5);
  const cluster = (points, kind) => {
    const out = [];
    for (const p of points) {
      const hit = out.find((c) => Math.abs(c.price - p.price) <= tolerance);
      if (hit) {
        hit.touches += 1;
        hit.lastTouch = p.time;
        hit.confirmedAt = p.confirmedAt;
        hit.confirmedTime = p.confirmedTime;
        hit.price = (hit.price * (hit.touches - 1) + p.price) / hit.touches;
      } else {
        out.push({ kind, price: p.price, touches: 1, firstTouch: p.time, lastTouch: p.time, confirmedAt: p.confirmedAt, confirmedTime: p.confirmedTime });
      }
    }
    return out.filter((c) => c.touches >= 2);
  };

  const pools = [...cluster(ctx.swings.highs, 'buyside'), ...cluster(ctx.swings.lows, 'sellside')];
  for (const pool of pools) {
    const pad = Math.max(ctx.atrValue * 0.15, 0.3);
    notes.emitted.push(evidence({
      engine: 'liquidity',
      type: pool.kind === 'buyside' ? 'LIQUIDITY_POOL_ABOVE' : 'LIQUIDITY_POOL_BELOW',
      timeframe: tfId,
      zone: { low: pool.price - pad, high: pool.price + pad },
      direction: pool.kind === 'buyside' ? 'SHORT' : 'LONG',
      score: Math.min(0.4 + pool.touches * 0.12, 0.85),
      conditions: [`${pool.touches} swings within ${round(tolerance, 2)} of ${round(pool.price, 2)}`],
      reason: `Equal ${pool.kind === 'buyside' ? 'highs' : 'lows'} form a resting liquidity pool.`,
      bars,
      atIndex: ctx.lastIndex,
      sourceTime: pool.confirmedTime,
    }));
  }

  // Sweep: wick beyond a pool then close back inside, on the most recent closed bars.
  for (let i = Math.max(1, bars.length - 12); i <= ctx.lastIndex; i++) {
    const b = bars[i];
    for (const pool of pools) {
      const sweptHigh = pool.kind === 'buyside' && b.high > pool.price && b.close < pool.price;
      const sweptLow = pool.kind === 'sellside' && b.low < pool.price && b.close > pool.price;
      if (sweptHigh || sweptLow) {
        const pad = Math.max(ctx.atrValue * 0.15, 0.3);
        notes.emitted.push(evidence({
          engine: 'liquidity',
          type: 'LIQUIDITY_SWEEP',
          timeframe: tfId,
          zone: { low: (sweptHigh ? pool.price : b.low) - pad, high: (sweptHigh ? b.high : pool.price) + pad },
          direction: sweptHigh ? 'SHORT' : 'LONG',
          score: 0.7,
          conditions: [`wick beyond ${round(pool.price, 2)}`, 'close back inside the pool'],
          reason: sweptHigh ? 'Buy-side liquidity swept then rejected.' : 'Sell-side liquidity swept then reclaimed.',
          bars,
          atIndex: i,
        }));
      }
    }
  }

  if (!notes.emitted.length) notes.withheld.push('no qualifying swing clusters or sweeps');
  return notes;
}

/* --------------------------------------------------------------------- levels */

export function runLevels(bars, tfId, ctx) {
  const notes = { engine: 'levels', timeframe: tfId, emitted: [], withheld: [] };
  if (ctx.swings.highs.length + ctx.swings.lows.length < 4) {
    notes.withheld.push('insufficient swing history');
    return notes;
  }
  const tolerance = Math.max(ctx.atrValue * 0.35, 0.75);
  const all = [...ctx.swings.highs.map((s) => ({ ...s, side: 'resistance' })), ...ctx.swings.lows.map((s) => ({ ...s, side: 'support' }))];
  const clusters = [];
  for (const p of all.sort((a, b) => a.price - b.price)) {
    const hit = clusters.find((c) => Math.abs(c.price - p.price) <= tolerance && c.side === p.side);
    if (hit) {
      hit.touches += 1;
      hit.price = (hit.price * (hit.touches - 1) + p.price) / hit.touches;
      hit.lastTouch = p.time;
      hit.confirmedTime = p.confirmedTime;
    } else {
      clusters.push({ price: p.price, side: p.side, touches: 1, lastTouch: p.time, confirmedTime: p.confirmedTime });
    }
  }
  for (const c of clusters) {
    if (c.touches < 3) continue;
    const pad = Math.max(ctx.atrValue * 0.2, 0.4);
    notes.emitted.push(evidence({
      engine: 'levels',
      type: c.side === 'support' ? 'SUPPORT_LEVEL' : 'RESISTANCE_LEVEL',
      timeframe: tfId,
      zone: { low: c.price - pad, high: c.price + pad },
      direction: c.side === 'support' ? 'LONG' : 'SHORT',
      score: Math.min(0.45 + c.touches * 0.1, 0.9),
      conditions: [`${c.touches} swing touches within ${round(tolerance, 2)}`],
      reason: `${c.side === 'support' ? 'Support' : 'Resistance'} level with ${c.touches} reactions.`,
      bars,
      atIndex: ctx.lastIndex,
      sourceTime: c.confirmedTime,
    }));
  }
  if (!notes.emitted.length) notes.withheld.push('fewer than 3 touches within tolerance');
  return notes;
}

/* -------------------------------------------------------------- supply/demand */

export function runSupplyDemand(bars, tfId, ctx) {
  const notes = { engine: 'supplyDemand', timeframe: tfId, emitted: [], withheld: [] };
  if (bars.length < 30 || !ctx.atrValue) {
    notes.withheld.push('insufficient closed bars or no ATR');
    return notes;
  }
  const minImpulse = ctx.atrValue * 1.5;
  let found = 0;
  for (let i = 3; i < bars.length - 1; i++) {
    const impulse = bars[i];
    const leg = Math.abs(impulse.close - impulse.open);
    if (leg < minImpulse) continue;
    // The base is the consolidation immediately before the impulse.
    const baseStart = Math.max(0, i - 4);
    const base = bars.slice(baseStart, i);
    if (!base.length) continue;
    const baseRange = Math.max(...base.map((b) => b.high)) - Math.min(...base.map((b) => b.low));
    if (baseRange > leg) continue; // base must be tighter than the impulse leg
    const bull = impulse.close > impulse.open;
    const low = Math.min(...base.map((b) => b.low));
    const high = Math.max(...base.map((b) => b.high));
    found += 1;
    notes.emitted.push(evidence({
      engine: 'supplyDemand',
      type: bull ? 'DEMAND_ZONE' : 'SUPPLY_ZONE',
      timeframe: tfId,
      zone: { low, high },
      direction: bull ? 'LONG' : 'SHORT',
      score: Math.min(0.5 + leg / (ctx.atrValue * 6), 0.9),
      conditions: [`impulse ${round(leg, 2)} >= 1.5x ATR ${round(ctx.atrValue, 2)}`, `base range ${round(baseRange, 2)} tighter than impulse`],
      reason: bull ? 'Demand base followed by an impulsive advance.' : 'Supply base followed by an impulsive decline.',
      bars,
      atIndex: i,
    }));
  }
  if (!found) notes.withheld.push('no impulse leg of at least 1.5x ATR with a tighter base');
  return notes;
}

/* ------------------------------------------------------------------ structure */

export function runStructure(bars, tfId, ctx) {
  const notes = { engine: 'structure', timeframe: tfId, emitted: [], withheld: [] };
  const { highs, lows } = ctx.swings;
  if (highs.length + lows.length < 4) {
    notes.withheld.push('fewer than 4 confirmed swings');
    return notes;
  }
  const seq = [...highs.map((h) => ({ ...h, kind: 'H' })), ...lows.map((l) => ({ ...l, kind: 'L' }))]
    .sort((a, b) => a.confirmedAt - b.confirmedAt);
  let trend = 'RANGE';
  const events = [];
  let prevHigh = null;
  let prevLow = null;
  for (const p of seq) {
    if (p.kind === 'H') {
      if (prevHigh) {
        if (p.price > prevHigh.price) {
          if (trend === 'DOWN') events.push({ type: 'CHOCH', direction: 'LONG', at: p, level: prevHigh.price });
          else events.push({ type: 'BOS', direction: 'LONG', at: p, level: prevHigh.price });
          trend = 'UP';
        } else if (trend === 'UP') {
          events.push({ type: 'CHOCH', direction: 'SHORT', at: p, level: prevHigh.price });
          trend = 'DOWN';
        }
      }
      prevHigh = p;
    } else {
      if (prevLow) {
        if (p.price < prevLow.price) {
          if (trend === 'UP') events.push({ type: 'CHOCH', direction: 'SHORT', at: p, level: prevLow.price });
          else events.push({ type: 'BOS', direction: 'SHORT', at: p, level: prevLow.price });
          trend = 'DOWN';
        } else if (trend === 'DOWN') {
          events.push({ type: 'CHOCH', direction: 'LONG', at: p, level: prevLow.price });
          trend = 'UP';
        }
      }
      prevLow = p;
    }
  }

  for (const e of events.slice(-6)) {
    const pad = Math.max(ctx.atrValue * 0.2, 0.4);
    notes.emitted.push(evidence({
      engine: 'structure',
      type: e.type === 'BOS' ? 'BREAK_OF_STRUCTURE' : 'CHANGE_OF_CHARACTER',
      timeframe: tfId,
      zone: { low: e.level - pad, high: e.level + pad },
      direction: e.direction,
      score: e.type === 'BOS' ? 0.75 : 0.65,
      conditions: [`swing ${e.at.kind === 'H' ? 'high' : 'low'} ${round(e.at.price, 2)} vs prior structure ${round(e.level, 2)}`],
      reason: e.type === 'BOS'
        ? `Structure continued ${e.direction === 'LONG' ? 'higher' : 'lower'} beyond ${round(e.level, 2)}.`
        : `Character changed to ${e.direction === 'LONG' ? 'bullish' : 'bearish'} at ${round(e.level, 2)}.`,
      bars,
      atIndex: e.at.confirmedAt,
      sourceTime: e.at.confirmedTime,
    }));
  }
  notes.trend = trend;
  if (!events.length) notes.withheld.push('swing sequence shows no structural break');
  return notes;
}

/* ------------------------------------------------------------------ imbalance */

export function runImbalance(bars, tfId, ctx, structure) {
  const notes = { engine: 'imbalance', timeframe: tfId, emitted: [], withheld: [] };
  if (bars.length < 10) {
    notes.withheld.push('fewer than 10 closed bars');
    return notes;
  }
  const minGap = Math.max(ctx.atrValue * 0.1, 0.15);
  let gaps = 0;
  for (let i = 2; i < bars.length; i++) {
    const a = bars[i - 2];
    const c = bars[i];
    if (c.low > a.high && c.low - a.high >= minGap) {
      gaps += 1;
      notes.emitted.push(evidence({
        engine: 'imbalance',
        type: 'FAIR_VALUE_GAP_BULLISH',
        timeframe: tfId,
        zone: { low: a.high, high: c.low },
        direction: 'LONG',
        score: 0.6,
        conditions: [`gap ${round(c.low - a.high, 2)} >= 0.1x ATR`],
        reason: 'Three-bar bullish imbalance left unfilled.',
        bars,
        atIndex: i,
      }));
    } else if (a.low > c.high && a.low - c.high >= minGap) {
      gaps += 1;
      notes.emitted.push(evidence({
        engine: 'imbalance',
        type: 'FAIR_VALUE_GAP_BEARISH',
        timeframe: tfId,
        zone: { low: c.high, high: a.low },
        direction: 'SHORT',
        score: 0.6,
        conditions: [`gap ${round(a.low - c.high, 2)} >= 0.1x ATR`],
        reason: 'Three-bar bearish imbalance left unfilled.',
        bars,
        atIndex: i,
      }));
    }
  }

  // Order blocks: the last opposing candle before a confirmed structural break.
  let obs = 0;
  for (const ev of (structure?.emitted || []).filter((e) => e.type === 'BREAK_OF_STRUCTURE')) {
    const idx = ev.sourceBarIndex;
    if (!Number.isFinite(idx)) continue;
    for (let j = idx - 1; j >= Math.max(0, idx - 5); j--) {
      const b = bars[j];
      const opposing = ev.direction === 'LONG' ? b.close < b.open : b.close > b.open;
      if (!opposing) continue;
      obs += 1;
      notes.emitted.push(evidence({
        engine: 'imbalance',
        type: ev.direction === 'LONG' ? 'BULLISH_ORDER_BLOCK' : 'BEARISH_ORDER_BLOCK',
        timeframe: tfId,
        zone: { low: b.low, high: b.high },
        direction: ev.direction,
        score: 0.7,
        conditions: [`last opposing candle before break at ${ev.sourceBarTime}`],
        reason: 'Order block from the impulse that broke structure.',
        bars,
        atIndex: j,
      }));
      break;
    }
  }

  if (!gaps && !obs) notes.withheld.push('no gap >= 0.1x ATR and no BOS-linked order block');
  return notes;
}

/* ------------------------------------------------------------------- breakout */

export function runBreakout(bars, tfId, ctx, levels) {
  const notes = { engine: 'breakout', timeframe: tfId, emitted: [], withheld: [] };
  const candidates = (levels?.emitted || []).filter((e) => e.type === 'SUPPORT_LEVEL' || e.type === 'RESISTANCE_LEVEL');
  if (!candidates.length) {
    notes.withheld.push('no support/resistance level available to break');
    return notes;
  }
  const i = bars.length - 1;
  const b = bars[i];
  let found = 0;
  for (const lvl of candidates) {
    const price = (lvl.zone.low + lvl.zone.high) / 2;
    if (Math.abs(b.close - price) < ctx.atrValue * 0.05) continue;
    const brokeUp = b.close > price && bars[i - 1]?.close <= price;
    const brokeDown = b.close < price && bars[i - 1]?.close >= price;
    if (brokeUp || brokeDown) {
      found += 1;
      notes.emitted.push(evidence({
        engine: 'breakout',
        type: 'BREAKOUT_CONFIRMED',
        timeframe: tfId,
        zone: lvl.zone,
        direction: brokeUp ? 'LONG' : 'SHORT',
        score: 0.68,
        conditions: [`close ${round(b.close, 2)} beyond level ${round(price, 2)}`, `previous close ${round(bars[i - 1]?.close ?? 0, 2)}`],
        reason: `${brokeUp ? 'Bullish' : 'Bearish'} close beyond a ${lvl.type.includes('SUPPORT') ? 'support' : 'resistance'} level.`,
        bars,
        atIndex: i,
      }));
    }
    // Fake breakout: wicked beyond the level then closed back inside it.
    const fakedUp = b.high > price && b.close < price && bars[i - 1]?.close <= price;
    const fakedDown = b.low < price && b.close > price && bars[i - 1]?.close >= price;
    if (fakedUp || fakedDown) {
      found += 1;
      notes.emitted.push(evidence({
        engine: 'breakout',
        type: 'FAKE_BREAKOUT',
        timeframe: tfId,
        zone: lvl.zone,
        direction: fakedUp ? 'SHORT' : 'LONG',
        score: 0.62,
        conditions: [`wick beyond ${round(price, 2)}`, `close back at ${round(b.close, 2)}`],
        reason: 'Breakout failed and price closed back inside the level.',
        bars,
        atIndex: i,
      }));
    }
  }
  if (!found) notes.withheld.push('no confirmed breakout or fake breakout on the latest closed bar');
  return notes;
}

/** Run every engine for one timeframe over its closed bars. */
export function runAllEngines(bars, tfId) {
  if (!bars || bars.length < 20) {
    return {
      timeframe: tfId,
      status: 'INSUFFICIENT_HISTORY',
      closedBars: bars ? bars.length : 0,
      engines: ENGINE_IDS.map((id) => ({ engine: id, emitted: [], withheld: ['fewer than 20 closed bars'] })),
      trend: 'UNKNOWN',
    };
  }
  const ctx = buildContext(bars);
  const structure = runStructure(bars, tfId, ctx);
  const levels = runLevels(bars, tfId, ctx);
  const results = [
    runLiquidity(bars, tfId, ctx),
    levels,
    runSupplyDemand(bars, tfId, ctx),
    structure,
    runImbalance(bars, tfId, ctx, structure),
    runBreakout(bars, tfId, ctx, levels),
  ];
  return {
    timeframe: tfId,
    status: 'OK',
    closedBars: bars.length,
    atr: round(ctx.atrValue, 2),
    trend: structure.trend || 'UNKNOWN',
    engines: results,
    evidence: results.flatMap((r) => r.emitted),
  };
}

export { buildContext };
