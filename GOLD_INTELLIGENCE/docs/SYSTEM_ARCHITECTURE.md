# SYSTEM_ARCHITECTURE.md — المعمارية التقنية

> **المستند 8 من 17** | الحالة: مسودة PHASE 4 | آخر تحديث: 2026-11-14
> **ينتظر موافقة المالك (PHASE 4) قبل أي تنفيذ.**

## 1. المبادئ المعمارية

| المبدأ | التطبيق |
|---|---|
| Modular Monolith أولًا | خدمة واحدة بحدود وحدات واضحة (أبسط من Microservices في MVP) |
| فصل المخاوف | Ingestion / Validation / Analysis / AI / Lifecycle / API / UI |
| مصدر حقيقة واحد | الـ Backend الحتمي يملك كل رقم |
| لا Look-ahead | كل تقييم يستخدم بيانات ≤ توقيت القرار |
| Audit-first | كل انتقال حالة يُسجَّل مع السبب والوقت والمصدر |
| Failure-safe | فشل مكوّن لا يُنتج تأكيدات كاذبة |
| Cost-aware | طبقة AI رقيقة + Cost Ledger |

## 2. المخطط العام

```
                    ┌───────────────────────────────────┐
                    │         CLIENTS                   │
                    │  Web (Desktop/Tablet/Mobile)      │
                    │  RTL Arabic UI + Charts           │
                    └────────────────┬──────────────────┘
                                     │ HTTPS / JSON
                    ┌────────────────▼──────────────────┐
                    │           API GATEWAY             │
                    │  AuthN/AuthZ · Rate Limit ·       │
                    │  Input Validation · CORS · Audit  │
                    └────────────────┬──────────────────┘
                                     │
        ┌────────────────────────────┼────────────────────────────┐
        │                            │                            │
┌───────▼────────┐        ┌──────────▼──────────┐      ┌──────────▼─────────┐
│  READ APIs     │        │  WRITE APIs         │      │  AI GATEWAY        │
│  (Public/User) │        │  (Operator-only)    │      │  Orchestrator      │
│  scenarios     │        │  publish/cancel     │      │  Cost Ledger       │
│  ledger        │        │  approve rules      │      │  Grounding Check   │
│  brief         │        └──────────┬──────────┘      └──────────┬─────────┘
└───────┬────────┘                   │                            │
        │                            │                            │
        └────────────┬───────────────┴────────────┬───────────────┘
                     │                            │
          ┌──────────▼──────────┐      ┌──────────▼──────────┐
          │   DOMAIN CORE        │      │   AI LAYER (Thin)   │
          │  (Deterministic)     │◄─────│  read-only contract │
          │                      │      │  explains only      │
          │ • Market Context     │      └─────────────────────┘
          │ • Scenario Generator │
          │ • Quality Gate       │
          │ • Lifecycle Machine  │
          │ • Risk/RR Calculator │
          │ • Ledger/Outcome     │
          └──────────┬───────────┘
                     │
          ┌──────────▼───────────┐      ┌─────────────────────┐
          │   DATA LAYER         │      │  BACKGROUND JOBS    │
          │  PostgreSQL          │◄─────│  Ingestion loop     │
          │  (TimescaleDB ext.)  │      │  Validation         │
          │  Redis (cache/locks) │      │  Monitoring loop    │
          │  Object Storage      │      │  Daily Brief job    │
          └──────────┬───────────┘      │  Ledger recompute   │
                     │                  └─────────────────────┘
          ┌──────────▼───────────┐
          │  EXTERNAL PROVIDERS  │
          │  Primary: Twelve Data│
          │  Backup:  Polygon    │
          │  Calendar: TBD       │
          └──────────────────────┘
```

## 3. اختيار التقنيات وتبريرها

| الطبقة | الاختيار المقترح | البديل | التبرير |
|---|---|---|---|
| Frontend | **React + TypeScript + Vite** | Next.js | SPA كثيفة البيانات؛ Charts تفاعلية؛ لا حاجة SSR ثقيل في MVP |
| Charts | **Lightweight Charts (TradingView)** أو **Apache ECharts** | D3 خام | أداء مالي عالٍ؛ Lightweight Charts مجاني وخفيف؛ ECharts للتحكم الكامل |
| Styling | **CSS Tokens + Tailwind** | CSS-in-JS | نظام تصميم موحّد (راجع DESIGN_SYSTEM.md)؛ صفر runtime |
| Backend | **Python (FastAPI)** | Node/TypeScript | أفضل مكتبات تحليل مالي (pandas/numpy)؛ async؛ Pydantic للتحقق |
| DB | **PostgreSQL + TimescaleDB** | InfluxDB/ClickHouse | SQL كامل + موثوقية + Hypertables للشموع + ضغط تلقائي |
| Cache/Locks | **Redis** | Memcached | Cache + Distributed Lock + Rate limit + Pub/Sub |
| Queue/Jobs | **Redis + RQ / Celery** | BullMQ (Node) | بسيط، يعمل مع Python؛ كفاية للـ MVP |
| Object Storage | **S3-compatible (MinIO محليًا)** | — | ملفات (تقارير، لقطات، نسخ احتياطي) |
| Auth | **Session/JWT خفيف + OAuth لاحقًا** | Auth0 | لا كلفة مبكرة؛ ينتقل لاحقًا عند الحاجة |
| Deployment | **Docker + Compose (MVP)** → Kubernetes لاحقًا | — | MVP لا يحتاج K8s؛ ترقية تدريجية |
| Observability | **Prometheus + Grafana + Loki** | Datadog | مفتوح المصدر، بلا كلفة، كافٍ للـ MVP |
| CI/CD | **GitHub Actions** | GitLab CI | المستودع على GitHub |

**ملاحظة مهمة:** الاختيارات أعلاه **مقترحة للتفاوض**، لا مفروضة. الطلب يطلب التبرير لا
الاتباع الأعمى للشعبية. كل اختيار قابل للاستبدال ببديله المذكور.

## 4. حدود الوحدات (Module Boundaries)

| الوحدة | المسؤولية | لا تفعل |
|---|---|---|
| `ingestion` | جلب البيانات من المزوّدين | لا تحلّل، لا تقرر |
| `validation` | فحوص الجودة، الوسم `STALE/INVALID` | لا تُصلح بيانات بصمت |
| `context` | بناء لقطة سياق السوق | لا تُنشئ سيناريو |
| `scenario` | توليد وتقييم السيناريو + Quality Gate | لا تُنفّذ، لا تنشر بلا اجتياز |
| `lifecycle` | آلة الحالة والانتقالات | لا تحسب أرقامًا مالية |
| `risk` | R/R والتكاليف (حتمي) | لا تستدعي AI |
| `ledger` | النتائج Append-only | لا تُعدّل سجلًا |
| `ai` | شرح/تلخيص/إجابة | **لا تكتب في جداول الحقيقة** |
| `api` | التحقق، الترخيص، التنسيق | لا منطق أعمال |
| `jobs` | المهام الدورية | لا تتجاوز العقود |

## 5. تدفق البيانات (Data Flow)

### 5.1 تدفق الاستيعاب (Ingestion Flow)

```
[Provider API] → fetch → raw_candles → validate → candles(valid)
   │                                        │
   │                                        ├─ invalid → validation_events (سجل)
   │                                        └─ stale  → freshness_state
   └─ stream (WS) → live_ticks → spread/cost calc → last_quote
```

### 5.2 تدفق السيناريو (Scenario Flow)

```
candles → context_engine → market_context_snapshot
                │
                ▼
        scenario_generator → candidate
                │
                ▼
          quality_gate ──fail──► NO_VALID_OPPORTUNITY (مسجّل)
                │pass
                ▼
        scenario (PUBLISHED) + evidence_records
                │
                ▼
        lifecycle_machine ←── monitoring loop (candles)
                │
                ▼
        scenario_outcome (Append-only) → ledger
                │
                ▼
        ai_explainer (read-only) → ai_explanations
```

## 6. آلة الحالة — التنفيذ

- الانتقالات معرّفة في جدول `scenario_state_transitions` (حالة → حالة + شرط).
- كل انتقال **ذرّي (Atomic)** داخل Transaction مع قفل على السيناريو.
- **Idempotency:** كل انتقال يحمل `transition_key` فريد لمنع التكرار عند إعادة المعالجة.
- **ترتيب الحسم عند التعارض:** `INVALIDATED` > `STOP_LOSS_REACHED` > `TARGET_REACHED`
  (الافتراض المتحفظ — راجع EC-3/EC-4/EC-13 في PRODUCT_REQUIREMENTS).
- **لا انتقال عكسي:** الحالات النهائية لا تعود.
- **Repaint:** إن تغيّر سعر شمعة استُخدمت في قرار، يُسجَّل `REVISION` لكن **القرار الأصلي
  يبقى** (Audit-grade).

## 7. معالجة الفشل (Failure Handling)

| الفشل | السلوك |
|---|---|
| فشل المزوّد الأساسي | التحويل للاحتياطي؛ إن فشل الاثنان → `DATA_UNAVAILABLE`، منع التأكيدات |
| بيانات قديمة (Stale) | `STALE` معلن، منع التأكيدات الجديدة، استمرار العرض الصادق |
| فشل AI | «الشرح غير متاح حاليًا»؛ لا بديل مُخترع |
| فشل Job | إعادة محاولة بـ Backoff؛ تنبيه بعد N محاولات |
| تعطّل DB | Circuit breaker؛ قراءة من Cache مع وسم «بيانات مخزّنة» |
| فشل التحقق | رفض الشمعة، تسجيل، لا تأكيد |
| تعارض المصادر | `DATA_CONFLICT`، إيقاف النشر حتى الحل |

## 8. الأمان المعماري (ملخص — التفصيل في SECURITY_PLAN.md)

- لا مفاتيح مزوّدين في المتصفح — كل الاتصال بالمزوّد من الـ Backend.
- فصل مسارات القراءة (عام) عن الكتابة (Operator-only + Token).
- Rate limit على كل endpoint.
- Input validation عبر Schema (Pydantic) في كل مدخل.
- Audit log لكل فعل كتابة.
- أسرار في Environment/Secret Manager، لا في الكود ولا Git.

## 9. الأداء والتوسع

| البُعد | الـ MVP | مسار التوسع |
|---|---|---|
| قراءة السيناريوهات | فهارس + Redis cache | Read replicas |
| الشموع | TimescaleDB hypertables + compression | Partitioning متقدم |
| الرصد | حلقة واحدة لكل الأطر | Workers أفقية لكل إطار |
| AI | Routing + Cache | Pool + Batch |
| المهام | RQ بسيط | Celery + Queues متعددة |

## 10. المكونات والنشر (MVP)

```
docker-compose.yml
  ├── api          (FastAPI)
  ├── worker       (RQ/Celery: ingestion, monitoring, brief)
  ├── db           (PostgreSQL + TimescaleDB)
  ├── redis        (cache + queue + locks)
  ├── web          (static build served by API or nginx)
  ├── prometheus   (metrics)
  ├── grafana      (dashboards)
  └── minio        (object storage, محليًا)
```

**الإنتاج لاحقًا:** نفس المكونات على Kubernetes مع Managed DB، Secrets Manager، CDN للـ Web.

## 11. نقاط القرار المعماري المعلّقة

| # | القرار | ينتظر |
|---|---|---|
| AR-1 | Frontend: React SPA مقابل Next.js | مقارنة أداء/تكلفة عند البدء |
| AR-2 | Chart library النهائية | اختبار أداء على XAUUSD |
| AR-3 | Backend: Python مقابل Node | يعتمد على ثقل التحليل الرقمي (Python يرجّح) |
| AR-4 | مرونة التوسع (Monolith → Services) | بعد إثبات الـ MVP |
| AR-5 | مصدر التقويم الاقتصادي | DECISION D |

**🔒 PHASE 4 (موافقة المعمارية) ينتظر المالك. لا تنفيذ قبل الموافقة.**
