import { createFileRoute } from "@tanstack/react-router";
import { useEffect, useMemo, useState, type ReactNode } from "react";
import {
  LayoutGrid, LineChart, Layers, Radio, ShieldCheck, Ghost, Eye, FlaskConical, BookOpen,
  ListChecks, BadgeCheck, Stamp, Landmark, GitBranch, AlertTriangle, CalendarClock,
  LifeBuoy, ScrollText, SlidersHorizontal, Power, Home, Activity, Cpu,
} from "lucide-react";
import markImg from "@/assets/astra-mark.png";
import { CandlestickChart } from "@/components/astra/CandlestickChart";
import { MATRIX, SIGNALS, TF_ROLE, TIMEFRAMES, mockMarket, type State, type Timeframe } from "@/lib/astra-data";

export const Route = createFileRoute("/")({
  head: () => ({
    meta: [
      { title: "ASTRA Terminal — XAUUSD Market Intelligence" },
      { name: "description", content: "ASTRA workstation: XAUUSD candlestick chart, nine-timeframe intelligence, shadow signals and risk. Shadow only." },
      { property: "og:title", content: "ASTRA Terminal — XAUUSD Market Intelligence" },
      { property: "og:description", content: "Institutional XAUUSD market-intelligence workstation. Shadow only." },
      { property: "og:type", content: "website" },
      { name: "twitter:card", content: "summary_large_image" },
    ],
  }),
  component: AstraShell,
});

type Section =
  | "Dashboard" | "Market" | "Timeframes" | "Signals" | "Risk" | "Shadow Positions" | "Observation"
  | "Research" | "Knowledge" | "Candidates" | "Validation"
  | "Approval Center" | "Governance" | "Evolution" | "Incidents"
  | "Schedule" | "Recovery" | "Audit" | "Configuration";

const NAV: { group: string; items: { name: Section; icon: typeof Home }[] }[] = [
  { group: "Monitoring", items: [
    { name: "Dashboard", icon: LayoutGrid }, { name: "Market", icon: LineChart }, { name: "Timeframes", icon: Layers },
    { name: "Signals", icon: Radio }, { name: "Risk", icon: ShieldCheck }, { name: "Shadow Positions", icon: Ghost },
    { name: "Observation", icon: Eye },
  ] },
  { group: "Intelligence", items: [
    { name: "Research", icon: FlaskConical }, { name: "Knowledge", icon: BookOpen },
    { name: "Candidates", icon: ListChecks }, { name: "Validation", icon: BadgeCheck },
  ] },
  { group: "Governance", items: [
    { name: "Approval Center", icon: Stamp }, { name: "Governance", icon: Landmark },
    { name: "Evolution", icon: GitBranch }, { name: "Incidents", icon: AlertTriangle },
  ] },
  { group: "System", items: [
    { name: "Schedule", icon: CalendarClock }, { name: "Recovery", icon: LifeBuoy },
    { name: "Audit", icon: ScrollText }, { name: "Configuration", icon: SlidersHorizontal },
  ] },
];

/* ---------- primitives ---------- */

function AstraLogo({ compact = false }: { compact?: boolean }) {
  return (
    <div className="flex min-w-0 items-center gap-2.5">
      <img src={markImg} alt="ASTRA" className={compact ? "h-6 w-auto shrink-0" : "h-8 w-auto shrink-0"} />
      <div className="min-w-0 leading-none">
        <div className={compact ? "text-[15px] font-semibold tracking-[0.2em]" : "text-lg font-semibold tracking-[0.22em]"}>ASTRA</div>
        {!compact && <div className="label-xs mt-1.5 !text-[9px] leading-snug">XAUUSD<br />Market Intelligence</div>}
      </div>
    </div>
  );
}

const STATE_CLS: Record<State, string> = {
  HEALTHY: "text-healthy", WARNING: "text-warning", CRITICAL: "text-critical",
  UNKNOWN: "text-subtle", "NOT AVAILABLE": "text-subtle", EMPTY: "text-subtle",
};
const DOT_CLS: Record<State, string> = {
  HEALTHY: "bg-healthy", WARNING: "bg-warning", CRITICAL: "bg-critical",
  UNKNOWN: "bg-subtle", "NOT AVAILABLE": "border border-subtle", EMPTY: "border border-subtle",
};

function StatusBadge({ state, label }: { state: State; label?: string }) {
  return (
    <span className={`inline-flex items-center gap-1.5 text-[10px] font-medium tracking-[0.12em] ${STATE_CLS[state]}`}>
      <span className={`h-1.5 w-1.5 shrink-0 rounded-full ${DOT_CLS[state]}`} />
      {label ?? state}
    </span>
  );
}

function ShadowOnly({ size = "sm" }: { size?: "sm" | "lg" }) {
  return (
    <span className={`inline-flex shrink-0 items-center gap-2 border border-accent/40 bg-elevated font-semibold tracking-[0.18em] text-accent ${size === "lg" ? "px-3 py-1.5 text-[11px]" : "px-2 py-1 text-[10px]"}`}>
      <span className="h-1.5 w-1.5 bg-accent" /> SHADOW ONLY
    </span>
  );
}

function PanelHead({ title, right }: { title: string; right?: ReactNode }) {
  return (
    <div className="flex items-center justify-between gap-3 border-b border-border px-3.5 py-2.5">
      <h3 className="text-[11px] font-semibold tracking-[0.16em] text-muted-foreground uppercase">{title}</h3>
      {right}
    </div>
  );
}

function Field({ k, v, cls = "" }: { k: string; v: ReactNode; cls?: string }) {
  return (
    <div className="flex items-center justify-between gap-3 border-b border-border/50 py-2 last:border-0">
      <span className="label-xs">{k}</span>
      <span className={`num text-xs ${cls}`}>{v}</span>
    </div>
  );
}

/* ---------- shell ---------- */

function useClock() {
  const [now, setNow] = useState<Date | null>(null);
  useEffect(() => {
    setNow(new Date());
    const id = setInterval(() => setNow(new Date()), 1000);
    return () => clearInterval(id);
  }, []);
  return now ? now.toISOString().slice(11, 19) + " UTC" : "--:--:-- UTC";
}

function AstraShell() {
  const [section, setSection] = useState<Section>("Dashboard");
  const [timeframe, setTimeframe] = useState<Timeframe>("M15");
  const [exitOpen, setExitOpen] = useState(false);
  const [closed, setClosed] = useState(false);
  const clock = useClock();

  if (closed) {
    return (
      <div className="flex h-screen flex-col items-center justify-center gap-6 bg-background">
        <AstraLogo />
        <p className="label-xs">ASTRA shut down safely · state checkpointed</p>
        <button onClick={() => setClosed(false)} className="border border-border px-4 py-2 text-[11px] tracking-[0.16em] text-muted-foreground hover:border-accent hover:text-foreground">RELAUNCH</button>
      </div>
    );
  }

  return (
    <div className="flex h-screen w-full overflow-hidden bg-background text-foreground">
      {/* Sidebar */}
      <aside className="hidden w-[232px] shrink-0 flex-col border-r border-border bg-sidebar lg:flex">
        <div className="border-b border-border px-5 py-5"><AstraLogo /></div>
        <nav className="no-scrollbar flex-1 overflow-y-auto py-3">
          {NAV.map((g) => (
            <div key={g.group} className="mb-3">
              <div className="label-xs px-5 pb-1.5 pt-2 !text-[9px]">{g.group}</div>
              {g.items.map((it) => {
                const active = section === it.name;
                return (
                  <button
                    key={it.name}
                    onClick={() => setSection(it.name)}
                    className={`group relative flex w-full items-center gap-3 px-5 py-[7px] text-left text-[12.5px] transition-colors duration-150 ${active ? "bg-elevated text-foreground" : "text-subtle hover:bg-muted hover:text-muted-foreground"}`}
                  >
                    {active && <span className="absolute left-0 top-1 bottom-1 w-[2px] bg-accent" />}
                    <it.icon className="h-[14px] w-[14px] shrink-0" strokeWidth={1.5} />
                    {it.name}
                  </button>
                );
              })}
            </div>
          ))}
        </nav>
        <div className="border-t border-border p-3">
          <button onClick={() => setExitOpen(true)} className="flex w-full items-center justify-center gap-2 border border-border py-2 text-[11px] font-medium tracking-[0.24em] text-subtle transition-colors hover:border-critical/60 hover:text-critical">
            <Power className="h-3.5 w-3.5" strokeWidth={1.5} /> EXIT
          </button>
        </div>
      </aside>

      <div className="flex min-w-0 flex-1 flex-col">
        {/* Top system bar */}
        <header className="grid h-12 shrink-0 grid-cols-[minmax(0,1fr)_auto] items-center gap-4 border-b border-border bg-sidebar px-4 lg:px-5">
          <div className="flex min-w-0 items-center gap-4">
            <div className="lg:hidden"><AstraLogo compact /></div>
            <div className="hidden min-w-0 items-baseline gap-3 lg:flex">
              <span className="text-[13px] font-semibold tracking-[0.2em]">ASTRA</span>
              <span className="h-3 w-px bg-border" />
              <span className="truncate text-[11px] tracking-[0.16em] text-muted-foreground">XAUUSD MARKET INTELLIGENCE</span>
            </div>
          </div>
          <div className="flex items-center gap-5">
            <div className="hidden items-center gap-5 xl:flex">
              <HeaderStat k="System health"><StatusBadge state="HEALTHY" /></HeaderStat>
              <HeaderStat k="Data streams"><span className="num text-[11px]">9 / 9</span></HeaderStat>
              <HeaderStat k="Renderer"><span className="num text-[11px] text-muted-foreground">GPU · 60fps</span></HeaderStat>
            </div>
            <span className="num hidden text-[11px] text-muted-foreground sm:inline">{clock}</span>
            <ShadowOnly />
          </div>
        </header>

        <main className="min-h-0 flex-1 overflow-y-auto pb-16 lg:pb-0">
          {section === "Dashboard" || section === "Market" ? (
            <Dashboard timeframe={timeframe} setTimeframe={setTimeframe} />
          ) : (
            <SectionView section={section} timeframe={timeframe} setTimeframe={setTimeframe} />
          )}
        </main>

        {/* Footer bar */}
        <footer className="hidden h-7 shrink-0 items-center gap-5 border-t border-border bg-sidebar px-5 text-[10px] tracking-[0.12em] text-subtle lg:flex">
          <span className="text-muted-foreground">ASTRA RUNTIME</span>
          <StatusBadge state="HEALTHY" label="9 STREAMS" />
          <StatusBadge state="HEALTHY" label="PERSISTENCE" />
          <StatusBadge state="HEALTHY" label="RECOVERY ARMED" />
          <span>CHECKPOINT 10:58:12</span>
          <span className="ml-auto">PROTOTYPE · SAMPLE DATA · LIVE EXECUTION DISABLED</span>
        </footer>

        {/* Mobile bottom nav */}
        <MobileNav section={section} setSection={setSection} onExit={() => setExitOpen(true)} />
      </div>

      {exitOpen && <ExitDialog onCancel={() => setExitOpen(false)} onExit={() => { setExitOpen(false); setClosed(true); }} />}
    </div>
  );
}

function HeaderStat({ k, children }: { k: string; children: ReactNode }) {
  return (
    <div className="flex items-center gap-2">
      <span className="label-xs !text-[9px]">{k}</span>
      {children}
    </div>
  );
}

function MobileNav({ section, setSection, onExit }: { section: Section; setSection: (s: Section) => void; onExit: () => void }) {
  const items: { label: string; target: Section; icon: typeof Home }[] = [
    { label: "Home", target: "Dashboard", icon: Home },
    { label: "Market", target: "Timeframes", icon: LineChart },
    { label: "Signals", target: "Signals", icon: Activity },
    { label: "Intel", target: "Research", icon: FlaskConical },
    { label: "System", target: "Recovery", icon: Cpu },
  ];
  return (
    <nav className="fixed inset-x-0 bottom-0 z-30 grid h-14 grid-cols-6 border-t border-border bg-sidebar lg:hidden">
      {items.map((it) => {
        const active = section === it.target;
        return (
          <button key={it.label} onClick={() => setSection(it.target)} className={`relative flex flex-col items-center justify-center gap-1 text-[9px] tracking-[0.14em] uppercase ${active ? "text-foreground" : "text-subtle"}`}>
            {active && <span className="absolute top-0 h-[2px] w-6 bg-accent" />}
            <it.icon className="h-4 w-4" strokeWidth={1.5} />
            {it.label}
          </button>
        );
      })}
      <button onClick={onExit} className="flex flex-col items-center justify-center gap-1 text-[9px] tracking-[0.14em] text-subtle uppercase">
        <Power className="h-4 w-4" strokeWidth={1.5} /> Exit
      </button>
    </nav>
  );
}

function ExitDialog({ onCancel, onExit }: { onCancel: () => void; onExit: () => void }) {
  useEffect(() => {
    const k = (e: KeyboardEvent) => e.key === "Escape" && onCancel();
    window.addEventListener("keydown", k);
    return () => window.removeEventListener("keydown", k);
  }, [onCancel]);
  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-background/80 p-4 backdrop-blur-sm animate-in fade-in duration-150" onClick={onCancel}>
      <div role="dialog" aria-modal className="panel w-full max-w-sm animate-in zoom-in-95 duration-150" onClick={(e) => e.stopPropagation()}>
        <div className="border-b border-border px-5 py-4">
          <h2 className="text-sm font-semibold tracking-[0.2em]">EXIT ASTRA?</h2>
        </div>
        <p className="px-5 py-5 text-[13px] leading-relaxed text-muted-foreground">Closing ASTRA will safely shut down the application.</p>
        <div className="grid grid-cols-2 gap-px border-t border-border bg-border">
          <button onClick={onCancel} className="bg-card py-3 text-[11px] font-medium tracking-[0.2em] text-muted-foreground hover:bg-elevated hover:text-foreground">CANCEL</button>
          <button onClick={onExit} className="bg-card py-3 text-[11px] font-semibold tracking-[0.2em] text-critical hover:bg-elevated">EXIT</button>
        </div>
      </div>
    </div>
  );
}

/* ---------- dashboard ---------- */

function TimeframeSelector({ value, onChange }: { value: Timeframe; onChange: (t: Timeframe) => void }) {
  return (
    <div className="no-scrollbar flex overflow-x-auto border border-border bg-muted">
      {TIMEFRAMES.map((tf) => {
        const active = tf === value;
        const structural = tf === "H4";
        return (
          <button
            key={tf}
            onClick={() => onChange(tf)}
            title={TF_ROLE[tf]}
            className={`num relative min-w-[44px] shrink-0 border-r border-border px-3 py-1.5 text-[11px] transition-colors duration-150 last:border-r-0 ${active ? "bg-elevated font-semibold text-foreground" : "text-subtle hover:text-muted-foreground"}`}
          >
            {active && <span className="absolute inset-x-2 bottom-0 h-[2px] bg-accent" />}
            {tf}
            {structural && !active && <span className="absolute right-1 top-1 h-1 w-1 rounded-full bg-subtle" />}
          </button>
        );
      })}
    </div>
  );
}

function MetricRail() {
  const m: { k: string; v: ReactNode }[] = [
    { k: "System", v: <span className="text-healthy">HEALTHY</span> },
    { k: "Streams", v: "9 / 9" },
    { k: "Signals", v: <span className="text-muted-foreground">SHADOW</span> },
    { k: "Risk", v: <span className="text-healthy">LOW</span> },
    { k: "Mode", v: <span className="text-accent">SHADOW ONLY</span> },
  ];
  return (
    <div className="no-scrollbar flex overflow-x-auto border-y border-border lg:border-y-0">
      {m.map((x) => (
        <div key={x.k} className="flex shrink-0 flex-col justify-center gap-1 border-r border-border px-4 py-1 first:pl-0 last:border-r-0 lg:first:pl-4">
          <span className="label-xs !text-[9px]">{x.k}</span>
          <span className="num text-[12px] font-medium tracking-wider">{x.v}</span>
        </div>
      ))}
    </div>
  );
}

function MarketHeader({ timeframe }: { timeframe: Timeframe }) {
  const { candles } = useMemo(() => mockMarket("D1", 3), []);
  const prev = candles[candles.length - 2]!.close;
  const last = 4958.42;
  const ch = last - prev;
  const pct = (ch / prev) * 100;
  const up = ch >= 0;
  return (
    <div className="flex flex-col gap-4 xl:flex-row xl:items-center xl:justify-between">
      <div className="flex flex-wrap items-end gap-x-6 gap-y-3">
        <div>
          <div className="flex items-baseline gap-3">
            <h1 className="text-2xl font-semibold tracking-[0.08em]">XAUUSD</h1>
            <span className="text-xs text-muted-foreground">Gold / US Dollar</span>
          </div>
          <div className="label-xs mt-1">Market Intelligence · {timeframe} · {TF_ROLE[timeframe]}</div>
        </div>
        <div className="flex items-end gap-6">
          <div>
            <div className="label-xs !text-[9px]">Current price</div>
            <div className="num text-[22px] font-medium leading-tight">{last.toFixed(2)}</div>
          </div>
          <div>
            <div className="label-xs !text-[9px]">Daily change</div>
            <div className={`num text-[13px] ${up ? "text-bull" : "text-bear"}`}>{up ? "+" : ""}{ch.toFixed(2)} ({up ? "+" : ""}{pct.toFixed(2)}%)</div>
          </div>
          <div>
            <div className="label-xs !text-[9px]">Market state</div>
            <div className="text-[12px] font-medium tracking-wider">OPEN · LONDON</div>
          </div>
          <div>
            <div className="label-xs !text-[9px]">Data quality</div>
            <div className="num text-[12px] text-healthy">99.8%</div>
          </div>
        </div>
      </div>
      <MetricRail />
    </div>
  );
}

function ChartBlock({ timeframe, setTimeframe, tall = false }: { timeframe: Timeframe; setTimeframe: (t: Timeframe) => void; tall?: boolean }) {
  const view = useMemo(() => mockMarket(timeframe), [timeframe]);
  return (
    <section className="panel flex flex-col">
      <div className="flex flex-wrap items-center justify-between gap-3 border-b border-border px-3 py-2">
        <TimeframeSelector value={timeframe} onChange={setTimeframe} />
        <div className="flex items-center gap-4 text-[10px] tracking-[0.12em] text-subtle">
          <span className="hidden sm:inline">OHLC · {view.candles.length} BARS</span>
          <span className="hidden md:inline">ROLE <span className="text-muted-foreground">{TF_ROLE[timeframe]?.toUpperCase()}</span></span>
          <span className="border border-border px-1.5 py-0.5">SAMPLE DATA</span>
        </div>
      </div>
      <div key={timeframe} className={`animate-in fade-in duration-200 ${tall ? "h-[62vh]" : "h-[46vh] min-h-[320px] lg:h-[52vh]"}`}>
        <CandlestickChart candles={view.candles} timeframe={timeframe} />
      </div>
    </section>
  );
}

function SignalsPanel({ full = false }: { full?: boolean }) {
  const dirCls = (d: string) => (d === "LONG" ? "text-bull" : d === "SHORT" ? "text-bear" : "text-muted-foreground");
  return (
    <section className="panel min-w-0">
      <PanelHead title="Shadow Signals" right={<span className="label-xs !text-[9px]">Analytical only</span>} />
      <div className="overflow-x-auto">
        <table className="w-full text-[11.5px]">
          <thead>
            <tr className="label-xs !text-[9px]">
              {["TF", "Dir", "Score", "Conf", "Time", ...(full ? [] : []), "State"].map((h) => <th key={h} className="px-3.5 py-2 text-left font-medium">{h}</th>)}
            </tr>
          </thead>
          <tbody className="num">
            {SIGNALS.map((s) => (
              <tr key={s.tf + s.time} className="border-t border-border/50 hover:bg-elevated/60">
                <td className="px-3.5 py-2 text-foreground">{s.tf}</td>
                <td className={`px-3.5 py-2 font-medium ${dirCls(s.dir)}`}>{s.dir}</td>
                <td className="px-3.5 py-2">
                  <div className="flex items-center gap-2">
                    <span className="w-5">{s.score}</span>
                    <span className="h-[3px] w-10 bg-border"><span className="block h-full bg-accent" style={{ width: `${s.score}%` }} /></span>
                  </div>
                </td>
                <td className="px-3.5 py-2 text-muted-foreground">{s.conf.toFixed(2)}</td>
                <td className="px-3.5 py-2 text-muted-foreground">{s.time}</td>
                <td className="px-3.5 py-2 text-[10px] tracking-wider text-subtle">{s.state}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </section>
  );
}

function RiskPanel() {
  return (
    <section className="panel">
      <PanelHead title="Risk" right={<StatusBadge state="HEALTHY" label="LOW" />} />
      <div className="px-3.5 py-1.5">
        <Field k="Risk state" v="LOW" cls="text-healthy" />
        <Field k="Risk gate" v="CLOSED · NO EXECUTION" cls="text-muted-foreground" />
        <Field k="Exposure" v="0.00 lots" />
        <Field k="Limits" v="WITHIN" cls="text-muted-foreground" />
        <Field k="Safety" v="ARMED" cls="text-healthy" />
      </div>
    </section>
  );
}

function ShadowPositions() {
  return (
    <section className="panel flex flex-col">
      <PanelHead title="Shadow Positions" right={<ShadowOnly />} />
      <div className="flex flex-1 flex-col items-center justify-center gap-2 px-4 py-8 text-center">
        <Ghost className="h-5 w-5 text-subtle" strokeWidth={1.25} />
        <div className="text-[11px] font-medium tracking-[0.18em] text-muted-foreground">NO ACTIVE SHADOW POSITIONS</div>
        <div className="text-[10px] text-subtle">Simulated tracking only · not broker positions</div>
      </div>
    </section>
  );
}

function TimeframeMatrix({ selected, onSelect }: { selected: Timeframe; onSelect: (t: Timeframe) => void }) {
  return (
    <section className="panel min-w-0">
      <PanelHead title="Timeframe Intelligence" right={<span className="label-xs !text-[9px]">9 streams · sample</span>} />
      <div className="overflow-x-auto">
        <table className="w-full min-w-[720px] text-[11.5px]">
          <thead>
            <tr className="label-xs !text-[9px]">
              {["Timeframe", "Health", "Quality", "Sequence", "Freshness", "Last closed bar", "Signal / Setup"].map((h) => (
                <th key={h} className="px-3.5 py-2 text-left font-medium">{h}</th>
              ))}
            </tr>
          </thead>
          <tbody className="num">
            {TIMEFRAMES.map((tf) => {
              const r = MATRIX[tf];
              const op = tf === "M15";
              const st = tf === "H4";
              return (
                <tr key={tf} onClick={() => onSelect(tf)} className={`relative cursor-pointer border-t border-border/50 transition-colors hover:bg-elevated/60 ${selected === tf ? "bg-elevated" : ""}`}>
                  <td className="relative px-3.5 py-2">
                    {(op || st) && <span className={`absolute left-0 top-1 bottom-1 w-[2px] ${op ? "bg-accent" : "bg-subtle"}`} />}
                    <span className="font-medium text-foreground">{tf}</span>
                    {(op || st) && <span className="ml-2 text-[9px] tracking-[0.14em] text-subtle">{op ? "OPERATIONAL" : "STRUCTURAL"}</span>}
                  </td>
                  <td className="px-3.5 py-2"><StatusBadge state={r.health} /></td>
                  <td className="px-3.5 py-2 text-muted-foreground">{r.quality ? `${r.quality.toFixed(1)}%` : "N/A"}</td>
                  <td className={`px-3.5 py-2 ${r.seq === "OK" ? "text-muted-foreground" : r.seq === "—" ? "text-subtle" : "text-warning"}`}>{r.seq}</td>
                  <td className="px-3.5 py-2 text-muted-foreground">{r.fresh}</td>
                  <td className="px-3.5 py-2 text-muted-foreground">{r.last}</td>
                  <td className={`px-3.5 py-2 text-[10.5px] tracking-wider ${r.signal.includes("LONG") || r.signal.includes("↑") ? "text-bull" : "text-subtle"}`}>{r.signal}</td>
                </tr>
              );
            })}
          </tbody>
        </table>
      </div>
    </section>
  );
}

function Dashboard({ timeframe, setTimeframe }: { timeframe: Timeframe; setTimeframe: (t: Timeframe) => void }) {
  return (
    <div className="flex flex-col gap-3 p-3 lg:gap-4 lg:p-5">
      <MarketHeader timeframe={timeframe} />
      <ChartBlock timeframe={timeframe} setTimeframe={setTimeframe} />
      <div className="grid gap-3 lg:gap-4 xl:grid-cols-[1.5fr_1fr_1fr]">
        <SignalsPanel />
        <RiskPanel />
        <ShadowPositions />
      </div>
      <TimeframeMatrix selected={timeframe} onSelect={setTimeframe} />
      <div className="grid gap-3 lg:gap-4 xl:grid-cols-4">
        {(["Research", "Knowledge", "Validation", "Recovery"] as Section[]).map((s) => (
          <section key={s} className="panel">
            <PanelHead title={s} />
            <div className="px-3.5 py-1.5">
              {(SECTION_DATA[s]?.fields ?? []).slice(0, 3).map((f) => <Field key={f[0]} k={f[0]} v={f[1]} cls={stateCls(f[1])} />)}
            </div>
          </section>
        ))}
      </div>
    </div>
  );
}

/* ---------- secondary sections ---------- */

const NA = "NOT AVAILABLE";
function stateCls(v: string) {
  if (v === NA || v === "EMPTY" || v === "UNKNOWN") return "text-subtle";
  if (["HEALTHY", "OK", "ARMED", "PASSED", "ACTIVE", "VALID"].includes(v)) return "text-healthy";
  if (["WARNING", "PENDING", "REVIEW"].includes(v)) return "text-warning";
  if (["CRITICAL", "FAILED"].includes(v)) return "text-critical";
  return "text-muted-foreground";
}

type SectionData = {
  intro: string;
  fields: [string, string][];
  events?: { t: string; title: string; state: string; sev?: string }[];
  controls?: string[];
};

const SECTION_DATA: Partial<Record<Section, SectionData>> = {
  Observation: { intro: "Passive observation of market structure. No actions are taken.", fields: [["Observer", "ACTIVE"], ["Session", "LONDON"], ["Watched levels", "4"], ["Last event", "10:45 UTC"]],
    events: [{ t: "10:45", title: "M15 close above session VWAP", state: "NOTED" }, { t: "10:12", title: "H1 range expansion", state: "NOTED" }, { t: "08:00", title: "H4 structural higher low confirmed", state: "NOTED" }] },
  Research: { intro: "Research workstreams feeding the knowledge base.", fields: [["Active studies", "3"], ["Last run", "09:40 UTC"], ["Queue", "EMPTY"], ["Backlog", NA]],
    events: [{ t: "09:40", title: "Session-open volatility profile", state: "ACTIVE" }, { t: "03 OCT", title: "H4 structure persistence study", state: "REVIEW" }, { t: "01 OCT", title: "D1 gap behaviour", state: "PASSED" }] },
  Knowledge: { intro: "Curated, validated knowledge entries.", fields: [["Entries", "128"], ["Validated", "VALID"], ["Last update", "02 OCT"], ["Sources", NA]] },
  Candidates: { intro: "Hypotheses awaiting validation. Not used for decisions.", fields: [["Candidates", "5"], ["In validation", "2"], ["Rejected (30d)", "7"], ["Promoted", "EMPTY"]],
    events: [{ t: "C-041", title: "M15 pullback-to-structure setup", state: "REVIEW" }, { t: "C-039", title: "London open fade", state: "PENDING" }] },
  Validation: { intro: "Out-of-sample validation of candidates.", fields: [["Status", "HEALTHY"], ["Last suite", "PASSED"], ["Running", "1"], ["Coverage", NA]] },
  "Approval Center": { intro: "Pending human approvals. Approvals never enable live execution.", fields: [["Pending", "1"], ["Approved (30d)", "4"], ["Rejected (30d)", "2"], ["Approver", "OPERATOR"]],
    events: [{ t: "10:20", title: "Promote C-041 to shadow observation", state: "PENDING", sev: "LOW" }] },
  Governance: { intro: "Policies and controls governing the system.", fields: [["Mode", "SHADOW ONLY"], ["Live execution", "DISABLED"], ["Policy version", "v1.4"], ["Last review", "28 SEP"]] },
  Evolution: { intro: "Version history of models and rules.", fields: [["Current build", "2026.10.1"], ["Changes (30d)", "6"], ["Rollback point", "2026.09.4"], ["Drift", NA]] },
  Incidents: { intro: "Operational incidents with severity and audit trail.", fields: [["Open", "1"], ["Critical", "EMPTY"], ["MTTR (30d)", "14m"], ["Last", "03 OCT"]],
    events: [{ t: "03 OCT 22:04", title: "D1 sequence gap detected (1 bar)", state: "WARNING", sev: "MEDIUM" }, { t: "29 SEP 06:11", title: "Stream reconnect M1", state: "OK", sev: "LOW" }] },
  Schedule: { intro: "Scheduled jobs and checkpoints.", fields: [["Next checkpoint", "11:00 UTC"], ["Next research run", "12:00 UTC"], ["Jobs", "8"], ["Missed", "EMPTY"]] },
  Recovery: { intro: "Operational recovery console.", fields: [["Persistence", "HEALTHY"], ["Checkpoint", "10:58:12 UTC"], ["Recovery state", "ARMED"], ["Watchdog", "OK"], ["Last known state", "CONSISTENT"]],
    controls: ["REFRESH", "CHECKPOINT", "PAUSE", "RESUME", "STOP", "RECOVERY"] },
  Audit: { intro: "Immutable audit log.", fields: [["Entries today", "312"], ["Integrity", "VALID"], ["Retention", "365d"], ["Export", NA]],
    events: [{ t: "10:58", title: "Checkpoint written", state: "OK" }, { t: "10:20", title: "Approval request created", state: "PENDING" }, { t: "09:40", title: "Research run started", state: "OK" }] },
  Configuration: { intro: "Runtime configuration (read-only in prototype).", fields: [["Symbol", "XAUUSD"], ["Default timeframe", "M15"], ["Execution", "DISABLED"], ["About", "ASTRA · prototype build"]] },
};

function SectionView({ section, timeframe, setTimeframe }: { section: Section; timeframe: Timeframe; setTimeframe: (t: Timeframe) => void }) {
  const head = (
    <div className="flex flex-wrap items-end justify-between gap-3">
      <div>
        <div className="label-xs">{NAV.find((g) => g.items.some((i) => i.name === section))?.group}</div>
        <h1 className="mt-1 text-xl font-semibold tracking-[0.08em]">{section.toUpperCase()}</h1>
      </div>
      <ShadowOnly />
    </div>
  );

  if (section === "Timeframes") return <div className="flex flex-col gap-4 p-3 lg:p-5">{head}<TimeframeMatrix selected={timeframe} onSelect={setTimeframe} /><ChartBlock timeframe={timeframe} setTimeframe={setTimeframe} /></div>;
  if (section === "Signals") return <div className="flex flex-col gap-4 p-3 lg:p-5">{head}<ChartBlock timeframe={timeframe} setTimeframe={setTimeframe} /><SignalsPanel full /></div>;
  if (section === "Risk") return <div className="flex flex-col gap-4 p-3 lg:p-5">{head}<div className="grid gap-4 xl:grid-cols-2"><RiskPanel /><ShadowPositions /></div></div>;
  if (section === "Shadow Positions") return <div className="flex flex-col gap-4 p-3 lg:p-5">{head}<ShadowPositions /></div>;

  const d = SECTION_DATA[section];
  if (!d) return <div className="p-5">{head}<p className="mt-6 text-sm text-subtle">{NA}</p></div>;
  return (
    <div className="flex flex-col gap-4 p-3 lg:p-5">
      {head}
      <p className="max-w-2xl text-[13px] text-muted-foreground">{d.intro}</p>
      <div className="grid gap-4 xl:grid-cols-[1fr_1.4fr]">
        <section className="panel">
          <PanelHead title="State" right={<span className="label-xs !text-[9px]">Sample</span>} />
          <div className="px-3.5 py-1.5">{d.fields.map((f) => <Field key={f[0]} k={f[0]} v={f[1]} cls={stateCls(f[1])} />)}</div>
          {d.controls && (
            <div className="grid grid-cols-3 gap-px border-t border-border bg-border">
              {d.controls.map((c) => (
                <button key={c} className={`bg-card py-2.5 text-[10px] font-medium tracking-[0.18em] hover:bg-elevated ${c === "STOP" ? "text-critical" : "text-muted-foreground hover:text-foreground"}`}>{c}</button>
              ))}
            </div>
          )}
        </section>
        <section className="panel">
          <PanelHead title="Timeline" />
          {d.events?.length ? (
            <ol className="relative px-3.5 py-2">
              {d.events.map((e) => (
                <li key={e.t + e.title} className="grid grid-cols-[96px_minmax(0,1fr)_auto] items-center gap-3 border-b border-border/50 py-2.5 last:border-0">
                  <span className="num text-[11px] text-subtle">{e.t}</span>
                  <span className="truncate text-[12.5px]">{e.title}{e.sev && <span className="ml-2 text-[9px] tracking-[0.14em] text-subtle">SEV {e.sev}</span>}</span>
                  <span className={`text-[10px] font-medium tracking-[0.14em] ${stateCls(e.state)}`}>{e.state}</span>
                </li>
              ))}
            </ol>
          ) : (
            <div className="px-4 py-10 text-center text-[11px] tracking-[0.18em] text-subtle">{NA}</div>
          )}
        </section>
      </div>
    </div>
  );
}
