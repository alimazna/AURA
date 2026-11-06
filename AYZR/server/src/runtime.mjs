// AYZR runtime.
//
// Market monitoring is a service independent of the AI (brief §4, §6): a timer ingests
// venue data, runs the engines, advances the region lifecycle, and evaluates virtual
// trades. The AI is never on this path.

import { SYMBOL, TIMEFRAMES, TIMEFRAME_BY_ID, DEFAULT_TIMEFRAME, RUNTIME } from './config.mjs';
import * as bq from './biquote/client.mjs';
import { runAllEngines } from './engines/index.mjs';
import { advanceRegion, expireStale, regionKey, TERMINAL_STATES } from './engines/regions.mjs';
import { signalFromRegion, virtualTradeFromRegion, evaluateVirtualTrade, closeVirtualTrade, statistics, isActionable, eligibleBarsFor, manualTradeFromDrawing, validateDrawing } from './engines/trades.mjs';
import { isoNow, round, stableId } from './engines/primitives.mjs';
import { requestPlan, buildContext, providerStatus, newRequestId } from './ai/planner.mjs';

const MAX_REGION_AGE_MS = 72 * 3600 * 1000;

export class Runtime {
  constructor(store) {
    this.store = store;
    this.quote = null;
    this.bars = new Map();          // tfId -> bars
    this.analyses = new Map();      // tfId -> engine output
    this.regions = new Map();       // regionKey -> region
    this.streamState = { transport: 'none', state: 'not_started', detail: '', updatedAt: null };
    this.lastIngest = null;
    this.lastError = null;
    this.ingestCount = 0;
    this.timer = null;
    this.stream = null;
    this.ingestLock = false;
    this.tickBuffer = [];
    this.restore();
  }

  /** Recover region and trade state from persistence rather than assuming a clean start. */
  restore() {
    for (const r of this.store.getRegions({ limit: 2000 })) {
      if (TERMINAL_STATES.has(r.state)) continue;
      this.regions.set(`${r.engine}:${r.type}:${r.zone.low?.toFixed(2)}:${r.zone.high?.toFixed(2)}`, { ...r, lastIndex: 0 });
    }
    this.store.audit('system', 'RESTORE', `recovered ${this.regions.size} open regions`, 'runtime');
  }

  start() {
    this.ingest().catch((e) => { this.lastError = e.message; });
    this.timer = setInterval(() => {
      this.ingest().catch((e) => { this.lastError = e.message; });
    }, RUNTIME.pollMs);
    this.startStream();
    return this;
  }

  stop() {
    if (this.timer) clearInterval(this.timer);
    this.timer = null;
    if (this.stream) this.stream.close();
  }

  /** Tick stream is a bonus transport. If it fails, REST polling already covers ingest. */
  async startStream() {
    try {
      this.stream = await bq.streamTicks([SYMBOL], (tick) => {
        this.quote = tick;
        this.tickBuffer.push({ at: Date.now(), mid: tick.mid });
        if (this.tickBuffer.length > 5000) this.tickBuffer.splice(0, 2500);
      }, {
        onState: (s) => {
          this.streamState = { transport: 'signalr', state: s.state, detail: s.reason || '', updatedAt: isoNow() };
          this.store.setStreamState(SYMBOL, 'signalr', s.state, s.reason || '');
          if (s.state === 'closed' && !this.stopping) {
            setTimeout(() => this.startStream().catch(() => {}), 5000);
          }
        },
      });
      this.streamState = { transport: 'signalr', state: 'connecting', detail: '', updatedAt: isoNow() };
    } catch (err) {
      this.streamState = { transport: 'rest-poll', state: 'stream_unavailable', detail: err.message, updatedAt: isoNow() };
      this.store.setStreamState(SYMBOL, 'rest-poll', 'stream_unavailable', err.message);
      this.store.audit('data', 'STREAM_UNAVAILABLE', err.message, 'runtime');
    }
  }

  /** One ingest cycle: quote, bars, engines, regions, trades. */
  async ingest() {
    if (this.ingestLock) return;
    this.ingestLock = true;
    try {
      // Direct timeframes are fetched from their own interval. Derived timeframes
      // (W1, MN1) are aggregated from daily bars, so daily history is fetched once,
      // deeper than the display window, and reused for every derived timeframe.
      const direct = TIMEFRAMES.filter((tf) => !tf.derived);
      const deepest = Math.max(RUNTIME.barsPerTimeframe, RUNTIME.derivedSourceBars);

      const [q, ...directResults] = await Promise.all([
        bq.quote(SYMBOL),
        ...direct.map((tf) => bq.ohlc(SYMBOL, tf.stream, tf.id === 'D1' ? deepest : RUNTIME.barsPerTimeframe)
          .catch((err) => ({ error: err.message, bars: [] }))),
      ]);
      this.quote = q;

      const byId = new Map();
      direct.forEach((tf, i) => byId.set(tf.id, directResults[i]));
      const dailyBars = byId.get('D1')?.bars || [];

      for (const tf of TIMEFRAMES) {
        let bars = [];
        if (tf.derived) {
          bars = dailyBars.length ? bq.aggregateBars(dailyBars, tf.minutes) : [];
        } else {
          const r = byId.get(tf.id);
          bars = r && !r.error ? r.bars : [];
          if (r?.error) this.store.audit('data', 'TIMEFRAME_UNAVAILABLE', { timeframe: tf.id, error: r.error }, 'runtime');
        }
        if (bars.length) {
          this.bars.set(tf.id, bars);
          this.store.upsertBars(tf.id, bars.slice(-1000));
        } else {
          this.bars.delete(tf.id);
        }
      }

      // Engines run on every timeframe; M15 is the operational default.
      for (const tf of TIMEFRAMES) {
        const bars = this.bars.get(tf.id);
        if (!bars || bars.length < 20) {
          this.analyses.set(tf.id, { timeframe: tf.id, status: 'NOT_AVAILABLE', engines: [], evidence: [] });
          continue;
        }
        const closed = bars.filter((b) => !b.isOpen);
        const analysis = runAllEngines(closed, tf.id);
        analysis.barsAvailable = bars.length;
        analysis.closedBars = closed.length;
        analysis.derived = tf.derived;
        this.analyses.set(tf.id, analysis);
        this.applyEvidence(tf, analysis, closed, q);
        this.advanceRegions(tf, closed, analysis.atr || 0, q);
      }

      this.evaluateOpenTrades();
      this.lastIngest = isoNow();
      this.ingestCount += 1;
      this.lastError = null;
      this.store.audit('data', 'INGEST', `cycle ${this.ingestCount} ok`, 'runtime');
    } catch (err) {
      this.lastError = err.message;
      this.store.audit('data', 'INGEST_FAILED', err.message, 'runtime');
      throw err;
    } finally {
      this.ingestLock = false;
    }
  }

  /** Turn fresh engine evidence into tracked regions, signals and virtual trades. */
  applyEvidence(tf, analysis, closedBars, quote) {
    const nowMs = Date.now();
    for (const ev of analysis.evidence || []) {
      const key = regionKey(ev);
      if (this.regions.has(key)) {
        const existing = this.regions.get(key);
        existing.touches = (existing.touches || 0) + 1;
        existing.score = Math.max(existing.score || 0, ev.score);
        continue;
      }
      const region = {
        id: ev.id,
        engine: ev.engine,
        type: ev.type,
        timeframe: tf.id,
        direction: ev.direction,
        zone: ev.zone,
        state: 'DISCOVERED',
        score: ev.score,
        conditions: ev.conditions,
        reason: ev.reason,
        sourceTime: ev.sourceBarTime,
        source: 'engine',
        discoveredAt: isoNow(),
        discoveredAtMs: nowMs,
        updatedAt: isoNow(),
        touches: 0,
        lastIndex: ev.sourceBarIndex ?? 0,
      };
      this.regions.set(key, region);
      this.store.upsertRegion(region);
      this.store.recordRegionEvent(region.id, null, 'DISCOVERED', ev.reason, ev.sourceBarTime);
      this.store.audit('region', 'DISCOVERED', { id: region.id, engine: region.engine, type: region.type, zone: region.zone }, 'engine');
    }
  }

  /** Advance every non-terminal region on this timeframe with the latest closed bar. */
  advanceRegions(tf, closedBars, atr, quote) {
    const nowMs = Date.now();
    const bar = closedBars[closedBars.length - 1];
    if (!bar) return;

    for (const region of this.regions.values()) {
      if (region.timeframe !== tf.id || TERMINAL_STATES.has(region.state)) continue;

      const expired = expireStale(region, nowMs, MAX_REGION_AGE_MS);
      if (expired) {
        this.transition(region, expired.state, expired.reason, bar.openTime);
        continue;
      }

      const next = advanceRegion(region, bar, atr || 1);
      if (next.transitioned && next.state !== region.state) {
        this.transition(region, next.state, next.reason, bar.openTime);
        if (isActionable(next.state)) this.onActionable(region, { atr, quote, tfId: tf.id, nowMs, closedBars });
      } else {
        region.touches = (region.touches || 0) + 1;
      }
    }
  }

  transition(region, toState, reason, sourceTime) {
    const from = region.state;
    region.state = toState;
    region.updatedAt = isoNow();
    this.store.upsertRegion(region);
    this.store.recordRegionEvent(region.id, from, toState, reason, sourceTime);
    this.store.audit('region', 'TRANSITION', { id: region.id, from, to: toState, reason }, 'engine');
  }

  /**
   * The two parallel paths. A signal is published for the operator; a virtual trade is
   * created independently of any operator decision (brief §5).
   */
  onActionable(region, { atr, quote, tfId, nowMs, closedBars }) {
    const spread = quote?.spread ?? 0;

    const signal = signalFromRegion(region, { atr, spread, timeframe: tfId, nowMs });
    this.store.upsertSignal(signal);
    this.store.audit('signal', 'PUBLISHED', { id: signal.id, region: region.id, direction: signal.direction }, 'engine');

    const existing = this.store.getVirtualTrades({ timeframe: tfId, limit: 500 }).find((t) => t.regionId === region.id);
    if (existing) return;

    const trade = virtualTradeFromRegion(region, {
      atr, spread, timeframe: tfId, nowMs,
      triggerBarTime: closedBars[closedBars.length - 1]?.openTime || null,
    });
    this.store.upsertVirtualTrade(trade);
    this.store.audit('virtual_trade', 'OPENED', { id: trade.id, region: region.id, direction: trade.direction, entry: trade.entry, costs: trade.costs }, 'engine');
  }

  /** Walk every open virtual trade forward over bars that closed after it opened. */
  evaluateOpenTrades() {
    const open = this.store.getVirtualTrades({ state: 'OPEN', limit: 1000 });
    for (const trade of open) {
      const bars = this.bars.get(trade.timeframe);
      if (!bars || !bars.length) continue;
      const closed = bars.filter((b) => !b.isOpen);

      // The look-ahead guard lives in the trade engine so every caller shares one rule.
      const eligible = eligibleBarsFor(trade, closed);
      for (const b of eligible) {
        const r = evaluateVirtualTrade(trade, b);
        if (r.closed) {
          const finalised = closeVirtualTrade(trade, { exit: r.exit, result: r.result, exitReason: r.exitReason, atMs: r.atMs });
          this.store.upsertVirtualTrade(finalised);
          this.store.audit('virtual_trade', 'CLOSED', { id: finalised.id, result: finalised.result, exit: finalised.exit, pnlUsd: finalised.pnlUsd, rMultiple: finalised.rMultiple }, 'engine');
          break;
        }
      }
    }
  }

  setOperatorDecision(signalId, decision) {
    this.store.setSignalDecision(signalId, decision);
    this.store.audit('signal', 'OPERATOR_DECISION', { signalId, decision }, 'operator');
    return { ok: true };
  }

  /**
   * Create a shadow trade from a drawing the operator made on the chart.
   *
   * The result is a virtual trade like any other, so `evaluateOpenTrades` judges it by
   * the same rules and it appears in the same statistics (separated by origin). This
   * still creates no order: there is no broker path in AYZR.
   */
  createManualTrade({ direction, entry, stop, target, timeframe, operator, note }) {
    const tfId = TIMEFRAME_BY_ID.has(timeframe) ? timeframe : DEFAULT_TIMEFRAME;
    const nowMs = Date.now();

    const errors = validateDrawing({
      direction, entry, stop, target,
      quote: this.quote,
      maxRiskPoints: RUNTIME.manualMaxRiskPoints,
    });
    if (errors.length) return { ok: false, errors };

    const bars = this.bars.get(tfId) || [];
    const closed = bars.filter((b) => !b.isOpen);
    const trade = manualTradeFromDrawing({
      direction, entry, stop, target,
      timeframe: tfId,
      operator,
      note,
      nowMs,
      spread: this.quote?.spread ?? 0,
      // Judged only against bars closing after the last one the operator could see,
      // which is the same look-ahead guard the engine trades use.
      triggerBarTime: closed[closed.length - 1]?.openTime || null,
    });

    this.store.upsertVirtualTrade(trade);
    this.store.audit('virtual_trade', 'MANUAL_OPENED', {
      id: trade.id, operator, direction, entry: trade.entry, stop: trade.stop,
      target: trade.target, riskReward: trade.riskReward, timeframe: tfId, note: note || '',
    }, 'operator');

    return { ok: true, trade };
  }

  /** Close a shadow trade early at the operator's request. Shadow only, never a venue call. */
  closeManualTrade({ id, exit, reason, operator }) {
    const trade = this.store.getVirtualTrades({ limit: 5000 }).find((t) => t.id === id);
    if (!trade) return { ok: false, errors: ['no such virtual trade'] };
    if (trade.state !== 'OPEN') return { ok: false, errors: ['trade is already closed'] };
    if (trade.origin !== 'manual') {
      return { ok: false, errors: ['only manual trades can be closed by hand; engine trades close on their own rules'] };
    }
    if (typeof exit !== 'number' || !Number.isFinite(exit) || exit <= 0) {
      return { ok: false, errors: ['exit must be a positive number'] };
    }

    const isLong = trade.direction === 'LONG';
    const grossPoints = isLong ? exit - trade.entry : trade.entry - exit;
    const result = grossPoints > 0 ? 'WIN' : grossPoints < 0 ? 'LOSS' : 'BREAKEVEN';
    const finalised = closeVirtualTrade(trade, {
      exit, result,
      exitReason: reason || 'closed by operator',
      atMs: Date.now(),
    });

    this.store.upsertVirtualTrade(finalised);
    this.store.audit('virtual_trade', 'MANUAL_CLOSED', {
      id, operator, exit, result, pnlUsd: finalised.pnlUsd, rMultiple: finalised.rMultiple,
    }, 'operator');
    return { ok: true, trade: finalised };
  }

  /** AI entry point. Operator-only; called from the API, never from a timer. */
  async requestAnalysis({ prompt, operator, timeframe }) {
    const tfId = TIMEFRAME_BY_ID.has(timeframe) ? timeframe : DEFAULT_TIMEFRAME;
    const analysis = this.analyses.get(tfId) || { engines: [] };
    const bars = this.bars.get(tfId) || [];
    const regions = [...this.regions.values()].filter((r) => r.timeframe === tfId);
    const context = buildContext({ symbol: SYMBOL, timeframe: tfId, quote: this.quote, bars, analysis, regions });
    const result = await requestPlan({ config: RUNTIME.ai, context });

    const record = {
      id: newRequestId(prompt, operator),
      operator,
      prompt,
      timeframe: tfId,
      plan: result.plan,
      verdict: result.verdict,
      provider: result.provider || 'none',
      createdAt: isoNow(),
    };
    this.store.saveAiRequest(record);
    this.store.audit('ai', 'ANALYSIS_REQUESTED', { id: record.id, operator, verdict: result.verdict, reason: result.reason }, 'operator');
    return { ...record, reason: result.reason, model: result.model || '' };
  }

  snapshot(timeframe) {
    const tfId = TIMEFRAME_BY_ID.has(timeframe) ? timeframe : DEFAULT_TIMEFRAME;
    const bars = this.bars.get(tfId) || [];
    const closed = bars.filter((b) => !b.isOpen);
    const analysis = this.analyses.get(tfId) || { engines: [], status: 'NOT_AVAILABLE' };
    const regions = [...this.regions.values()].filter((r) => r.timeframe === tfId && !TERMINAL_STATES.has(r.state));
    const trades = this.store.getVirtualTrades({ timeframe: tfId, limit: 200 });
    const signals = this.store.getSignals({ timeframe: tfId, limit: 50 });

    return {
      symbol: SYMBOL,
      timeframe: tfId,
      capturedAt: isoNow(),
      mode: 'SHADOW_ONLY',
      liveTrading: 'DISABLED',
      quote: this.quote,
      data: {
        source: 'BiQuote',
        transport: this.streamState.transport,
        streamState: this.streamState.state,
        streamDetail: this.streamState.detail,
        barsAvailable: bars.length,
        closedBars: closed.length,
        newestClosedBar: closed[closed.length - 1]?.openTime ?? null,
        derived: !!TIMEFRAME_BY_ID.get(tfId)?.derived,
        lastIngest: this.lastIngest,
        lastError: this.lastError,
        ingestCycles: this.ingestCount,
      },
      candles: closed.slice(-200).map((b) => ({ t: b.openTime, o: b.open, h: b.high, l: b.low, c: b.close, v: b.tickVolume })),
      analysis: {
        status: analysis.status,
        trend: analysis.trend || 'UNKNOWN',
        atr: analysis.atr ?? null,
        engines: (analysis.engines || []).map((e) => ({
          engine: e.engine,
          emitted: e.emitted.length,
          withheld: e.withheld,
          evidence: e.emitted.slice(-4),
        })),
      },
      regions: regions.map((r) => ({
        id: r.id, engine: r.engine, type: r.type, direction: r.direction, state: r.state,
        zone: r.zone, score: r.score, conditions: r.conditions, reason: r.reason,
        sourceTime: r.sourceTime, discoveredAt: r.discoveredAt, touches: r.touches,
      })),
      signals,
      virtualTrades: trades,
      statistics: statistics(trades),
      ai: providerStatus(RUNTIME.ai),
      engineSpecs: null,
    };
  }

  timeframeMatrix() {
    return TIMEFRAMES.map((tf) => {
      const bars = this.bars.get(tf.id) || [];
      const closed = bars.filter((b) => !b.isOpen);
      const a = this.analyses.get(tf.id);
      const regions = [...this.regions.values()].filter((r) => r.timeframe === tf.id && !TERMINAL_STATES.has(r.state));
      const newest = closed[closed.length - 1];
      const ageSec = newest ? Math.round((Date.now() - Date.parse(newest.openTime)) / 1000) : null;
      return {
        id: tf.id,
        label: tf.label,
        role: tf.role,
        stream: tf.stream,
        derived: tf.derived,
        status: bars.length === 0 ? 'NOT_AVAILABLE' : (a?.status || 'OK'),
        closedBars: closed.length,
        newestClosedBar: newest?.openTime ?? null,
        freshnessSeconds: ageSec,
        isOpenBarPresent: bars.some((b) => b.isOpen),
        atr: a?.atr ?? null,
        trend: a?.trend ?? 'UNKNOWN',
        activeRegions: regions.length,
        evidence: (a?.evidence || []).length,
      };
    });
  }
}
