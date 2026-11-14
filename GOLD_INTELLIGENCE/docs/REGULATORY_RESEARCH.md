# REGULATORY_RESEARCH.md — البحث التنظيمي

> **المستند 13 من 17** | الحالة: مسودة PHASE 1 | آخر تحديث: 2026-11-14
> **تنبيه صريح:** هذا المستند **بحث تقني ومرجعي، لا استنتاج قانوني**. كل ما فيه من تحليل
> هو **تقدير هندسي** يهدف إلى تحديد **ما يجب أن يفحصه محامٍ متخصص**. **لا يُتخذ أي قرار
> تجاري بناءً عليه دون مراجعة قانونية مؤهّلة.**
> **منهجية:** كل اقتباس مأخوذ من صفحة رسمية جُلبت فعليًا (راجع `research/RESEARCH_LOG.md` §4).

## 1. السؤال القانوني المركزي

**هل يشكّل نشر سيناريوهات XAUUSD العامة مع أسعار دخول/وقف خسارة/أهداف «investment advice»
منظّمًا بموجب قانون الاتحاد الأوروبي (MiFID II)؟**

## 2. الإطار التنظيمي ذو الصلة

### 2.1 MiFID II — تعريف «investment advice»

- **المصدر الرسمي:** https://www.esma.europa.eu/publications-and-data/interactive-single-rulebook/mifid-ii/article-4-definitions
- **اقتباس حرفي (Article 4(1) تعريف):** «'investment advice' means the provision of personal
  recommendations to a client, either upon its request or at the initiative of the investment
  firm, in respect of one or more transactions relating to financial instruments;»

**العناصر الأساسية للتعريف:**
1. «personal recommendations» — أي **شخصية**، موجّهة لعميل محدّد.
2. «to a client» — إلى **عميل** (علاقة).
3. «relating to financial instruments» — تتعلق بأدوات مالية.

**التحليل الهندسي (ليس قانونيًا):** منتج يعرض سيناريوهات **عامة** لأداة واحدة لمستخدمين غير
محدّدين شخصيًا قد يكون **خارج** نطاق «personal recommendation». **لكن** إذا قدّم المنتج
تخصيصًا (مثل «سيناريوهات مناسبة لك») أو قنوات تنبيه موجّهة فرديًا، فقد يقترب من التصنيف.
**هذا التمييز حرج ويجب أن يحسمه محامٍ متخصص.**

### 2.2 MiFID II Article 24 — إعلام العملاء والعدالة

- **المصدر الرسمي:** https://www.esma.europa.eu/publications-and-data/interactive-single-rulebook/mifid-ii/article-24-general-principles-and
- **اقتباس حرفي (24.3):** «All information, including marketing communications, addressed by
  the investment firm to clients or potential clients shall be fair, clear and not misleading.
  Marketing communications shall be clearly identifiable as such.»
- **اقتباس حرفي (24.4(b)):** «the information on financial instruments and proposed investment
  strategies must include appropriate guidance on and warnings of the risks associated with
  investments in those instruments or in respect of particular investment strategies and whether
  the financial instrument is intended for retail or professional clients...»

**الأثر المباشر على المنتج (بغض النظر عن التصنيف):**

1. أي عرض لسيناريو يجب أن يكون «fair, clear and not misleading».
2. التحذير من المخاطر **إلزامي** و«appropriate».
3. «Marketing communications shall be clearly identifiable as such» — أي محتوى ترويجي يجب
   أن يكون واضحًا أنه ترويجي.

**هذه العناصر تدعم مباشرة مبادئ المنتج الموجودة (الشفافية، شرح المخاطر، عدم المبالغة).**

### 2.3 ESMA — تدخّل منتج CFDs (الذهب تحديدًا)

- **المصدر الرسمي:** https://www.esma.europa.eu/press-news/esma-news/esma-adopts-final-product-intervention-measures-cfds-and-binary-options
- **اقتباسات حرفية:**
  - «Leverage limits on the opening of a position by a retail client from 30:1 to 2:1, which vary
    according to the volatility of the underlying: … **20:1 for non-major currency pairs, gold
    and major indices**; …»
  - «a standardised risk warning, including the percentage of losses on a CFD provider's retail
    investor accounts».
  - «a prohibition on the marketing, distribution or sale of binary options to retail investors».

**الأثر:** الذهب موضوع **تدخّل تنظيمي صريح** في الاتحاد الأوروبي (رفع أقصى 20:1 للذهب عند
المتداول الفردي). هذا:

1. يؤكد **حساسية الأداة تنظيميًا**.
2. يدعم **تحذير مخاطر بارزًا** في المنتج مستوحى من التحذير المعياري.
3. يُبرّر ألا يُوحي المنتج برافعة عالية أو ربح سريع.

### 2.4 MAR — حماية السوق (Regulation 596/2014)

- **المصدر الرسمي:** https://www.esma.europa.eu/policy-activities/market-abuse
- **اقتباس حرفي:** «The concept of market abuse typically consists of insider dealing, unlawful
  disclosure of inside information and market manipulation.» و«Regulation No 596/2014 on market
  abuse (MAR) was adopted. Together with Directive 2014/57/EU on criminal sanctions for market
  abuse (CS MAD), MAR applies as of 3 July 2016.»

**الأثر على هذا المنتج:** **منخفض** — المنتج رصدي بلا تداول ولا تأثير سعري. لكن يجب توثيقه
كيف لا يتلاعب المنتج بالإدراك السعري (مثلًا: لا ادعاءات حركة سعرية مُضلّلة).

### 2.5 AFM (هولندا) — لم تُتحقق

- **المصدر الرسمي:** https://www.afm.nl — الصفحة الرئيسية جُلبت بنجاح، وتعريف AFM: «De AFM
  maakt zich sterk voor eerlijke en transparante financiële markten. Als onafhankelijke
  gedragstoezichthouder dragen wij bij aan duurzaam financieel welzijn in Nederland.»
- **ملاحظة صريحة:** صفحات AFM الإنجليزية المحددة (beleggingsadvies، vergunningplicht) أعادت
  **404** في الجلب الآلي. **لم تُتحقق بنودها بالاقتباس.** لا أستنتج متطلبات AFM بلا مصدر.
- **المطلوب:** قراءة يدوية في مرحلة المراجعة القانونية.

## 3. مصفوفة المخاطر التنظيمية

| # | النشاط المحتمل | التصنيف المحتمل | الخطورة | المعالجة المقترحة |
|---|---|---|---|---|
| RG-1 | نشر سيناريوهات عامة (دخول/SL/TP) | قد يُقارَب من «investment advice» | **عالية** | مراجعة قانونية؛ صياغة تحليلية؛ لا صيغة «اشترِ» |
| RG-2 | تنبيهات موجّهة لمستخدم بعينه | أقرب لـ «personal recommendation» | **عالية** | تجنّب التخصيص أو مراجعته قانونيًا |
| RG-3 | ادعاءات أداء | «fair, clear and not misleading» | متوسطة | سجل كامل + مقام معلن + لا ضمان |
| RG-4 | محتوى ترويجي | «marketing communications identifiable» | متوسطة | وسم واضح للمحتوى الترويجي |
| RG-5 | عرض رافعة/ربح سريع | مخالف لروح تدخّل ESMA | عالية | لا ذكر رافعة؛ تحذير بارز |
| RG-6 | حماية البيانات (GDPR) | لائحة ملزمة | متوسطة | سياسة خصوصية + تقليل البيانات |
| RG-7 | عمر المستخدم (قاصرون) | قد يتطلب تحقق عمر | متوسطة | بوّابة تأكيد سن إن لزم |
| RG-8 | حماية المستهلك عبر الحدود | يختلف بالاختصاص | متوسطة | حصر الاختصاص أولًا |

## 4. خيارات نموذج الامتثال (للمناقشة مع محامٍ)

| الخيار | الوصف | المزايا | العيوب |
|---|---|---|---|
| **CM-1: تحليلي/تعليمي** | المنتج تحليل عام + تعليم، بلا توصيات شخصية | أقل عبء تنظيمي | يحتاج صياغة دقيقة وتوثيقًا |
| **CM-2: ترخيص كامل** | ترخيص استشارات/وساطة | وضوح قانوني | عبء رأسمالي وتنظيمي كبير |
| **CM-3: اختصاص بديل** | تسجيل في اختصاص أخف تنظيميًا | أقل عبء | قيود على السوق المستهدف |
| **CM-4: هجين** | تحليل عام + طبقة مدفوعة مرخّصة | توازن | تعقيد تشغيلي |

**التوصية الأولية (للمناقشة):** **CM-1** في البداية مع بنية تسمح بالانتقال إلى CM-4. **السبب:**
المنتج رصدي/تحليلي بطبيعته، والتصنيف يعتمد على «personal recommendation». **لكن هذا القرار
يجب أن يتخذه المالك بعد مراجعة قانونية، لا أنا.**

## 5. الإجراءات الوقائية المقترحة (بغض النظر عن القرار)

1. **مراجعة قانونية متخصصة إلزامية** قبل أي إطلاق تجاري — لا يُغني عنها أي disclaimer.
2. **تحذير معياري** لمخاطر الذهب والرافعة، مستوحى من تحذير ESMA القياسي.
3. **صياغة تحليلية** — «السيناريو المقترح» لا «يجب أن تشتري».
4. **تمييز صريح** بين تحليل عام / سيناريو منشور / محتوى تعليمي / (لا) نصيحة شخصية.
5. **حفظ Audit Trail** لكل نشر سيناريو (من، بماذا، متى) — للدفاع القانوني.
6. **لا ادعاءات أداء** بلا مقام معلن ومنهجية موثقة.
7. **لا ذكر رافعة أو ربح سريع.**
8. **سياسة خصوصية GDPR** كاملة قبل جمع أي بيانات مستخدم.
9. **تسجيل عمر المستخدم** إن اتُّخذ قرار بوّابة عمرية.
10. **مراجعة دورية** عند أي تغيير جوهري في نموذج العمل.

## 6. ما لا أستطيع تأكيده (حدود البحث)

| # | البند | السبب |
|---|---|---|
| RC-1 | التصنيف النهائي تحت MiFID II | يحتاج محامٍ متخصص |
| RC-2 | متطلبات AFM المحددة | صفحات 404 في الجلب الآلي |
| RC-3 | متطلبات الاختصاصات خارج EU (UK/US/آسيا) | لم تُبحث بعد |
| RC-4 | متطلبات GDPR التفصيلية لمنتجنا | يحتاج مراجعة خصوصية |
| RC-5 | هل «السيناريو المنشور» = «personal recommendation» | **السؤال الحاسم** — يحتاج محاميًا |

## 7. الخلاصة

**القرب الشديد بين «سيناريو منشور بمستويات دخول وخروج» و«توصية بصفقة» يجعل التصنيف
التنظيمي سؤالًا مفتوحًا وخطيرًا.** المنتج يملك **مزايا دفاعية** بنيوية (رصدي، عام، بلا
تخصيص شخصي، سجل شفاف، تحذير مخاطر)، لكن **لا يمكن الجزم** بأنها كافية دون مراجعة قانونية.

**التوصية القاطعة:** **لا إطلاق تجاري قبل مراجعة قانونية مؤهّلة** (DECISION G).
