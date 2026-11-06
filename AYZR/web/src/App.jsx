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
    <Panel
      title={`شارت XAUUSD · ${timeframe}`}
      hint={snapshot ? `${snapshot.data.closedBars} شمعة مغلقة${snapshot.data.derived ? ' · مجمّعة من اليومي' : ''}` : ''}
      right={<span className="chip dim">شموع مغلقة فقط</span>}
      style={{ minWidth: 0 }}
    >
      <div style={{ padding: '8px 10px 0' }}>
        <TimeframeRail timeframes={timeframes} selected={timeframe} onSelect={onSelectTimeframe} defaultTf="M15" />
      </div>
      <div style={{ height: 440 }}>
        <CandlestickChart candles={snapshot?.candles || []} timeframe={timeframe} regions={regions} trades={trades} />
      </div>
    </Panel>
  ), [snapshot, timeframe, regions, trades, timeframes, onSelectTimeframe]);

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
    <div style={{ display: 'flex', height: '100%', overflow: 'hidden', background: 'var(--page-deep)', direction: 'ltr' }}>
      {/* Sidebar — left, as in the reference dashboard (measured edge at x≈237 of 1672) */}
      <aside dir="rtl" style={{
        width: 232, flex: '0 0 232px', background: 'var(--sidebar)',
        borderRight: '1px solid var(--border)', display: 'flex', flexDirection: 'column', minHeight: 0,
      }}>
        <div style={{ padding: '16px 16px 14px', borderBottom: '1px solid var(--border)' }}>
          <BrandMark />
        </div>

        <nav style={{ flex: 1, overflowY: 'auto', padding: '8px 0' }}>
          {NAV.map((g) => (
            <div key={g.group} style={{ marginBottom: 8 }}>
              <div className="label" style={{ padding: '8px 16px 4px', color: 'var(--gold-dim)' }}>{g.group}</div>
              {g.items.map((it) => {
                const active = section === it.id;
                return (
                  <button
                    key={it.id}
                    onClick={() => goTo(it.id)}
                    style={{
                      display: 'block', width: '100%', textAlign: 'right', padding: '7px 16px',
                      fontSize: 12.5, background: active ? 'var(--sidebar-active)' : 'transparent',
                      color: active ? 'var(--text)' : 'var(--text-dim)',
                      borderRight: active ? '2px solid var(--gold)' : '2px solid transparent',
                    }}
                  >
                    {it.ar}
                  </button>
                );
              })}
            </div>
          ))}
        </nav>

        {/* operator token */}
        <div style={{ padding: 10, borderTop: '1px solid var(--border)' }}>
          {operator ? (
            <div className="row-between">
              <span className="chip up">وضع المشغل</span>
              <button className="tiny faint" onClick={clearToken}>خروج</button>
            </div>
          ) : (
            <form onSubmit={onSetToken} className="stack" style={{ gap: 6 }}>
              <input
                type="password"
                placeholder="رمز المشغل (اختياري)"
                value={tokenDraft}
                onChange={(e) => setTokenDraft(e.target.value)}
                style={{ width: '100%', fontSize: 11.5 }}
              />
              <button className="chip blue" type="submit" style={{ justifyContent: 'center' }}>تفعيل المشغل</button>
            </form>
          )}
        </div>

        <div style={{ padding: 10, borderTop: '1px solid var(--border)' }}>
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
          height: 48, flex: '0 0 48px', display: 'flex', alignItems: 'center', gap: 16,
          padding: '0 16px', background: 'var(--page)', borderBottom: '1px solid var(--border)',
        }}>
          <div className="row" style={{ gap: 10, minWidth: 0 }}>
            <span style={{ fontWeight: 600, letterSpacing: '0.1em' }} className="ltr">AYZR</span>
            <span style={{ width: 1, height: 14, background: 'var(--border)' }} />
            <span className="small muted nowrap">منصة ذكاء وتحليل الذهب XAUUSD</span>
          </div>
          <div className="row" style={{ gap: 14, marginRight: 'auto', flexWrap: 'wrap' }}>
            <HeaderStat k="صحة النظام">
              {health?.status === 'HEALTHY'
                ? <span className="chip up"><i className="dot up" />سليم</span>
                : health ? <span className="chip warn"><i className="dot warn" />متدهور</span> : <span className="chip dim">—</span>}
            </HeaderStat>
            <HeaderStat k="تدفقات البيانات">
              <span className="num small">{timeframes.filter((t) => t.status !== 'NOT_AVAILABLE').length} / {timeframes.length || 9}</span>
            </HeaderStat>
            <HeaderStat k="النقل">
              <span className="ltr tiny muted">{health?.stream?.transport || '—'} · {health?.stream?.state || '—'}</span>
            </HeaderStat>
            <HeaderStat k="الوقت">
              <span className="num small">{clockUTC(now.toISOString())}</span>
            </HeaderStat>
            <span className="chip gold">وضع الظل فقط</span>
          </div>
        </header>

        {error && (
          <div style={{ padding: '6px 16px', background: 'rgba(217,59,75,0.12)', color: '#f0a5a2', fontSize: 11.5, borderBottom: '1px solid var(--border)' }}>
            {error}
          </div>
        )}

        <main style={{ flex: 1, overflowY: 'auto', padding: 12, minHeight: 0 }}>
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
          flex: '0 0 26px', display: 'flex', alignItems: 'center', gap: 14, padding: '0 16px',
          background: 'var(--page)', borderTop: '1px solid var(--border)', fontSize: 10, color: 'var(--text-faint)',
        }}>
          <span className="ltr">AYZR v1.0.0</span>
          <span>مصدر البيانات: <span className="ltr">BiQuote</span></span>
          <span>آخر تحديث: <span className="num">{timeUTC(health?.lastIngest, true)}</span></span>
          <span>دورات التحديث: <span className="num">{num(health?.ingestCycles)}</span></span>
          <span style={{ marginRight: 'auto' }}>لا يوجد تنفيذ تداول حقيقي · نتائج افتراضية لقياس الأداء</span>
        </footer>
      </div>

      {/* Right column — price ladder, data source and telemetry, as in the reference */}
      {snapshot && (
        <aside dir="rtl" style={{
          width: 320, flex: '0 0 320px', background: 'var(--surface-2)',
          borderLeft: '1px solid var(--border)', display: 'flex', flexDirection: 'column',
          minHeight: 0, overflowY: 'auto', padding: 10, gap: 10,
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
    <div className="row" style={{ gap: 6 }}>
      <span className="label">{k}</span>
      {children}
    </div>
  );
}

function BrandMark({ size = 22 }) {
  return (
    <div className="row" style={{ gap: 10 }}>
      <svg width={size} height={size} viewBox="0 0 32 32" aria-hidden="true">
        <path d="M16 2 L30 30 L22 30 L16 17 L10 30 L2 30 Z" fill="#f3d36c" />
        <path d="M16 17 L19 23 L13 23 Z" fill="#04132d" />
      </svg>
      <div>
        <div style={{ fontSize: 15, fontWeight: 700, letterSpacing: '0.14em' }} className="ltr">AYZR</div>
        <div className="label" style={{ marginTop: 2 }}>ذكاء وتحليل الذهب · <span className="ltr">XAUUSD</span></div>
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
      style={{ position: 'fixed', inset: 0, background: 'rgba(1,10,24,0.8)', display: 'flex', alignItems: 'center', justifyContent: 'center', zIndex: 50 }}
      onClick={onCancel}
    >
      <div className="panel" style={{ width: 380 }} onClick={(e) => e.stopPropagation()}>
        <div className="panel-head" style={{ fontSize: 13 }}>إغلاق AYZR؟</div>
        <div style={{ padding: '16px 14px', fontSize: 12.5, lineHeight: 1.9, color: 'var(--text-mid)' }}>
          الإغلاق يُنهي تشغيل الواجهة بشكل آمن. تُحفظ الحالة والمناطق والصفقات الافتراضية في قاعدة البيانات قبل الإنهاء.
          لا يوجد تنفيذ لأي أمر وسيط.
        </div>
        <div className="grid-2" style={{ gap: 1, background: 'var(--border)' }}>
          <button onClick={onCancel} style={{ background: 'var(--surface)', padding: '11px 0', fontSize: 12 }}>إلغاء</button>
          <button onClick={onConfirm} style={{ background: 'var(--surface)', padding: '11px 0', fontSize: 12, color: 'var(--down)', fontWeight: 600 }}>إغلاق</button>
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
                      <td className="tiny muted">{t.exitReason}</td>
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
            <Field k="المصدر" tone="var(--text-mid)">BiQuote</Field>
            <Field k="النقل" tone="var(--text-mid)">{d.transport} · {d.streamState}</Field>
            <Field k="الشموع المتاحة">{num(d.barsAvailable)}</Field>
            <Field k="الشموع المغلقة">{num(d.closedBars)}</Field>
            <Field k="أحدث شمعة مغلقة" tone="var(--text-mid)">{timeUTC(d.newestClosedBar, true)}</Field>
            <Field k="دورات التحديث">{num(d.ingestCycles)}</Field>
            <Field k="آخر خطأ" tone={d.lastError ? 'var(--down)' : 'var(--text-faint)'}>{d.lastError || 'لا يوجد'}</Field>
          </Panel>
          <Panel title="صلاحية الشموع">
            <div style={{ padding: '10px 12px' }} className="small muted">
              <div style={{ lineHeight: 1.9 }}>
                يُعرض فقط ما وسمه المصدر كشمعة مغلقة (<span className="ltr">isOpen = false</span>).
                لا يُعاد رسم تاريخ ولا تُختلق شموع، ولا تُدمج أطر زمنية مختلفة.
              </div>
              <div className="tiny faint" style={{ marginTop: 8 }}>
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
          <Field k="الرمز" tone="var(--text-mid)">XAUUSD</Field>
          <Field k="الإطار الافتراضي" tone="var(--text-mid)">M15</Field>
          <Field k="التنفيذ" tone="var(--down)">معطّل</Field>
          <Field k="وضع التشغيل" tone="var(--gold)">ظل فقط</Field>
          <Field k="مزود الذكاء الاصطناعي" tone="var(--text-mid)">{health?.ai?.available ? (health?.ai?.provider || '—') : 'غير مهيّأ'}</Field>
          <Field k="وضع المشغل" tone={operator ? 'var(--up)' : 'var(--text-faint)'}>{operator ? 'مفعّل' : 'معطّل'}</Field>
          <Field k="الإصدار" tone="var(--text-faint)">AYZR v1.0.0</Field>
        </Panel>
        <ControlPanel operator={operator} onControl={onControl} paused={health?.paused} status={busy ? 'جارٍ التنفيذ…' : ''} />
      </div>
    );
  }

  return <div className="empty">القسم غير متاح.</div>;
}
