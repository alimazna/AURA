// Formatting helpers. Latin numerals are forced inside RTL Arabic text so prices,
// percentages and identifiers never reorder or localise.

const nf = (digits) => new Intl.NumberFormat('en-US', { minimumFractionDigits: digits, maximumFractionDigits: digits });

export function price(v, digits = 2) {
  if (v === null || v === undefined || Number.isNaN(v)) return '—';
  return nf(digits).format(v);
}

export function pct(v, digits = 2) {
  if (v === null || v === undefined || Number.isNaN(v)) return '—';
  return `${v > 0 ? '+' : ''}${nf(digits).format(v)}%`;
}

export function signed(v, digits = 2) {
  if (v === null || v === undefined || Number.isNaN(v)) return '—';
  return `${v > 0 ? '+' : ''}${nf(digits).format(v)}`;
}

export function num(v, digits = 0) {
  if (v === null || v === undefined || Number.isNaN(v)) return '—';
  return nf(digits).format(v);
}

export function timeUTC(iso, withSeconds = false) {
  if (!iso) return '—';
  const d = new Date(iso);
  if (Number.isNaN(d.getTime())) return '—';
  const p = (x) => String(x).padStart(2, '0');
  const base = `${p(d.getUTCHours())}:${p(d.getUTCMinutes())}`;
  return withSeconds ? `${base}:${p(d.getUTCSeconds())}` : base;
}

export function dateUTC(iso) {
  if (!iso) return '—';
  const d = new Date(iso);
  if (Number.isNaN(d.getTime())) return '—';
  const p = (x) => String(x).padStart(2, '0');
  return `${p(d.getUTCDate())}/${p(d.getUTCMonth() + 1)}`;
}

export function clockUTC(iso) {
  if (!iso) return '—';
  const d = new Date(iso);
  if (Number.isNaN(d.getTime())) return '—';
  const p = (x) => String(x).padStart(2, '0');
  return `${p(d.getUTCHours())}:${p(d.getUTCMinutes())}:${p(d.getUTCSeconds())} UTC`;
}

export function age(seconds) {
  if (seconds === null || seconds === undefined) return '—';
  if (seconds < 60) return `${seconds}s`;
  if (seconds < 3600) return `${Math.round(seconds / 60)}m`;
  return `${Math.round(seconds / 3600)}h`;
}

export function money(v) {
  if (v === null || v === undefined || Number.isNaN(v)) return '—';
  return `${v > 0 ? '+' : ''}$${nf(2).format(v)}`;
}

export function rMultiple(v) {
  if (v === null || v === undefined || Number.isNaN(v)) return '—';
  return `${v > 0 ? '+' : ''}${nf(2).format(v)}R`;
}

/** Arabic labels for the canonical engine and state vocabularies. */
export const ENGINE_AR = {
  liquidity: 'السيولة',
  levels: 'الدعم والمقاومة',
  supplyDemand: 'العرض والطلب',
  structure: 'بنية السوق',
  imbalance: 'الاختلال (FVG/OB)',
  breakout: 'الاختراق والاختراق الكاذب',
};

export const STATE_AR = {
  DISCOVERED: 'مكتشفة',
  WAITING_APPROACH: 'بانتظار الوصول',
  APPROACH: 'اقتراب',
  UNDER_VALIDATION: 'قيد التحقق',
  CONFIRMED_RETEST: 'ارتداد مؤكد',
  CONFIRMED_BREAKOUT: 'اختراق مؤكد',
  FAKE_BREAKOUT: 'اختراق كاذب',
  EXPIRED: 'منتهية',
};

/** Market state as reported by the venue. Anything unrecognised is shown verbatim, not guessed. */
export const MARKET_STATE_AR = {
  open: 'مفتوح',
  closed: 'مغلق',
  premarket: 'ما قبل الافتتاح',
  afterhours: 'ما بعد الإغلاق',
  halted: 'موقوف',
  unknown: 'غير معروف',
};

export const TYPE_AR = {
  LIQUIDITY_POOL_ABOVE: 'تجمع سيولة أعلى',
  LIQUIDITY_POOL_BELOW: 'تجمع سيولة أدنى',
  LIQUIDITY_SWEEP: 'كنس سيولة',
  SUPPORT_LEVEL: 'مستوى دعم',
  RESISTANCE_LEVEL: 'مستوى مقاومة',
  DEMAND_ZONE: 'منطقة طلب',
  SUPPLY_ZONE: 'منطقة عرض',
  BREAK_OF_STRUCTURE: 'كسر البنية',
  CHANGE_OF_CHARACTER: 'تغير الطابع',
  FAIR_VALUE_GAP_BULLISH: 'فجوة قيمة صاعدة',
  FAIR_VALUE_GAP_BEARISH: 'فجوة قيمة هابطة',
  BULLISH_ORDER_BLOCK: 'أوردر بلوك صاعد',
  BEARISH_ORDER_BLOCK: 'أوردر بلوك هابط',
  BREAKOUT_CONFIRMED: 'اختراق مؤكد',
  FAKE_BREAKOUT: 'اختراق كاذب',
};

export const DIR_AR = { LONG: 'شراء', SHORT: 'بيع', NEUTRAL: 'محايد', WAIT: 'انتظار' };

/** Short names for the chart. A band label has room for one word, not a sentence, so the
 *  full TYPE_AR wording is reserved for the table where there is space for it. */
export const TYPE_SHORT_AR = {
  LIQUIDITY_POOL_ABOVE: 'سيولة',
  LIQUIDITY_POOL_BELOW: 'سيولة',
  LIQUIDITY_SWEEP: 'كنس سيولة',
  SUPPORT_LEVEL: 'دعم',
  RESISTANCE_LEVEL: 'مقاومة',
  DEMAND_ZONE: 'طلب',
  SUPPLY_ZONE: 'عرض',
  BREAK_OF_STRUCTURE: 'كسر بنية',
  CHANGE_OF_CHARACTER: 'تغيّر طابع',
  FAIR_VALUE_GAP_BULLISH: 'فجوة صاعدة',
  FAIR_VALUE_GAP_BEARISH: 'فجوة هابطة',
  BULLISH_ORDER_BLOCK: 'أوردر صاعد',
  BEARISH_ORDER_BLOCK: 'أوردر هابط',
  BREAKOUT_CONFIRMED: 'اختراق',
  FAKE_BREAKOUT: 'اختراق كاذب',
};

/** What each region type means for price, in one line. Shown on hover so a reader who does
 *  not know the abbreviation is never left guessing. */
export const TYPE_MEANING_AR = {
  LIQUIDITY_POOL_ABOVE: 'قمم متساوية أعلى السعر تجذب السعر نحوها (مغناطيس)',
  LIQUIDITY_POOL_BELOW: 'قيعان متساوية أدنى السعر تجذب السعر نحوها (مغناطيس)',
  LIQUIDITY_SWEEP: 'كنس السيولة ثم الانعكاس',
  SUPPORT_LEVEL: 'منطقة ارتداد صاعدة متكررة',
  RESISTANCE_LEVEL: 'منطقة ارتداد هابطة متكررة',
  DEMAND_ZONE: 'منطقة طلب: مشترون سابقون',
  SUPPLY_ZONE: 'منطقة عرض: بائعون سابقون',
  BREAK_OF_STRUCTURE: 'كسر بنية السوق',
  CHANGE_OF_CHARACTER: 'تغيّر طابع السوق: انعكاس محتمل',
  FAIR_VALUE_GAP_BULLISH: 'فجوة سعرية صاعدة غير مُعاد اختبارها',
  FAIR_VALUE_GAP_BEARISH: 'فجوة سعرية هابطة غير مُعاد اختبارها',
  BULLISH_ORDER_BLOCK: 'أوردر بلوك صاعد',
  BEARISH_ORDER_BLOCK: 'أوردر بلوك هابط',
  BREAKOUT_CONFIRMED: 'اختراق مؤكد بمتابعة',
  FAKE_BREAKOUT: 'اختراق كاذب: فشل ثم انعكاس',
};

/** Region categories, ordered by how directly each one answers "where might price turn".
 *  Support/resistance and supply/demand are the levels a trader acts on; liquidity pools are
 *  magnets, not turning points, so they rank last. This order decides which bands win a slot
 *  on the chart, since there are always far more regions than can be drawn. */
export const REGION_CATEGORY_AR = {
  levels: 'دعم ومقاومة',
  supplyDemand: 'عرض وطلب',
  structure: 'بنية السوق',
  breakout: 'اختراق',
  imbalance: 'فجوات سعرية',
  liquidity: 'سيولة',
};

export const REGION_CATEGORY_ORDER = [
  'levels', 'supplyDemand', 'structure', 'breakout', 'imbalance', 'liquidity',
];

export const ROLE_AR = {
  execution: 'سياق تنفيذي',
  operational: 'تشغيلي',
  context: 'سياق',
  structural: 'بنية',
  horizon: 'أفق بعيد',
};
