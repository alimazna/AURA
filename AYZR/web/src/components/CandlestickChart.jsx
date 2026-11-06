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
import { price, dateUTC, timeUTC, ENGINE_AR } from '../lib/format.js';

const PAD = { top: 30, right: 86, bottom: 34, left: 14 };

// Zone styling. The border carries engine identity; the fill stays almost transparent so
// overlapping zones never turn the chart into a wash.
const ENGINE_STROKE = {
  liquidity: '#f3d36c',
  levels: '#4d8dff',
  supplyDemand: '#1bd1a2',
  structure: '#966ee6',
  imbalance: '#d93b4b',
  breakout: '#4d8dff',
};
const ENGINE_FILL = {
  liquidity: 'rgba(243,211,108,0.07)',
  levels: 'rgba(77,141,255,0.07)',
  supplyDemand: 'rgba(27,209,162,0.07)',
  structure: 'rgba(150,110,230,0.07)',
  imbalance: 'rgba(217,59,75,0.06)',
  breakout: 'rgba(77,141,255,0.07)',
};

const UP = '#1bd1a2';
const DOWN = '#d93b4b';
const GRID = '#0f1f36';
const AXIS_TEXT = '#5b7691';

export function CandlestickChart({ candles, timeframe, regions = [], trades = [], height = 440 }) {
  const plotRef = useRef(null);
  const clipId = useId();
  const [size, setSize] = useState({ w: 1000, h: height });
  const [hover, setHover] = useState(null);
  const [showRegions, setShowRegions] = useState(true);
  const [full, setFull] = useState(false);
  // slots = pixels per bar. Larger means more zoomed in.
  const [slot, setSlot] = useState(8);

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

  const y = useCallback((v) => PAD.top + ((hi - v) / (hi - lo || 1)) * plotH, [hi, lo, plotH]);
  const xAt = useCallback((i) => PAD.left + i * cw + cw / 2, [cw]);

  const ticks = useMemo(() => {
    const out = [];
    for (let i = 0; i <= 6; i++) out.push(lo + ((hi - lo) * i) / 6);
    return out;
  }, [lo, hi]);

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
  const visibleRegions = useMemo(() => {
    if (!showRegions || !bars.length) return [];
    return regions
      .filter((r) => r.zone && r.zone.high >= lo && r.zone.low <= hi)
      .map((r) => {
        const st = r.sourceTime ? Date.parse(r.sourceTime) : NaN;
        const frac = Number.isFinite(st) ? (st - firstTime) / span : 0;
        return { ...r, startIdx: Math.max(0, Math.min(bars.length - 1, Math.round(frac * (bars.length - 1)))) };
      })
      .sort((a, b) => b.score - a.score)
      .slice(0, 16);
  }, [regions, showRegions, bars.length, firstTime, span, lo, hi]);

  const last = bars[bars.length - 1];
  const hovered = hover !== null ? bars[hover] : null;
  const active = hovered || last;
  const bull = last ? last.c >= last.o : true;

  const openTrades = useMemo(() => trades.filter((t) => t.state === 'OPEN'), [trades]);

  // Legend lists only the engines actually drawn, so it never claims absent evidence.
  const legend = useMemo(() => {
    const seen = [...new Set(visibleRegions.map((r) => r.engine))];
    return seen.map((e) => ({ engine: e, color: ENGINE_STROKE[e] || '#7d93ad', label: ENGINE_AR[e] || e }));
  }, [visibleRegions]);

  const onMove = useCallback((e) => {
    const rect = e.currentTarget.getBoundingClientRect();
    const i = Math.floor((e.clientX - rect.left) / cw);
    setHover(i >= 0 && i < bars.length ? i : null);
  }, [cw, bars.length]);

  const stage = (
    <div
      dir="ltr"
      onWheel={onWheel}
      style={
        full
          ? { position: 'fixed', inset: 0, zIndex: 1000, background: '#04132d', display: 'flex', flexDirection: 'column' }
          : { position: 'relative', width: '100%', height, background: '#081120', borderRadius: 3, overflow: 'hidden', display: 'flex', flexDirection: 'column' }
      }
    >
      {/* top bar: identity, timeframe, OHLC readout, zoom and fullscreen */}
      <div style={{
        display: 'flex', alignItems: 'center', gap: 10, padding: '5px 10px',
        borderBottom: '1px solid #0f1f36', background: '#061124', flex: '0 0 auto', direction: 'rtl',
      }}>
        <span className="ltr num" style={{ fontSize: 11, color: '#e8eef7', fontWeight: 600 }}>XAUUSD</span>
        <span className="ltr num" style={{ fontSize: 10.5, color: '#5b7691' }}>{timeframe}</span>
        <span className="ltr num" style={{ fontSize: 10, color: '#486078' }}>
          {bars.length} / {data.length}
        </span>

        <div className="ltr" style={{ display: 'flex', gap: 10, fontSize: 11, fontFamily: 'JetBrains Mono, monospace', marginInlineStart: 6 }}>
          <span style={{ color: '#5b7691' }}>O <b style={{ color: '#e8eef7' }}>{price(active?.o)}</b></span>
          <span style={{ color: '#5b7691' }}>H <b style={{ color: '#e8eef7' }}>{price(active?.h)}</b></span>
          <span style={{ color: '#5b7691' }}>L <b style={{ color: '#e8eef7' }}>{price(active?.l)}</b></span>
          <span style={{ color: '#5b7691' }}>C <b style={{ color: (active?.c ?? 0) >= (active?.o ?? 0) ? UP : DOWN }}>{price(active?.c)}</b></span>
        </div>

        <div style={{ marginInlineStart: 'auto', display: 'flex', alignItems: 'center', gap: 6 }}>
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

      <div ref={plotRef} style={{ position: 'relative', flex: 1, minHeight: 0, overflow: 'hidden' }}>
        <svg width={size.w} height={size.h} style={{ display: 'block' }} onMouseLeave={() => setHover(null)}>
          <defs>
            <clipPath id={clipId}>
              <rect x={PAD.left} y={PAD.top} width={plotW} height={plotH} />
            </clipPath>
          </defs>

          {/* faint grid, behind everything */}
          {ticks.map((t, i) => (
            <line key={`g${i}`} x1={PAD.left} x2={PAD.left + plotW} y1={y(t)} y2={y(t)} stroke={GRID} strokeWidth={1} />
          ))}
          {timeTicks.map((tk, i) => (
            <line key={`v${i}`} x1={tk.x} x2={tk.x} y1={PAD.top} y2={PAD.top + plotH} stroke={GRID} strokeWidth={1} />
          ))}

          <g clipPath={`url(#${clipId})`}>
            {/* zone bands, anchored at their origin bar */}
            {visibleRegions.map((r) => {
              const yTop = y(Math.min(hi, r.zone.high));
              const yBot = y(Math.max(lo, r.zone.low));
              const h = Math.max(2, yBot - yTop);
              const stroke = ENGINE_STROKE[r.engine] || '#7d93ad';
              const x0 = Math.max(PAD.left, xAt(r.startIdx) - cw / 2);
              const w = Math.max(2, PAD.left + plotW - x0);
              return (
                <g key={r.id}>
                  <rect x={x0} y={yTop} width={w} height={h} fill={ENGINE_FILL[r.engine] || 'rgba(125,147,173,0.06)'} />
                  <line x1={x0} x2={PAD.left + plotW} y1={yTop} y2={yTop} stroke={stroke} strokeWidth={0.9} opacity={0.75} />
                  <line x1={x0} x2={PAD.left + plotW} y1={yBot} y2={yBot} stroke={stroke} strokeWidth={0.9} opacity={0.75} />
                  <line x1={x0} x2={x0} y1={yTop} y2={yTop + h} stroke={stroke} strokeWidth={1.4} opacity={0.95} />
                </g>
              );
            })}

            {/* open virtual trade levels */}
            {openTrades.map((t) => (
              <g key={t.id}>
                <line x1={PAD.left} x2={PAD.left + plotW} y1={y(t.entry)} y2={y(t.entry)} stroke="#f3d36c" strokeWidth={1} strokeDasharray="5 4" opacity={0.9} />
                <line x1={PAD.left} x2={PAD.left + plotW} y1={y(t.stop)} y2={y(t.stop)} stroke={DOWN} strokeWidth={1} strokeDasharray="4 4" opacity={0.75} />
                <line x1={PAD.left} x2={PAD.left + plotW} y1={y(t.target)} y2={y(t.target)} stroke={UP} strokeWidth={1} strokeDasharray="4 4" opacity={0.75} />
              </g>
            ))}

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
          {ticks.map((t, i) => (
            <text key={`pt${i}`} x={size.w - 8} y={y(t) + 3.5} textAnchor="end"
              fill={AXIS_TEXT} fontSize={10} fontFamily="JetBrains Mono, monospace">
              {price(t)}
            </text>
          ))}
          {timeTicks.map((tk, i) => (
            <text key={`tt${i}`} x={tk.x} y={size.h - 12} textAnchor="middle"
              fill={AXIS_TEXT} fontSize={10} fontFamily="JetBrains Mono, monospace">
              {isDaily ? dateUTC(tk.t) : timeUTC(tk.t)}
            </text>
          ))}

          {/* last price marker */}
          {last && (
            <>
              <line x1={PAD.left} x2={PAD.left + plotW} y1={y(last.c)} y2={y(last.c)}
                stroke={bull ? UP : DOWN} strokeWidth={1} strokeDasharray="2 3" opacity={0.8} />
              <rect x={PAD.left + plotW + 4} y={y(last.c) - 9} width={74} height={18} rx={2} fill={bull ? UP : DOWN} />
              <text x={PAD.left + plotW + 10} y={y(last.c) + 4} fill="#04132d" fontSize={10.5}
                fontWeight={700} fontFamily="JetBrains Mono, monospace">
                {price(last.c)}
              </text>
            </>
          )}

          {/* crosshair */}
          {hover !== null && bars[hover] && (
            <>
              <line x1={xAt(hover)} x2={xAt(hover)} y1={PAD.top} y2={PAD.top + plotH} stroke="#7d93ad" strokeWidth={1} strokeDasharray="3 3" opacity={0.8} />
              <line x1={PAD.left} x2={PAD.left + plotW} y1={y(bars[hover].c)} y2={y(bars[hover].c)} stroke="#7d93ad" strokeWidth={1} strokeDasharray="3 3" opacity={0.5} />
              <rect x={PAD.left + plotW + 4} y={y(bars[hover].c) - 9} width={74} height={18} rx={2} fill="#1f3350" />
              <text x={PAD.left + plotW + 10} y={y(bars[hover].c) + 4} fill="#e8eef7" fontSize={10.5}
                fontFamily="JetBrains Mono, monospace">
                {price(bars[hover].c)}
              </text>
              <text x={xAt(hover)} y={PAD.top - 8} textAnchor="middle" fill="#a8bdd6" fontSize={10}
                fontFamily="JetBrains Mono, monospace">
                {isDaily ? dateUTC(Date.parse(bars[hover].t)) : timeUTC(Date.parse(bars[hover].t))}
              </text>
            </>
          )}

          <rect x={PAD.left} y={PAD.top} width={plotW} height={plotH} fill="transparent"
            onMouseMove={onMove} style={{ cursor: 'crosshair' }} />
        </svg>

        {/* legend: only engines actually on screen */}
        <div style={{
          position: 'absolute', top: PAD.top + 2, insetInlineStart: PAD.left + 6, display: 'flex', gap: 10,
          flexWrap: 'wrap', maxWidth: '62%', pointerEvents: 'none', direction: 'rtl',
        }}>
          {legend.map((l) => (
            <span key={l.engine} className="tiny" style={{ display: 'flex', alignItems: 'center', gap: 4, color: '#8ba4bf' }}>
              <i style={{ width: 8, height: 8, background: l.color, borderRadius: 1, opacity: 0.85 }} />
              {l.label}
            </span>
          ))}
        </div>

        {!bars.length && (
          <div className="empty" style={{ position: 'absolute', inset: 0, display: 'flex', flexDirection: 'column', justifyContent: 'center', gap: 6 }}>
            <div>لا توجد شموع مغلقة لعرضها</div>
            <div className="tiny">TIMEFRAME: <span className="ltr">{timeframe}</span> · STATUS: NOT AVAILABLE</div>
          </div>
        )}
      </div>

      {/* bottom bar */}
      <div style={{
        display: 'flex', alignItems: 'center', gap: 10, padding: '4px 10px',
        borderTop: '1px solid #0f1f36', background: '#061124', flex: '0 0 auto', direction: 'rtl',
      }}>
        <button className="chart-btn" onClick={() => setShowRegions((v) => !v)} style={{ width: 'auto', padding: '2px 8px', fontSize: 10 }}>
          {showRegions ? 'إخفاء المناطق' : 'إظهار المناطق'}
        </button>
        <span className="tiny" style={{ color: '#486078' }}>
          {visibleRegions.length > 0
            ? <><span className="num">{visibleRegions.length}</span> منطقة معروضة</>
            : 'لا مناطق في النطاق'}
        </span>
        {openTrades.length > 0 && (
          <span className="tiny" style={{ color: '#486078' }}>
            · <span className="num">{openTrades.length}</span> صفقة افتراضية مفتوحة
          </span>
        )}
        <span className="tiny ltr" style={{ marginInlineStart: 'auto', color: '#3d5470' }}>
          ctrl + scroll للتقريب
        </span>
      </div>
    </div>
  );

  if (full) return createPortal(stage, document.body);
  return stage;
}
