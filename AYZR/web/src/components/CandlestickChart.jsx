// Candlestick chart with zone overlays, crosshair readout, zoom and fullscreen.
//
// Design intent: the candles are the only strong element on the surface. Zones are
// drawn as thin bordered bands anchored at the bar that produced them, so a band reads
// as "this level has existed since here" instead of as an unexplained stripe. Everything
// else (grid, axes, overlays) is deliberately low-contrast so price action dominates.
//
// Only closed bars are ever drawn. The server strips the forming bar, and this component
// independently ignores any bar flagged open.

import { useCallback, useEffect, useId, useLayoutEffect, useMemo, useRef, useState } from 'react';
import { createPortal } from 'react-dom';
import {
  price, dateUTC, timeUTC, ENGINE_AR, STATE_AR,
  TYPE_AR, TYPE_SHORT_AR, TYPE_MEANING_AR, REGION_CATEGORY_ORDER,
} from '../lib/format.js';

const PAD = { top: 30, right: 86, bottom: 34, left: 14 };

// Zone styling. The border carries engine identity; the fill stays almost transparent so
// overlapping zones never turn the chart into a wash.
const ENGINE_STROKE = {
  liquidity: '#fbbf1d',
  levels: '#4d8dff',
  supplyDemand: '#1bd1a2',
  structure: '#966ee6',
  imbalance: '#d93b4b',
  breakout: '#4d8dff',
};
const ENGINE_FILL = {
  liquidity: 'rgba(251,191,29,0.05)',
  levels: 'rgba(77,141,255,0.05)',
  supplyDemand: 'rgba(27,209,162,0.05)',
  structure: 'rgba(150,110,230,0.05)',
  imbalance: 'rgba(217,59,75,0.045)',
  breakout: 'rgba(77,141,255,0.05)',
};

const UP = '#01cd75';
const DOWN = '#e93d53';
const GRID = '#061428';
const AXIS_TEXT = '#8daede';
// One label size for every piece of chart text, so the plot never looks like a pile of
// slightly different type sizes.
const LABEL = 11;

/** Overlay colours. Chosen to stay distinguishable from each other and from the candle
 *  colours (green/red), because an overlay that reads as a candle colour is a lie. */
const IND_COLOR = {
  ema20: '#4d8dff',
  ema50: '#f3a33c',
  ema200: '#c77dff',
  sma50: '#7d93ad',
  sma200: '#a8bdd6',
  vwap: '#1bd1a2',
  bollingerUpper: '#6d87a6',
  bollingerMid: '#8ba4bf',
  bollingerLower: '#6d87a6',
  rsi: '#c77dff',
  macdLine: '#4d8dff',
  macdSignal: '#f3a33c',
  macdHistUp: 'rgba(18,196,139,0.55)',
  macdHistDown: 'rgba(224,75,92,0.55)',
  stochK: '#4d8dff',
  stochD: '#f3a33c',
  adx: '#f3d36c',
  plusDI: '#12c48b',
  minusDI: '#e04b5c',
  volume: '#2b4a72',
  volumeSma: '#f3a33c',
};

/** Human labels for the chart's own legend, kept beside the colours so the two cannot drift. */
export const INDICATOR_LABELS = {
  ema20: 'EMA 20',
  ema50: 'EMA 50',
  ema200: 'EMA 200',
  sma50: 'SMA 50',
  sma200: 'SMA 200',
  vwap: 'VWAP',
  bollinger: 'Bollinger',
  rsi: 'RSI 14',
  macd: 'MACD',
  stochastic: 'Stoch',
  adx: 'ADX',
  volume: 'Volume',
};

/** Overlays draw on the price plot; panes get their own strip below it. */
export const OVERLAY_IDS = ['ema20', 'ema50', 'ema200', 'sma50', 'sma200', 'vwap', 'bollinger'];
export const PANE_IDS = ['rsi', 'macd', 'stochastic', 'adx', 'volume'];

/** Reference lines a pane needs to be readable (overbought/oversold, zero). */
const PANE_GUIDES = {
  rsi: [30, 50, 70],
  stochastic: [20, 50, 80],
  adx: [25],
  macd: [0],
};

/**
 * Build an SVG path for an indicator series.
 *
 * The series is split into contiguous runs of defined values. A single path would draw a
 * straight line across the null gap at the start of a series - joining the first real point
 * back to wherever the path began - which is a line the data does not contain. `xAt` is passed
 * in so the caller controls horizontal placement.
 */
function seriesPath(series, xForIndex, toY) {
  if (!series) return '';
  const parts = [];
  let run = [];
  for (let i = 0; i < series.length; i++) {
    const v = series[i];
    if (v === null || v === undefined || !Number.isFinite(v)) {
      if (run.length) { parts.push(run); run = []; }
      continue;
    }
    run.push(`${xForIndex(i).toFixed(1)},${toY(v).toFixed(1)}`);
  }
  if (run.length) parts.push(run);
  return parts.map((p) => (p.length === 1 ? `M${p[0]}L${p[0]}` : `M${p.join('L')}`)).join(' ');
}
// Bands drawn at once. 16 overlapping zones washed the candles out, so only the
// highest-scoring are kept and the footer states how many are shown.
const MAX_ZONES = 11;

// Panes drawn at once. Five strips in a 440px chart left ~24px each, where the name, the
// current value and the guide labels collided into noise. Three stays readable.
export const MAX_PANES = 3;

/**
 * One indicator toggle. Disabled when the server did not return that series, so a chip can
 * never appear selectable for something the chart has no data for.
 */
function IndChip({ id, active, available, def, onToggle, onInfo }) {
  const label = INDICATOR_LABELS[id] || id;
  return (
    <span style={{ display: 'inline-flex', alignItems: 'center', gap: 3 }}>
      <button
        className="chart-btn"
        disabled={!available}
        title={available ? label : `${label} — لا تتوفر بيانات كافية`}
        onClick={() => onToggle?.(id)}
        style={{
          width: 'auto', padding: '2px 8px', fontSize: 'var(--t-small)',
          opacity: available ? 1 : 0.4,
          cursor: available ? 'pointer' : 'not-allowed',
          background: active ? 'var(--blue)' : undefined,
          color: active ? '#fff' : undefined,
          borderColor: active ? 'var(--blue)' : undefined,
        }}
      >
        {label}
      </button>
      {def && (
        <button
          className="chart-btn"
          title="التعريف والمصدر"
          onClick={() => onInfo?.(id)}
          style={{ width: 'auto', padding: '2px 5px', fontSize: 'var(--t-micro)' }}
        >
          ؟
        </button>
      )}
    </span>
  );
}

export function CandlestickChart({
  candles, timeframe, regions = [], trades = [], height = 440,
  operator = false, onCreateTrade, onCloseTrade, riskReward = 2,
  indicators = null, activeIndicators = [], onToggleIndicator, indicatorDefs = {},
  maxRiskPoints = 60,
}) {
  const plotRef = useRef(null);
  const clipId = useId();
  const [size, setSize] = useState({ w: 1000, h: height });
  const [hover, setHover] = useState(null);
  const [showRegions, setShowRegions] = useState(true);
  const [full, setFull] = useState(false);
  // slots = pixels per bar. Larger means more zoomed in.
  const [slot, setSlot] = useState(8);
  // Drawing state. `draft` is a trade being placed; `drag` tracks the pointer while
  // a level is being pulled into position.
  const [drawMode, setDrawMode] = useState(false);
  const [draft, setDraft] = useState(null);
  const [drag, setDrag] = useState(null);
  const [notice, setNotice] = useState(null);
  // Region under the pointer, so its band can be emphasised and explained.
  const [hotRegion, setHotRegion] = useState(null);
  // Indicator picker visibility, and which definition the reader is asking about.
  const [showInd, setShowInd] = useState(false);
  const [indInfo, setIndInfo] = useState(null);
  const [saving, setSaving] = useState(false);
  const endDragRef = useRef(null);

  // Measure the plot area itself, not the wrapper: the wrapper also holds the header
  // and footer bars, and sizing the svg from the wrapper would overflow by their height.
  useLayoutEffect(() => {
    const el = plotRef.current;
    if (!el) return undefined;
    const measure = () => {
      const r = el.getBoundingClientRect();
      if (r.width > 0) setSize({ w: r.width, h: r.height });
    };
    measure();
    const ro = new ResizeObserver(measure);
    ro.observe(el);
    window.addEventListener('resize', measure);
    return () => { ro.disconnect(); window.removeEventListener('resize', measure); };
  }, [full]);

  // Fullscreen: lock the page behind the overlay and let Escape close it.
  useEffect(() => {
    if (!full) return undefined;
    const onKey = (e) => {
      if (e.key === 'Escape') { e.stopPropagation(); setFull(false); }
    };
    const prev = document.body.style.overflow;
    document.body.style.overflow = 'hidden';
    window.addEventListener('keydown', onKey);
    return () => {
      document.body.style.overflow = prev;
      window.removeEventListener('keydown', onKey);
    };
  }, [full]);

  // Ctrl/⌘ + wheel zooms; a plain wheel is left to the page so scrolling still works.
  const onWheel = useCallback((e) => {
    if (!(e.ctrlKey || e.metaKey)) return;
    e.preventDefault();
    setSlot((s) => Math.min(28, Math.max(2.5, s * (e.deltaY < 0 ? 1.12 : 0.89))));
  }, []);

  const data = useMemo(
    () => (candles || []).filter((c) => c && Number.isFinite(c.h) && c.h > 0),
    [candles],
  );

  const plotW = Math.max(80, size.w - PAD.left - PAD.right);
  const plotH = Math.max(80, size.h - PAD.top - PAD.bottom);
  const visible = Math.max(12, Math.floor(plotW / slot));
  const bars = data.slice(-visible);
  const cw = bars.length ? plotW / bars.length : plotW;

  // Indicator panes share the height with the price plot instead of being stacked on top of
  // it, because a pane that floats over the candles hides the thing the operator is reading.
  // The price plot keeps a majority share; each pane gets an equal slice of what is left.
  const activePanes = useMemo(
    () => PANE_IDS.filter((id) => activeIndicators.includes(id) && indicators && indicators[id]).slice(0, MAX_PANES),
    [activeIndicators, indicators],
  );
  const selectedPaneCount = activeIndicators.filter((id) => PANE_IDS.includes(id)).length;
  const PANE_H = 62;
  const paneGap = 4;
  // Panes may never take more than half the plot: with four panes on a short chart the price
  // area shrank to the point where a small drag covered a huge price range and every drawing
  // was rejected. The price plot keeps at least half, always.
  const maxPaneSpace = Math.floor(plotH * 0.5);
  const wantedPanes = activePanes.length * (PANE_H + paneGap);
  const paneScaleFactor = wantedPanes > maxPaneSpace ? maxPaneSpace / wantedPanes : 1;
  const panesHeight = Math.round(wantedPanes * paneScaleFactor);
  const priceH = Math.max(120, plotH - panesHeight);
  // The indicator series are computed on the server's last 200 closed bars; the chart shows a
  // window of that. Align by taking the same tail, so index i means the same bar in both.
  const indOffset = useMemo(() => {
    const n = indicators?.bars ?? 0;
    return Math.max(0, n - bars.length);
  }, [indicators, bars.length]);
  const indAt = useCallback((series, i) => {
    if (!series) return null;
    const v = series[indOffset + i];
    return v === undefined ? null : v;
  }, [indOffset]);

  /** Current value of a pane's headline series, formatted for its own pane label. A pane
   *  whose number is left implicit forces the reader to eyeball the scale. */
  const paneReadout = useCallback((paneId) => {
    const i = bars.length - 1;
    if (i < 0) return '';
    const fmt = (v, d = 2) => (v === null || v === undefined ? '' : v.toFixed(d));
    if (paneId === 'rsi') return fmt(indAt(indicators?.rsi, i));
    if (paneId === 'macd') return fmt(indAt(indicators?.macd?.line, i), 3);
    if (paneId === 'stochastic') return `K ${fmt(indAt(indicators?.stochastic?.k, i), 1)}  D ${fmt(indAt(indicators?.stochastic?.d, i), 1)}`;
    if (paneId === 'adx') return `ADX ${fmt(indAt(indicators?.adx?.adx, i), 1)}  +DI ${fmt(indAt(indicators?.adx?.plusDI, i), 1)}  -DI ${fmt(indAt(indicators?.adx?.minusDI, i), 1)}`;
    if (paneId === 'volume') return fmt(indAt(indicators?.volume, i), 0);
    return '';
  }, [bars.length, indAt, indicators]);

  const { lo, hi } = useMemo(() => {
    if (!bars.length) return { lo: 0, hi: 1 };
    let mn = Infinity;
    let mx = -Infinity;
    for (const b of bars) { mn = Math.min(mn, b.l); mx = Math.max(mx, b.h); }
    for (const t of trades) {
      if (t.state !== 'OPEN') continue;
      for (const v of [t.entry, t.stop, t.target]) {
        if (Number.isFinite(v)) { mn = Math.min(mn, v); mx = Math.max(mx, v); }
      }
    }
    const pad = (mx - mn) * 0.08 || 1;
    return { lo: mn - pad, hi: mx + pad };
  }, [bars, trades]);

  const y = useCallback((v) => PAD.top + ((hi - v) / (hi - lo || 1)) * priceH, [hi, lo, priceH]);
  const xAt = useCallback((i) => PAD.left + i * cw + cw / 2, [cw]);

  const ticks = useMemo(() => {
    // Tick count follows the height the price plot actually has, and the label pitch is checked
    // after rounding: two ticks 20px apart printed "4,060.86" and "4,064.90" on top of each
    // other once the plot got short, because 11px text needs ~14px of clearance.
    const n = Math.max(3, Math.min(6, Math.round(priceH / 58)));
    const out = [];
    for (let i = 0; i <= n; i++) out.push(lo + ((hi - lo) * i) / n);
    const pitch = priceH / n;
    return pitch >= 16 ? out : out.filter((_, i) => i % 2 === 0);
  }, [lo, hi, priceH]);

  const firstTime = bars.length ? Date.parse(bars[0].t) : 0;
  const lastTime = bars.length ? Date.parse(bars[bars.length - 1].t) : 0;
  const span = lastTime - firstTime || 1;
  const isDaily = timeframe === 'D1' || timeframe === 'W1' || timeframe === 'MN1';
  const timeTicks = useMemo(() => {
    const out = [];
    for (let i = 0; i <= 6; i++) {
      const t = firstTime + (span * i) / 6;
      const idx = Math.round(((t - firstTime) / span) * (bars.length - 1));
      out.push({ t, x: xAt(Math.max(0, Math.min(bars.length - 1, idx))) });
    }
    return out;
  }, [firstTime, span, bars.length, xAt]);

  // Zones are anchored at the bar that produced them, so every band has a real origin.
  //
  // Selection matters more than drawing here. There are ~180 regions and only ~9 slots, and
  // they are overwhelmingly liquidity pools (97 of 179), which are magnets rather than turning
  // points. Sorting by score alone let those crowd out the support/resistance levels a trader
  // actually reads, so regions are ranked by category first and score second. Every drawn band
  // carries its name, and the footer reports what the cap left out.
  const visibleRegions = useMemo(() => {
    if (!showRegions || !bars.length) return [];
    return regions
      .filter((r) => r.zone && r.zone.high >= lo && r.zone.low <= hi)
      .map((r) => {
        const st = r.sourceTime ? Date.parse(r.sourceTime) : NaN;
        const frac = Number.isFinite(st) ? (st - firstTime) / span : 0;
        return { ...r, startIdx: Math.max(0, Math.min(bars.length - 1, Math.round(frac * (bars.length - 1)))) };
      })
      .sort((a, b) => {
        const ra = REGION_CATEGORY_ORDER.indexOf(a.engine);
        const rb = REGION_CATEGORY_ORDER.indexOf(b.engine);
        const ca = ra === -1 ? REGION_CATEGORY_ORDER.length : ra;
        const cb = rb === -1 ? REGION_CATEGORY_ORDER.length : rb;
        if (ca !== cb) return ca - cb;
        return b.score - a.score;
      })
      .slice(0, MAX_ZONES);
  }, [regions, showRegions, bars.length, firstTime, span, lo, hi]);

  const hotRegionData = useMemo(
    () => visibleRegions.find((r) => r.id === hotRegion) || null,
    [visibleRegions, hotRegion],
  );

  const hiddenZones = useMemo(() => {
    if (!showRegions || !bars.length) return 0;
    const inRange = regions.filter((r) => r.zone && r.zone.high >= lo && r.zone.low <= hi);
    return Math.max(0, inRange.length - visibleRegions.length);
  }, [regions, showRegions, bars.length, lo, hi, visibleRegions.length]);

  const last = bars[bars.length - 1];
  const hovered = hover !== null ? bars[hover] : null;
  const active = hovered || last;
  const bull = last ? last.c >= last.o : true;

  const openTrades = useMemo(() => trades.filter((t) => t.state === 'OPEN'), [trades]);
  const manualOpen = useMemo(() => openTrades.filter((t) => t.origin === 'manual'), [openTrades]);

  // Only a bounded number of trades is drawn. With ~70 open at once, no arrangement of
  // boxes or lines is readable - they simply overlap. Manual trades are always drawn
  // (there are few, and they are the operator's own), then the most recent engine trades
  // fill the remaining slots. The footer states how many are hidden, so a capped view is
  // never mistaken for the whole picture.
  const MAX_DRAWN = 12;
  const drawnTrades = useMemo(() => {
    if (openTrades.length <= MAX_DRAWN) return openTrades;
    const manual = openTrades.filter((t) => t.origin === 'manual');
    const engine = openTrades.filter((t) => t.origin !== 'manual');
    const room = Math.max(0, MAX_DRAWN - manual.length);
    const recentEngine = engine.slice(-room);
    return [...recentEngine, ...manual];
  }, [openTrades]);
  const hiddenTrades = openTrades.length - drawnTrades.length;

  // Which bar a trade was triggered on, as an index into the visible window. Used to
  // anchor a trade's risk box at its own start instead of running it across the plot.
  const barIndexFor = useCallback((iso) => {
    const ts = Date.parse(iso);
    if (!Number.isFinite(ts) || !bars.length) return 0;
    const frac = (ts - firstTime) / (span || 1);
    return Math.max(0, Math.min(bars.length - 1, Math.round(frac * (bars.length - 1))));
  }, [bars.length, firstTime, span]);

  // Exactly one trade is "focused" and gets its levels extended into full lines. The
  // focused trade is whichever one the pointer is over, falling back to the newest
  // manual trade, because that is the one the operator just placed.
  const focusedId = useMemo(() => {
    if (!drawnTrades.length) return null;
    if (hover !== null) {
      const cx = xAt(hover);
      const cy = y(bars[hover]?.c);
      let best = null;
      let bestD = Infinity;
      for (const t of drawnTrades) {
        const top = Math.min(y(t.entry), y(t.stop));
        const h = Math.max(1.5, Math.abs(y(t.stop) - y(t.entry)));
        const left = Math.max(PAD.left, xAt(barIndexFor(t.triggerBarTime)));
        if (cx < left - 4) continue;
        if (cy < top - 3 || cy > top + h + 3) continue;
        const d = Math.hypot(cx - left, cy - (top + h / 2));
        if (d < bestD) { bestD = d; best = t.id; }
      }
      if (best) return best;
    }
    const manual = drawnTrades.filter((t) => t.origin === 'manual');
    return (manual.length ? manual[manual.length - 1] : drawnTrades[drawnTrades.length - 1]).id;
  }, [drawnTrades, hover, bars, xAt, y, barIndexFor]);

  const focusedTrade = useMemo(
    () => drawnTrades.find((t) => t.id === focusedId) || null,
    [drawnTrades, focusedId],
  );

  // Legend lists only the engines actually drawn, so it never claims absent evidence.
  const legend = useMemo(() => {
    const seen = [...new Set(visibleRegions.map((r) => r.engine))];
    const zoneEntries = seen.map((e) => ({ engine: e, color: ENGINE_STROKE[e] || '#8ba4bf', label: ENGINE_AR[e] || e }));
    // Active overlays get a legend entry too, with the same colour the line is drawn in, so a
    // line on the chart is never unattributed.
    const overlayEntries = OVERLAY_IDS
      .filter((id) => activeIndicators.includes(id) && indicators && indicators[id])
      .map((id) => ({ engine: `ind:${id}`, color: IND_COLOR[id] || '#8ba4bf', label: INDICATOR_LABELS[id] || id }));
    return [...zoneEntries, ...overlayEntries];
  }, [visibleRegions, activeIndicators, indicators]);

  /**
   * Where each zone's name label goes, or null when it must be skipped.
   *
   * Several zones often sit at nearly the same price, and their labels were drawn on top of
   * each other into an unreadable pile - 39 collisions on a typical screen. Labels are placed
   * greedily in priority order (visibleRegions is already priority-sorted) and any label that
   * would collide with one already placed is dropped. The band is still drawn; only the word is
   * omitted, which is better than a stack of overlapping words.
   */

  const zoneLabelY = useMemo(() => {
    const placed = [];
    const out = new Map();
    const MIN_GAP = 13;
    for (const r of visibleRegions) {
      const yTop = y(Math.min(hi, r.zone.high));
      const x0 = Math.max(PAD.left, xAt(r.startIdx) - cw / 2);
      const w = Math.max(2, PAD.left + plotW - x0);
      if (w <= 62) { out.set(r.id, null); continue; }
      const ly = yTop + 11;
      if (placed.some((p) => Math.abs(p - ly) < MIN_GAP)) { out.set(r.id, null); continue; }
      placed.push(ly);
      out.set(r.id, ly);
    }
    return out;
  }, [visibleRegions, hi, lo, y, xAt, cw, plotW]);

  const onMove = useCallback((e) => {
    const rect = e.currentTarget.getBoundingClientRect();
    const i = Math.floor((e.clientX - rect.left) / cw);
    setHover(i >= 0 && i < bars.length ? i : null);
  }, [cw, bars.length]);

  /* ------------------------------------------------------ drawing a trade --
   *
   * The operator places three levels: entry, stop, target. Entry and stop come from
   * one drag on the candles; the direction follows from which side the stop lands on,
   * so a single downward drag means long and an upward drag means short. The target is
   * then placed automatically at the configured reward multiple and can be dragged
   * afterwards, which keeps the common case one gesture.
   */

  // Pixel -> price. Inverse of y(), so a drawn level is exactly where it was released.
  const priceAt = useCallback((py) => {
    const frac = (py - PAD.top) / (priceH || 1);
    return hi - frac * (hi - lo);
  }, [hi, lo, priceH]);

  /** Vertical scale for an indicator pane. Each pane has its own domain because RSI (0..100),
   *  MACD (unbounded around 0) and volume (0..max) cannot share one. A fixed domain is used
   *  where the definition has one, so the 70 line on RSI is always in the same place. */
  const paneScale = useCallback((paneId) => {
    const top = PAD.top + priceH + paneGap;
    const h = Math.round(PANE_H * paneScaleFactor);
    const bottom = top + h;
    const fixed = { rsi: [0, 100], stochastic: [0, 100], adx: [0, 100] };
    if (fixed[paneId]) {
      const [d0, d1] = fixed[paneId];
      return { top, bottom, toY: (v) => bottom - ((v - d0) / (d1 - d0)) * h };
    }
    let mn = Infinity;
    let mx = -Infinity;
    const consider = (v) => { if (v !== null && Number.isFinite(v)) { if (v < mn) mn = v; if (v > mx) mx = v; } };
    if (paneId === 'volume') {
      for (let i = 0; i < bars.length; i++) { consider(indAt(indicators?.volume, i)); consider(indAt(indicators?.volumeSma, i)); }
      if (mn === Infinity) mn = 0;
      mn = 0;
    } else if (paneId === 'macd') {
      for (let i = 0; i < bars.length; i++) {
        consider(indAt(indicators?.macd?.line, i));
        consider(indAt(indicators?.macd?.signal, i));
        consider(indAt(indicators?.macd?.hist, i));
      }
      const m = Math.max(Math.abs(mn === Infinity ? 1 : mn), Math.abs(mx === -Infinity ? 1 : mx)) || 1;
      mn = -m; mx = m;
    }
    if (mn === Infinity || mx === -Infinity) { mn = 0; mx = 1; }
    if (mn === mx) { mn -= 1; mx += 1; }
    const pad = (mx - mn) * 0.08;
    const d0 = mn - pad;
    const d1 = mx + pad;
    return { top, bottom, toY: (v) => bottom - ((v - d0) / (d1 - d0)) * h };
  }, [priceH, bars.length, indAt, indicators, paneScaleFactor]);

  /**
   * Which pane guide labels actually get printed, and where.
   *
   * Placement runs over every pane in one pass, top to bottom, keeping a label only when it is
   * at least 14px clear of the last one kept. De-cluttering per pane was not enough: RSI's 30
   * and the next pane's 20 sit either side of a 4px gap and printed on top of each other. The
   * guide lines are always drawn; only an unreadable number is dropped.
   */
  const paneGuides = useMemo(() => {
    const kept = new Map();
    const axisYs = [];
    let lastY = -Infinity;
    for (const paneId of activePanes) {
      const sc = paneScale(paneId);
      const rows = [];
      for (const g of PANE_GUIDES[paneId] || []) {
        const gy = sc.toY(g);
        axisYs.push(gy);
        if (gy - lastY < 14) continue;
        rows.push({ g, y: gy });
        lastY = gy;
      }
      kept.set(paneId, rows);
    }
    return { kept, axisYs };
  }, [activePanes, paneScale]);

  // Pointer -> price. The plot element is measured directly rather than through
  // e.currentTarget, because during a drag the events arrive from the window, whose
  // currentTarget is not an element at all.
  const pointerPrice = useCallback((e) => {
    const el = plotRef.current;
    if (!el) return null;
    const r = el.getBoundingClientRect();
    return priceAt(e.clientY - r.top);
  }, [priceAt]);

  const beginDrag = useCallback((e) => {
    if (!drawMode) return;
    e.preventDefault();
    const p = pointerPrice(e);
    if (p === null) return;
    setDrag({ kind: 'entryStop', entry: p, stop: p });
    setDraft(null);
    setNotice(null);
  }, [drawMode, pointerPrice]);

  const moveDrag = useCallback((e) => {
    if (!drag) return;
    const p = pointerPrice(e);
    if (p === null) return;
    if (drag.kind === 'entryStop') setDrag((d) => ({ ...d, stop: p }));
    else if (drag.kind === 'target') setDraft((d) => (d ? { ...d, target: p } : d));
  }, [drag, pointerPrice]);

  // While a level is being pulled, follow the pointer on the window rather than on one
  // element: the pointer crosses the svg, the readout panel and the drag handle, and a
  // per-element handler would drop the drag at each boundary.
  useEffect(() => {
    if (!drag) return undefined;
    const onUp = () => endDragRef.current?.();
    window.addEventListener('mousemove', moveDrag);
    window.addEventListener('mouseup', onUp);
    return () => {
      window.removeEventListener('mousemove', moveDrag);
      window.removeEventListener('mouseup', onUp);
    };
  }, [drag, moveDrag]);

  const endDrag = useCallback(() => {
    if (!drag) return;
    if (drag.kind === 'entryStop') {
      const entry = drag.entry;
      const stop = drag.stop;
      if (Math.abs(entry - stop) < 0.05) {
        // A click rather than a drag: nothing meaningful to place.
        setDrag(null);
        return;
      }
      const direction = stop < entry ? 'LONG' : 'SHORT';
      const risk = Math.abs(entry - stop);
      const target = direction === 'LONG' ? entry + risk * riskReward : entry - risk * riskReward;
      setDraft({ entry, stop, target, direction });
    }
    setDrag(null);
  }, [drag, riskReward]);
  endDragRef.current = endDrag;

  const riskPoints = draft ? Math.abs(draft.entry - draft.stop) : 0;
  const overRisk = riskPoints > maxRiskPoints;

  const confirmDraft = useCallback(async () => {
    if (!draft || !onCreateTrade) return;
    setSaving(true);
    const res = await onCreateTrade({
      direction: draft.direction,
      entry: draft.entry,
      stop: draft.stop,
      target: draft.target,
      timeframe,
    });
    setSaving(false);
    if (res?.ok) {
      setDraft(null);
      setDrag(null);
      setNotice({ tone: 'up', text: 'أُنشئت صفقة افتراضية يدوية — ظل فقط، بلا تنفيذ.' });
    } else {
      setNotice({ tone: 'down', text: res?.error || 'تعذّر إنشاء الصفقة.' });
    }
  }, [draft, onCreateTrade, timeframe]);

  const cancelDraft = useCallback(() => {
    setDraft(null);
    setDrag(null);
    setNotice(null);
  }, []);

  // Escape clears a draft before it reaches the fullscreen handler.
  useEffect(() => {
    const onKey = (e) => {
      if (e.key === 'Escape' && (draft || drag)) { e.stopPropagation(); cancelDraft(); }
    };
    window.addEventListener('keydown', onKey, true);
    return () => window.removeEventListener('keydown', onKey, true);
  }, [draft, drag, cancelDraft]);

  // Leaving draw mode with a draft in progress must not silently keep it.
  useEffect(() => {
    if (!drawMode) { setDraft(null); setDrag(null); }
  }, [drawMode]);

  const live = draft || (drag && drag.kind === 'entryStop' && Math.abs(drag.entry - drag.stop) >= 0.05
    ? (() => {
      const direction = drag.stop < drag.entry ? 'LONG' : 'SHORT';
      const risk = Math.abs(drag.entry - drag.stop);
      return {
        entry: drag.entry, stop: drag.stop, direction,
        target: direction === 'LONG' ? drag.entry + risk * riskReward : drag.entry - risk * riskReward,
      };
    })()
    : null);

  const stage = (
    <div
      dir="ltr"
      onWheel={onWheel}
      style={
        full
          ? { position: 'fixed', inset: 0, zIndex: 1000, background: '#000818', display: 'flex', flexDirection: 'column' }
          : { position: 'relative', width: '100%', background: 'var(--chart)', borderRadius: 3, overflow: 'hidden', display: 'flex', flexDirection: 'column' }
      }
    >
      {/* top bar: identity, timeframe, OHLC readout, zoom and fullscreen */}
      <div style={{
        display: 'flex', alignItems: 'center', gap: 10, padding: '5px 10px',
        borderBottom: '1px solid var(--line)', background: 'var(--surface)', flex: '0 0 auto', direction: 'rtl',
      }}>
        <span className="ltr num" style={{ fontSize: 'var(--t-small)', color: '#eaf1fa', fontWeight: 600 }}>XAUUSD</span>
        <span className="ltr num" style={{ fontSize: 'var(--t-small)', color: '#6d87a6' }}>{timeframe}</span>
        <span className="ltr num" style={{ fontSize: 'var(--t-small)', color: '#6d87a6' }}>
          {bars.length} / {data.length}
        </span>

        <div className="ltr" style={{ display: 'flex', gap: 10, fontSize: 'var(--t-small)', fontFamily: 'JetBrains Mono, monospace', marginInlineStart: 6 }}>
          <span style={{ color: '#6d87a6' }}>O <b style={{ color: '#eaf1fa' }}>{price(active?.o)}</b></span>
          <span style={{ color: '#6d87a6' }}>H <b style={{ color: '#eaf1fa' }}>{price(active?.h)}</b></span>
          <span style={{ color: '#6d87a6' }}>L <b style={{ color: '#eaf1fa' }}>{price(active?.l)}</b></span>
          <span style={{ color: '#6d87a6' }}>C <b style={{ color: (active?.c ?? 0) >= (active?.o ?? 0) ? UP : DOWN }}>{price(active?.c)}</b></span>
        </div>

        <div style={{ marginInlineStart: 'auto', display: 'flex', alignItems: 'center', gap: 6 }}>
          <button
            className="chart-btn"
            title="المؤشرات"
            onClick={() => setShowInd((v) => !v)}
            style={{
              width: 'auto', padding: '2px 9px', fontSize: 'var(--t-small)',
              background: showInd ? 'var(--blue)' : undefined,
              color: showInd ? '#fff' : undefined,
              borderColor: showInd ? 'var(--blue)' : undefined,
            }}
          >
            ⚙ المؤشرات{activeIndicators.length > 0 ? ` (${activeIndicators.length})` : ''}
          </button>
          <span style={{ width: 1, height: 16, background: '#143259' }} />
          {operator && (
            <>
              <button
                className="chart-btn"
                title="رسم صفقة افتراضية: اسحب على الشموع لتحديد الدخول والوقف"
                onClick={() => setDrawMode((v) => !v)}
                style={{
                  width: 'auto', padding: '2px 9px', fontSize: 'var(--t-small)',
                  background: drawMode ? 'var(--blue)' : undefined,
                  color: drawMode ? '#fff' : undefined,
                  borderColor: drawMode ? 'var(--blue)' : undefined,
                }}
              >
                {drawMode ? 'إلغاء الرسم' : '✎ ارسم صفقة'}
              </button>
              <span style={{ width: 1, height: 16, background: '#143259' }} />
            </>
          )}
          <button className="chart-btn" title="تصغير (شموع أكثر)" onClick={() => setSlot((s) => Math.max(2.5, s * 0.8))}>−</button>
          <button className="chart-btn" title="تكبير (شموع أقل)" onClick={() => setSlot((s) => Math.min(28, s * 1.25))}>+</button>
          <button className="chart-btn" title="إعادة ضبط العرض" onClick={() => setSlot(8)}>⟲</button>
          <button
            className="chart-btn"
            title={full ? 'إنهاء ملء الشاشة (Esc)' : 'ملء الشاشة'}
            onClick={() => setFull((v) => !v)}
          >
            {full ? '✕' : '⛶'}
          </button>
        </div>
      </div>

      {/* Indicator picker. Grouped into overlays and panes because they behave differently:
          an overlay draws over the candles, a pane takes its own strip. Each entry carries an
          info toggle that reveals the formula and its source, so a number can always be traced. */}
      {showInd && (
        <div dir="rtl" style={{
          padding: '7px 10px', background: 'var(--surface)', borderBottom: '1px solid #143259',
          display: 'flex', flexDirection: 'column', gap: 6, flex: '0 0 auto',
        }}>
          <div className="row" style={{ gap: 14, alignItems: 'flex-start', flexWrap: 'wrap' }}>
            <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
              <span className="caption" style={{ color: '#647f9b' }}>على السعر</span>
              <div className="row" style={{ gap: 4, flexWrap: 'wrap' }}>
                {OVERLAY_IDS.map((id) => (
                  <IndChip key={id} id={id} active={activeIndicators.includes(id)}
                    available={!!indicators?.[id]} def={indicatorDefs[id]}
                    onToggle={onToggleIndicator} onInfo={setIndInfo} />
                ))}
              </div>
            </div>
            <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
              <span className="caption" style={{ color: '#647f9b' }}>ألواح منفصلة</span>
              <div className="row" style={{ gap: 4, flexWrap: 'wrap' }}>
                {PANE_IDS.map((id) => (
                  <IndChip key={id} id={id} active={activeIndicators.includes(id)}
                    available={!!indicators?.[id]} def={indicatorDefs[id]}
                    onToggle={onToggleIndicator} onInfo={setIndInfo} />
                ))}
              </div>
            </div>
          </div>
          {indInfo && indicatorDefs[indInfo] && (
            <div style={{
              border: '1px solid #143259', borderRadius: 4, padding: '6px 9px',
              background: 'rgba(4,16,34,0.6)', display: 'flex', flexDirection: 'column', gap: 3,
            }}>
              <div className="row" style={{ gap: 8, alignItems: 'center' }}>
                <b style={{ fontSize: 'var(--t-small)', color: '#eaf1fa' }}>{indicatorDefs[indInfo].name}</b>
                <span className="caption" style={{ color: '#647f9b' }}>{indicatorDefs[indInfo].kind === 'pane' ? 'لوح منفصل' : 'على السعر'}</span>
                <button className="chart-btn" style={{ marginInlineStart: 'auto' }} onClick={() => setIndInfo(null)}>✕</button>
              </div>
              <span className="caption ltr" style={{ color: '#a6bcd8' }}>{indicatorDefs[indInfo].formula}</span>
              <span className="caption" style={{ color: '#647f9b' }}>المصدر: {indicatorDefs[indInfo].source}</span>
            </div>
          )}
          {selectedPaneCount > MAX_PANES && (
            <span className="caption" style={{ color: '#f3a33c' }}>
              اخترت {selectedPaneCount} ألواح؛ يُعرض {MAX_PANES} منها فقط ليبقى كل لوح مقروءاً.
            </span>
          )}
          <span className="caption" style={{ color: '#647f9b' }}>
            المؤشرات تُحسب على الشموع المغلقة فقط. حجم التداول هنا حجم تيك من المنصة، وهو تقريب وليس حجماً حقيقياً.
          </span>
        </div>
      )}

      {drawMode && !draft && !drag && (
        <div style={{
          padding: '5px 10px', background: 'rgba(26,108,240,0.12)', borderBottom: '1px solid #143259',
          fontSize: 'var(--t-small)', color: '#a6bcd8', direction: 'rtl', flex: '0 0 auto',
        }}>
          اسحب على الشموع: نقطة البداية هي الدخول، ونقطة النهاية هي الوقف. السحب لأسفل يعني شراء، وللأعلى يعني بيع.
          الهدف يُوضع تلقائياً عند <span className="num">{riskReward}</span> أضعاف المخاطرة ويمكن سحبه لاحقاً.
        </div>
      )}

      {notice && (
        <div style={{
          padding: '5px 10px', flex: '0 0 auto', direction: 'rtl',
          fontSize: 'var(--t-small)', borderBottom: '1px solid #143259',
          background: notice.tone === 'up' ? 'rgba(18,196,139,0.12)' : 'rgba(224,75,92,0.12)',
          color: notice.tone === 'up' ? '#12c48b' : '#f0a5a2',
        }}>
          {notice.text}
        </div>
      )}

      {/* In fullscreen the plot fills what is left; otherwise it is exactly `height` tall, so
          the panes always get their full share no matter how much chrome is showing. */}
      <div ref={plotRef} style={{
        position: 'relative', minHeight: 0, overflow: 'hidden',
        flex: full ? 1 : '0 0 auto',
        height: full ? undefined : height,
      }}>
        <svg width={size.w} height={size.h} style={{ display: 'block' }} onMouseLeave={() => setHover(null)}>
          <defs>
            <clipPath id={clipId}>
              <rect x={PAD.left} y={PAD.top} width={plotW} height={priceH} />
            </clipPath>
          </defs>

          {/* Grid: horizontal price lines only. The vertical time lines were removed -
              they crossed every candle and added nothing the time axis does not already
              say. Horizontal lines stay because price levels are what trades reference. */}
          {ticks.map((t, i) => (
            <line key={`g${i}`} x1={PAD.left} x2={PAD.left + plotW} y1={y(t)} y2={y(t)} stroke={GRID} strokeWidth={1} />
          ))}

          <g clipPath={`url(#${clipId})`}>
            {/* Zones, drawn as a bordered band with its name on it.
                Each band is labelled with what it IS - دعم, مقاومة, عرض, طلب, سيولة - because
                an unlabelled coloured stripe forces the reader to memorise a legend and then
                look it up. The band's left edge marks where the zone was born, so a band never
                looks like it has existed forever. Hovering a band explains the type in words. */}
            {visibleRegions.map((r) => {
              const yTop = y(Math.min(hi, r.zone.high));
              const yBot = y(Math.max(lo, r.zone.low));
              const h = Math.max(2, yBot - yTop);
              const stroke = ENGINE_STROKE[r.engine] || '#8ba4bf';
              const x0 = Math.max(PAD.left, xAt(r.startIdx) - cw / 2);
              const w = Math.max(2, PAD.left + plotW - x0);
              const label = TYPE_SHORT_AR[r.type] || r.type;
              const isHot = hotRegion === r.id;
              // Placement is decided up front so two names never land on the same row.
              const labelY = zoneLabelY.get(r.id) ?? null;
              return (
                <g key={r.id} onMouseEnter={() => setHotRegion(r.id)} onMouseLeave={() => setHotRegion(null)}>
                  <rect x={x0} y={yTop} width={w} height={h} fill={ENGINE_FILL[r.engine] || 'rgba(125,147,173,0.06)'} />
                  <rect x={x0} y={yTop} width={w} height={h} fill="none" stroke={stroke}
                    strokeWidth={isHot ? 1.4 : 0.9} strokeOpacity={isHot ? 0.95 : 0.5} />
                  <line x1={x0} x2={x0} y1={yTop} y2={yTop + h} stroke={stroke} strokeWidth={1.6} opacity={0.9} />
                  {labelY !== null && (
                    <text x={PAD.left + plotW - 6} y={labelY} textAnchor="end"
                      fill={stroke} fontSize={LABEL} opacity={isHot ? 1 : 0.9}
                      fontFamily="IBM Plex Sans Arabic, sans-serif" style={{ pointerEvents: 'none' }}>
                      {label}
                    </text>
                  )}
                </g>
              );
            })}

            {/* Open trades.
                A trade is a BOX from entry to stop, not three full-width lines. Full-width
                lines are what made this chart unreadable: with ~70 trades open that was 207
                hairlines competing with the candles. A box is bounded, so it shows where a
                trade sits without cutting the whole chart in half. Only the focused trade
                gets its levels extended, so the operator can read exact prices on demand. */}
            {drawnTrades.map((t) => {
              const isManual = t.origin === 'manual';
              const isFocus = focusedId === t.id;
              const long = t.direction === 'LONG';
              const yEntry = y(t.entry);
              const yStop = y(t.stop);
              const boxTop = Math.min(yEntry, yStop);
              const boxH = Math.max(1.5, Math.abs(yStop - yEntry));
              const right = PAD.left + plotW;
              // Risk box spans the trade's own recent life, not the whole plot.
              const startX = Math.max(PAD.left, xAt(barIndexFor(t.triggerBarTime)));
              const w = Math.max(6, right - startX);
              const col = long ? UP : DOWN;
              return (
                <g key={t.id} opacity={isFocus ? 1 : 0.75}>
                  {/* risk box, entry -> stop */}
                  <rect x={startX} y={boxTop} width={w} height={boxH} fill={col} opacity={isFocus ? 0.16 : 0.09} />
                  <rect x={startX} y={boxTop} width={w} height={boxH} fill="none" stroke={col}
                    strokeWidth={isManual ? 1.3 : 1} strokeOpacity={isFocus ? 0.95 : 0.55}
                    strokeDasharray={isManual ? '5 3' : undefined} />
                  {/* entry line only for the focused trade, so one price stays readable */}
                  {isFocus && (
                    <>
                      <line x1={PAD.left} x2={right} y1={yEntry} y2={yEntry} stroke={col} strokeWidth={1.1} opacity={0.9} />
                      <line x1={PAD.left} x2={right} y1={y(t.target)} y2={y(t.target)} stroke={UP} strokeWidth={1} strokeDasharray="4 3" opacity={0.7} />
                    </>
                  )}
                  {isManual && (
                    <>
                      <rect x={Math.min(startX + 3, PAD.left + plotW - 42)} y={boxTop - 8} width={40} height={15} rx={2} fill="rgba(0,8,24,0.85)" />
                      <text x={Math.min(startX + 23, PAD.left + plotW - 22)} y={boxTop + 3.5} textAnchor="middle" fill="#f3d36c"
                        fontSize={LABEL} fontFamily="IBM Plex Sans Arabic, sans-serif">يدوي</text>
                    </>
                  )}
                </g>
              );
            })}

            {/* the trade being drawn, before it is confirmed */}
            {live && (
              <g>
                <rect x={PAD.left} y={Math.min(y(live.entry), y(live.stop))}
                  width={plotW} height={Math.max(2, Math.abs(y(live.stop) - y(live.entry)))}
                  fill="rgba(224,75,92,0.10)" />
                <line x1={PAD.left} x2={PAD.left + plotW} y1={y(live.entry)} y2={y(live.entry)}
                  stroke="#f3d36c" strokeWidth={1.4} strokeDasharray="6 3" />
                <line x1={PAD.left} x2={PAD.left + plotW} y1={y(live.stop)} y2={y(live.stop)}
                  stroke={DOWN} strokeWidth={1.4} strokeDasharray="4 3" />
                <line x1={PAD.left} x2={PAD.left + plotW} y1={y(live.target)} y2={y(live.target)}
                  stroke={UP} strokeWidth={1.4} strokeDasharray="4 3" />
              </g>
            )}

            {/* Indicator overlays. Drawn before the candles so price stays the topmost
                element, and clipped to the price plot so they cannot bleed into a pane.
                Each series is split into contiguous runs, because a single path would join
                across the null gaps at the start of a series and draw a line to nowhere. */}
            {indicators && OVERLAY_IDS.filter((id) => activeIndicators.includes(id)).map((id) => {
              const xForIndex = (i) => xAt(i - indOffset);
              if (id === 'bollinger') {
                if (!indicators.bollinger) return null;
                const { mid, upper, lower } = indicators.bollinger;
                return (
                  <g key="bollinger">
                    <path d={seriesPath(upper, xForIndex, (v) => y(v))} fill="none" stroke={IND_COLOR.bollingerUpper} strokeWidth={1} opacity={0.55} />
                    <path d={seriesPath(lower, xForIndex, (v) => y(v))} fill="none" stroke={IND_COLOR.bollingerLower} strokeWidth={1} opacity={0.55} />
                    <path d={seriesPath(mid, xForIndex, (v) => y(v))} fill="none" stroke={IND_COLOR.bollingerMid} strokeWidth={1} strokeDasharray="4 3" opacity={0.7} />
                  </g>
                );
              }
              const series = indicators[id];
              if (!series) return null;
              return (
                <path key={id} d={seriesPath(series, xForIndex, (v) => y(v))} fill="none"
                  stroke={IND_COLOR[id] || '#8ba4bf'} strokeWidth={1.3} opacity={0.9} />
              );
            })}

            {/* candles */}
            {bars.map((b, i) => {
              const up = b.c >= b.o;
              const cx = xAt(i);
              const bw = Math.max(1.5, cw * 0.66);
              const top = y(Math.max(b.o, b.c));
              const bh = Math.max(1, Math.abs(y(b.o) - y(b.c)));
              const col = up ? UP : DOWN;
              const dim = hover !== null && hover !== i;
              return (
                <g key={b.t} opacity={dim ? 0.5 : 1}>
                  <line x1={cx} x2={cx} y1={y(b.h)} y2={y(b.l)} stroke={col} strokeWidth={1} />
                  <rect x={cx - bw / 2} y={top} width={bw} height={bh} fill={up ? col : '#081120'} stroke={col} strokeWidth={1} />
                </g>
              );
            })}
          </g>

          {/* axes drawn above the plot so they stay legible over bands */}
          {ticks.map((t, i) => {
            // The right column carries the price scale, the pane guide values and the last-price
            // badge, so a tick can land on either of the other two and print two numbers on one
            // row. Both are checked; the guide and the badge carry more meaning than a scale tick.
            const ty = y(t);
            if (paneGuides.axisYs.some((gy) => Math.abs(gy - ty) < 14)) return null;
            if (last && Math.abs(y(last.c) - ty) < 16) return null;
            return (
              <text key={`pt${i}`} x={size.w - 8} y={ty + 3.5} textAnchor="end"
                fill={AXIS_TEXT} fontSize={LABEL} fontFamily="JetBrains Mono, monospace">
                {price(t)}
              </text>
            );
          })}
          {timeTicks.map((tk, i) => (
            <text key={`tt${i}`} x={tk.x} y={size.h - 12} textAnchor="middle"
              fill={AXIS_TEXT} fontSize={LABEL} fontFamily="JetBrains Mono, monospace">
              {isDaily ? dateUTC(tk.t) : timeUTC(tk.t)}
            </text>
          ))}

          {/* last price marker */}
          {last && (
            <>
              <line x1={PAD.left} x2={PAD.left + plotW} y1={y(last.c)} y2={y(last.c)}
                stroke={bull ? UP : DOWN} strokeWidth={1} strokeDasharray="2 3" opacity={0.8} />
              <rect x={PAD.left + plotW + 4} y={y(last.c) - 9} width={74} height={18} rx={2} fill={bull ? UP : DOWN} />
              <text x={PAD.left + plotW + 10} y={y(last.c) + 4} fill="#000818" fontSize={LABEL}
                fontWeight={700} fontFamily="JetBrains Mono, monospace">
                {price(last.c)}
              </text>
            </>
          )}

          {/* crosshair */}
          {hover !== null && bars[hover] && (
            <>
              <line x1={xAt(hover)} x2={xAt(hover)} y1={PAD.top} y2={PAD.top + priceH} stroke="#7d93ad" strokeWidth={1} strokeDasharray="3 3" opacity={0.8} />
              <line x1={PAD.left} x2={PAD.left + plotW} y1={y(bars[hover].c)} y2={y(bars[hover].c)} stroke="#7d93ad" strokeWidth={1} strokeDasharray="3 3" opacity={0.5} />
              <rect x={PAD.left + plotW + 4} y={y(bars[hover].c) - 9} width={74} height={18} rx={2} fill="#1f3350" />
              <text x={PAD.left + plotW + 10} y={y(bars[hover].c) + 4} fill="#e8eef7" fontSize={LABEL}
                fontFamily="JetBrains Mono, monospace">
                {price(bars[hover].c)}
              </text>
              <text x={xAt(hover)} y={PAD.top - 8} textAnchor="middle" fill="#a8bdd6" fontSize={LABEL}
                fontFamily="JetBrains Mono, monospace">
                {isDaily ? dateUTC(Date.parse(bars[hover].t)) : timeUTC(Date.parse(bars[hover].t))}
              </text>
            </>
          )}

          {/* Indicator panes. Each gets its own strip below the price plot with its own scale,
              its own guides, and a label naming it and its current value - because a pane whose
              number you have to guess is not information. */}
          {activePanes.map((paneId, paneIdx) => {
            const effPaneH = Math.round(PANE_H * paneScaleFactor);
            const top = PAD.top + priceH + paneGap + paneIdx * (effPaneH + paneGap);
            const bottom = top + effPaneH;
            const sc = paneScale(paneId);
            const xForIndex = (i) => xAt(i - indOffset);
            const guides = PANE_GUIDES[paneId] || [];
            const label = INDICATOR_LABELS[paneId] || paneId;
            const readout = paneReadout(paneId);
            return (
              <g key={`pane-${paneId}`}>
                <rect x={PAD.left} y={top} width={plotW} height={effPaneH} fill="rgba(4,16,34,0.55)" stroke="#0f1f36" strokeWidth={1} />
                {guides.map((g) => (
                  <line key={g} x1={PAD.left} x2={PAD.left + plotW} y1={sc.toY(g)} y2={sc.toY(g)}
                    stroke={GRID} strokeWidth={1} strokeDasharray="2 4" />
                ))}
                {paneId === 'volume' ? (
                  <>
                    {bars.map((b, i) => {
                      const v = indAt(indicators?.volume, i);
                      if (v === null) return null;
                      const yv = sc.toY(v);
                      const up = b.c >= b.o;
                      const bw = Math.max(1.5, cw * 0.66);
                      return (
                        <rect key={b.t} x={xAt(i) - bw / 2} y={yv} width={bw} height={Math.max(0.5, bottom - yv)}
                          fill={up ? 'rgba(18,196,139,0.5)' : 'rgba(224,75,92,0.5)'} />
                      );
                    })}
                    <path d={seriesPath(indicators?.volumeSma, xForIndex, sc.toY)} fill="none" stroke={IND_COLOR.volumeSma} strokeWidth={1.1} opacity={0.9} />
                  </>
                ) : paneId === 'macd' ? (
                  <>
                    {bars.map((b, i) => {
                      const h = indAt(indicators?.macd?.hist, i);
                      if (h === null) return null;
                      const zero = sc.toY(0);
                      const yh = sc.toY(h);
                      const bw = Math.max(1.5, cw * 0.6);
                      return (
                        <rect key={b.t} x={xAt(i) - bw / 2} y={Math.min(zero, yh)} width={bw} height={Math.max(0.5, Math.abs(yh - zero))}
                          fill={h >= 0 ? IND_COLOR.macdHistUp : IND_COLOR.macdHistDown} />
                      );
                    })}
                    <path d={seriesPath(indicators?.macd?.line, xForIndex, sc.toY)} fill="none" stroke={IND_COLOR.macdLine} strokeWidth={1.2} />
                    <path d={seriesPath(indicators?.macd?.signal, xForIndex, sc.toY)} fill="none" stroke={IND_COLOR.macdSignal} strokeWidth={1.1} opacity={0.9} />
                  </>
                ) : paneId === 'stochastic' ? (
                  <>
                    <path d={seriesPath(indicators?.stochastic?.k, xForIndex, sc.toY)} fill="none" stroke={IND_COLOR.stochK} strokeWidth={1.2} />
                    <path d={seriesPath(indicators?.stochastic?.d, xForIndex, sc.toY)} fill="none" stroke={IND_COLOR.stochD} strokeWidth={1.1} opacity={0.9} />
                  </>
                ) : paneId === 'adx' ? (
                  <>
                    <path d={seriesPath(indicators?.adx?.plusDI, xForIndex, sc.toY)} fill="none" stroke={IND_COLOR.plusDI} strokeWidth={1} opacity={0.75} />
                    <path d={seriesPath(indicators?.adx?.minusDI, xForIndex, sc.toY)} fill="none" stroke={IND_COLOR.minusDI} strokeWidth={1} opacity={0.75} />
                    <path d={seriesPath(indicators?.adx?.adx, xForIndex, sc.toY)} fill="none" stroke={IND_COLOR.adx} strokeWidth={1.4} />
                  </>
                ) : (
                  <path d={seriesPath(indicators?.[paneId], xForIndex, sc.toY)} fill="none" stroke={IND_COLOR[paneId]} strokeWidth={1.3} />
                )}
                {/* pane identity and current value, top-left of the strip */}
                <text x={PAD.left + 6} y={top + 11} fill="#8ba4bf" fontSize={LABEL}
                  fontFamily="IBM Plex Sans Arabic, sans-serif">{label}</text>
                {readout && (
                  <text x={PAD.left + 6 + label.length * 6 + 10} y={top + 11} fill="#eaf1fa" fontSize={LABEL}
                    fontFamily="JetBrains Mono, monospace">{readout}</text>
                )}
                {/* right-hand guide labels, de-cluttered across all panes at once */}
                {(paneGuides.kept.get(paneId) || []).map(({ g, y }) => (
                  <text key={`gl${g}`} x={size.w - 8} y={y + 3.5} textAnchor="end"
                    fill={AXIS_TEXT} fontSize={LABEL} fontFamily="JetBrains Mono, monospace">{g}</text>
                ))}
              </g>
            );
          })}

          <rect x={PAD.left} y={PAD.top} width={plotW} height={priceH} fill="transparent"
            onMouseMove={onMove}
            onMouseDown={beginDrag}
            onMouseLeave={() => setHover(null)}
            style={{ cursor: drawMode ? 'crosshair' : 'default' }} />
        </svg>

        {/* Live readout of the trade being drawn, with its own confirm/cancel. */}
        {live && (
          <div dir="rtl" style={{
            position: 'absolute', top: PAD.top + 2, insetInlineStart: PAD.left + 6,
            background: 'rgba(0,8,24,0.9)', border: '1px solid #143259', borderRadius: 4,
            padding: '8px 10px', display: 'flex', flexDirection: 'column', gap: 6, minWidth: 220,
          }}>
            <div className="row" style={{ gap: 8 }}>
              <span className={`chip ${live.direction === 'LONG' ? 'up' : 'down'}`}>
                {live.direction === 'LONG' ? 'شراء' : 'بيع'}
              </span>
              <span className="caption">صفقة افتراضية يدوية</span>
            </div>
            <div className="num caption" style={{ display: 'grid', gridTemplateColumns: 'auto 1fr', gap: '3px 10px', direction: 'rtl' }}>
              <span className="caption">الدخول</span><span style={{ color: '#f3d36c' }}>{price(live.entry)}</span>
              <span className="caption">الوقف</span><span style={{ color: DOWN }}>{price(live.stop)}</span>
              <span className="caption">الهدف</span><span style={{ color: UP }}>{price(live.target)}</span>
              <span className="caption">المخاطرة</span>
              <span>{Math.abs(live.entry - live.stop).toFixed(2)} نقطة</span>
              <span className="caption">R:R</span>
              <span>{(Math.abs(live.target - live.entry) / (Math.abs(live.entry - live.stop) || 1)).toFixed(2)}</span>
            </div>
            {/* The server refuses a drawing riskier than its limit. Saying so here means the
                operator sees the problem while still holding the drag, not after confirming. */}
            {overRisk && (
              <div style={{ color: DOWN, fontSize: 'var(--t-small)' }}>
                المخاطرة {riskPoints.toFixed(1)} نقطة تتجاوز الحد المسموح {maxRiskPoints} نقطة — قرّب الوقف من الدخول.
              </div>
            )}
            {draft && (
              <div className="row" style={{ gap: 6, marginTop: 2 }}>
                <button className="chip up" onClick={confirmDraft} disabled={saving || overRisk}
                  title={overRisk ? 'المخاطرة تتجاوز الحد المسموح' : undefined}
                  style={{ padding: '5px 12px', opacity: overRisk ? 0.5 : 1 }}>
                  {saving ? 'جارٍ الحفظ…' : 'تأكيد الصفقة'}
                </button>
                <button className="chip dim" onClick={cancelDraft} style={{ padding: '5px 12px' }}>إلغاء</button>
              </div>
            )}
            {!draft && <span className="caption faint">أفلت لإنشاء المستويات</span>}
          </div>
        )}

        {/* Focused trade. Exactly one trade shows its numbers, so ~70 open trades never
            become ~70 overlapping labels. This is also where the operator acts on a
            manual trade, since a box on the chart is not a button. */}
        {!draft && focusedTrade && (
          <div dir="rtl" style={{
            position: 'absolute', top: PAD.top + 2, insetInlineStart: PAD.left + 6,
            background: 'rgba(0,8,24,0.88)', border: '1px solid #143259', borderRadius: 4,
            padding: '7px 9px', display: 'flex', flexDirection: 'column', gap: 5, minWidth: 200,
          }}>
            <div className="row" style={{ gap: 7, alignItems: 'center' }}>
              <span className={`chip ${focusedTrade.direction === 'LONG' ? 'up' : 'down'}`}>
                {focusedTrade.direction === 'LONG' ? 'شراء' : 'بيع'}
              </span>
              <span className="caption">{focusedTrade.origin === 'manual' ? 'يدوية' : (ENGINE_AR[focusedTrade.engine] || focusedTrade.engine)}</span>
              <span className="caption ltr" style={{ marginInlineStart: 'auto', color: '#647f9b' }}>{focusedTrade.timeframe}</span>
            </div>
            <div className="num caption" style={{ display: 'grid', gridTemplateColumns: 'auto 1fr', gap: '2px 10px' }}>
              <span className="caption">الدخول</span><span style={{ color: '#f3d36c' }}>{price(focusedTrade.entry)}</span>
              <span className="caption">الوقف</span><span style={{ color: DOWN }}>{price(focusedTrade.stop)}</span>
              <span className="caption">الهدف</span><span style={{ color: UP }}>{price(focusedTrade.target)}</span>
              <span className="caption">المخاطرة</span>
              <span>{Math.abs(focusedTrade.entry - focusedTrade.stop).toFixed(2)} نقطة</span>
            </div>
            {focusedTrade.origin === 'manual' && onCloseTrade && (
              <button
                className="chip dim"
                onClick={() => onCloseTrade(focusedTrade.id, last?.c)}
                style={{ padding: '4px 10px', fontSize: 'var(--t-small)' }}
              >
                إغلاق الصفقة الآن
              </button>
            )}
          </div>
        )}

        {/* Drag handle for the target, so the reward can be adjusted after placing. */}
        {draft && (
          <div
            onMouseDown={(e) => { e.preventDefault(); setDrag({ kind: 'target' }); }}
            title="اسحب لتعديل الهدف"
            style={{
              position: 'absolute', top: y(draft.target) - 6, insetInlineStart: PAD.left + plotW - 20,
              width: 14, height: 12, cursor: 'ns-resize',
              background: UP, borderRadius: 2, opacity: 0.9,
            }}
          />
        )}

        {/* Region tooltip. Names the type and explains what it means for price, so the
            abbreviation on the band is never the only thing the reader gets. */}
        {!draft && hotRegionData && (
          <div dir="rtl" style={{
            position: 'absolute', bottom: PAD.bottom + 8, insetInlineStart: PAD.left + 6,
            background: 'rgba(0,8,24,0.94)', border: '1px solid #143259', borderRadius: 4,
            padding: '6px 9px', maxWidth: 320, pointerEvents: 'none',
            display: 'flex', flexDirection: 'column', gap: 3,
          }}>
            <div className="row" style={{ gap: 6, alignItems: 'center' }}>
              <i style={{ width: 8, height: 8, borderRadius: 1, background: ENGINE_STROKE[hotRegionData.engine] || '#8ba4bf' }} />
              <b style={{ fontSize: 'var(--t-small)', color: '#eaf1fa' }}>{TYPE_AR[hotRegionData.type] || hotRegionData.type}</b>
              <span className="num caption" style={{ marginInlineStart: 'auto', color: '#647f9b' }}>
                {price(hotRegionData.zone.low)}–{price(hotRegionData.zone.high)}
              </span>
            </div>
            <span className="caption" style={{ color: '#a6bcd8' }}>
              {TYPE_MEANING_AR[hotRegionData.type] || ''}
            </span>
            <span className="caption" style={{ color: '#647f9b' }}>
              الدرجة <span className="num">{hotRegionData.score.toFixed(2)}</span>
              {' · '}{STATE_AR[hotRegionData.state] || hotRegionData.state}
              {hotRegionData.touches > 0 && <>{' · '}لمسات <span className="num">{hotRegionData.touches}</span></>}
            </span>
          </div>
        )}

        {/* legend: only engines actually on screen. Pinned to the plot's right edge,
            clear of the readout panels on the left. */}
        <div style={{
          position: 'absolute', top: PAD.top + 2, right: PAD.right + 8, display: 'flex', gap: 10,
          flexWrap: 'wrap', maxWidth: '52%', justifyContent: 'flex-end', pointerEvents: 'none', direction: 'rtl',
        }}>
          {legend.map((l) => (
            <span key={l.engine} className="caption" style={{ display: 'flex', alignItems: 'center', gap: 4, color: '#a6bcd8' }}>
              <i style={{ width: 8, height: 8, background: l.color, borderRadius: 1, opacity: 0.85 }} />
              {l.label}
            </span>
          ))}
        </div>

        {!bars.length && (
          <div className="empty" style={{ position: 'absolute', inset: 0, display: 'flex', flexDirection: 'column', justifyContent: 'center', gap: 6 }}>
            <div>لا توجد شموع مغلقة لعرضها</div>
            <div className="caption">TIMEFRAME: <span className="ltr">{timeframe}</span> · STATUS: NOT AVAILABLE</div>
          </div>
        )}
      </div>

      {/* bottom bar */}
      <div style={{
        display: 'flex', alignItems: 'center', gap: 10, padding: '4px 10px',
        borderTop: '1px solid #0f1f36', background: 'var(--surface)', flex: '0 0 auto', direction: 'rtl',
      }}>
        <button className="chart-btn" onClick={() => setShowRegions((v) => !v)} style={{ width: 'auto', padding: '2px 8px', fontSize: 'var(--t-small)' }}>
          {showRegions ? 'إخفاء المناطق' : 'إظهار المناطق'}
        </button>
        <span className="caption" style={{ color: '#6d87a6' }}>
          {visibleRegions.length > 0
            ? <>
              <span className="num">{visibleRegions.length}</span> منطقة معروضة
              {hiddenZones > 0 && <> · <span className="num">{hiddenZones}</span> مخفية</>}
            </>
            : 'لا مناطق في النطاق'}
        </span>
        {openTrades.length > 0 && (
          <span className="caption" style={{ color: '#6d87a6' }}>
            · <span className="num">{openTrades.length}</span> صفقة افتراضية مفتوحة
            {manualOpen.length > 0 && <> (منها <span className="num">{manualOpen.length}</span> يدوية)</>}
            {hiddenTrades > 0 && <> · يُعرض <span className="num">{drawnTrades.length}</span> منها، و<span className="num">{hiddenTrades}</span> مخفية</>}
          </span>
        )}
        {operator && manualOpen.length > 0 && (
          <select
            className="caption"
            value=""
            onChange={(e) => { if (e.target.value) onCloseTrade?.(e.target.value, last?.c); }}
            style={{ background: 'var(--surface)', border: '1px solid #143259', color: '#a6bcd8', borderRadius: 3, padding: '3px 6px' }}
          >
            <option value="">إغلاق صفقة يدوية…</option>
            {manualOpen.map((t) => (
              <option key={t.id} value={t.id}>
                {t.direction === 'LONG' ? 'شراء' : 'بيع'} {price(t.entry)}
              </option>
            ))}
          </select>
        )}
        <span className="caption ltr" style={{ marginInlineStart: 'auto', color: '#647f9b' }}>
          {drawMode ? 'draw mode: drag on the candles' : 'ctrl + scroll للتقريب'}
        </span>
      </div>
    </div>
  );

  if (full) return createPortal(stage, document.body);
  return stage;
}
