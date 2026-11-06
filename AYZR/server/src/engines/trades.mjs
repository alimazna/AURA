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
 */
export function evaluateVirtualTrade(trade, bar) {
  if (trade.state !== 'OPEN') return { closed: false };
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
  };
}

export function isActionable(state) {
  return ACTIONABLE_STATES.has(state);
}
