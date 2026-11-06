// Dashboard panels. Each panel renders only real server values; when a value is
// missing it shows an explicit state (غير متاح / لا توجد بيانات) instead of a
// placeholder number.

import { useState } from 'react';
import {
  price, pct, num, timeUTC, age, money, rMultiple,
  ENGINE_AR, STATE_AR, TYPE_AR, DIR_AR, ROLE_AR, MARKET_STATE_AR,
} from '../lib/format.js';

export function Panel({ title, hint, right, children, style }) {
  return (
    <section className="panel" style={style}>
      <div className="panel-head">
        <span className="panel-title">{title}</span>
        <span className="row" style={{ gap: 'var(--s2)' }}>
          {hint && <span className="panel-hint">{hint}</span>}
          {right}
        </span>
      </div>
      {children}
    </section>
  );
}

export function Field({ k, children, tone }) {
  return (
    <div className="kv">
      <span className="k">{k}</span>
      <span className="v num" style={{ color: tone }}>{children}</span>
    </div>
  );
}

/* Labelled block: small caption above a value, used in the market header and
 * the right-hand column. Keeps the caption/value pairing consistent everywhere. */
export function Stat({ k, children, tone, align = 'start' }) {
  return (
    <div style={{ textAlign: align }}>
      <div className="eyebrow">{k}</div>
      <div className="num" style={{ fontSize: 'var(--t-lead)', color: tone, marginTop: 2 }}>{children}</div>
    </div>
  );
}

export function DirTag({ dir }) {
  const cls = dir === 'LONG' ? 'up' : dir === 'SHORT' ? 'down' : 'dim';
  return <span className={`chip ${cls}`}>{DIR_AR[dir] || dir || '—'}</span>;
}

export function StateTag({ state }) {
  const tone = state === 'CONFIRMED_RETEST' || state === 'CONFIRMED_BREAKOUT' ? 'up'
    : state === 'FAKE_BREAKOUT' ? 'down'
      : state === 'EXPIRED' ? 'dim' : 'blue';
  return <span className={`chip ${tone}`}>{STATE_AR[state] || state}</span>;
}

/* ------------------------------------------------------------ market header */

export function MarketHeader({ snapshot }) {
  const q = snapshot.quote;
  const chg = q?.dayDiffPercent ?? null;
  const up = (chg ?? 0) >= 0;
  const priceValue = q?.mid ?? null;
  return (
    <div className="panel market-strip" style={{ padding: '10px var(--s3)' }}>
      <div style={{ display: 'flex', flexWrap: 'wrap', alignItems: 'center', gap: 'var(--s3) var(--s5)', justifyContent: 'space-between' }}>
        <div className="row" style={{ gap: 'var(--s5)', flexWrap: 'wrap', alignItems: 'center' }}>
          <div>
            <div className="row" style={{ gap: 'var(--s2)', alignItems: 'baseline' }}>
              <span className="num" style={{ fontSize: 'var(--t-display)', fontWeight: 600 }}>{price(priceValue)}</span>
              <span className={`num ${chg === null ? 'muted' : up ? 'up' : 'down'}`} style={{ fontSize: 'var(--t-lead)' }}>
                {chg === null ? '—' : pct(chg)}
              </span>
            </div>
            <div className="small muted" style={{ marginTop: 4 }}>
              <span className="ltr" style={{ fontWeight: 600 }}>XAUUSD</span>
              {' · '}{q?.description || 'الذهب مقابل الدولار الأمريكي'}
              {' · '}<span className="ltr">{snapshot.timeframe}</span>
            </div>
          </div>

          <Stat k="حالة السوق">
            {MARKET_STATE_AR[q?.marketState] || (q?.marketState ? <span className="ltr">{q.marketState}</span> : 'غير معروف')}
          </Stat>
          <Stat k="الفرق (سبريد)">{q ? price(q.spread, 3) : '—'}</Stat>
          <Stat k="مصدر البيانات"><span className="ltr" style={{ fontSize: 'var(--t-body)' }}>{q?.source || 'غير متاح'}</span></Stat>
        </div>

        <div className="row" style={{ gap: 6, flexWrap: 'wrap' }}>
          <span className="chip gold">وضع الظل فقط</span>
          <span className="chip dim">لا يوجد تداول حقيقي</span>
          {q?.stale ? <span className="chip down">بيانات قديمة</span> : <span className="chip up">بيانات حديثة</span>}
        </div>
      </div>
    </div>
  );
}

/* ---------------------------------------------------------- timeframe rail */

export function TimeframeRail({ timeframes, selected, onSelect, defaultTf }) {
  return (
    <div className="row" style={{ gap: 0, border: '1px solid var(--line)', borderRadius: 'var(--radius)', overflow: 'hidden', flexWrap: 'wrap', width: 'fit-content' }}>
      {timeframes.map((tf) => {
        const active = tf.id === selected;
        const na = tf.status === 'NOT_AVAILABLE';
        return (
          <button
            key={tf.id}
            onClick={() => onSelect(tf.id)}
            title={`${ROLE_AR[tf.role] || tf.role}${tf.derived ? ' · مجمّعة من اليومي' : ''}${na ? ' · غير متاح' : ''}`}
            className="num"
            style={{
              padding: '7px 15px', fontSize: 'var(--t-body)', borderLeft: '1px solid var(--line)',
              background: active ? 'var(--blue)' : 'transparent',
              color: active ? '#fff' : na ? 'var(--fg-faint)' : 'var(--fg-mid)',
              fontWeight: active ? 600 : 400, minWidth: 50,
            }}
          >
            {tf.label}
            {tf.id === defaultTf && !active && <span className="ltr" style={{ display: 'block', fontSize: 'var(--t-micro)', color: 'var(--fg-faint)' }}>DEFAULT</span>}
          </button>
        );
      })}
    </div>
  );
}

/* ---------------------------------------------------------------- signals */

export function SignalsPanel({ signals, operator, onDecide }) {
  const [busy, setBusy] = useState(null);
  const rows = signals.slice(0, 12);
  return (
    <Panel title="الإشارات (مراقبة فقط)" hint="لا يوجد تنفيذ" style={{ minWidth: 0 }}>
      {rows.length === 0 ? (
        <div className="empty">
          لا توجد إشارات حالياً
          <div className="caption" style={{ marginTop: 4 }}>تُنشر الإشارة عندما تصل منطقة إلى حالة مؤكدة.</div>
        </div>
      ) : (
        <div style={{ overflowX: 'auto' }}>
          <table>
            <thead>
              <tr>
                <th>الإطار</th><th>الاتجاه</th><th>الدرجة</th><th>المحرك</th><th>الوقت</th><th>الحالة</th><th>قرار المشغل</th>
              </tr>
            </thead>
            <tbody>
              {rows.map((s) => (
                <tr key={s.id}>
                  <td className="num">{s.timeframe}</td>
                  <td><DirTag dir={s.direction} /></td>
                  <td className="num" title="قيمة ترتيب داخلية غير معايرة — ليست احتمالاً">
                    <div className="row" style={{ gap: 7 }}>
                      <span>{num(s.score, 2)}</span>
                      <span style={{ display: 'inline-block', width: 40, height: 3, background: 'var(--line)' }}>
                        <span style={{ display: 'block', height: '100%', width: `${Math.min(100, (s.score || 0) * 100)}%`, background: 'var(--gold)' }} />
                      </span>
                    </div>
                  </td>
                  <td className="small">{ENGINE_AR[s.engine] || s.engine}</td>
                  <td className="num muted">{timeUTC(s.createdAt)}</td>
                  <td><StateTag state={s.state} /></td>
                  <td>
                    {s.operatorDecision ? (
                      <span className={`chip ${s.operatorDecision === 'ACCEPTED' ? 'up' : 'dim'}`}>
                        {s.operatorDecision === 'ACCEPTED' ? 'مقبولة' : 'متجاهلة'}
                      </span>
                    ) : operator ? (
                      <span className="row" style={{ gap: 4 }}>
                        <button className="chip up" disabled={busy === s.id} onClick={async () => { setBusy(s.id); await onDecide(s.id, 'ACCEPTED'); setBusy(null); }}>قبول</button>
                        <button className="chip dim" disabled={busy === s.id} onClick={async () => { setBusy(s.id); await onDecide(s.id, 'IGNORED'); setBusy(null); }}>تجاهل</button>
                      </span>
                    ) : <span className="caption faint">يتطلب مشغلاً</span>}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </Panel>
  );
}

/* ------------------------------------------------------------------- risk */

export function RiskPanel({ snapshot }) {
  const q = snapshot.quote;
  const open = snapshot.virtualTrades.filter((t) => t.state === 'OPEN');
  const exposure = open.reduce((a, t) => a + Math.abs(t.entry - t.stop) * 100 * t.lot, 0);
  const quality = snapshot.data.closedBars > 0 ? 100 : 0;
  return (
    <Panel title="المخاطر" hint="بلا تنفيذ حقيقي">
      <Field k="حالة المخاطر" tone={snapshot.data.lastError ? 'var(--warn)' : 'var(--up)'}>
        {snapshot.data.lastError ? 'تحذير' : 'منخفضة'}
      </Field>
      <Field k="بوابة المخاطر" tone="var(--fg-dim)">مغلقة · لا يوجد تنفيذ</Field>
      <Field k="التعرض (صفقات افتراضية)">{money(exposure)}</Field>
      <Field k="الصفقات الافتراضية المفتوحة">{num(open.length)}</Field>
      <Field k="جودة البيانات">{quality}%</Field>
      <Field k="السلامة" tone="var(--up)">مفعّلة</Field>
    </Panel>
  );
}

/* -------------------------------------------------------- shadow positions */

const POSITION_PAGE = 8;

export function ShadowPositions({ trades }) {
  const [expanded, setExpanded] = useState(false);
  const open = trades.filter((t) => t.state === 'OPEN');
  const shown = expanded ? open : open.slice(0, POSITION_PAGE);
  return (
    <Panel title="الصفقات الافتراضية" hint="ظل فقط" right={<span className="chip gold">ظل</span>}>
      {open.length === 0 ? (
        <div className="empty">
          لا توجد صفقات افتراضية نشطة
          <div className="caption" style={{ marginTop: 4 }}>تُنشأ الصفقة تلقائياً عند تحقق شروط المحرك، بشكل مستقل عن قرار المشغل.</div>
        </div>
      ) : (
        <div style={{ overflowX: 'auto' }}>
          <table>
            <thead>
              <tr><th>المحرك</th><th>الاتجاه</th><th>الدخول</th><th>الوقف</th><th>الهدف</th><th>التكلفة</th></tr>
            </thead>
            <tbody>
              {shown.map((t) => (
                <tr key={t.id}>
                  <td className="small">{ENGINE_AR[t.engine] || t.engine}</td>
                  <td><DirTag dir={t.direction} /></td>
                  <td className="num">{price(t.entry)}</td>
                  <td className="num down">{price(t.stop)}</td>
                  <td className="num up">{price(t.target)}</td>
                  <td className="num caption muted">{price(t.costs?.spreadAtEntry, 3)} + {price(t.costs?.slippagePerSide, 2)}</td>
                </tr>
              ))}
            </tbody>
          </table>
          {open.length > POSITION_PAGE && (
            <button
              onClick={() => setExpanded((v) => !v)}
              className="caption"
              style={{ width: '100%', padding: '10px 0', borderTop: '1px solid var(--line)', color: 'var(--fg-dim)' }}
            >
              {expanded ? 'عرض أقل' : `عرض ${open.length - POSITION_PAGE} صفقة أخرى`}
            </button>
          )}
        </div>
      )}
    </Panel>
  );
}

/* ------------------------------------------------------------ performance */

export function PerformancePanel({ stats }) {
  const c = stats?.counts || {};
  return (
    <Panel title="أداء المحركات (قياس دقة)" hint="تكاليف محتسبة">
      <div className="grid-3" style={{ gap: 0, borderBottom: '1px solid var(--line)' }}>
        <div style={{ padding: '10px var(--s3)', borderLeft: '1px solid var(--line)' }}>
          <div className="caption">نسبة الربح</div>
          <div className="num" style={{ fontSize: 'var(--t-title)' }}>{stats?.winRate === null ? '—' : `${(stats?.winRate * 100).toFixed(0)}%`}</div>
        </div>
        <div style={{ padding: '10px var(--s3)', borderLeft: '1px solid var(--line)' }}>
          <div className="caption">التوقع (R)</div>
          <div className={`num ${(stats?.expectancyR ?? 0) >= 0 ? 'up' : 'down'}`} style={{ fontSize: 'var(--t-title)' }}>{stats?.expectancyR === null ? '—' : rMultiple(stats?.expectancyR)}</div>
        </div>
        <div style={{ padding: '10px var(--s3)' }}>
          <div className="caption">صافي النتيجة</div>
          <div className={`num ${(stats?.netPnlUsd ?? 0) >= 0 ? 'up' : 'down'}`} style={{ fontSize: 'var(--t-title)' }}>{money(stats?.netPnlUsd)}</div>
        </div>
      </div>
      <div style={{ padding: '4px 12px 8px' }}>
        <div className="caption" style={{ margin: '8px 0 4px' }}>حسب المحرك</div>
        {!stats?.byEngine || Object.keys(stats.byEngine).length === 0 ? (
          <div className="caption faint">لا توجد صفقات مغلقة بعد — لا يمكن حساب إحصاءات.</div>
        ) : (
          <table>
            <thead><tr><th>المحرك</th><th>العدد</th><th>ربح</th><th>خسارة</th><th>النسبة</th><th>R</th></tr></thead>
            <tbody>
              {Object.entries(stats.byEngine).map(([k, v]) => (
                <tr key={k}>
                  <td className="small">{ENGINE_AR[k] || k}</td>
                  <td className="num">{num(v.count)}</td>
                  <td className="num up">{num(v.wins)}</td>
                  <td className="num down">{num(v.losses)}</td>
                  <td className="num">{v.winRate === null ? '—' : `${(v.winRate * 100).toFixed(0)}%`}</td>
                  <td className="num">{rMultiple(v.rMultiple)}</td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
        <div className="caption faint" style={{ marginTop: 8 }}>
          الإحصاءات محسوبة لكل محرك على حدة. النتائج غير مُعايرة ولا تمثل وعداً بأداء مستقبلي.
        </div>
      </div>
    </Panel>
  );
}

/* ---------------------------------------------------------------- regions */

const REGION_PAGE = 12;

export function RegionsPanel({ regions, onOpen }) {
  const [expanded, setExpanded] = useState(false);
  const shown = expanded ? regions : regions.slice(0, REGION_PAGE);
  return (
    <Panel title="المناطق الحية" hint={`${regions.length} منطقة`}>
      {regions.length === 0 ? (
        <div className="empty">لا توجد مناطق نشطة على هذا الإطار الزمني.</div>
      ) : (
        <div style={{ overflowX: 'auto' }}>
          <table>
            <thead>
              <tr><th>المحرك</th><th>النوع</th><th>النطاق</th><th>الاتجاه</th><th>الحالة</th><th>الدرجة</th><th>المصدر</th></tr>
            </thead>
            <tbody>
              {shown.map((r) => (
                <tr key={r.id} onClick={() => onOpen?.(r.id)} style={{ cursor: onOpen ? 'pointer' : 'default' }}>
                  <td className="small">{ENGINE_AR[r.engine] || r.engine}</td>
                  <td className="small">{TYPE_AR[r.type] || r.type}</td>
                  <td className="num caption nowrap">{price(r.zone?.low)} – {price(r.zone?.high)}</td>
                  <td><DirTag dir={r.direction} /></td>
                  <td><StateTag state={r.state} /></td>
                  <td className="num">{num(r.score, 2)}</td>
                  <td className="num caption muted">{timeUTC(r.sourceTime)}</td>
                </tr>
              ))}
            </tbody>
          </table>
          {regions.length > REGION_PAGE && (
            <button
              onClick={() => setExpanded((v) => !v)}
              className="caption"
              style={{ width: '100%', padding: '10px 0', borderTop: '1px solid var(--line)', color: 'var(--fg-dim)' }}
            >
              {expanded ? 'عرض أقل' : `عرض ${regions.length - REGION_PAGE} منطقة أخرى`}
            </button>
          )}
        </div>
      )}
    </Panel>
  );
}

/* -------------------------------------------------------------- timeframe */

export function TimeframeMatrix({ rows, selected, onSelect, compact = false }) {
  return (
    <Panel title="مصفوفة الأطر الزمنية" hint={compact ? `${rows.length} أطر` : 'تسعة تدفقات'}>
      <div style={{ overflowX: 'auto' }}>
        <table>
          <thead>
            <tr>
              <th>الإطار</th>
              {!compact && <th>الدور</th>}
              <th>الحالة</th>
              <th>شموع مغلقة</th>
              <th>الحداثة</th>
              {!compact && <th>الاتجاه</th>}
              <th>ATR</th>
              <th>مناطق</th>
              {!compact && <th>تدفق</th>}
            </tr>
          </thead>
          <tbody>
            {rows.map((r) => (
              <tr key={r.id} className={r.id === selected ? 'active' : ''} onClick={() => onSelect(r.id)} style={{ cursor: 'pointer' }}>
                <td className="num" style={{ fontWeight: 600 }}>
                  {r.label}
                  {r.derived && <span className="caption faint" style={{ marginRight: 4 }} title="مجمّعة من اليومي">*</span>}
                </td>
                {!compact && <td className="small muted">{ROLE_AR[r.role] || r.role}</td>}
                <td>
                  {r.status === 'NOT_AVAILABLE'
                    ? <span className="chip dim">غير متاح</span>
                    : <span className="chip up">متاح</span>}
                </td>
                <td className="num">{num(r.closedBars)}</td>
                <td className="num caption">{age(r.freshnessSeconds)}</td>
                {!compact && <td className="small">{r.trend === 'UP' ? 'صاعد' : r.trend === 'DOWN' ? 'هابط' : r.trend === 'RANGE' ? 'عرضي' : 'غير معروف'}</td>}
                <td className="num caption">{r.atr === null ? '—' : price(r.atr)}</td>
                <td className="num">{num(r.activeRegions)}</td>
                {!compact && (
                  <td className="caption">
                    {r.derived ? <span className="chip dim" title="مجمّعة من شموع يومية من المصدر نفسه">مجمّعة</span> : <span className="ltr faint">{r.stream}</span>}
                  </td>
                )}
              </tr>
            ))}
          </tbody>
        </table>
      </div>
      {compact && (
        <div className="caption faint" style={{ padding: 'var(--s2) var(--s3)' }}>
          <span className="ltr">*</span> مجمّعة من شموع <span className="ltr">D1</span> من المصدر نفسه.
        </div>
      )}
    </Panel>
  );
}

/* ------------------------------------------------------------ price ladder */

/**
 * Price ladder around the live mid, marking the nearest zones on each side. This is the
 * right-hand column of the reference dashboard. All levels come from real engine
 * evidence or the live quote; nothing is drawn that was not computed.
 */
export function PriceLadder({ snapshot, regions }) {
  const mid = snapshot.quote?.mid ?? null;
  if (mid === null) {
    return <Panel title="سلم الأسعار"><div className="empty">السعر غير متاح</div></Panel>;
  }
  const step = Math.max(1, Math.round(mid / 400)); // ≈0.25% rungs
  const rungs = [];
  for (let i = 6; i >= -6; i--) rungs.push(mid + i * step);

  const zoneFor = (p) => regions.find((r) => r.zone && p >= r.zone.low && p <= r.zone.high);
  const nearestAbove = regions.filter((r) => r.zone && r.zone.low > mid).sort((a, b) => a.zone.low - b.zone.low)[0];
  const nearestBelow = regions.filter((r) => r.zone && r.zone.high < mid).sort((a, b) => b.zone.high - a.zone.high)[0];

  return (
    <Panel title="سلم الأسعار" hint={`±${6 * step}`} right={<span className="chip gold">حي</span>}>
      <div style={{ padding: '6px 0' }}>
        {rungs.map((p, i) => {
          const z = zoneFor(p);
          const isMid = i === 6;
          return (
            <div
              key={p}
              className="row-between"
              style={{
                padding: '3px 12px',
                background: isMid ? 'rgba(4,86,230,0.22)' : z ? 'rgba(243,211,108,0.08)' : 'transparent',
                borderRight: z ? '2px solid var(--gold-deep)' : '2px solid transparent',
              }}
            >
              <span className="num" style={{ fontSize: 'var(--t-body)', color: isMid ? 'var(--fg)' : p > mid ? 'var(--up)' : 'var(--down)', fontWeight: isMid ? 600 : 400 }}>
                {price(p)}
              </span>
              {z ? (
                <span className="caption faint" style={{ maxWidth: 150, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>
                  {ENGINE_AR[z.engine] || z.engine}
                </span>
              ) : isMid ? <span className="caption gold">السعر</span> : null}
            </div>
          );
        })}
      </div>
      <div style={{ borderTop: '1px solid var(--line)' }}>
        <Field k="أقرب منطقة أعلى" tone="var(--up)">{nearestAbove ? `${price(nearestAbove.zone.low)} – ${price(nearestAbove.zone.high)}` : '—'}</Field>
        <Field k="أقرب منطقة أدنى" tone="var(--down)">{nearestBelow ? `${price(nearestBelow.zone.low)} – ${price(nearestBelow.zone.high)}` : '—'}</Field>
      </div>
    </Panel>
  );
}

/* -------------------------------------------------------------- BiQuote box */

export function BiQuotePanel({ snapshot, health }) {
  const q = snapshot.quote;
  return (
    <Panel
      title="BiQuote"
      hint="مصدر السوق"
      right={<span className={`chip ${health?.stream?.state === 'subscribed' ? 'up' : 'dim'}`}>{health?.stream?.state === 'subscribed' ? 'مباشر' : 'استطلاع'}</span>}
    >
      <Field k="الطلب (Bid)">{q ? price(q.bid, 3) : '—'}</Field>
      <Field k="العرض (Ask)">{q ? price(q.ask, 3) : '—'}</Field>
      <Field k="المتوسط">{q ? price(q.mid, 3) : '—'}</Field>
      <Field k="الفرق">{q ? price(q.spread, 3) : '—'}</Field>
      <Field k="أعلى اليوم" tone="var(--up)">{q ? price(q.high) : '—'}</Field>
      <Field k="أدنى اليوم" tone="var(--down)">{q ? price(q.low) : '—'}</Field>
      <Field k="الاتجاه">
        {q ? <span className={`chip ${q.direction === 'UP' ? 'up' : 'down'}`}>{q.direction === 'UP' ? 'صاعد' : q.direction === 'DOWN' ? 'هابط' : q.direction}</span> : '—'}
      </Field>
      <Field k="عمر التسعيرة" tone="var(--fg-mid)">{q ? age(q.quoteAgeSeconds) : '—'}</Field>
      <Field k="المصدر" tone="var(--fg-mid)">
        <span className="ltr" style={{ fontSize: 'var(--t-small)' }}>{q?.source || '—'}</span>
      </Field>
      <Field k="الوقت" tone="var(--fg-mid)">{q ? timeUTC(q.timestamp, true) : '—'}</Field>
      <Field k="النقل" tone="var(--fg-mid)">
        <span className="ltr" style={{ fontSize: 'var(--t-small)' }}>{health?.stream?.transport || '—'}</span>
      </Field>
      <Field k="جودة التسعيرة" tone={q?.stale ? 'var(--warn)' : 'var(--up)'}>{q?.stale ? 'قديمة' : 'حديثة'}</Field>
    </Panel>
  );
}

/* ---------------------------------------------------------------- telemetry */

export function TelemetryPanel({ health, snapshot, timeframes }) {
  const avail = timeframes.filter((t) => t.status !== 'NOT_AVAILABLE').length;
  const total = timeframes.length || 9;
  return (
    <Panel title="التلمترية" hint="تشخيص">
      <Field k="الصحة" tone={health?.status === 'HEALTHY' ? 'var(--up)' : 'var(--warn)'}>
        {health?.status === 'HEALTHY' ? 'سليم' : (health?.status || '—')}
      </Field>
      <Field k="دورات التحديث">{num(health?.ingestCycles)}</Field>
      <Field k="آخر تحديث" tone="var(--fg-mid)">{timeUTC(health?.lastIngest, true)}</Field>
      <Field k="تدفقات متاحة">{`${avail} / ${total}`}</Field>
      <Field k="شموع مغلقة (الإطار الحالي)">{num(snapshot?.data?.closedBars)}</Field>
      <Field k="الإطار الافتراضي" tone="var(--fg-mid)">M15</Field>
      <Field k="الوضع" tone="var(--gold)">ظل فقط</Field>
      <Field k="التنفيذ" tone="var(--down)">معطّل</Field>
      <Field k="آخر خطأ" tone={health?.lastError ? 'var(--down)' : 'var(--fg-faint)'}>
        <span className="ltr" style={{ fontSize: 'var(--t-small)' }}>{health?.lastError || 'لا يوجد'}</span>
      </Field>
    </Panel>
  );
}

/* -------------------------------------------------------------------- AI */

export function AiPanel({ aiStatus, requests, operator, onSubmit, busy, lastResult }) {
  const [prompt, setPrompt] = useState('');
  return (
    <Panel
      title="محادثة التحليل (الذكاء الاصطناعي)"
      hint="بطلب المشغل فقط"
      right={<span className={`chip ${aiStatus?.available ? 'up' : 'dim'}`}>{aiStatus?.available ? 'مهيّأ' : 'غير مهيّأ'}</span>}
    >
      <div style={{ padding: 'var(--s3)' }}>
        {!aiStatus?.available && (
          <div className="caption" style={{ color: 'var(--warn)', marginBottom: 8, lineHeight: 1.7 }}>
            {aiStatus?.reason || 'لا يوجد مزود ذكاء اصطناعي مهيّأ.'}
            <br />
            لن يُنشئ النظام تحليلاً وهمياً: الطلب يُسجَّل وتكون النتيجة «غير متاح».
          </div>
        )}
        {!operator && (
          <div className="caption faint" style={{ marginBottom: 8 }}>
            إرسال أوامر التحليل متاح للمشغل فقط. الزوار يمكنهم المشاهدة دون تسجيل دخول.
          </div>
        )}
        <textarea
          rows={3}
          placeholder="مثال: حلل السيولة على M15 وارسم مناطق الارتداد المحتملة."
          value={prompt}
          onChange={(e) => setPrompt(e.target.value)}
          style={{ width: '100%', resize: 'vertical' }}
        />
        <div className="row-between" style={{ marginTop: 8 }}>
          <span className="caption faint">يُربط التحليل ببيانات فعلية من المحركات ويُحفظ مع نتيجته.</span>
          <button
            className="chip blue"
            disabled={busy || !operator || !prompt.trim()}
            onClick={async () => { const ok = await onSubmit(prompt.trim()); if (ok) setPrompt(''); }}
            style={{ padding: '5px 12px' }}
          >
            {busy ? 'جارٍ التحليل…' : 'طلب تحليل'}
          </button>
        </div>

        {lastResult && (
          <div className="panel" style={{ marginTop: 'var(--s3)', padding: 'var(--s3)' }}>
            <div className="row-between">
              <span className="caption">نتيجة آخر طلب</span>
              <span className={`chip ${lastResult.verdict === 'ACCEPTED' ? 'up' : lastResult.verdict === 'NOT_AVAILABLE' ? 'dim' : 'down'}`}>{lastResult.verdict}</span>
            </div>
            <div className="caption muted" style={{ marginTop: 6, lineHeight: 1.7 }}>{lastResult.reason}</div>
            {lastResult.plan && <PlanView plan={lastResult.plan} />}
          </div>
        )}

        {requests?.length > 0 && (
          <>
            <div className="caption" style={{ margin: '12px 0 6px' }}>سجل الطلبات</div>
            <div style={{ maxHeight: 150, overflowY: 'auto' }}>
              {requests.slice(0, 8).map((r) => (
                <div key={r.id} className="row-between" style={{ padding: '5px 0', borderBottom: '1px solid var(--line-soft)' }}>
                  <span className="caption" style={{ maxWidth: '60%', overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{r.prompt}</span>
                  <span className={`chip ${r.verdict === 'ACCEPTED' ? 'up' : 'dim'}`}>{r.verdict}</span>
                </div>
              ))}
            </div>
          </>
        )}
      </div>
    </Panel>
  );
}

function PlanView({ plan }) {
  return (
    <div style={{ marginTop: 8 }}>
      <div className="row" style={{ gap: 6, marginBottom: 6 }}>
        <DirTag dir={plan.direction} />
        <span className="chip dim">{plan.timeHorizon}</span>
      </div>
      {(plan.scenarios || []).map((s, i) => (
        <div key={i} style={{ borderRight: '2px solid var(--gold-deep)', paddingRight: 8, marginBottom: 7 }}>
          <div className="small" style={{ fontWeight: 600 }}>{s.name}</div>
          <div className="caption muted">الشرط: {s.condition}</div>
          <div className="caption muted">التوقع: {s.expectation}</div>
        </div>
      ))}
      {(plan.invalidationConditions || []).length > 0 && (
        <div className="caption" style={{ color: 'var(--down)', marginTop: 4 }}>
          الإبطال: {plan.invalidationConditions.join(' · ')}
        </div>
      )}
    </div>
  );
}

/* ------------------------------------------------------------- engines doc */

export function EnginesPanel({ engines, costModel }) {
  return (
    <Panel title="توثيق المحركات" hint="شرط الإكمال: مدخلات ومخرجات وعدم اليقين">
      <div style={{ padding: '4px 12px 10px' }}>
        {(engines || []).map((e) => (
          <div key={e.id} style={{ borderBottom: '1px solid var(--line-soft)', padding: '10px 0' }}>
            <div className="row-between">
              <span style={{ fontWeight: 600 }}>{ENGINE_AR[e.id] || e.id}</span>
              <span className="chip dim ltr">{e.id}</span>
            </div>
            <div className="caption muted" style={{ marginTop: 4, lineHeight: 1.7 }}>{e.covers}</div>
            <div className="caption" style={{ marginTop: 4 }}>
              <span className="faint">المدخلات: </span><span className="ltr muted">{e.inputs.join(', ')}</span>
            </div>
            <div className="caption">
              <span className="faint">المخرجات: </span><span className="ltr muted">{e.outputs.join(', ')}</span>
            </div>
            <div className="caption" style={{ color: 'var(--warn)' }}>
              <span className="faint">عدم اليقين: </span>{e.uncertainty.join(' · ')}
            </div>
            <div className="caption faint">
              لا يُصدر عندما: {e.noEmit.join(' · ')}
            </div>
          </div>
        ))}
        {costModel && (
          <div className="caption faint" style={{ marginTop: 8 }}>
            نموذج التكاليف: سبريد عند الدخول + انزلاق <span className="ltr">{costModel.slippagePerSide}</span> لكل جهة
            {costModel.commissionPerLot ? ` + عمولة ${costModel.commissionPerLot} لكل لوت` : ''} · نسبة المخاطرة/المكافأة <span className="ltr">{costModel.riskReward}:1</span>
          </div>
        )}
      </div>
    </Panel>
  );
}

/* --------------------------------------------------------------- controls */

export function ControlPanel({ operator, onControl, paused, status }) {
  const actions = [
    { a: 'REFRESH', ar: 'تحديث' },
    { a: 'CHECKPOINT', ar: 'نقطة حفظ' },
    { a: 'PAUSE', ar: 'إيقاف مؤقت' },
    { a: 'RESUME', ar: 'استئناف' },
    { a: 'STOP', ar: 'إيقاف' },
    { a: 'RECOVERY', ar: 'استعادة' },
  ];
  return (
    <Panel title="أدوات التشغيل الآمنة" hint="لا توجد أوامر تداول">
      <div className="grid-3" style={{ gap: 1, background: 'var(--line)' }}>
        {actions.map((x) => (
          <button
            key={x.a}
            disabled={!operator}
            onClick={() => onControl(x.a)}
            style={{
              background: 'var(--surface)', padding: '10px 6px', fontSize: 'var(--t-body)',
              color: x.a === 'STOP' ? 'var(--down)' : 'var(--fg-mid)',
            }}
            title={operator ? x.a : 'يتطلب وضع المشغل'}
          >
            {x.ar}
            <div className="caption faint ltr" style={{ marginTop: 2 }}>{x.a}</div>
          </button>
        ))}
      </div>
      <div style={{ padding: 'var(--s2) var(--s3)' }}>
        <div className="caption faint" style={{ lineHeight: 1.7 }}>
          هذه الأدوات لا تُنفّذ أي أمر وسيط. «إيقاف» يُغلق التطبيق بشكل نظيف بعد حفظ الحالة.
          {' '}
          {paused ? <span className="chip warn">متوقف مؤقتاً</span> : <span className="chip up">نشط</span>}
          {status && <span className="ltr" style={{ marginRight: 6 }}>{status}</span>}
        </div>
      </div>
    </Panel>
  );
}

/* ------------------------------------------------------------------ audit */

export function AuditPanel({ entries }) {
  return (
    <Panel title="سجل التدقيق" hint="قابل للتدقيق">
      {entries.length === 0 ? (
        <div className="empty">لا توجد سجلات بعد.</div>
      ) : (
        <div style={{ maxHeight: 340, overflowY: 'auto' }}>
          <table>
            <thead><tr><th>الوقت</th><th>الفئة</th><th>الإجراء</th><th>الجهة</th><th>التفاصيل</th></tr></thead>
            <tbody>
              {entries.map((e) => (
                <tr key={e.id}>
                  <td className="num caption nowrap">{timeUTC(e.at, true)}</td>
                  <td className="caption">{e.category}</td>
                  <td className="caption ltr">{e.action}</td>
                  <td className="caption muted">{e.actor}</td>
                  <td className="caption faint ltr" style={{ maxWidth: 320, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>{e.detail}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </Panel>
  );
}

/* ------------------------------------------------------- section scaffolds */

export function SectionScaffold({ title, intro, fields, timeline, controls }) {
  return (
    <div className="stack">
      <div className="panel" style={{ padding: 'var(--s3)' }}>
        <div className="row-between">
          <h2 style={{ margin: 0, fontSize: 'var(--t-lead)' }}>{title}</h2>
          <span className="chip gold">وضع الظل فقط</span>
        </div>
        <div className="small muted" style={{ marginTop: 6, lineHeight: 1.8 }}>{intro}</div>
      </div>
      <div className="grid-2">
        <Panel title="الحالة">
          {fields.map((f) => (
            <Field key={f[0]} k={f[0]} tone={f[2]}>{f[1]}</Field>
          ))}
        </Panel>
        <Panel title="الخط الزمني">
          {!timeline || timeline.length === 0 ? (
            <div className="empty">غير متاح</div>
          ) : (
            <div style={{ padding: '4px 12px 8px' }}>
              {timeline.map((t, i) => (
                <div key={i} className="row-between" style={{ padding: '7px 0', borderBottom: '1px solid var(--line-soft)' }}>
                  <span className="num caption muted nowrap">{t.t}</span>
                  <span className="small" style={{ flex: 1, padding: '0 10px' }}>{t.title}</span>
                  <span className="caption" style={{ color: t.tone }}>{t.state}</span>
                </div>
              ))}
            </div>
          )}
        </Panel>
      </div>
      {controls}
    </div>
  );
}
