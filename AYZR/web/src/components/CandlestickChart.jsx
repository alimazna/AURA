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

const UP = '#12c48b';
const DOWN = '#e04b5c';
const GRID = '#0a1a30';
const AXIS_TEXT = '#6d87a6';
// One label size for every piece of chart text, so the plot never looks like a pile of
// slightly different type sizes.
const LABEL = 11;

export function CandlestickChart({
  candles, timeframe, regions = [], trades = [], height = 440,
  operator = false, onCreateTrade, onCloseTrade, riskReward = 2,
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
  const manualOpen = useMemo(() => openTrades.filter((t) => t.origin === 'manual'), [openTrades]);

  // Legend lists only the engines actually drawn, so it never claims absent evidence.
  const legend = useMemo(() => {
    const seen = [...new Set(visibleRegions.map((r) => r.engine))];
    return seen.map((e) => ({ engine: e, color: ENGINE_STROKE[e] || '#8ba4bf', label: ENGINE_AR[e] || e }));
  }, [visibleRegions]);

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
    const frac = (py - PAD.top) / (plotH || 1);
    return hi - frac * (hi - lo);
  }, [hi, lo, plotH]);

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
          : { position: 'relative', width: '100%', height, background: '#000a1c', borderRadius: 3, overflow: 'hidden', display: 'flex', flexDirection: 'column' }
      }
    >
      {/* top bar: identity, timeframe, OHLC readout, zoom and fullscreen */}
      <div style={{
        display: 'flex', alignItems: 'center', gap: 10, padding: '5px 10px',
        borderBottom: '1px solid #0f1f36', background: '#00142a', flex: '0 0 auto', direction: 'rtl',
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
              const stroke = ENGINE_STROKE[r.engine] || '#8ba4bf';
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

            {/* open virtual trade levels. Manual trades are drawn with a label so a
                hand-placed level is never mistaken for an engine's own level. */}
            {openTrades.map((t) => (
              <g key={t.id}>
                <line x1={PAD.left} x2={PAD.left + plotW} y1={y(t.entry)} y2={y(t.entry)}
                  stroke="#f3d36c" strokeWidth={t.origin === 'manual' ? 1.4 : 1} strokeDasharray="5 4" opacity={0.9} />
                <line x1={PAD.left} x2={PAD.left + plotW} y1={y(t.stop)} y2={y(t.stop)}
                  stroke={DOWN} strokeWidth={1} strokeDasharray="4 4" opacity={0.75} />
                <line x1={PAD.left} x2={PAD.left + plotW} y1={y(t.target)} y2={y(t.target)}
                  stroke={UP} strokeWidth={1} strokeDasharray="4 4" opacity={0.75} />
                {t.origin === 'manual' && (
                  <>
                    <rect x={PAD.left + 4} y={y(t.entry) - 8} width={46} height={15} rx={2} fill="rgba(243,211,108,0.16)" />
                    <text x={PAD.left + 27} y={y(t.entry) + 3.5} textAnchor="middle" fill="#f3d36c"
                      fontSize={LABEL} fontFamily="IBM Plex Sans Arabic, sans-serif">يدوي</text>
                  </>
                )}
              </g>
            ))}

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
              fill={AXIS_TEXT} fontSize={LABEL} fontFamily="JetBrains Mono, monospace">
              {price(t)}
            </text>
          ))}
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
              <line x1={xAt(hover)} x2={xAt(hover)} y1={PAD.top} y2={PAD.top + plotH} stroke="#7d93ad" strokeWidth={1} strokeDasharray="3 3" opacity={0.8} />
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

          <rect x={PAD.left} y={PAD.top} width={plotW} height={plotH} fill="transparent"
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
            {draft && (
              <div className="row" style={{ gap: 6, marginTop: 2 }}>
                <button className="chip up" onClick={confirmDraft} disabled={saving} style={{ padding: '5px 12px' }}>
                  {saving ? 'جارٍ الحفظ…' : 'تأكيد الصفقة'}
                </button>
                <button className="chip dim" onClick={cancelDraft} style={{ padding: '5px 12px' }}>إلغاء</button>
              </div>
            )}
            {!draft && <span className="caption faint">أفلت لإنشاء المستويات</span>}
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

        {/* legend: only engines actually on screen */}
        <div style={{
          position: 'absolute', top: PAD.top + 2, insetInlineStart: PAD.left + 6, display: 'flex', gap: 10,
          flexWrap: 'wrap', maxWidth: '62%', pointerEvents: 'none', direction: 'rtl',
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
        borderTop: '1px solid #0f1f36', background: '#00142a', flex: '0 0 auto', direction: 'rtl',
      }}>
        <button className="chart-btn" onClick={() => setShowRegions((v) => !v)} style={{ width: 'auto', padding: '2px 8px', fontSize: 'var(--t-small)' }}>
          {showRegions ? 'إخفاء المناطق' : 'إظهار المناطق'}
        </button>
        <span className="caption" style={{ color: '#6d87a6' }}>
          {visibleRegions.length > 0
            ? <><span className="num">{visibleRegions.length}</span> منطقة معروضة</>
            : 'لا مناطق في النطاق'}
        </span>
        {openTrades.length > 0 && (
          <span className="caption" style={{ color: '#6d87a6' }}>
            · <span className="num">{openTrades.length}</span> صفقة افتراضية مفتوحة
            {manualOpen.length > 0 && <> (منها <span className="num">{manualOpen.length}</span> يدوية)</>}
          </span>
        )}
        {operator && manualOpen.length > 0 && (
          <select
            className="caption"
            value=""
            onChange={(e) => { if (e.target.value) onCloseTrade?.(e.target.value, last?.c); }}
            style={{ background: '#00142a', border: '1px solid #143259', color: '#a6bcd8', borderRadius: 3, padding: '3px 6px' }}
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
