# RESEARCH_LOG.md — سجل البحث (PHASE 1)

> الحالة: مسودة PHASE 1 | آخر تحديث: 2026-11-14 | جميع الأسعار بالدولار الأمريكي إلا حيث يُذكر.
> **منهجية:** كل رقم أو ادعاء في هذا السجل مأخوذ من صفحة رسمية للمزوّد تم جلبها فعليًا في
> 2026-11-14. الصفحات التي فشل جلبها مذكورة صراحة كـ «لم تُتحقق».

## 1. مزوّدو بيانات XAUUSD

### 1.1 Twelve Data

- **المصدر الرسمي:** https://twelvedata.com/pricing و https://twelvedata.com/pricing.md
- **دعم XAUUSD:** مؤكد. استعلام `symbol_search?symbol=XAU/USD` أعاد:
  `{"symbol":"XAU/USD","instrument_name":"Gold Spot / US Dollar","exchange":"COMMODITY",
  "mic_code":"COMMODITY","exchange_timezone":"Australia/Sydney","instrument_type":"Precious Metal",
  "currency":"USD"}`
- **نقطة مهمة:** `forex_pairs?currency_base=XAU` أعاد `{"data":[],"count":0}` — أي أن الذهب
  مُصنّف تحت `COMMODITY` وليس `forex` في هذا المزوّد. هذا يؤثر على تصميم طبقة الـ Adapter.
- **مفتاح `demo`:** مرفوض على `time_series` و`price` (HTTP 401). أي أن التحقق الفعلي من
  جودة الشموع يحتاج مفتاحًا حقيقيًا (بند تنفيذ، لا تخطيط).

**الأسعار (Individual — للاستخدام الشخصي/غير التجاري):**

| الخطة | السعر/شهر | API credits/min | WS credits | Markets |
|---|---|---|---|---|
| Basic | $0 | 8 (سقف 800/يوم) | 8 (trial) | US |
| Grow | $29 | 55 | 8 (trial) | 20+ |
| Grow | $49 | 144 | 8 (trial) | 20+ |
| Grow | $79 | 377 | 8 (trial) | 20+ |
| Pro | $99 | 610 | 500 | 70+ |
| Pro | $149 | 987 | 1,000 | 70+ |
| Pro | $229 | 1,597 | 1,500 | 70+ |
| Ultra | $329 | 2,584 | 2,500 | All |
| Ultra | $499 | 4,181 | 5,000 | All |
| Ultra | $999 | 10,946 | 10,000 | All |

**الأسعار (Business — للاستخدام التجاري):**

| الخطة | السعر/شهر | API credits/min | WS credits | Markets |
|---|---|---|---|---|
| Basic | $0 | 8 (سقف 800/يوم) | 8 (trial) | US |
| Venture | $149 | 610 | 500 | 70+ |
| Venture | $299 | 1,597 | 1,500 | 70+ |
| Venture | $499 | 2,584 | 2,500 | 70+ |
| Enterprise | $1,099 | 10,946 | 10,000 | All |
| Enterprise+ | Custom | Custom | Custom | All |

**اقتباسات حرفية مهمة (حقوق الاستخدام):**

- «**Individual plans** are for individuals only — personal research, hobby projects, learning,
  internal analytics for your own use, or non-commercial side projects. Display and distribution
  are limited to internal personal use.»
- «**Business plans** are required for any company, team, fund, or organization using Twelve Data
  — even if the data is only used internally and never displayed externally. Business plans also
  unlock external display rights and, on Enterprise tiers, external distribution.»
- «If you are unsure which plan applies, contact sales — using the wrong plan tier may violate the
  terms of service.»
- شرح Grow: «Internal display data access» — والبيانات «may be displayed but cannot be
  programmatically processed, stored, transformed, or redistributed.»
- شرح Venture: «External display data access» — والبيانات «may not be redistributed or made
  available to external parties.»
- شرح Enterprise: «External distribution market data».

**الاستنتاج الحرج:** منتج SaaS تجاري يعرض بيانات الذهب للجمهور **يحتاج خطة Business**، وليس
Individual. وأي **إعادة توزيع** (مثل واجهة API عامة) تحتاج Enterprise. هذا قيد تعاقدي مباشر
على نموذج المنتج ويجب أن يدخل قرار المالك (DECISION D).

**المزايا:** WebSocket حقيقي، Technical Indicators مدمجة، SDKs رسمية، SOC 2 Type II،
خصم 20% للشركات الناشئة والطلاب، Non-profits مجانًا، خصم 17% للدفع السنوي.

### 1.2 Polygon.io (الآن «Massive» / Massive.com)

- **المصدر الرسمي:** https://polygon.io/pricing → يُعيد التوجيه إلى https://massive.com/pricing
- **ملاحظة:** الشركة أعادت تسمية نفسها إلى **Massive** (massive.com). الاسم في الوثائق قديم.
- **الأسعار (Currencies / Forex — للاستخدام الفردي):**

| الخطة | السعر/شهر | الحدود | التاريخ | ملاحظات |
|---|---|---|---|---|
| Currencies Basic | $0 | 5 API calls/min | سنتان | EOD فقط، Reference، Technical Indicators، Minute Aggregates |
| Currencies Starter | $49 | Unlimited API calls | 10+ سنوات | Real-time، WebSockets، Flat Files، Snapshot، Second Aggregates، Quotes |

- **مهم:** كلتا الخطة **Individual use** — و«View business pricing» منفصل. لا يوجد XAU/USD
  مؤكد في الوثائق التي جُلبت (فئة «Currencies» تشمل Forex و Crypto؛ لا تصنيف «Gold Spot»
  صريح مثل Twelve Data). **يحتاج تحققًا عمليًا** عند وجود مفتاح.
- **المزايا:** عمق تاريخي ممتاز (10+ سنوات)، WebSocket، Flat Files.

### 1.3 Alpha Vantage

- **المصدر الرسمي:** https://www.alphavantage.co/documentation/ و
  https://www.alphavantage.co/premium/
- **دعم الذهب مؤكد في الوثائق:** يوجد قسم Commodities مع:
  - `GOLD_SILVER_SPOT` — «This API returns the live spot prices of gold and silver metals.»
  - `GOLD_SILVER_HISTORY` — «This API returns the historical gold and silver prices in daily,
    weekly, and monthly horizons.» و`symbol=GOLD` أو `symbol=XAU` مقبولان.
  - `interval` المقبول: `daily`, `weekly`, `monthly` — **لا دقائق أو ساعات**.
- **الحد المجاني:** «our standard API usage limit (25 API requests per day)».
- **أسعار Premium:** 75 req/min = $49.99/شهر … 1200 req/min = $2,499/شهر.
- **الاستنتاج:** غير مناسب كـ **Real-time intraday** أساسي (لا M1/M5/M15، ولا Real-time
  streaming). مناسب كـ **مصدر احتياطي للبيانات اليومية/الأسبوعية** والتحقق المتقاطع.

### 1.4 Metals.dev (مخصص للمعادن)

- **المصدر الرسمي:** https://metals.dev/pricing و https://metals.dev/faq
- **الأسعار:** Free $0 (100 req/شهر)، Copper $1.79، Silver $9.99، Platinum $19.99،
  Palladium $39.99، Gold $99.99 (500,000 req/شهر). Enterprise من $250/شهر.
- **المزايا:** Spot Metal Prices، Bid & Ask، LBMA، LME، MCX & IBJA، Currency Conversion.
- **قيود حرفية:** «with a maximum delay of 60 seconds» على كل الخطط بما فيها Free.
  و«Our data is sourced from a 15+ trusted & reliable data sources».
- **الاستنتاج:** تخصص ممتاز في المعادن (LBMA مرجعي)، لكن التأخير 60 ثانية والتركيز على
  Spot/LBMA لا على شموع Intraday للتداول. مرشّح جيد كـ **مصدر ثانٍ للمرجع السعري والتحقق
  المتقاطع**، لا كمصدر أساسي لشموع M1/M5/M15.

### 1.5 TraderMade

- **المصدر الرسمي:** https://www.tradermade.com/pricing
- **السعر المعلن:** «FX and crypto data from £149 a month. Pick the refresh rate you need.»
- **قيود حرفية:** «These plans cover FX and crypto only. CFD symbols (400+) are **not included**,
  and brokers and prop firms need a platform licence.» و«Sub-20ms FX, CFD and crypto pricing
  with display rights for your clients» (لـ Prop Firms & Brokers).
- **الاستنتاج:** عرض قوي مع «display rights» صريحة، لكن CFD (ويشمل الذهب عادة) يحتاج
  «platform licence» مسعّرة عند الطلب. يحتاج تفاوضًا تجاريًا.

### 1.6 مقارنة سريعة

| المزوّد | Real-time | Intraday (M1–H4) | عمق تاريخي | WebSocket | حقوق العرض التجاري | السعر الابتدائي |
|---|---|---|---|---|---|---|
| Twelve Data | نعم | نعم | جيد | نعم | Venture $149/شهر | $149/شهر |
| Polygon/Massive | نعم (Starter) | نعم | ممتاز (10+ سنوات) | نعم | Business منفصل | $49/شهر (فردي) |
| Alpha Vantage | لا (Spot فقط) | لا (Daily+) | جيد (Daily) | لا | Premium | $49.99/شهر |
| Metals.dev | 60s delay | لا | Spot/LBMA | — | غير مؤكد | $99.99/شهر |
| TraderMade | نعم | نعم | جيد | نعم | تحتاج licence | £149/شهر |

### 1.7 التوصية (تحتاج موافقة المالك — DECISION D)

- **الأساسي المقترح:** **Twelve Data Business / Venture ($149/شهر)** — أول مزوّد يؤكد
  وجود `XAU/USD` صراحة كـ Precious Metal مع WebSocket وTechnical Indicators مدمجة وحقوق
  عرض خارجي (External display) — وهي الحاجة الدقيقة لمنتج SaaS.
- **الاحتياطي المقترح:** **Polygon/Massive Currencies Starter ($49/شهر)** للتحقق المتقاطع
  وعمق تاريخي أكبر، **أو** Alpha Vantage (Daily) كطبقة تحقق متقاطع مجانية للبيانات اليومية.
- **مصدر مرجعي ثالث (اختياري):** Metals.dev للمرجع السعري LBMA/Spot والتحقق المتقاطع.
- **شرط إلزامي:** عدم الاشتراك في أي خطة حتى **التحقق العملي** من جودة شموع XAUUSD الفعلية
  (استمرارية، فجوات، اكتمال الشمعة، السبريد، الانقطاعات) — بند تنفيذي ينتظر التفويض.

## 2. مزوّدو الذكاء الاصطناعي

### 2.1 OpenAI

- **المصدر الرسمي:** https://platform.openai.com/docs/pricing
- **ملاحظة:** https://openai.com/api/pricing/ أعاد 403 للجلب الآلي — استُخدم مصدر الوثائق.
- **أمثلة أسعار (Standard، لكل 1M tokens، Short context):**

| Model | Input | Output | Cached input |
|---|---|---|---|
| gpt-5.4-mini | $0.75 | $4.50 | $0.075 |
| gpt-5.4-nano | $0.20 | $1.25 | $0.02 |
| gpt-4.1-mini | $0.40 | $1.60 | $0.10 |
| gpt-4o-mini | $0.15 | $0.60 | $0.075 |

- يوجد **Batch API** بخصم 50% و**Flex** — مناسبان للمهام غير الحيّة (Gold Daily Brief،
  التفسيرات الدفعية) وليس لشرح تفاعلي لحظي.

### 2.2 Anthropic (Claude)

- **المصدر الرسمي:** https://docs.claude.com/en/docs/about-claude/pricing
- **أسعار (لكل MTok):** Claude Haiku 5.5 — Input $0.10 / Output $0.50 (حتى 100K tokens)؛
  Claude Sonnet 5.5 — Input $2 / Output $10. يوجد Prompt caching وBatch.
- **الاستنتاج:** Haiku مناسب للمهام عالية الحجم منخفضة التعقيد (التصنيف، الاستخراج، التوجيه)؛
  Sonnet للمهام الأعمق (شرح السيناريو، الموجز اليومي).

### 2.3 Google Gemini

- **المصدر الرسمي:** https://ai.google.dev/gemini-api/docs/pricing و
  https://ai.google.dev/gemini-api/docs/rate-limits
- **Free tier:** موجود «For developers and small projects» مع «Limited access to certain models»
  و«Free input & output tokens» — لكن «Content used to improve our products». الخطة المدفوعة:
  «Content **not** used to improve our products».
- **مثال سعري:** Gemini 4 Carbon — Input $2.00 / Output $10.00 (لكل 1M tokens، Standard).
  Gemini 4 Hydrogen — Input $1.50 / Output $7.50 (Standard) و$0.75/$3.75 (Batch/Flex).
- **Usage tiers:** Free / Tier 1 (billing) / Tier 2 ($100 + 3 أيام) / Tier 3 ($1,000 + 30 يومًا).
  مثال rate limits (Gemini Decision 1): Free 200 RPM / 50,000 TPM / 10,000 RPD.
- **قيد مهم:** «Specified rate limits are not guaranteed and actual capacity may vary.» و
  في الخطة المجانية تُستخدم بياناتك لتحسين المنتجات — **غير مناسب لبيانات إنتاجية تجارية**.

### 2.4 DeepSeek

- **المصدر الرسمي:** https://api-docs.deepseek.com/quick_start/pricing
- **أسعار (لكل 1M tokens):**

| Model | Cache Hit (off-peak/peak) | Cache Miss (off-peak/peak) | Output (off-peak/peak) |
|---|---|---|---|
| deepseek-flash (V4.1-Flash-1017) | $0.003 / $0.006 | $0.15 / $0.30 | $0.60 / $1.20 |
| deepseek-v4-pro | $0.022 / $0.044 | $0.66 / $1.32 | $1.98 / $3.96 |

- **مزايا:** Context 1M، Max output 384K، JSON Output، Tool Calls، متوافق مع OpenAI/Anthropic API.
- **ساعات الذروة:** «Peak hours are 01:00 - 04:00 and 06:00 - 10:00 UTC, Monday through Friday».
  خارجها السعر **نصف** — مهم لجدولة المهام غير الحيّة.
- **الاستنتاج:** **الأرخص بفارق كبير** للمهام التحليلية عالية الحجم. مرشّح قوي جدًا كـ
  «طبقة شرح/تصنيف عالية الحجم»، مع اعتبار المتوافقية (OpenAI format) لتقليل كلفة التبديل.

### 2.5 التوصية (تحتاج موافقة المالك — DECISION F)

معمارية متعددة الطبقات بحسب الكلفة والقدرة:

1. **طبقة عالية الحجم/منخفضة التعقيد** (تصنيف الأدلة، وسم السياق، الترجمة، الصياغة):
   نموذج رخيص جدًا — DeepSeek Flash (off-peak) أو Haiku.
2. **طبقة الشرح التفاعلي** (أسئلة المستخدم عن السيناريو): نموذج متوسط — Sonnet أو GPT-5.x-mini.
3. **طبقة الموجز اليومي والتحليل العميق** (غير حيّ، قابل للجدولة): Batch/Flex من مزوّد
   رئيسي أو DeepSeek v4-pro.
4. **قاعدة:** لا تُشترى أي خطة قبل موافقة المالك، وكل مكالمة AI تُسجَّل بالتكلفة (Cost Ledger).

## 3. تقويم الأحداث الاقتصادية والأخبار

| المصدر | الوصف | الحالة |
|---|---|---|
| Trading Economics API | وثائق `Economic Calendar` موجودة (docs.tradingeconomics.com/economic_calendar). يحتاج خطة مدفوعة | لم تُتحقق الأسعار |
| Twelve Data | لا يوجد endpoint تقويم اقتصادي مؤكد (محاولة `economic_calendar` أعادت 404) | لا يوجد تقويم |
| Polygon/Massive | «Partner Data: TMX — Corporate event calendar data from Canada's premier exchange» — **تقويم شركات** لا تقويم اقتصادي كلي | غير مناسب |
| Alpha Vantage | لا يوجد تقويم اقتصادي في الوثائق المجلوبة | غير مناسب |

**الاستنتاج:** تقويم الأحداث الاقتصادية الكلي (Fed، CPI، NFP) هو **فجوة بيانات حقيقية** في
المزوّدين السعريين. يحتاج مزوّدًا متخصصًا (Trading Economics) أو مصدرًا رسميًا (FRED/BLS/
Federal Reserve) — كلاهما يحتاج تحققًا عمليًا. هذا يخصّ **PILLAR 5 (Event Reaction Lab)**
ويجب أن يُخطَّط له صراحة.

## 4. التنظيم القانوني (EU / هولندا)

### 4.1 تعريف «investment advice» — MiFID II

- **المصدر الرسمي:** https://www.esma.europa.eu/publications-and-data/interactive-single-rulebook/mifid-ii/article-4-definitions
- **اقتباس حرفي (Article 4):** «'investment advice' means the provision of personal
  recommendations to a client, either upon its request or at the initiative of the investment
  firm, in respect of one or more transactions relating to financial instruments;»
- **التحليل:** تعريف التوصية الشخصية يشترط «personal recommendation to a client». منتج يعرض
  سيناريوهات **عامة** لأداة واحدة (XAUUSD) لمستخدمين غير محدّدين شخصيًا قد يكون **خارج** نطاق
  «personal recommendation» — لكن هذا **تقدير تقني لا استنتاج قانوني**. القرب الشديد بين
  «سيناريو منشور» و«توصية بصفقة» يجعل التصنيف **قابلًا للجدل** ويجب أن يُحسم بمراجعة قانونية.

### 4.2 إعلام العملاء والعدالة والوضوح — MiFID II Article 24

- **المصدر الرسمي:** https://www.esma.europa.eu/publications-and-data/interactive-single-rulebook/mifid-ii/article-24-general-principles-and
- **اقتباس حرفي (24.3):** «All information, including marketing communications, addressed by the
  investment firm to clients or potential clients shall be fair, clear and not misleading.
  Marketing communications shall be clearly identifiable as such.»
- **اقتباس حرفي (24.4(b)):** «the information on financial instruments and proposed investment
  strategies must include appropriate guidance on and warnings of the risks associated with
  investments in those instruments or in respect of particular investment strategies and whether
  the financial instrument is intended for retail or professional clients...»
- **الأثر على المنتج:** أي عرض لسيناريو يجب أن يكون «fair, clear and not misleading» مع تحذير
  مخاطر مناسب. هذا يدعم مباشرة مبدأ «الدرجة ليست احتمالًا» ومبدأ «شرح المخاطر».

### 4.3 ملاءمة المنتج للأفراد — ESMA CFD Product Intervention

- **المصدر الرسمي:** https://www.esma.europa.eu/press-news/esma-news/esma-adopts-final-product-intervention-measures-cfds-and-binary-options
- **اقتباسات حرفية:** «Leverage limits on the opening of a position by a retail client from 30:1
  to 2:1, which vary according to the volatility of the underlying: … 20:1 for non-major currency
  pairs, **gold** and major indices; …» و«a standardised risk warning, including the percentage
  of losses on a CFD provider's retail investor accounts».
- **الأثر:** الذهب تحديدًا موضوع تدخّل تنظيمي ESMA (رفع الرافعة الأقصى 20:1 للذهب عند المتداول
  الفردي في CFDs). هذا يؤكد حساسية الأداة تنظيميًا ويُبرّر تحذير مخاطر بارزًا في المنتج.

### 4.4 حماية السوق — MAR (Regulation 596/2014)

- **المصدر الرسمي:** https://www.esma.europa.eu/policy-activities/market-abuse
- **اقتباس حرفي:** «The concept of market abuse typically consists of insider dealing, unlawful
  disclosure of inside information and market manipulation.» و«Regulation No 596/2014 on market
  abuse (MAR) was adopted. Together with Directive 2014/57/EU on criminal sanctions for market
  abuse (CS MAD), MAR applies as of 3 July 2016.»
- **الأثر:** منخفض للمنتج الرصدي (لا تداول ولا تأثير سعري)، لكن يجب توثيقه كبند مراجعة.

### 4.5 AFM (هولندا)

- **المصدر الرسمي:** https://www.afm.nl (الصفحة الرئيسية جُلبت بنجاح: «De AFM maakt zich sterk
  voor eerlijke en transparante financiële markten.»)
- **ملاحظة:** صفحات AFM الإنجليزية المحددة (beleggingsadvies/vergunningplicht) أعادت 404 في
  الجلب الآلي. **لم تُتحقق بنودها بالاقتباس الحرفي.** يجب أن تُقرأ يدويًا في مرحلة المراجعة
  القانونية. **لا أستنتج متطلبات AFM بلا مصدر.**

### 4.6 التوصية القانونية

1. **مراجعة قانونية متخصصة إلزامية** قبل أي إطلاق تجاري — لا يُغني عنها أي disclaimer.
2. إضافة **التحذير المعياري** لمخاطر الذهب والرافعة، مستوحى من تحذير ESMA القياسي.
3. توثيق **التمييز** بين: تحليل عام / سيناريو منشور / محتوى تعليمي / نصيحة شخصية.
4. عدم تقديم أي شيء بصيغة «يجب أن تشتري» أو ما يوازيها.
5. حفظ **Audit Trail** لكل نشر سيناريو (من نشره، بأي بيانات، متى) — للدفاع القانوني.
6. إذا لزم الترخيص، تقرّر البنية (منتج تعليمي/بحثي مقابل خدمة توصيات مرخّصة).

## 5. مكدّس التصميم المرجعي (Design Reference)

- **الصورة المرجعية:** `aa/AYZR_Reference_Dashboard.png` (1672×941) — قُيست ألوانها فعليًا بـ PIL.
- **الأسطح المقيسة:** top bar `#010917`، sidebar `#030d1e`، chart `#020b1a`، right panel
  `#020e20`، hairline `#010a19`.
- **اللمسات المقيسة:** blue `#064dcf` (الأكثر تكرارًا: 303+229 بكسل)، cyan `#13b8fa`،
  gold `#fbbf1d`، green `#01cd75`، red `#e93d53`، text `#ffffff` / muted `#9abbe5`.
- **ملاحظة مهمة:** كل الأسعار والأرقام داخل الصورة **تجريبية/افتراضية** — لا تُنسخ كبيانات سوق.
- **مرجع Dribbble:** https://dribbble.com/shots/26556040-Forex-Trading-App-Learn-Practice-Trade —
  الجلب الآلي أعاد HTTP 202 (لا محتوى). **لم تُفحص بصريًا.** إلهام فقط، لا نسخ.

## 6. بنود لم تُتحقق (يجب معالجتها في PHASE 6/7)

| البند | السبب |
|---|---|
| أسعار Trading Economics | الصفحة تحتاج تفاعلًا/تسجيلًا |
| بنود AFM المحددة بالاقتباس | صفحات 404 في الجلب الآلي |
| محتوى Dribbble البصري | 202 / حماية ضد الجلب |
| جودة شموع XAUUSD الفعلية لكل مزوّد | تحتاج مفاتيح API حقيقية (بند تنفيذ) |
| حقوق إعادة توزيع Metals.dev التعاقدية | صفحة Terms 404 |
| أسعار TraderMade لترخيص CFD | «priced on request» |
