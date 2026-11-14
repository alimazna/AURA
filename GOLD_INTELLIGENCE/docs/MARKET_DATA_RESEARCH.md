# MARKET_DATA_RESEARCH.md — بحث مزوّدي بيانات السوق

> **المستند 11 من 17** | الحالة: مسودة PHASE 1 | آخر تحديث: 2026-11-14
> **منهجية:** كل رقم هنا مأخوذ من صفحة رسمية جُلبت فعليًا في 2026-11-14 (راجع
> `research/RESEARCH_LOG.md` §1 للأدلة الحرفية). ما لم يُتحقق مذكور صراحة.

## 1. متطلبات بيانات المنتج (Derived from Product)

| المتطلب | التفصيل | الأولوية |
|---|---|---|
| الأداة | XAUUSD (Gold Spot / US Dollar) حصريًا | حرج |
| الأطر الزمنية | M1, M5, M15, M30, H1, H4, D1, W1, MN1 | حرج |
| الشموع | OHLC مؤكد الإغلاق (Closed-bar) لا Live-bar | حرج |
| Bid/Ask/Spread | لتكاليف المحاكاة | حرج |
| Real-time | للرصد اللحظي لدورة السيناريو | حرج |
| عمق تاريخي | ≥ 2 سنة لـ H4/D1 (للسياق)، M1/M5 حديثة | مرتفع |
| تقويم اقتصادي | Fed/CPI/NFP/الفائدة | مرتفع (PILLAR 5) |
| حقوق عرض تجاري | عرض للجمهور في SaaS | **حرج قانونيًا** |
| حقوق إعادة توزيع | فقط إن أتاحنا API عامة | متوسط |
| استقرار API | SLA معلن | مرتفع |

## 2. المزوّدون المُقيَّمون

### 2.1 Twelve Data — المرشّح الأساسي

**الأدلة المؤكدة:**
- `symbol_search?symbol=XAU/USD` أعاد: `{"symbol":"XAU/USD","instrument_name":"Gold Spot / US Dollar",
  "exchange":"COMMODITY","mic_code":"COMMODITY","exchange_timezone":"Australia/Sydney",
  "instrument_type":"Precious Metal","currency":"USD"}`
- `commodities` endpoint أعاد XAG (Silver Spot) وGold Gram (GAU/USD) — يؤكد تغطية المعادن.
- **تحذير معماري:** `forex_pairs?currency_base=XAU` أعاد `{"data":[],"count":0}` — الذهب تحت
  `COMMODITY` لا `forex`. طبقة الـ Adapter يجب أن تتعامل مع هذا التصنيف.

**التقييم:**

| المعيار | التقييم |
|---|---|
| الموثوقية | عالية — SOC 2 Type II، GDPR، CCPA |
| العمق التاريخي | جيد (يحتاج تحققًا عمليًا للأطر) |
| Real-time | نعم — WebSocket + REST |
| تكرار التحديث | WS credits تسمح باشتراك رموز متعددة |
| دقة OHLC | يحتاج تحققًا عمليًا |
| حدود المعدل | API credits/min (Venture: 610) + WS credits (500) |
| التسعير | Business/Venture $149/شهر |
| الترخيص التجاري | **Venture = External display**؛ Enterprise = External distribution |
| إعادة التوزيع | Enterprise فقط |
| استقرار API | SLA 99.95% (Ultra/Individual) و99.99% (Enterprise) |
| التقويم/الأخبار | **غير مؤكد** — لا endpoint تقويم اقتصادي |

**نقاط قوة حرجة:** المزوّد الوحيد الذي **أكّد** تصنيف XAU/USD مع WebSocket + Technical
Indicators + حقوق عرض خارجي على خطة Business معقولة السعر.

**قيود:** لا تقويم اقتصادي؛ لا إعادة توزيع على Venture.

### 2.2 Polygon / Massive — المرشّح الاحتياطي

**الأدلة المؤكدة:**
- الموقع أعاد التوجيه: `polygon.io/pricing` → `massive.com/pricing`.
- خطة Currencies Basic: $0 — «All Forex and Crypto Tickers, 5 API Calls/Minute, 2 Years
  Historical Data, End of Day Data, Reference Data, Technical Indicators, Minute Aggregates».
- خطة Currencies Starter: $49/شهر — «All Forex and Crypto Tickers, Unlimited API Calls, 10+ Years
  Historical Data, Real-time Data, ... WebSockets, Snapshot, Second Aggregates, Quotes».
- **تحذير:** كلتا الخطة **Individual use**. لا تصنيف «Gold Spot» صريح في الوثائق المجلوبة.

**التقييم:**

| المعيار | التقييم |
|---|---|
| الموثوقية | عالية (مزوّد معروف في الأسهم) |
| العمق التاريخي | **ممتاز — 10+ سنوات** |
| Real-time | نعم (Starter) |
| تكرار التحديث | Second aggregates |
| حدود المعدل | Unlimited (Starter) |
| التسعير | $49/شهر (فردي) — business منفصل |
| الترخيص التجاري | **يحتاج خطة Business منفصلة** |
| التقويم الاقتصادي | لا — «TMX = corporate event calendar» لا اقتصادي |

**نقاط قوة:** عمق تاريخي ممتاز (مهم لـ PILLAR 6 Replay)، Unlimited API calls.

**قيود:** Individual use فقط في الخطط المعلنة؛ لا تأكيد XAUUSD صريح؛ يحتاج تحققًا عمليًا.

### 2.3 Alpha Vantage — احتياطي Daily

**الأدلة المؤكدة:**
- `GOLD_SILVER_SPOT` — «returns the live spot prices of gold and silver metals».
- `GOLD_SILVER_HISTORY` — «returns the historical gold and silver prices in daily, weekly, and
  monthly horizons»؛ `symbol=GOLD` أو `symbol=XAU` مقبولان؛ `interval` = daily/weekly/monthly.
- الحد المجاني: «25 API requests per day».
- Premium: 75 req/min = $49.99/شهر … 1200 req/min = $2,499/شهر.

**التقييم:** **غير مناسب كـ Real-time intraday** (لا M1–H4، لا streaming). مناسب جدًا
**كمصدر تحقق متقاطع للبيانات اليومية** وبسعر منخفض.

### 2.4 Metals.dev — مرجعي للمعادن

**الأدلة المؤكدة:**
- Gold $99.99/شهر = 500,000 req/شهر؛ Free $0 = 100 req/شهر.
- «with a maximum delay of 60 seconds» على كل الخطط.
- «Spot Metal Prices, Bid & Ask Prices, LBMA Metal Prices, LME Metal Prices, MCX & IBJA Prices».

**التقييم:** تخصص ممتاز (LBMA مرجعي، Bid/Ask) لكن **60 ثانية تأخير** لا يكفي للرصد اللحظي.
مناسب **كمرجع تحقق ثالث** لا كمصدر أساسي.

### 2.5 TraderMade

**الأدلة المؤكدة:**
- «FX and crypto data from £149 a month.»
- «These plans cover FX and crypto only. CFD symbols (400+) are **not included**, and brokers
  and prop firms need a platform licence.»
- «Sub-20ms FX, CFD and crypto pricing with display rights for your clients».

**التقييم:** عرض قوي مع «display rights» صريحة، لكن الذهب (CFD) يحتاج «platform licence»
مسعّرة عند الطلب. يحتاج تفاوضًا تجاريًا.

## 3. مصفوفة القرار

| المعيار (الوزن) | Twelve Data | Polygon/Massive | Alpha Vantage | Metals.dev | TraderMade |
|---|---|---|---|---|---|
| تأكيد XAUUSD (حرج) | ✅ مؤكد | ❓ غير مؤكد | ✅ مؤكد | ✅ مؤكد | ❓ غير مؤكد |
| Intraday M1–H4 (حرج) | ✅ | ✅ | ❌ | ❌ | ✅ |
| Real-time (حرج) | ✅ | ✅ | ❌ | ⚠️ 60s | ✅ |
| حقوق عرض SaaS (حرج) | ✅ Venture | ❓ Business منفصل | ✅ Premium | ❓ | ⚠️ licence |
| عمق تاريخي (مرتفع) | ✅ جيد | ✅✅ ممتاز | ✅ Daily | ⚠️ | ✅ |
| تقويم اقتصادي (مرتفع) | ❌ | ❌ | ❌ | ❌ | ❌ |
| السعر (متوسط) | $149 | $49 (فردي) | $49.99 | $99.99 | £149 |
| SLA (متوسط) | ✅ 99.99% | ✅ | — | — | — |
| **المجموع المرجّح** | **الأعلى** | مرتفع | منخفض | منخفض | متوسط |

## 4. التوصية النهائية

### 4.1 البنية المقترحة

```
┌─────────────────────────────────────────────────────────┐
│              Market Data Abstraction Layer               │
│   (Interface موحّد: Candles, Quote, Calendar, Stream)   │
└──────────┬────────────────────┬─────────────────┬────────┘
           │                    │                 │
    ┌──────▼──────┐      ┌──────▼──────┐   ┌──────▼──────┐
    │ PRIMARY     │      │ BACKUP      │   │ REFERENCE   │
    │ Twelve Data │      │ Polygon/    │   │ Alpha       │
    │ Business    │      │ Massive     │   │ Vantage     │
    │ Venture     │      │ Currencies  │   │ (Daily      │
    │ $149/شهر    │      │ Starter     │   │  cross-check)│
    └─────────────┘      │ $49/شهر     │   └─────────────┘
                         └─────────────┘
```

### 4.2 التبرير

1. **Twelve Data أساسي:** الوحيد الذي يؤكد XAU/USD + WebSocket + Indicators + حقوق عرض
   خارجي على خطة Business معقولة.
2. **Polygon احتياطي:** عمق تاريخي ممتاز + Unlimited calls؛ يُستخدم للتحقق المتقاطع وللـ
   Replay (PILLAR 6) ولحالات فشل الأساسي.
3. **Alpha Vantage مرجعي مجاني:** تحقق متقاطع يومي بسعر صفر — لا يُشترى إلا عند الحاجة.
4. **قاعدة تحقق متقاطع إلزامية:** عند اختلاف الأساسي والاحتياطي في شمعة مغلقة بأكثر من
   عتبة معلنة → يُعلن `DATA_CONFLICT` ويُوقف النشر حتى الحل. لا يُختار أحدهما عشوائيًا.

### 4.3 التكلفة التقديرية

| البند | شهريًا | سنويًا |
|---|---|---|
| Twelve Data Business Venture | $149 | $1,788 |
| Polygon Currencies Starter | $49 | $588 |
| **المجموع** | **$198** | **$2,376** |
| (خصم سنوي ~17–20%) | — | قد ينخفض إلى ~$1,900 |

**🔒 الاشتراك ينتظر موافقة المالك (DECISION D).**

## 5. طبقة التحقق من البيانات (Data Validation Layer)

بما أن «لا تلفيق» و«البيانات الناقصة تُعرض بصدق» مبدآن غير قابلين للتفاوض، فطبقة التحقق
جزء إلزامي من المعمارية:

| الفحص | القاعدة | الإجراء عند الفشل |
|---|---|---|
| اكتمال الشمعة | الشمعة غير المكتملة لا تُستخدم للتأكيد | `PENDING_CLOSE` |
| استمرارية زمنية | لا فجوة غير مبررة داخل الأطر | `GAP_DETECTED` + تسجيل |
| قيمة منطقية | High ≥ max(O,C)، Low ≤ min(O,C)، High ≥ Low | رفض الشمعة `INVALID_OHLC` |
| تسلسل زمني | لا شمعة بتاريخ مستقبلي | رفض `FUTURE_TIMESTAMP` |
| تكرار | لا شمعتان بنفس التوقيت | Dedup |
| حداثة | عمر آخر شمعة < عتبة لكل إطار | `STALE` + منع تأكيدات جديدة |
| تباين السبريد | السبريد خارج نطاق تاريخي معقول | `SPREAD_ANOMALY` |
| تعارض المصادر | اختلاف الأساسي/الاحتياطي فوق العتبة | `DATA_CONFLICT` + إيقاف النشر |
| Repaint | تغيّر شمعة تاريخية مغلقة | تسجيل `REVISION`؛ النتيجة الأصلية لا تُعدَّل |

**قاعدة حاكمة:** أي فشل تحقق يمنع **التأكيدات الجديدة**، لكنه لا يمنع **العرض الصادق**
لحالة «البيانات غير موثوقة».

## 6. بنود تحتاج تحققًا عمليًا (PHASE 6)

| # | البند | لماذا مهم |
|---|---|---|
| MD-1 | دقة OHLC الفعلية لـ XAUUSD على M1/M5 لكل مزوّد | قرار المزوّد |
| MD-2 | عمق تاريخ M1/M5 المتاح فعليًا | PILLAR 6 |
| MD-3 | استمرارية التدفق اللحظي وأوقات الانقطاع | موثوقية الرصد |
| MD-4 | السبريد الفعلي وعمر التحديث | دقة المحاكاة |
| MD-5 | حدود المعدل الفعلية تحت الحمل | تصميم الـ Ingestion |
| MD-6 | حقوق إعادة التوزيع التعاقدية بالحرف | الترخيص |
| MD-7 | مصدر تقويم اقتصادي (Trading Economics/FRED/BLS) | PILLAR 5 |
| MD-8 | سلوك المصدر أثناء الأحداث الكبرى (Fed/NFP) | اختبار الضغط |
| MD-9 | سياسة تصحيح الشموع التاريخية (Repaint) | نزاهة السجل |

## 7. فجوة بيانات موثقة: التقويم الاقتصادي

لا مزوّد سعري واحد من المُقيَّمين يوفّر تقويمًا اقتصاديًا كليًا. هذا **قيد معماري حقيقي**
على PILLAR 5. الخيارات:

1. **Trading Economics API** — وثائق `Economic Calendar` موجودة؛ يحتاج خطة مدفوعة (السعر
   لم يُتحقق).
2. **مصادر رسمية مجانية:** FRED (St. Louis Fed)، BLS (CPI/Employment)، Federal Reserve
   (FOMC dates). تتطلب تجميعًا (Aggregation) خاصًا بالمنتج.
3. **مزيج:** مصادر رسمية للجدولة + Trading Economics إن لزم للتفاصيل.

**🔒 اختيار مصدر التقويم ينتظر موافقة المالك ضمن DECISION D.**
