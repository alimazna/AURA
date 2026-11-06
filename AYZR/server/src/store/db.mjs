// Persistence.
//
// SQLite via node:sqlite (no native build step). Every analytical output is stored with
// its source and timestamp so any published value can be audited back to the bar it came
// from. Recovery replays from these tables rather than from in-memory state.

import { DatabaseSync } from 'node:sqlite';
import { mkdirSync } from 'node:fs';
import { dirname } from 'node:path';

export class Store {
  constructor(dbPath) {
    mkdirSync(dirname(dbPath), { recursive: true });
    this.db = new DatabaseSync(dbPath);
    this.db.exec('PRAGMA journal_mode = WAL');
    this.migrate();
  }

  migrate() {
    this.db.exec(`
      CREATE TABLE IF NOT EXISTS bars (
        timeframe TEXT NOT NULL,
        open_time TEXT NOT NULL,
        open REAL, high REAL, low REAL, close REAL,
        tick_volume REAL, is_open INTEGER, derived INTEGER,
        ingested_at TEXT NOT NULL,
        PRIMARY KEY (timeframe, open_time)
      );

      CREATE TABLE IF NOT EXISTS regions (
        id TEXT PRIMARY KEY,
        engine TEXT NOT NULL,
        type TEXT NOT NULL,
        timeframe TEXT NOT NULL,
        direction TEXT NOT NULL,
        zone_low REAL, zone_high REAL,
        state TEXT NOT NULL,
        score REAL,
        conditions TEXT,
        reason TEXT,
        source_time TEXT,
        discovered_at TEXT NOT NULL,
        discovered_at_ms INTEGER NOT NULL,
        updated_at TEXT NOT NULL,
        touches INTEGER DEFAULT 0
      );

      CREATE TABLE IF NOT EXISTS region_events (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        region_id TEXT NOT NULL,
        from_state TEXT,
        to_state TEXT NOT NULL,
        reason TEXT,
        source_time TEXT,
        at TEXT NOT NULL
      );

      CREATE TABLE IF NOT EXISTS signals (
        id TEXT PRIMARY KEY,
        region_id TEXT,
        engine TEXT, timeframe TEXT, direction TEXT,
        score REAL, state TEXT,
        entry REAL, stop REAL, target REAL,
        conditions TEXT, reason TEXT, source_time TEXT,
        created_at TEXT NOT NULL,
        operator_decision TEXT,
        operator_decision_at TEXT
      );

      CREATE TABLE IF NOT EXISTS virtual_trades (
        id TEXT PRIMARY KEY,
        region_id TEXT, signal_id TEXT,
        engine TEXT, timeframe TEXT, direction TEXT,
        state TEXT NOT NULL,
        entry REAL, entry_raw REAL, stop REAL, target REAL, lot REAL,
        costs TEXT, conditions TEXT, reason TEXT,
        opened_at TEXT NOT NULL, opened_at_ms INTEGER NOT NULL, trigger_bar_time TEXT,
        closed_at TEXT, closed_at_ms INTEGER,
        exit REAL, result TEXT, r_multiple REAL, pnl_usd REAL, exit_reason TEXT
      );

      CREATE TABLE IF NOT EXISTS ai_requests (
        id TEXT PRIMARY KEY,
        operator TEXT NOT NULL,
        prompt TEXT NOT NULL,
        timeframe TEXT,
        plan TEXT,
        verdict TEXT,
        provider TEXT,
        created_at TEXT NOT NULL
      );

      CREATE TABLE IF NOT EXISTS audit_log (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        category TEXT NOT NULL,
        action TEXT NOT NULL,
        detail TEXT,
        actor TEXT,
        at TEXT NOT NULL
      );

      CREATE TABLE IF NOT EXISTS stream_state (
        symbol TEXT PRIMARY KEY,
        transport TEXT NOT NULL,
        state TEXT NOT NULL,
        detail TEXT,
        updated_at TEXT NOT NULL
      );

      CREATE INDEX IF NOT EXISTS idx_regions_state ON regions(state);
      CREATE INDEX IF NOT EXISTS idx_vt_state ON virtual_trades(state);
      CREATE INDEX IF NOT EXISTS idx_audit_at ON audit_log(at);
    `);
  }

  audit(category, action, detail = '', actor = 'engine') {
    this.db.prepare('INSERT INTO audit_log (category, action, detail, actor, at) VALUES (?,?,?,?,?)')
      .run(category, action, typeof detail === 'string' ? detail : JSON.stringify(detail), actor, new Date().toISOString());
  }

  upsertBars(timeframe, bars) {
    const stmt = this.db.prepare(`
      INSERT INTO bars (timeframe, open_time, open, high, low, close, tick_volume, is_open, derived, ingested_at)
      VALUES (?,?,?,?,?,?,?,?,?,?)
      ON CONFLICT(timeframe, open_time) DO UPDATE SET
        open=excluded.open, high=excluded.high, low=excluded.low, close=excluded.close,
        tick_volume=excluded.tick_volume, is_open=excluded.is_open, derived=excluded.derived,
        ingested_at=excluded.ingested_at
    `);
    const now = new Date().toISOString();
    this.db.exec('BEGIN');
    try {
      for (const b of bars) {
        stmt.run(timeframe, b.openTime, b.open, b.high, b.low, b.close, b.tickVolume ?? 0, b.isOpen ? 1 : 0, b.derived ? 1 : 0, now);
      }
      this.db.exec('COMMIT');
    } catch (err) {
      this.db.exec('ROLLBACK');
      throw err;
    }
  }

  getBars(timeframe, limit = 400) {
    const rows = this.db.prepare('SELECT * FROM bars WHERE timeframe = ? ORDER BY open_time DESC LIMIT ?').all(timeframe, limit);
    return rows.reverse().map((r) => ({
      openTime: r.open_time, time: Date.parse(r.open_time),
      open: r.open, high: r.high, low: r.low, close: r.close,
      tickVolume: r.tick_volume, isOpen: !!r.is_open, derived: !!r.derived,
    }));
  }

  upsertRegion(region) {
    this.db.prepare(`
      INSERT INTO regions (id, engine, type, timeframe, direction, zone_low, zone_high, state, score, conditions, reason, source_time, discovered_at, discovered_at_ms, updated_at, touches)
      VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)
      ON CONFLICT(id) DO UPDATE SET
        state=excluded.state, score=excluded.score, updated_at=excluded.updated_at, touches=excluded.touches
    `).run(
      region.id, region.engine, region.type, region.timeframe, region.direction,
      region.zone?.low ?? null, region.zone?.high ?? null, region.state, region.score ?? null,
      JSON.stringify(region.conditions || []), region.reason || '', region.sourceTime || null,
      region.discoveredAt || new Date().toISOString(), region.discoveredAtMs || Date.now(),
      new Date().toISOString(), region.touches || 0,
    );
  }

  recordRegionEvent(regionId, fromState, toState, reason, sourceTime) {
    this.db.prepare('INSERT INTO region_events (region_id, from_state, to_state, reason, source_time, at) VALUES (?,?,?,?,?,?)')
      .run(regionId, fromState, toState, reason || '', sourceTime || null, new Date().toISOString());
  }

  getRegions({ timeframe, limit = 200 } = {}) {
    const rows = timeframe
      ? this.db.prepare('SELECT * FROM regions WHERE timeframe = ? ORDER BY discovered_at_ms DESC LIMIT ?').all(timeframe, limit)
      : this.db.prepare('SELECT * FROM regions ORDER BY discovered_at_ms DESC LIMIT ?').all(limit);
    return rows.map(mapRegion);
  }

  getRegionEvents(regionId, limit = 50) {
    return this.db.prepare('SELECT * FROM region_events WHERE region_id = ? ORDER BY id DESC LIMIT ?').all(regionId, limit)
      .map((r) => ({ from: r.from_state, to: r.to_state, reason: r.reason, sourceTime: r.source_time, at: r.at }));
  }

  upsertSignal(signal) {
    this.db.prepare(`
      INSERT INTO signals (id, region_id, engine, timeframe, direction, score, state, entry, stop, target, conditions, reason, source_time, created_at)
      VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?)
      ON CONFLICT(id) DO NOTHING
    `).run(
      signal.id, signal.regionId, signal.engine, signal.timeframe, signal.direction, signal.score, signal.state,
      signal.entry, signal.stop, signal.target, JSON.stringify(signal.conditions || []), signal.reason || '',
      signal.sourceTime || null, signal.createdAt,
    );
  }

  setSignalDecision(id, decision, at = new Date().toISOString()) {
    this.db.prepare('UPDATE signals SET operator_decision = ?, operator_decision_at = ? WHERE id = ?').run(decision, at, id);
  }

  getSignals({ timeframe, limit = 100 } = {}) {
    const rows = timeframe
      ? this.db.prepare('SELECT * FROM signals WHERE timeframe = ? ORDER BY created_at DESC LIMIT ?').all(timeframe, limit)
      : this.db.prepare('SELECT * FROM signals ORDER BY created_at DESC LIMIT ?').all(limit);
    return rows.map((r) => ({
      id: r.id, regionId: r.region_id, engine: r.engine, timeframe: r.timeframe, direction: r.direction,
      score: r.score, state: r.state, entry: r.entry, stop: r.stop, target: r.target,
      conditions: safeJson(r.conditions, []), reason: r.reason, sourceTime: r.source_time,
      createdAt: r.created_at, operatorDecision: r.operator_decision, operatorDecisionAt: r.operator_decision_at,
    }));
  }

  upsertVirtualTrade(t) {
    this.db.prepare(`
      INSERT INTO virtual_trades (id, region_id, signal_id, engine, timeframe, direction, state, entry, entry_raw, stop, target, lot, costs, conditions, reason, opened_at, opened_at_ms, trigger_bar_time, closed_at, closed_at_ms, exit, result, r_multiple, pnl_usd, exit_reason)
      VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)
      ON CONFLICT(id) DO UPDATE SET
        state=excluded.state, closed_at=excluded.closed_at, closed_at_ms=excluded.closed_at_ms,
        exit=excluded.exit, result=excluded.result, r_multiple=excluded.r_multiple,
        pnl_usd=excluded.pnl_usd, exit_reason=excluded.exit_reason
    `).run(
      t.id, t.regionId, t.signalId, t.engine, t.timeframe, t.direction, t.state,
      t.entry, t.entryRaw, t.stop, t.target, t.lot, JSON.stringify(t.costs || {}),
      JSON.stringify(t.conditions || []), t.reason || '', t.openedAt, t.openedAtMs,
      t.triggerBarTime || null,
      t.closedAt, t.closedAtMs, t.exit, t.result, t.rMultiple, t.pnlUsd, t.exitReason,
    );
  }

  getVirtualTrades({ timeframe, state, limit = 200 } = {}) {
    let sql = 'SELECT * FROM virtual_trades';
    const where = [];
    const args = [];
    if (timeframe) { where.push('timeframe = ?'); args.push(timeframe); }
    if (state) { where.push('state = ?'); args.push(state); }
    if (where.length) sql += ' WHERE ' + where.join(' AND ');
    sql += ' ORDER BY opened_at_ms DESC LIMIT ?';
    args.push(limit);
    return this.db.prepare(sql).all(...args).map((r) => ({
      id: r.id, regionId: r.region_id, signalId: r.signal_id, engine: r.engine, timeframe: r.timeframe,
      direction: r.direction, state: r.state, entry: r.entry, entryRaw: r.entry_raw, stop: r.stop,
      target: r.target, lot: r.lot, costs: safeJson(r.costs, {}), conditions: safeJson(r.conditions, []),
      reason: r.reason, openedAt: r.opened_at, openedAtMs: r.opened_at_ms, triggerBarTime: r.trigger_bar_time, closedAt: r.closed_at,
      closedAtMs: r.closed_at_ms, exit: r.exit, result: r.result, rMultiple: r.r_multiple,
      pnlUsd: r.pnl_usd, exitReason: r.exit_reason,
    }));
  }

  saveAiRequest(req) {
    this.db.prepare('INSERT INTO ai_requests (id, operator, prompt, timeframe, plan, verdict, provider, created_at) VALUES (?,?,?,?,?,?,?,?)')
      .run(req.id, req.operator, req.prompt, req.timeframe, JSON.stringify(req.plan), req.verdict, req.provider, req.createdAt);
  }

  getAiRequests(limit = 30) {
    return this.db.prepare('SELECT * FROM ai_requests ORDER BY created_at DESC LIMIT ?').all(limit)
      .map((r) => ({ id: r.id, operator: r.operator, prompt: r.prompt, timeframe: r.timeframe, plan: safeJson(r.plan, null), verdict: r.verdict, provider: r.provider, createdAt: r.created_at }));
  }

  setStreamState(symbol, transport, state, detail = '') {
    this.db.prepare(`
      INSERT INTO stream_state (symbol, transport, state, detail, updated_at) VALUES (?,?,?,?,?)
      ON CONFLICT(symbol) DO UPDATE SET transport=excluded.transport, state=excluded.state, detail=excluded.detail, updated_at=excluded.updated_at
    `).run(symbol, transport, state, typeof detail === 'string' ? detail : JSON.stringify(detail), new Date().toISOString());
  }

  getStreamState(symbol) {
    const r = this.db.prepare('SELECT * FROM stream_state WHERE symbol = ?').get(symbol);
    return r ? { symbol: r.symbol, transport: r.transport, state: r.state, detail: safeJson(r.detail, r.detail), updatedAt: r.updated_at } : null;
  }

  getAudit(limit = 100) {
    return this.db.prepare('SELECT * FROM audit_log ORDER BY id DESC LIMIT ?').all(limit)
      .map((r) => ({ id: r.id, category: r.category, action: r.action, detail: r.detail, actor: r.actor, at: r.at }));
  }

  checkpoint() {
    this.db.exec('PRAGMA wal_checkpoint(TRUNCATE)');
    const at = new Date().toISOString();
    this.audit('system', 'CHECKPOINT', 'sqlite wal checkpoint', 'operator');
    return at;
  }

  close() {
    try { this.db.close(); } catch { /* already closed */ }
  }
}

function safeJson(v, fallback) {
  if (v === null || v === undefined) return fallback;
  if (typeof v !== 'string') return v;
  try { return JSON.parse(v); } catch { return fallback; }
}

function mapRegion(r) {
  return {
    id: r.id, engine: r.engine, type: r.type, timeframe: r.timeframe, direction: r.direction,
    zone: { low: r.zone_low, high: r.zone_high }, state: r.state, score: r.score,
    conditions: safeJson(r.conditions, []), reason: r.reason, sourceTime: r.source_time,
    discoveredAt: r.discovered_at, discoveredAtMs: r.discovered_at_ms, updatedAt: r.updated_at, touches: r.touches,
  };
}
