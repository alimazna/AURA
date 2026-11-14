# DATA_MODEL.md — نموذج البيانات

> **المستند 9 من 17** | الحالة: مسودة | آخر تحديث: 2026-11-14
> كل التوقيتات **UTC** في قاعدة البيانات، وتُعرض محليًا في الواجهة.

## 1. المبادئ

| المبدأ | التطبيق |
|---|---|
| Append-only للحقيقة | `scenario_outcomes` و`audit_log` لا يُحدَّثان ولا يُحذفان |
| الأصل محفوظ | `scenarios` يحفظ النسخة المنشورة؛ التغييرات في `scenario_status_history` |
| Provenance | كل صف بيانات سوق يحمل `source_id` + `fetched_at` |
| Idempotency | مفاتيح فريدة على (instrument, timeframe, open_time, source_id) |
| لا حذف | السجلات التاريخية تُؤرشف (`archived_at`) لا تُحذف |
| التتبع | `created_at`, `updated_at`, `created_by` على الجداول القابلة للتحرير |

## 2. الكيانات الأساسية

### 2.1 Users & Access

```sql
users (
  id            UUID PK,
  email         CITEXT UNIQUE,
  display_name  TEXT,
  locale        TEXT DEFAULT 'ar',
  timezone      TEXT DEFAULT 'UTC',
  risk_ack_at   TIMESTAMPTZ,          -- متى أقرّ بمقدمة المخاطر
  is_operator   BOOLEAN DEFAULT false,
  created_at    TIMESTAMPTZ NOT NULL,
  updated_at    TIMESTAMPTZ NOT NULL
)

sessions (
  id            UUID PK,
  user_id       UUID FK,
  token_hash    TEXT,
  expires_at    TIMESTAMPTZ,
  created_at    TIMESTAMPTZ
)

audit_log (
  id            BIGSERIAL PK,
  actor_id      UUID NULL,
  actor_kind    TEXT,        -- 'user' | 'operator' | 'system'
  action        TEXT,        -- 'scenario.publish' | 'scenario.cancel' | ...
  entity_type   TEXT,
  entity_id     TEXT,
  before_state  JSONB,
  after_state   JSONB,
  reason        TEXT,
  ip            INET,
  created_at    TIMESTAMPTZ NOT NULL
)
```

### 2.2 Market Data

```sql
instruments (
  id            TEXT PK,               -- 'XAUUSD'
  name          TEXT,                  -- 'Gold Spot / US Dollar'
  quote_currency TEXT,                 -- 'USD'
  base_asset    TEXT,                  -- 'XAU'
  pip_size      NUMERIC,               -- حجم النقطة
  contract_size NUMERIC,
  is_active     BOOLEAN
)

data_sources (
  id            TEXT PK,               -- 'twelvedata' | 'polygon' | 'alphavantage'
  kind          TEXT,                  -- 'primary' | 'backup' | 'reference'
  license_type  TEXT,                  -- 'external_display' | 'internal'
  license_expires_at TIMESTAMPTZ,
  is_active     BOOLEAN
)

-- Hypertable (TimescaleDB) — الشموع
candles (
  instrument_id TEXT NOT NULL,
  timeframe     TEXT NOT NULL,         -- M1,M5,M15,M30,H1,H4,D1,W1,MN1
  open_time     TIMESTAMPTZ NOT NULL,
  close_time    TIMESTAMPTZ NOT NULL,
  open          NUMERIC NOT NULL,
  high          NUMERIC NOT NULL,
  low           NUMERIC NOT NULL,
  close         NUMERIC NOT NULL,
  tick_volume   BIGINT,
  spread_avg    NUMERIC,
  is_closed     BOOLEAN NOT NULL,      -- الشمعة المكتملة فقط تُستخدم للتأكيد
  source_id     TEXT NOT NULL,
  quality       TEXT NOT NULL,         -- 'VALID' | 'INVALID' | 'SUSPECT'
  fetched_at    TIMESTAMPTZ NOT NULL,
  PRIMARY KEY (instrument_id, timeframe, open_time, source_id)
)
-- Hypertable على open_time، ضغط بعد 30 يومًا، احتفاظ دائم للمغلق

-- سجل تصحيحات الشموع (Repaint)
candle_revisions (
  id            BIGSERIAL PK,
  instrument_id TEXT,
  timeframe     TEXT,
  open_time     TIMESTAMPTZ,
  source_id     TEXT,
  field         TEXT,                  -- 'close' مثلاً
  old_value     NUMERIC,
  new_value     NUMERIC,
  detected_at   TIMESTAMPTZ NOT NULL
)

-- حالة الحداثة لكل إطار
freshness_state (
  instrument_id TEXT,
  timeframe     TEXT,
  last_closed_bar_time TIMESTAMPTZ,
  last_received_at     TIMESTAMPTZ,
  state         TEXT,   -- 'FRESH' | 'AGING' | 'STALE' | 'INTERRUPTED'
  PRIMARY KEY (instrument_id, timeframe)
)

-- التيكات اللحظية (احتفاظ قصير)
quotes_live (
  instrument_id TEXT,
  ts            TIMESTAMPTZ,
  bid           NUMERIC,
  ask           NUMERIC,
  mid           NUMERIC,
  spread        NUMERIC,
  source_id     TEXT,
  PRIMARY KEY (instrument_id, ts, source_id)
)

validation_events (
  id            BIGSERIAL PK,
  instrument_id TEXT,
  timeframe     TEXT,
  open_time     TIMESTAMPTZ,
  source_id     TEXT,
  rule          TEXT,   -- 'INVALID_OHLC' | 'GAP_DETECTED' | 'STALE' | 'DATA_CONFLICT'
  detail        JSONB,
  created_at    TIMESTAMPTZ NOT NULL
)

market_sessions (
  id            TEXT PK,      -- 'SYDNEY' | 'TOKYO' | 'LONDON' | 'NEW_YORK'
  opens_at_utc  TIME,
  closes_at_utc TIME,
  timezone      TEXT
)
```

### 2.3 Economic Events

```sql
economic_events (
  id            UUID PK,
  title         TEXT NOT NULL,
  country       TEXT,
  category      TEXT,        -- 'RATES' | 'INFLATION' | 'EMPLOYMENT' | 'GROWTH' | 'OTHER'
  importance    SMALLINT,    -- 1..3
  scheduled_at  TIMESTAMPTZ NOT NULL,
  actual        TEXT,
  forecast      TEXT,
  previous      TEXT,
  is_unexpected BOOLEAN DEFAULT false,   -- حدث غير مجدول
  source_id     TEXT,
  fetched_at    TIMESTAMPTZ NOT NULL
)

event_reactions (              -- PILLAR 5
  id            UUID PK,
  event_id      UUID FK,
  instrument_id TEXT,
  window_minutes SMALLINT,      -- 15 | 60 | 240 | 1440
  price_before  NUMERIC,
  price_after   NUMERIC,
  max_adverse   NUMERIC,
  max_favorable NUMERIC,
  direction     TEXT,           -- 'UP' | 'DOWN' | 'FLAT'
  recorded_at   TIMESTAMPTZ NOT NULL
)
```

### 2.4 Context & Evidence

```sql
market_context_snapshots (
  id            UUID PK,
  instrument_id TEXT,
  as_of         TIMESTAMPTZ NOT NULL,
  higher_tf_structure JSONB,   -- بنية H4/D1/W1
  trend         TEXT,          -- 'UP' | 'DOWN' | 'RANGE'
  volatility    JSONB,         -- ATR وقيم مقارنة
  regime        TEXT,          -- 'TRENDING' | 'RANGING' | 'EXPANSION' | 'CONTRACTION' | 'EVENT'
  liquidity     JSONB,
  session       TEXT,
  usd_context   JSONB,
  rate_expectation JSONB,
  upcoming_events UUID[],
  data_quality  TEXT,          -- 'VERIFIED' | 'DEGRADED'
  engine_version TEXT NOT NULL,
  created_at    TIMESTAMPTZ NOT NULL
)

evidence_records (
  id            UUID PK,
  scenario_id   UUID FK NULL,  -- NULL إن كانت أدلة سياق عامة
  kind          TEXT NOT NULL, -- 'SUPPORTING' | 'CONTRADICTING'
  category      TEXT NOT NULL, -- 'MARKET_STRUCTURE' | 'TECHNICAL' | 'EVENT' | 'LIQUIDITY' | 'MACRO'
  description   TEXT NOT NULL,
  data_refs     JSONB NOT NULL,-- مراجع (candle ids / event ids / snapshot id)
  observed_at   TIMESTAMPTZ NOT NULL,
  freshness     TEXT NOT NULL, -- 'FRESH' | 'AGING' | 'STALE'
  quality       TEXT NOT NULL, -- 'VERIFIED' | 'DEGRADED' | 'UNVERIFIED'
  created_at    TIMESTAMPTZ NOT NULL
)
```

### 2.5 Scenarios

```sql
scenarios (
  id            UUID PK,                    -- ULID
  instrument_id TEXT NOT NULL,              -- 'XAUUSD'
  direction     TEXT NOT NULL,              -- 'BUY' | 'SELL'
  entry_type    TEXT NOT NULL,              -- 'PRICE' | 'ZONE'
  entry_low     NUMERIC,
  entry_high    NUMERIC,
  entry_price   NUMERIC,
  stop_loss     NUMERIC NOT NULL,
  take_profit_1 NUMERIC NOT NULL,
  take_profit_2 NUMERIC,
  risk_reward   NUMERIC NOT NULL,           -- محسوب حتميًا
  rationale     TEXT NOT NULL,
  beginner_note TEXT,                       -- شرح مبسّط (قد يكون من AI، موسوم)
  activation_conditions  JSONB NOT NULL,
  invalidation_conditions JSONB NOT NULL,
  expiration_conditions  JSONB NOT NULL,
  current_status TEXT NOT NULL,             -- الحالة الحالية
  context_snapshot_id UUID NOT NULL,
  rule_version  TEXT NOT NULL,
  quality_score NUMERIC,                    -- ترتيب داخلي غير مُعاير
  published_at  TIMESTAMPTZ NOT NULL,
  published_by  UUID,                       -- المشغل أو 'system'
  origin        TEXT NOT NULL,              -- 'engine' | 'operator'
  archived_at   TIMESTAMPTZ,
  created_at    TIMESTAMPTZ NOT NULL,
  updated_at    TIMESTAMPTZ NOT NULL
)

scenario_evidence (          -- ربط N:N
  scenario_id   UUID FK,
  evidence_id   UUID FK,
  PRIMARY KEY (scenario_id, evidence_id)
)

scenario_status_history (    -- كل انتقال حالة
  id            BIGSERIAL PK,
  scenario_id   UUID FK,
  from_status   TEXT,
  to_status     TEXT NOT NULL,
  reason        TEXT NOT NULL,
  triggering_data JSONB,      -- الشمعة/السعر/الحدث المُسبّب
  transition_key TEXT UNIQUE NOT NULL,   -- Idempotency
  engine_version TEXT NOT NULL,
  occurred_at   TIMESTAMPTZ NOT NULL,
  created_at    TIMESTAMPTZ NOT NULL
)

scenario_monitoring_events (
  id            BIGSERIAL PK,
  scenario_id   UUID FK,
  event_type    TEXT,         -- 'PRICE_ENTERED_ZONE' | 'TP1_HIT' | 'DATA_INTERRUPTED' | ...
  price         NUMERIC,
  candle_ref    JSONB,
  occurred_at   TIMESTAMPTZ NOT NULL
)

scenario_outcomes (          -- Append-only، لا يُحدَّث
  scenario_id   UUID PK,      -- 1:1 مع السيناريو
  final_status  TEXT NOT NULL,
  final_price   NUMERIC,
  exit_at       TIMESTAMPTZ,
  duration_minutes INTEGER,
  r_multiple    NUMERIC,      -- الربح/الخسارة بوحدات R
  costs_applied JSONB,        -- spread/slippage المحتسبة
  result_class  TEXT NOT NULL,-- 'WIN' | 'LOSS' | 'BREAKEVEN' | 'UNRESOLVED'
  computation_notes TEXT,
  computed_at   TIMESTAMPTZ NOT NULL
)

scenario_follows (           -- متابعة المستخدم (محاكاة)
  id            UUID PK,
  user_id       UUID FK,
  scenario_id   UUID FK,
  action        TEXT,         -- 'FOLLOWED' | 'IGNORED'
  acted_at      TIMESTAMPTZ NOT NULL,
  UNIQUE (user_id, scenario_id)
)
```

### 2.6 AI & Content

```sql
ai_explanations (
  id            UUID PK,
  scenario_id   UUID FK NULL,
  kind          TEXT,        -- 'SCENARIO_EXPLAIN' | 'BEGINNER_SIMPLIFY' | 'ASSISTANT_ANSWER'
  body_ar       TEXT NOT NULL,
  referenced_data JSONB,     -- كل رقم ومصدره
  confidence_band TEXT,      -- 'high' | 'medium' | 'low'  (لا نسبة)
  is_interpretation BOOLEAN NOT NULL DEFAULT true,
  grounding_check TEXT NOT NULL,  -- 'passed' | 'failed'
  model         TEXT,
  prompt_tokens INTEGER,
  completion_tokens INTEGER,
  created_at    TIMESTAMPTZ NOT NULL
)

daily_briefs (
  id            UUID PK,
  brief_date    DATE NOT NULL UNIQUE,
  body_ar       TEXT NOT NULL,
  context_snapshot_id UUID,
  published_scenarios UUID[],
  risks         JSONB,
  uncertainties JSONB,
  invalidation_conditions JSONB,
  model         TEXT,
  created_at    TIMESTAMPTZ NOT NULL
)

ai_cost_ledger (
  id            BIGSERIAL PK,
  task_kind     TEXT,
  model         TEXT,
  provider      TEXT,
  prompt_tokens INTEGER,
  completion_tokens INTEGER,
  cost_usd      NUMERIC(10,6),
  related_entity TEXT,
  created_at    TIMESTAMPTZ NOT NULL
)
```

### 2.7 Engine Rules & Versions

```sql
engine_versions (
  version       TEXT PK,      -- semver
  description   TEXT,
  rules_doc     JSONB,        -- تعريف القواعد المطبقة
  activated_at  TIMESTAMPTZ,
  deactivated_at TIMESTAMPTZ
)

no_opportunity_log (          -- سجل «لا توجد فرصة» — مهم للشفافية
  id            BIGSERIAL PK,
  instrument_id TEXT,
  evaluated_at  TIMESTAMPTZ NOT NULL,
  reason        TEXT NOT NULL,
  context_snapshot_id UUID,
  candidates_evaluated INTEGER,
  rejected_reasons JSONB
)
```

## 3. الفهارس الحرجة

```sql
-- أداء الاستعلامات الرئيسية
CREATE INDEX ON candles (instrument_id, timeframe, open_time DESC);
CREATE INDEX ON scenarios (current_status, published_at DESC);
CREATE INDEX ON scenarios (instrument_id, published_at DESC);
CREATE INDEX ON scenario_status_history (scenario_id, occurred_at);
CREATE INDEX ON evidence_records (scenario_id, kind);
CREATE INDEX ON economic_events (scheduled_at) WHERE is_unexpected = false;
CREATE INDEX ON ai_explanations (scenario_id, created_at DESC);
```

## 4. منع Survivorship Bias (تنفيذ قاعدة DECISION C)

الاستعلامات الإحصائية **يجب** أن تعتمد على `scenarios` (كل المنشور) وليس على
`scenario_outcomes` وحدها. مثال صحيح:

```sql
-- الأداء الكامل مع كل الفئات (لا يخفي غير المفعّل)
SELECT
  COUNT(*) AS total_published,
  COUNT(*) FILTER (WHERE current_status = 'ACTIVATED') AS activated,
  COUNT(*) FILTER (WHERE current_status IN ('WAITING_FOR_ACTIVATION','NOT_ACTIVATED','EXPIRED')) AS never_activated,
  COUNT(*) FILTER (WHERE current_status = 'CANCELLED') AS cancelled,
  COUNT(*) FILTER (WHERE current_status = 'INVALIDATED') AS invalidated,
  COUNT(*) FILTER (WHERE current_status = 'TARGET_REACHED') AS wins,
  COUNT(*) FILTER (WHERE current_status = 'STOP_LOSS_REACHED') AS losses
FROM scenarios
WHERE instrument_id = 'XAUUSD'
  AND published_at >= :from AND published_at < :to;
```

**قاعدة إلزامية:** أي نسبة نجاح تُعرض **يجب** أن تحمل مقامها الصريح من هذا الاستعلام.

## 5. الاحتفاظ والنسخ الاحتياطي

| البيانات | الاحتفاظ | النسخ الاحتياطي |
|---|---|---|
| candles (مغلقة) | دائم | يومي كامل + WAL |
| candles (Live) | 7 أيام | لا (قابلة لإعادة الجلب) |
| quotes_live | 7 أيام | لا |
| scenarios + history + outcomes | دائم (Append-only) | يومي + احتفاظ طويل |
| audit_log | دائم | يومي + تخزين بارد |
| ai_explanations | دائم | يومي |
| ai_cost_ledger | 2 سنة | يومي |

**RPO ≤ 15 دقيقة، RTO ≤ 1 ساعة** (من NFR-9). اختبار استعادة دوري إلزامي.

## 6. قيود التصميم المحسومة

1. لا حذف لسجل تاريخي — فقط `archived_at`.
2. لا تعديل على `scenario_outcomes` بعد الكتابة.
3. لا كتابة من طبقة AI في جداول الحقيقة (تُفرض على مستوى DB Roles).
4. الشمعة غير المكتملة (`is_closed = false`) لا تُستخدم في أي قرار تفعيل/إبطال.
5. كل صف سوق يحمل `source_id` و`fetched_at` بلا استثناء.
