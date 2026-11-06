// AYZR application shell.
//
// Layout follows the supplied reference dashboard: RTL Arabic, fixed sidebar on the
// right (first in reading order for RTL), a system header, the timeframe rail, the
// dominant candlestick chart, then the analysis panels and the timeframe matrix.
//
// Visitors can read everything. Only an operator (token in the header) can send AI
// prompts, decide on signals, or use the safe controls.

import { useCallback, useEffect, useMemo, useState } from 'react';
import { api, isOperator, setOperatorToken, getOperatorToken } from './lib/api.js';
import { clockUTC, timeUTC, num } from './lib/format.js';
import { CandlestickChart } from './components/CandlestickChart.jsx';
import {
  Panel, Field, MarketHeader, TimeframeRail, SignalsPanel, RiskPanel, ShadowPositions,
  PerformancePanel, RegionsPanel, TimeframeMatrix, AiPanel, EnginesPanel, ControlPanel,
  AuditPanel, PriceLadder, BiQuotePanel, TelemetryPanel,
} from './components/Panels.jsx';

const NAV = [
  { group: 'المراقبة', items: [
    { id: 'dashboard', ar: 'لوحة التحكم' },
    { id: 'market', ar: 'السوق والشارت' },
    { id: 'timeframes', ar: 'الأطر الزمنية' },
    { id: 'signals', ar: 'الإشارات' },
    { id: 'risk', ar: 'المخاطر' },
    { id: 'positions', ar: 'الصفقات الافتراضية' },
    { id: 'regions', ar: 'المناطق' },
  ] },
  { group: 'الذكاء', items: [
    { id: 'ai', ar: 'محادثة التحليل' },
    { id: 'engines', ar: 'المحركات' },
    { id: 'performance', ar: 'الأداء' },
  ] },
  { group: 'النظام', items: [
    { id: 'data', ar: 'حالة البيانات' },
    { id: 'audit', ar: 'سجل التدقيق' },
    { id: 'config', ar: 'الإعدادات' },
  ] },
];

export default function App() {
  const [health, setHealth] = useState(null);
  const [timeframes, setTimeframes] = useState([]);
  const [timeframe, setTimeframe] = useState('M15');
  const [snapshot, setSnapshot] = useState(null);
  const [engines, setEngines] = useState(null);
  const [audit, setAudit] = useState([]);
  const [aiRequests, setAiRequests] = useState([]);
  const [section, setSection] = useState(() => {
    const h = (typeof window !== 'undefined' ? window.location.hash : '').replace(/^#\/?/, '');
    return h || 'dashboard';
  });
  const [error, setError] = useState(null);
  const [now, setNow] = useState(new Date());
  const [busy, setBusy] = useState(false);
  const [aiBusy, setAiBusy] = useState(false);
  const [lastAi, setLastAi] = useState(null);
  const [exitOpen, setExitOpen] = useState(false);
  const [closed, setClosed] = useState(false);
  const [operator, setOperator] = useState(isOperator());
  const [tokenDraft, setTokenDraft] = useState('');

  // clock
  useEffect(() => {
    const id = setInterval(() => setNow(new Date()), 1000);
    return () => clearInterval(id);
  }, []);

  // Hash routing: sections are linkable and survive a reload.
  useEffect(() => {
    const onHash = () => {
      const h = window.location.hash.replace(/^#\/?/, '');
      if (h) setSection(h);
    };
    window.addEventListener('hashchange', onHash);
    return () => window.removeEventListener('hashchange', onHash);
  }, []);

  const goTo = useCallback((id) => {
    setSection(id);
    try { window.location.hash = `/${id}`; } catch { /* file:// or blocked */ }
  }, []);

  const refresh = useCallback(async () => {
    try {
      const [h, tf, snap] = await Promise.all([
        api.health(),
        api.timeframes(),
        api.snapshot(timeframe),
      ]);
      setHealth(h);
      setTimeframes(tf.timeframes);
      setSnapshot(snap);
      setError(null);
    } catch (err) {
      setError(err.message);
    }
  }, [timeframe]);

  useEffect(() => { refresh(); }, [refresh]);
  useEffect(() => {
    const id = setInterval(refresh, 12000);
    return () => clearInterval(id);
  }, [refresh]);

  useEffect(() => {
    api.engines().then(setEngines).catch(() => {});
    api.aiRequests().then((r) => setAiRequests(r.requests || [])).catch(() => {});
  }, []);

  useEffect(() => {
    if (section === 'audit' || section === 'data') {
      api.audit(150).then((r) => setAudit(r.entries || [])).catch(() => {});
    }
  }, [section]);

  const onSelectTimeframe = useCallback((tf) => setTimeframe(tf), []);

  const onDecide = useCallback(async (signalId, decision) => {
    await api.decide(signalId, decision);
    refresh();
  }, [refresh]);

  const onControl = useCallback(async (action) => {
    setBusy(true);
    const r = await api.control(action);
    setBusy(false);
    if (!r.ok) setError(r.body?.error || 'فشل الإجراء');
    else refresh();
  }, [refresh]);

  const onCreateTrade = useCallback(async (drawing) => {
    const r = await api.createManualTrade({ ...drawing, operator: 'operator' });
    if (r.ok) refresh();
    return { ok: r.ok, error: r.body?.error };
  }, [refresh]);

  const onCloseTrade = useCallback(async (id, exit) => {
    const r = await api.closeManualTrade(id, exit, 'أُغلقت يدوياً من الشارت');
    if (!r.ok) setError(r.body?.error || 'تعذّر إغلاق الصفقة');
    else refresh();
  }, [refresh]);

  const onAnalyze = useCallback(async (prompt) => {
    setAiBusy(true);
    const r = await api.analyze(prompt, timeframe);
    setAiBusy(false);
    setLastAi({ ...r.body, httpStatus: r.status });
    api.aiRequests().then((x) => setAiRequests(x.requests || [])).catch(() => {});
    return r.ok;
  }, [timeframe]);

  const onSetToken = useCallback((e) => {
    e.preventDefault();
    setOperatorToken(tokenDraft.trim());
    setOperator(isOperator());
    setTokenDraft('');
  }, [tokenDraft]);

  const clearToken = useCallback(() => { setOperatorToken(''); setOperator(false); }, []);

  const regions = snapshot?.regions || [];
  const trades = snapshot?.virtualTrades || [];
  const stats = snapshot?.statistics;

  const chart = useMemo(() => (
    <section className="panel" style={{ minWidth: 0, padding: 0 }}>
      <div style={{ padding: 'var(--s2) var(--s3)', borderBottom: '1px solid var(--line)' }}>
        <TimeframeRail timeframes={timeframes} selected={timeframe} onSelect={onSelectTimeframe} defaultTf="M15" />
      </div>
      <CandlestickChart
        candles={snapshot?.candles || []}
        timeframe={timeframe}
        regions={regions}
        trades={trades}
        height={440}
        operator={operator}
        onCreateTrade={onCreateTrade}
        onCloseTrade={onCloseTrade}
      />
    </section>
  ), [snapshot, timeframe, regions, trades, timeframes, onSelectTimeframe, operator, onCreateTrade, onCloseTrade]);

  if (closed) {
    return (
      <div style={{ height: '100%', display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center', gap: 14 }}>
        <BrandMark size={30} />
        <div className="small muted">تم إغلاق AYZR بأمان · تم حفظ الحالة</div>
        <button className="chip blue" onClick={() => setClosed(false)} style={{ padding: '7px 14px' }}>إعادة التشغيل</button>
      </div>
    );
  }

  return (
    <div style={{ display: 'flex', height: '100%', overflow: 'hidden', background: 'var(--bg)', direction: 'ltr' }}>
      {/* Sidebar — right, first in RTL reading order, as in the reference */}
      <aside className="sidebar" dir="rtl" style={{
        width: 232, flex: '0 0 232px', background: 'var(--sidebar)',
        borderLeft: '1px solid var(--line)', display: 'flex', flexDirection: 'column', minHeight: 0,
      }}>
        <div style={{ padding: '18px 16px 16px', borderBottom: '1px solid var(--line)' }}>
          <BrandMark />
        </div>

        <nav style={{ flex: 1, overflowY: 'auto', padding: '10px 0' }}>
          {NAV.map((g) => (
            <div key={g.group} style={{ marginBottom: 12 }}>
              <div className="eyebrow" style={{ padding: '6px 16px' }}>{g.group}</div>
              {g.items.map((it) => {
                const active = section === it.id;
                return (
                  <button
                    key={it.id}
                    onClick={() => goTo(it.id)}
                    style={{
                      display: 'block', width: '100%', textAlign: 'right',
                      padding: '8px 16px', fontSize: 'var(--t-body)',
                      color: active ? 'var(--fg)' : 'var(--fg-mid)',
                      background: active ? 'var(--blue-soft)' : 'transparent',
                      borderRight: active ? '2px solid var(--gold)' : '2px solid transparent',
                      fontWeight: active ? 600 : 400,
                    }}
                  >
                    {it.ar}
                  </button>
                );
              })}
            </div>
          ))}
        </nav>

        <div style={{ padding: 'var(--s3)', borderTop: '1px solid var(--line)' }}>
          {operator ? (
            <div className="row-between">
              <span className="chip up"><i className="dot up" />وضع المشغل</span>
              <button className="small faint" onClick={clearToken}>خروج</button>
            </div>
          ) : (
            <form onSubmit={onSetToken} className="stack-sm">
              <input
                type="password"
                placeholder="رمز المشغل"
                value={tokenDraft}
                onChange={(e) => setTokenDraft(e.target.value)}
                style={{ width: '100%' }}
              />
              <button className="chip blue" type="submit" style={{ justifyContent: 'center', padding: '7px 0' }}>
                تفعيل المشغل
              </button>
            </form>
          )}
        </div>

        <div style={{ padding: '0 var(--s3) var(--s3)' }}>
          <button
            onClick={() => setExitOpen(true)}
            className="chip down"
            style={{ width: '100%', justifyContent: 'center', padding: '7px 0' }}
          >
            خروج
          </button>
        </div>
      </aside>

      {/* Main column */}
      <div dir="rtl" style={{ flex: 1, display: 'flex', flexDirection: 'column', minWidth: 0, minHeight: 0 }}>
        <header style={{
          height: 56, flex: '0 0 56px', display: 'flex', alignItems: 'center', gap: 'var(--s4)',
          padding: '0 var(--s4)', background: 'var(--surface)', borderBottom: '1px solid var(--line)',
        }}>
          <div className="row" style={{ gap: 'var(--s2)', minWidth: 0 }}>
            <span className="ltr" style={{ fontWeight: 700, letterSpacing: '0.12em' }}>AYZR</span>
            <span style={{ width: 1, height: 16, background: 'var(--line-strong)' }} />
            <span className="small muted nowrap">منصة ذكاء وتحليل الذهب <span className="ltr">XAUUSD</span></span>
          </div>
          <div className="row" style={{ gap: 'var(--s5)', marginRight: 'auto', flexWrap: 'wrap' }}>
            <HeaderStat k="صحة النظام">
              {health?.status === 'HEALTHY'
                ? <span className="chip up"><i className="dot up live" />سليم</span>
                : health ? <span className="chip warn"><i className="dot warn" />متدهور</span> : <span className="chip dim">—</span>}
            </HeaderStat>
            <HeaderStat k="تدفقات البيانات">
              <span className="num small">{timeframes.filter((t) => t.status !== 'NOT_AVAILABLE').length} / {timeframes.length || 9}</span>
            </HeaderStat>
            <HeaderStat k="النقل">
              <span className="ltr small muted">{health?.stream?.transport || '—'} · {health?.stream?.state || '—'}</span>
            </HeaderStat>
            <HeaderStat k="الوقت">
              <span className="num small">{clockUTC(now.toISOString())}</span>
            </HeaderStat>
            <span className="chip gold">وضع الظل فقط</span>
          </div>
        </header>

        {error && (
          <div style={{ padding: '8px var(--s4)', background: 'rgba(224,75,92,0.12)', color: '#f0a5a2', fontSize: 'var(--t-small)', borderBottom: '1px solid var(--line)' }}>
            {error}
          </div>
        )}

        <main style={{ flex: 1, overflowY: 'auto', padding: 'var(--s4)', minHeight: 0 }}>
          {!snapshot ? (
            <div className="empty">جارٍ تحميل بيانات السوق من BiQuote…</div>
          ) : (
            <SectionRouter
              section={section}
              snapshot={snapshot}
              stats={stats}
              regions={regions}
              trades={trades}
              timeframes={timeframes}
              timeframe={timeframe}
              onSelectTimeframe={onSelectTimeframe}
              chart={chart}
              operator={operator}
              onDecide={onDecide}
              onControl={onControl}
              onAnalyze={onAnalyze}
              aiBusy={aiBusy}
              lastAi={lastAi}
              aiRequests={aiRequests}
              health={health}
              engines={engines}
              audit={audit}
              busy={busy}
            />
          )}
        </main>

        <footer style={{
          flex: '0 0 30px', display: 'flex', alignItems: 'center', gap: 'var(--s4)', padding: '0 var(--s4)',
          background: 'var(--surface)', borderTop: '1px solid var(--line)', fontSize: 'var(--t-small)', color: 'var(--fg-faint)',
        }}>
          <span className="ltr">AYZR v1.0.0</span>
          <span>المصدر: <span className="ltr">BiQuote</span></span>
          <span>آخر تحديث: <span className="num">{timeUTC(health?.lastIngest, true)}</span></span>
          <span style={{ marginRight: 'auto' }}>لا تنفيذ تداول حقيقي · نتائج افتراضية لقياس الأداء</span>
        </footer>
      </div>

      {/* Right column — price ladder, data source and telemetry, as in the reference */}
      {snapshot && (
        <aside className="right-col" dir="rtl" style={{
          width: 320, flex: '0 0 320px', background: 'var(--surface)',
          borderRight: '1px solid var(--line)', display: 'flex', flexDirection: 'column',
          minHeight: 0, overflowY: 'auto', padding: 'var(--s3)', gap: 'var(--s3)',
        }}>
          <PriceLadder snapshot={snapshot} regions={regions} />
          <BiQuotePanel snapshot={snapshot} health={health} />
          <TelemetryPanel health={health} snapshot={snapshot} timeframes={timeframes} />
        </aside>
      )}

      {exitOpen && (
        <ExitDialog
          onCancel={() => setExitOpen(false)}
          onConfirm={() => { setExitOpen(false); setClosed(true); }}
        />
      )}
    </div>
  );
}

function HeaderStat({ k, children }) {
  return (
    <div className="stack-sm" style={{ gap: 1 }}>
      <span className="eyebrow">{k}</span>
      <span style={{ lineHeight: 1.3 }}>{children}</span>
    </div>
  );
}

function BrandMark({ size = 26 }) {
  return (
    <div className="row" style={{ gap: 10 }}>
      <svg width={size} height={size} viewBox="0 0 32 32" aria-hidden="true">
        <path d="M16 2 L30 30 L22 30 L16 17 L10 30 L2 30 Z" fill="#f3d36c" />
        <path d="M16 17 L19 23 L13 23 Z" fill="#000818" />
      </svg>
      <div>
        <div className="ltr" style={{ fontSize: 17, fontWeight: 700, letterSpacing: '0.16em', lineHeight: 1.1 }}>AYZR</div>
        <div className="micro faint" style={{ marginTop: 3, letterSpacing: 0 }}>
          ذكاء وتحليل الذهب · <span className="ltr">XAUUSD</span>
        </div>
      </div>
    </div>
  );
}

function ExitDialog({ onCancel, onConfirm }) {
  useEffect(() => {
    const k = (e) => e.key === 'Escape' && onCancel();
    window.addEventListener('keydown', k);
    return () => window.removeEventListener('keydown', k);
  }, [onCancel]);
  return (
    <div
      style={{ position: 'fixed', inset: 0, background: 'rgba(0,4,14,0.82)', display: 'flex', alignItems: 'center', justifyContent: 'center', zIndex: 50 }}
      onClick={onCancel}
    >
      <div className="panel" style={{ width: 400 }} onClick={(e) => e.stopPropagation()}>
        <div className="panel-head"><span className="panel-title">إغلاق AYZR؟</span></div>
        <div className="panel-body muted" style={{ fontSize: 'var(--t-body)', lineHeight: 1.9 }}>
          الإغلاق يُنهي تشغيل الواجهة بشكل آمن. تُحفظ الحالة والمناطق والصفقات الافتراضية في قاعدة البيانات قبل الإنهاء.
          لا يوجد تنفيذ لأي أمر وسيط.
        </div>
        <div className="grid-2" style={{ gap: 1, background: 'var(--line)' }}>
          <button onClick={onCancel} style={{ background: 'var(--surface)', padding: '12px 0', fontSize: 'var(--t-body)' }}>إلغاء</button>
          <button onClick={onConfirm} style={{ background: 'var(--surface)', padding: '12px 0', fontSize: 'var(--t-body)', color: 'var(--down)', fontWeight: 600 }}>إغلاق</button>
        </div>
      </div>
    </div>
  );
}

function SectionRouter(props) {
  const {
    section, snapshot, stats, regions, trades, timeframes, timeframe, onSelectTimeframe,
    chart, operator, onDecide, onControl, onAnalyze, aiBusy, lastAi, aiRequests, health,
    engines, audit, busy,
  } = props;

  if (section === 'dashboard' || section === 'market') {
    return (
      <div className="stack">
        <MarketHeader snapshot={snapshot} />
        {chart}
        <div className="grid-3">
          <SignalsPanel signals={snapshot.signals} operator={operator} onDecide={onDecide} />
          <RiskPanel snapshot={snapshot} />
          <ShadowPositions trades={trades} />
        </div>
        <div className="grid-3">
          <RegionsPanel regions={regions} />
          <PerformancePanel stats={stats} />
          <TimeframeMatrix rows={timeframes} selected={timeframe} onSelect={onSelectTimeframe} compact />
        </div>
      </div>
    );
  }

  if (section === 'timeframes') {
    return (
      <div className="stack">
        <TimeframeMatrix rows={timeframes} selected={timeframe} onSelect={onSelectTimeframe} />
        {chart}
      </div>
    );
  }

  if (section === 'signals') {
    return (
      <div className="stack">
        {chart}
        <SignalsPanel signals={snapshot.signals} operator={operator} onDecide={onDecide} />
      </div>
    );
  }

  if (section === 'risk') {
    return (
      <div className="grid-2">
        <RiskPanel snapshot={snapshot} />
        <ShadowPositions trades={trades} />
      </div>
    );
  }

  if (section === 'positions') {
    return (
      <div className="stack">
        <ShadowPositions trades={trades} />
        <PerformancePanel stats={stats} />
      </div>
    );
  }

  if (section === 'regions') {
    return (
      <div className="stack">
        {chart}
        <RegionsPanel regions={regions} />
      </div>
    );
  }

  if (section === 'ai') {
    return (
      <div className="stack">
        <AiPanel
          aiStatus={health?.ai}
          requests={aiRequests}
          operator={operator}
          onSubmit={onAnalyze}
          busy={aiBusy}
          lastResult={lastAi}
        />
        <RegionsPanel regions={regions} />
      </div>
    );
  }

  if (section === 'engines') {
    return <EnginesPanel engines={engines?.engines} costModel={engines?.costModel} />;
  }

  if (section === 'performance') {
    return (
      <div className="stack">
        <PerformancePanel stats={stats} />
        <Panel title="الصفقات المغلقة">
          {trades.filter((t) => t.state === 'CLOSED').length === 0 ? (
            <div className="empty">لا توجد صفقات مغلقة بعد.</div>
          ) : (
            <div style={{ overflowX: 'auto' }}>
              <table>
                <thead><tr><th>المحرك</th><th>الإطار</th><th>الاتجاه</th><th>الدخول</th><th>الخروج</th><th>النتيجة</th><th>R</th><th>الربح</th><th>السبب</th></tr></thead>
                <tbody>
                  {trades.filter((t) => t.state === 'CLOSED').map((t) => (
                    <tr key={t.id}>
                      <td className="small">{t.engine}</td>
                      <td className="num">{t.timeframe}</td>
                      <td className={t.direction === 'LONG' ? 'up' : 'down'}>{t.direction}</td>
                      <td className="num">{t.entry}</td>
                      <td className="num">{t.exit}</td>
                      <td className={t.result === 'WIN' ? 'up' : 'down'}>{t.result === 'WIN' ? 'ربح' : 'خسارة'}</td>
                      <td className="num">{t.rMultiple}R</td>
                      <td className={`num ${t.pnlUsd >= 0 ? 'up' : 'down'}`}>{t.pnlUsd}</td>
                      <td className="caption muted">{t.exitReason}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          )}
        </Panel>
      </div>
    );
  }

  if (section === 'data') {
    const d = snapshot.data;
    return (
      <div className="stack">
        <div className="grid-2">
          <Panel title="حالة البيانات">
            <Field k="المصدر" tone="var(--fg-mid)">BiQuote</Field>
            <Field k="النقل" tone="var(--fg-mid)">{d.transport} · {d.streamState}</Field>
            <Field k="الشموع المتاحة">{num(d.barsAvailable)}</Field>
            <Field k="الشموع المغلقة">{num(d.closedBars)}</Field>
            <Field k="أحدث شمعة مغلقة" tone="var(--fg-mid)">{timeUTC(d.newestClosedBar, true)}</Field>
            <Field k="دورات التحديث">{num(d.ingestCycles)}</Field>
            <Field k="آخر خطأ" tone={d.lastError ? 'var(--down)' : 'var(--fg-faint)'}>{d.lastError || 'لا يوجد'}</Field>
          </Panel>
          <Panel title="صلاحية الشموع">
            <div style={{ padding: '10px 12px' }} className="small muted">
              <div style={{ lineHeight: 1.9 }}>
                يُعرض فقط ما وسمه المصدر كشمعة مغلقة (<span className="ltr">isOpen = false</span>).
                لا يُعاد رسم تاريخ ولا تُختلق شموع، ولا تُدمج أطر زمنية مختلفة.
              </div>
              <div className="caption faint" style={{ marginTop: 8 }}>
                أطر <span className="ltr">W1</span> و <span className="ltr">MN1</span> غير متاحة مباشرة من المصدر،
                لذلك تُجمَّع من شموع <span className="ltr">D1</span> القادمة من المصدر نفسه، ويُوسم الناتج بأنه «مجمّع».
              </div>
            </div>
          </Panel>
        </div>
        <TimeframeMatrix rows={timeframes} selected={timeframe} onSelect={onSelectTimeframe} />
        <AuditPanel entries={audit} />
      </div>
    );
  }

  if (section === 'audit') {
    return <AuditPanel entries={audit} />;
  }

  if (section === 'config') {
    return (
      <div className="stack">
        <Panel title="الإعدادات (للقراءة)">
          <Field k="الرمز" tone="var(--fg-mid)">XAUUSD</Field>
          <Field k="الإطار الافتراضي" tone="var(--fg-mid)">M15</Field>
          <Field k="التنفيذ" tone="var(--down)">معطّل</Field>
          <Field k="وضع التشغيل" tone="var(--gold)">ظل فقط</Field>
          <Field k="مزود الذكاء الاصطناعي" tone="var(--fg-mid)">{health?.ai?.available ? (health?.ai?.provider || '—') : 'غير مهيّأ'}</Field>
          <Field k="وضع المشغل" tone={operator ? 'var(--up)' : 'var(--fg-faint)'}>{operator ? 'مفعّل' : 'معطّل'}</Field>
          <Field k="الإصدار" tone="var(--fg-faint)">AYZR v1.0.0</Field>
        </Panel>
        <ControlPanel operator={operator} onControl={onControl} paused={health?.paused} status={busy ? 'جارٍ التنفيذ…' : ''} />
      </div>
    );
  }

  return <div className="empty">القسم غير متاح.</div>;
}
