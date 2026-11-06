// Candlestick chart with region overlays.
//
// Renders closed bars only — the server strips the forming bar before it reaches this
// component, and this component additionally ignores any bar flagged open. The chart is
// the dominant element of the dashboard, matching the reference layout.
//
// Region overlays are drawn as price bands coloured by the engine that discovered them,
// so an operator can tell at a glance which engine produced which zone. Live AI text is
// never drawn here: only structured, sourced evidence.

import { useEffect, useMemo, useRef, useState } from 'react';
import { price, dateUTC, timeUTC } from '../lib/format.js';

const PAD = { top: 26, right: 78, bottom: 26, left: 10 };

// Engine -> band colour. Kept restrained and distinguishable on the dark surface.
const ENGINE_COLOR = {
  liquidity: 'rgba(243,211,108,0.14)',
  levels: 'rgba(4,86,230,0.18)',
  supplyDemand: 'rgba(27,209,162,0.14)',
  structure: 'rgba(150,110,230,0.15)',
  imbalance: 'rgba(217,59,75,0.13)',
  breakout: 'rgba(77,141,255,0.15)',
};
const ENGINE_STROKE = {
  liquidity: 'rgba(243,211,108,0.5)',
  levels: 'rgba(77,141,255,0.55)',
  supplyDemand: 'rgba(27,209,162,0.5)',
  structure: 'rgba(150,110,230,0.5)',
  imbalance: 'rgba(217,59,75,0.48)',
  breakout: 'rgba(77,141,255,0.5)',
};

export function CandlestickChart({ candles, timeframe, regions = [], trades = [], height = 430 }) {
  const wrapRef = useRef(null);
  const [size, setSize] = useState({ w: 1000, h: height });
  const [hover, setHover] = useState(null);
  const [showRegions, setShowRegions] = useState(true);

  useEffect(() => {
    const el = wrapRef.current;
    if (!el) return undefined;
    const ro = new ResizeObserver((entries) => {
      const e = entries[0];
      if (e) setSize({ w: e.contentRect.width, h: e.contentRect.height });
    });
    ro.observe(el);
    return () => ro.disconnect();
  }, []);

  // Closed bars only, newest slice that fits the available width.
  const data = useMemo(() => (candles || []).filter((c) => c && Number.isFinite(c.h) && c.h > 0), [candles]);

  const plotW = Math.max(60, size.w - PAD.left - PAD.right);
  const plotH = Math.max(60, size.h - PAD.top - PAD.bottom);
  const slot = 8;
  const visible = Math.max(24, Math.floor(plotW / slot));
  const bars = data.slice(-visible);
  const cw = bars.length ? plotW / bars.length : plotW;

  const { lo, hi } = useMemo(() => {
    if (!bars.length) return { lo: 0, hi: 1 };
    let mn = Infinity;
    let mx = -Infinity;
    for (const b of bars) { mn = Math.min(mn, b.l); mx = Math.max(mx, b.h); }
    // include open trade levels so they are not clipped
    for (const t of trades) {
      if (t.state !== 'OPEN') continue;
      for (const v of [t.entry, t.stop, t.target]) { if (Number.isFinite(v)) { mn = Math.min(mn, v); mx = Math.max(mx, v); } }
    }
    const pad = (mx - mn) * 0.08 || 1;
    return { lo: mn - pad, hi: mx + pad };
  }, [bars, trades]);

  const y = (v) => PAD.top + ((hi - v) / (hi - lo || 1)) * plotH;
  const x = (i) => PAD.left + i * cw + cw / 2;

  const ticks = useMemo(() => {
    const out = [];
    for (let i = 0; i <= 6; i++) out.push(lo + ((hi - lo) * i) / 6);
    return out;
  }, [lo, hi]);

  const firstTime = bars.length ? Date.parse(bars[0].t) : 0;
  const lastTime = bars.length ? Date.parse(bars[bars.length - 1].t) : 0;
  const span = lastTime - firstTime || 1;
  const timeTicks = useMemo(() => {
    const out = [];
    const n = 6;
    for (let i = 0; i <= n; i++) {
      const t = firstTime + (span * i) / n;
      const idx = Math.round(((t - firstTime) / span) * (bars.length - 1));
      out.push({ t, x: x(Math.max(0, Math.min(bars.length - 1, idx))) });
    }
    return out;
  }, [firstTime, span, bars.length, cw, size.w]);

  const last = bars[bars.length - 1];
  const hovered = hover !== null ? bars[hover] : null;
  const bull = last ? last.c >= last.o : true;

  // Regions visible in the current price window, clipped to the plot.
  const visibleRegions = showRegions
    ? regions.filter((r) => r.zone && r.zone.high >= lo && r.zone.low <= hi).slice(0, 24)
    : [];

  return (
    <div style={{ position: 'relative', width: '100%', height: '100%' }} ref={wrapRef}>
      <svg width={size.w} height={size.h} style={{ display: 'block' }} onMouseLeave={() => setHover(null)}>
        {/* grid */}
        {ticks.map((t) => (
          <g key={`t${t}`}>
            <line x1={PAD.left} x2={PAD.left + plotW} y1={y(t)} y2={y(t)} stroke="#12233d" strokeWidth={1} />
            <text x={size.w - 6} y={y(t) + 3.5} textAnchor="end" fill="#486078" fontSize={10} fontFamily="JetBrains Mono, monospace">
              {price(t)}
            </text>
          </g>
        ))}
        {timeTicks.map((tk, i) => (
          <g key={`x${i}`}>
            <line x1={tk.x} x2={tk.x} y1={PAD.top} y2={PAD.top + plotH} stroke="#0e1c33" strokeWidth={1} />
            <text x={tk.x} y={size.h - 8} textAnchor="middle" fill="#486078" fontSize={10} fontFamily="JetBrains Mono, monospace">
              {timeframe === 'D1' || timeframe === 'W1' || timeframe === 'MN1' ? dateUTC(tk.t) : timeUTC(tk.t)}
            </text>
          </g>
        ))}

        {/* region bands */}
        {visibleRegions.map((r) => {
          const yTop = y(Math.min(hi, r.zone.high));
          const yBot = y(Math.max(lo, r.zone.low));
          const h = Math.max(1.5, yBot - yTop);
          return (
            <g key={r.id}>
              <rect x={PAD.left} y={yTop} width={plotW} height={h} fill={ENGINE_COLOR[r.engine] || 'rgba(120,140,170,0.12)'} />
              <line x1={PAD.left} x2={PAD.left + plotW} y1={yTop} y2={yTop} stroke={ENGINE_STROKE[r.engine] || 'rgba(120,140,170,0.5)'} strokeWidth={0.8} strokeDasharray="3 3" />
              <line x1={PAD.left} x2={PAD.left + plotW} y1={yBot} y2={yBot} stroke={ENGINE_STROKE[r.engine] || 'rgba(120,140,170,0.5)'} strokeWidth={0.8} strokeDasharray="3 3" />
            </g>
          );
        })}

        {/* open virtual trade levels */}
        {trades.filter((t) => t.state === 'OPEN').map((t) => (
          <g key={t.id}>
            <line x1={PAD.left} x2={PAD.left + plotW} y1={y(t.entry)} y2={y(t.entry)} stroke="#f3d36c" strokeWidth={1} strokeDasharray="5 4" />
            <line x1={PAD.left} x2={PAD.left + plotW} y1={y(t.stop)} y2={y(t.stop)} stroke="#d93b4b" strokeWidth={1} strokeDasharray="4 4" />
            <line x1={PAD.left} x2={PAD.left + plotW} y1={y(t.target)} y2={y(t.target)} stroke="#1bd1a2" strokeWidth={1} strokeDasharray="4 4" />
          </g>
        ))}

        {/* candles */}
        {bars.map((b, i) => {
          const up = b.c >= b.o;
          const cx = x(i);
          const bw = Math.max(1.4, cw * 0.62);
          const top = y(Math.max(b.o, b.c));
          const bh = Math.max(1, Math.abs(y(b.o) - y(b.c)));
          const col = up ? '#1bd1a2' : '#d93b4b';
          const dim = hover !== null && hover !== i;
          return (
            <g key={b.t} opacity={dim ? 0.55 : 1}>
              <line x1={cx} x2={cx} y1={y(b.h)} y2={y(b.l)} stroke={col} strokeWidth={1} />
              <rect x={cx - bw / 2} y={top} width={bw} height={bh} fill={up ? col : '#061124'} stroke={col} strokeWidth={1} />
            </g>
          );
        })}

        {/* last price marker */}
        {last && (
          <>
            <line x1={PAD.left} x2={PAD.left + plotW} y1={y(last.c)} y2={y(last.c)} stroke={bull ? '#1bd1a2' : '#d93b4b'} strokeWidth={1} strokeDasharray="2 3" opacity={0.85} />
            <rect x={PAD.left + plotW + 3} y={y(last.c) - 9} width={70} height={18} fill={bull ? '#1bd1a2' : '#d93b4b'} />
            <text x={PAD.left + plotW + 8} y={y(last.c) + 4} fill="#04132d" fontSize={10.5} fontWeight={600} fontFamily="JetBrains Mono, monospace">
              {price(last.c)}
            </text>
          </>
        )}

        {/* crosshair */}
        {hover !== null && bars[hover] && (
          <line x1={x(hover)} x2={x(hover)} y1={PAD.top} y2={PAD.top + plotH} stroke="#486078" strokeWidth={1} strokeDasharray="3 3" />
        )}

        {/* hit area */}
        <rect
          x={PAD.left} y={PAD.top} width={plotW} height={plotH} fill="transparent"
          onMouseMove={(e) => {
            const rect = e.currentTarget.getBoundingClientRect();
            const i = Math.floor((e.clientX - rect.left) / cw);
            setHover(i >= 0 && i < bars.length ? i : null);
          }}
        />
      </svg>

      {/* OHLC readout */}
      <div style={{ position: 'absolute', top: 4, right: 88, display: 'flex', gap: 12, fontSize: 11, fontFamily: 'JetBrains Mono, monospace', direction: 'ltr', pointerEvents: 'none' }}>
        <span style={{ color: '#6f8aa8' }}>O <b style={{ color: '#e8eef7' }}>{price((hovered || last)?.o)}</b></span>
        <span style={{ color: '#6f8aa8' }}>H <b style={{ color: '#e8eef7' }}>{price((hovered || last)?.h)}</b></span>
        <span style={{ color: '#6f8aa8' }}>L <b style={{ color: '#e8eef7' }}>{price((hovered || last)?.l)}</b></span>
        <span style={{ color: '#6f8aa8' }}>C <b style={{ color: ((hovered || last)?.c ?? 0) >= ((hovered || last)?.o ?? 0) ? '#1bd1a2' : '#d93b4b' }}>{price((hovered || last)?.c)}</b></span>
      </div>

      <div style={{ position: 'absolute', bottom: 4, left: 12, display: 'flex', gap: 10, alignItems: 'center' }}>
        <button className="chip dim" onClick={() => setShowRegions((v) => !v)} style={{ fontSize: 9.5 }}>
          {showRegions ? 'إخفاء المناطق' : 'إظهار المناطق'}
        </button>
        {visibleRegions.length > 0 && (
          <span className="tiny faint ltr">{visibleRegions.length} منطقة معروضة</span>
        )}
      </div>

      {!bars.length && (
        <div className="empty" style={{ position: 'absolute', inset: 0, display: 'flex', flexDirection: 'column', justifyContent: 'center', gap: 6 }}>
          <div>لا توجد شموع مغلقة لعرضها</div>
          <div className="tiny">TIMEFRAME: <span className="ltr">{timeframe}</span> · STATUS: NOT AVAILABLE</div>
        </div>
      )}
    </div>
  );
}
