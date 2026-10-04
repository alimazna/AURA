import { useEffect, useRef, useState } from "react";
import type { Candle, Timeframe } from "@/lib/astra-data";

const PAD = { l: 8, r: 64, t: 12, b: 22 };

function fmtTime(t: number, tf: Timeframe) {
  const d = new Date(t);
  const p = (n: number) => String(n).padStart(2, "0");
  if (tf === "D1" || tf === "W1") return `${p(d.getUTCDate())}/${p(d.getUTCMonth() + 1)}`;
  if (tf === "MN1") return `${d.toLocaleString("en", { month: "short", timeZone: "UTC" })} ${String(d.getUTCFullYear()).slice(2)}`;
  return `${p(d.getUTCHours())}:${p(d.getUTCMinutes())}`;
}

export function CandlestickChart({ candles, timeframe }: { candles: Candle[]; timeframe: Timeframe }) {
  const ref = useRef<HTMLDivElement>(null);
  const [size, setSize] = useState({ w: 900, h: 440 });
  const [hover, setHover] = useState<number | null>(null);

  useEffect(() => {
    const el = ref.current;
    if (!el) return;
    const ro = new ResizeObserver((es) => { const e = es[0]; if (e) setSize({ w: e.contentRect.width, h: e.contentRect.height }); });
    ro.observe(el);
    return () => ro.disconnect();
  }, []);

  const pw = Math.max(50, size.w - PAD.l - PAD.r);
  const ph = Math.max(50, size.h - PAD.t - PAD.b);
  const slot = 9;
  const n = Math.min(candles.length, Math.max(20, Math.floor(pw / slot)));
  const data = candles.slice(-n);
  const cw = pw / n;
  const hi = Math.max(...data.map((c) => c.high));
  const lo = Math.min(...data.map((c) => c.low));
  const span = hi - lo || 1;
  const y = (v: number) => PAD.t + ((hi + span * 0.04 - v) / (span * 1.08)) * ph;
  const last = data[data.length - 1]!;
  const ticks = Array.from({ length: 7 }, (_, i) => lo + (span * i) / 6);
  const xTicks = data.map((_, i) => i).filter((i) => i % Math.ceil(n / 8) === 0);
  const h = (hover !== null ? data[hover] : last) ?? last;
  const bull = last.close >= last.open;

  return (
    <div ref={ref} className="relative h-full w-full select-none" onMouseLeave={() => setHover(null)}>
      <svg
        width={size.w}
        height={size.h}
        className="absolute inset-0"
        onMouseMove={(e) => {
          const x = e.clientX - e.currentTarget.getBoundingClientRect().left - PAD.l;
          const i = Math.floor(x / cw);
          setHover(i >= 0 && i < n ? i : null);
        }}
      >
        {ticks.map((t) => (
          <g key={t}>
            <line x1={PAD.l} x2={PAD.l + pw} y1={y(t)} y2={y(t)} className="stroke-border" strokeOpacity={0.5} />
            <text x={PAD.l + pw + 8} y={y(t) + 3} className="num fill-subtle" fontSize={10}>{t.toFixed(2)}</text>
          </g>
        ))}
        {xTicks.map((i) => (
          <g key={i}>
            <line x1={PAD.l + i * cw + cw / 2} x2={PAD.l + i * cw + cw / 2} y1={PAD.t} y2={PAD.t + ph} className="stroke-border" strokeOpacity={0.3} />
            <text x={PAD.l + i * cw + cw / 2} y={size.h - 6} textAnchor="middle" className="num fill-subtle" fontSize={10}>{fmtTime(data[i]!.time, timeframe)}</text>
          </g>
        ))}
        {data.map((c, i) => {
          const up = c.close >= c.open;
          const x = PAD.l + i * cw + cw / 2;
          const bw = Math.max(1, cw * 0.62);
          const top = y(Math.max(c.open, c.close));
          const bh = Math.max(1, Math.abs(y(c.open) - y(c.close)));
          const cls = up ? "fill-bull stroke-bull" : "fill-bear stroke-bear";
          return (
            <g key={c.time} className={cls} opacity={hover === null || hover === i ? 1 : 0.75}>
              <line x1={x} x2={x} y1={y(c.high)} y2={y(c.low)} strokeWidth={1} />
              <rect x={x - bw / 2} y={top} width={bw} height={bh} strokeWidth={0} />
            </g>
          );
        })}
        <line x1={PAD.l} x2={PAD.l + pw} y1={y(last.close)} y2={y(last.close)} className={bull ? "stroke-bull" : "stroke-bear"} strokeDasharray="2 3" />
        <rect x={PAD.l + pw + 2} y={y(last.close) - 9} width={60} height={18} className={bull ? "fill-bull" : "fill-bear"} />
        <text x={PAD.l + pw + 8} y={y(last.close) + 4} className="num fill-background" fontSize={10} fontWeight={600}>{last.close.toFixed(2)}</text>
        {hover !== null && (
          <line x1={PAD.l + hover * cw + cw / 2} x2={PAD.l + hover * cw + cw / 2} y1={PAD.t} y2={PAD.t + ph} className="stroke-subtle" strokeDasharray="3 3" />
        )}
      </svg>
      <div className="num pointer-events-none absolute left-3 top-2 flex gap-3 text-[11px] text-muted-foreground">
        <span>O <b className="font-medium text-foreground">{h.open.toFixed(2)}</b></span>
        <span>H <b className="font-medium text-foreground">{h.high.toFixed(2)}</b></span>
        <span>L <b className="font-medium text-foreground">{h.low.toFixed(2)}</b></span>
        <span>C <b className={h.close >= h.open ? "font-medium text-bull" : "font-medium text-bear"}>{h.close.toFixed(2)}</b></span>
      </div>
    </div>
  );
}
