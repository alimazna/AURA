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

export const ROLE_AR = {
  execution: 'سياق تنفيذي',
  operational: 'تشغيلي',
  context: 'سياق',
  structural: 'بنية',
  horizon: 'أفق بعيد',
};
