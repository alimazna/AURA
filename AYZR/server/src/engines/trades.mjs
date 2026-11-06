// Signals and independent virtual trades.
//
// The brief requires two parallel, non-conflated paths (§5):
//
//   * A signal is what AYZR shows the operator. The operator may accept or ignore it,
//     and that decision is recorded but never changes the virtual trade.
//   * A virtual trade is created independently when the region's own conditions are met,
//     so it measures engine accuracy even when the operator ignores the signal.
//
// No real order is ever sent. Costs are modelled explicitly so the statistics are not
// optimistic: spread at entry, plus configurable slippage and commission.

import { round, stableId, isoFromMs } from './primitives.mjs';
import { ACTIONABLE_STATES } from './regions.mjs';

export const COST_MODEL = {
  slippagePerSide: Number(process.env.AYZR_SLIPPAGE || 0.15),
  commissionPerLot: Number(process.env.AYZR_COMMISSION || 0),
  lotSize: Number(process.env.AYZR_LOT || 0.01),
  contractSize: 100,
  riskReward: Number(process.env.AYZR_RR || 2),
};

/** Build a signal from a region that reached an actionable state. */
export function signalFromRegion(region, { atr, spread, timeframe, nowMs }) {
  const entry = region.direction === 'LONG' ? region.zone.high : region.zone.low;
  const stopDistance = Math.max(atr * 1.2, 1.5);
  const stop = region.direction === 'LONG' ? entry - stopDistance : entry + stopDistance;
  const target = region.direction === 'LONG' ? entry + stopDistance * COST_MODEL.riskReward : entry - stopDistance * COST_MODEL.riskReward;

  // Internal ranking value in [0,1]; explicitly not a calibrated probability.
  const score = round(Math.min(0.95, region.score + (region.state === 'CONFIRMED_BREAKOUT' ? 0.1 : 0)), 3);

  return {
    id: stableId(['signal', region.id, region.state]),
    regionId: region.id,
    engine: region.engine,
    timeframe,
    direction: region.direction,
    score,
    scoreLabel: 'internal ranking (uncalibrated)',
    state: region.state,
    entry: round(entry),
    stop: round(stop),
    target: round(target),
    riskReward: COST_MODEL.riskReward,
    conditions: region.conditions,
    reason: region.reason,
    sourceTime: region.sourceTime,
    source: 'engine',
    createdAt: isoFromMs(nowMs),
    operatorDecision: null,
  };
}

/**
 * Create a virtual trade from a region. This is called on the same trigger as the
 * signal, but it is a separate object with a separate id and its own outcome tracking.
 */
export function virtualTradeFromRegion(region, { atr, spread, timeframe, nowMs, triggerBarTime }) {
  const isLong = region.direction === 'LONG';
  const rawEntry = isLong ? region.zone.high : region.zone.low;
  const slippage = COST_MODEL.slippagePerSide;
  const entry = round(isLong ? rawEntry + spread / 2 + slippage : rawEntry - spread / 2 - slippage);

  const stopDistance = Math.max(atr * 1.2, 1.5);
  const stop = round(isLong ? entry - stopDistance : entry + stopDistance);
  const target = round(isLong ? entry + stopDistance * COST_MODEL.riskReward : entry - stopDistance * COST_MODEL.riskReward);
  const lot = COST_MODEL.lotSize;
  const commission = COST_MODEL.commissionPerLot * lot;

  return {
    id: stableId(['vpos', region.id, nowMs]),
    regionId: region.id,
    signalId: stableId(['signal', region.id, region.state]),
    engine: region.engine,
    timeframe,
    direction: region.direction,
    state: 'OPEN',
    entry,
    entryRaw: round(rawEntry),
    stop,
    target,
    lot,
    riskReward: COST_MODEL.riskReward,
    costs: {
      spreadAtEntry: round(spread, 3),
      slippagePerSide: slippage,
      commission,
      modelled: true,
    },
    conditions: region.conditions,
    reason: region.reason,
    openedAt: isoFromMs(nowMs),
    openedAtMs: nowMs,
    closedAt: null,
    closedAtMs: null,
    exit: null,
    result: null,
    rMultiple: null,
    pnlUsd: null,
    exitReason: null,
    // The bar the trade was created on. Evaluation may only use bars that close
    // strictly after this one, which is what prevents look-ahead bias.
    triggerBarTime: triggerBarTime || null,
  };
}

/**
 * Build a virtual trade from a chart drawing the operator made.
 *
 * A manual trade is the same kind of object as an engine trade — same cost model, same
 * lot sizing, same look-ahead guard — so it is evaluated by the identical loop and its
 * result is directly comparable. Only two things differ, and both are recorded so the
 * statistics can be separated later:
 *
 *   * origin is 'manual' rather than 'engine', and operator is the person who drew it;
 *   * the levels come from the drawing instead of from a region.
 *
 * This still creates no real order. AYZR has no order path.
 */
export function manualTradeFromDrawing({
  direction, entry, stop, target, timeframe, operator, nowMs, triggerBarTime, spread = 0, note = '',
}) {
  // Every stored trade needs a real creation time: it is what the look-ahead guard
  // compares against, so a trade without one could never be evaluated.
  if (!Number.isFinite(nowMs)) {
    throw new Error('a manual trade requires a valid creation time');
  }
  const isLong = direction === 'LONG';
  const slippage = COST_MODEL.slippagePerSide;

  // Entry is taken at the drawn level, then charged the same entry costs an engine
  // trade pays, so a manual trade is not flattered by a free fill.
  const entryPx = round(isLong ? entry + spread / 2 + slippage : entry - spread / 2 - slippage);
  const stopPx = round(stop);
  const targetPx = round(target);
  const stopDistance = Math.abs(entryPx - stopPx);
  const lot = COST_MODEL.lotSize;
  const commission = COST_MODEL.commissionPerLot * lot;

  return {
    id: stableId(['mtrade', operator, timeframe, entryPx, stopPx, targetPx, nowMs]),
    regionId: null,
    signalId: null,
    engine: 'MANUAL',
    origin: 'manual',
    operator,
    note,
    timeframe,
    direction,
    state: 'OPEN',
    entry: entryPx,
    entryRaw: round(entry),
    stop: stopPx,
    target: targetPx,
    lot,
    riskReward: stopDistance > 0 ? round(Math.abs(targetPx - entryPx) / stopDistance, 2) : null,
    costs: {
      spreadAtEntry: round(spread, 3),
      slippagePerSide: slippage,
      commission,
      modelled: true,
    },
    conditions: [],
    reason: 'operator drawing',
    openedAt: isoFromMs(nowMs),
    openedAtMs: nowMs,
    closedAt: null,
    closedAtMs: null,
    exit: null,
    result: null,
    rMultiple: null,
    pnlUsd: null,
    exitReason: null,
    triggerBarTime: triggerBarTime || null,
  };
}

/**
 * Validate an operator's drawing before it becomes a trade. Returns a list of reasons
 * the drawing is unusable; empty means valid. Kept here rather than in the API so the
 * rules travel with the trade model.
 */
export function validateDrawing({ direction, entry, stop, target, quote, maxRiskPoints }) {
  const errors = [];
  const nums = { entry, stop, target };
  for (const [k, v] of Object.entries(nums)) {
    if (typeof v !== 'number' || !Number.isFinite(v) || v <= 0) errors.push(`${k} must be a positive number`);
  }
  if (errors.length) return errors;

  if (direction !== 'LONG' && direction !== 'SHORT') errors.push('direction must be LONG or SHORT');
  if (direction === 'LONG' && !(stop < entry)) errors.push('for a LONG trade the stop must be below the entry');
  if (direction === 'SHORT' && !(stop > entry)) errors.push('for a SHORT trade the stop must be above the entry');
  if (direction === 'LONG' && !(target > entry)) errors.push('for a LONG trade the target must be above the entry');
  if (direction === 'SHORT' && !(target < entry)) errors.push('for a SHORT trade the target must be below the entry');

  const risk = Math.abs(entry - stop);
  if (risk > 0 && risk < 0.05) errors.push('stop is too close to the entry to be meaningful');
  if (Number.isFinite(maxRiskPoints) && risk > maxRiskPoints) {
    errors.push(`risk of ${round(risk, 2)} points exceeds the ${maxRiskPoints} point limit`);
  }

  // A drawing far away from the live price is almost always a mis-drag.
  const mid = quote?.mid;
  if (typeof mid === 'number' && mid > 0) {
    const drift = Math.abs(entry - mid) / mid;
    if (drift > 0.05) errors.push(`entry is ${round(drift * 100, 1)}% away from the live price`);
  }

  return errors;
}

/**
 * Select the closed bars a trade is allowed to be judged against.
 *
 * This is the look-ahead guard, and it lives in one place so it cannot drift: a trade
 * created on the close of bar T may only be evaluated on bars that close strictly
 * after T. Without this, a trade is judged against price history that predates it.
 */
export function eligibleBarsFor(trade, closedBars) {
  const triggerTime = trade.triggerBarTime ? Date.parse(trade.triggerBarTime) : Date.parse(trade.openedAt);
  if (!Number.isFinite(triggerTime)) return [];
  return (closedBars || []).filter((b) => Date.parse(b.openTime) > triggerTime);
}

/**
 * Advance an open virtual trade bar by bar. Stops are assumed to be hit before targets
 * when a single bar contains both, which is the conservative assumption.
 *
 * The look-ahead rule is re-checked here rather than trusted to the caller, so a bar
 * that closed at or before the trade was created can never resolve it even if a future
 * call site forgets to filter first.
 */
export function evaluateVirtualTrade(trade, bar) {
  if (trade.state !== 'OPEN') return { closed: false };
  const created = trade.triggerBarTime ? Date.parse(trade.triggerBarTime) : Date.parse(trade.openedAt);
  const barTime = Date.parse(bar.openTime);
  if (!Number.isFinite(created) || !Number.isFinite(barTime) || barTime <= created) {
    return { closed: false, skipped: 'bar is not after the trade was created' };
  }
  const isLong = trade.direction === 'LONG';
  const hitStop = isLong ? bar.low <= trade.stop : bar.high >= trade.stop;
  const hitTarget = isLong ? bar.high >= trade.target : bar.low <= trade.target;

  if (hitStop) {
    return { closed: true, exit: trade.stop, result: 'LOSS', exitReason: 'stop hit', atMs: Date.parse(bar.openTime) };
  }
  if (hitTarget) {
    return { closed: true, exit: trade.target, result: 'WIN', exitReason: 'target hit', atMs: Date.parse(bar.openTime) };
  }
  return { closed: false };
}

export function closeVirtualTrade(trade, { exit, result, exitReason, atMs, contractSize = COST_MODEL.contractSize }) {
  const isLong = trade.direction === 'LONG';
  const grossPoints = isLong ? exit - trade.entry : trade.entry - exit;
  const gross = grossPoints * contractSize * trade.lot;
  const net = gross - (trade.costs.commission || 0);
  const risk = Math.abs(trade.entry - trade.stop) * contractSize * trade.lot;
  return {
    ...trade,
    state: 'CLOSED',
    exit: round(exit),
    result,
    exitReason,
    closedAt: isoFromMs(atMs),
    closedAtMs: atMs,
    pnlUsd: round(net, 2),
    rMultiple: risk > 0 ? round(net / risk, 2) : null,
  };
}

/**
 * Performance statistics. Kept separable by engine, timeframe and session so a single
 * strong engine cannot hide a weak one (brief §7).
 */
export function statistics(trades) {
  const closed = trades.filter((t) => t.state === 'CLOSED');
  const wins = closed.filter((t) => t.result === 'WIN');
  const losses = closed.filter((t) => t.result === 'LOSS');
  const grossWin = wins.reduce((a, t) => a + (t.pnlUsd || 0), 0);
  const grossLoss = Math.abs(losses.reduce((a, t) => a + (t.pnlUsd || 0), 0));
  const totalR = closed.reduce((a, t) => a + (t.rMultiple || 0), 0);
  const by = (key) => {
    const groups = {};
    for (const t of closed) {
      const k = t[key];
      groups[k] = groups[k] || { count: 0, wins: 0, losses: 0, pnlUsd: 0, rMultiple: 0 };
      groups[k].count += 1;
      if (t.result === 'WIN') groups[k].wins += 1;
      if (t.result === 'LOSS') groups[k].losses += 1;
      groups[k].pnlUsd = round(groups[k].pnlUsd + (t.pnlUsd || 0), 2);
      groups[k].rMultiple = round(groups[k].rMultiple + (t.rMultiple || 0), 2);
    }
    for (const g of Object.values(groups)) {
      g.winRate = g.count ? round(g.wins / g.count, 3) : null;
    }
    return groups;
  };
  return {
    counts: {
      discovered: trades.length,
      open: trades.filter((t) => t.state === 'OPEN').length,
      closed: closed.length,
      wins: wins.length,
      losses: losses.length,
    },
    winRate: closed.length ? round(wins.length / closed.length, 3) : null,
    profitFactor: grossLoss > 0 ? round(grossWin / grossLoss, 2) : null,
    expectancyR: closed.length ? round(totalR / closed.length, 2) : null,
    netPnlUsd: round(closed.reduce((a, t) => a + (t.pnlUsd || 0), 0), 2),
    costsModelled: true,
    byEngine: by('engine'),
    byTimeframe: by('timeframe'),
    bySession: by('session'),
    // Manual and engine trades are measured separately. Blending them would let a
    // discretionary trade hide or inflate an engine's record (brief §5).
    byOrigin: by('origin'),
  };
}

export function isActionable(state) {
  return ACTIONABLE_STATES.has(state);
}
