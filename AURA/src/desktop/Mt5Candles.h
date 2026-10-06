#ifndef AURA_DESKTOP_MT5CANDLES_H
#define AURA_DESKTOP_MT5CANDLES_H

// AURA — opt-in loader for real XAUUSD candles exported by the MT5 Python bridge.
//
// This is additive and non-breaking: when the bridge output is present it is
// converted into AURA's existing CandleSeries; when it is absent the caller keeps
// its existing (synthetic) source unchanged. Nothing here reaches into the
// runtime, places an order, reads a clock or fabricates a candle.
//
// Closed-bar rule: the bridge writes `time` as the bar's CLOSE time (MT5
// `copy_rates_from_pos` with start=1 returns closed bars only). Each loaded bar is
// marked closed and projected through `make_candle_series`, which drops any bar
// whose timeframe is not the requested one or whose OHLC is non-finite. The
// forming bar is therefore never represented and history never repaints.

#include "desktop/CandleChart.h"
#include "runtime/AdapterManager.h"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "desktop/Mt5Json.h"

namespace aura {
namespace desktop {
namespace mt5 {

// Outcome of one candle-file load. `loaded` is true only when a real, non-empty
// closed-bar series was produced. Every failure mode carries a human-readable
// `reason` and the path that was attempted, so the caller can report honestly.
struct LoadResult {
    bool loaded{false};
    std::string reason{"not attempted"};
    std::string path_tried{};
    std::string symbol{};
    std::string broker{};
    std::int64_t account_login{0};
    std::int64_t generated_at_utc{0};
    std::size_t count{0};
    CandleSeries series{};
};

namespace detail {

inline char separator() {
#if defined(_WIN32)
    return '\\';
#else
    return '/';
#endif
}

inline bool is_sep(char c) { return c == '/' || c == '\\'; }

inline std::string join_path(const std::string& dir, const std::string& leaf) {
    if (dir.empty()) return leaf;
    std::string out = dir;
    if (!out.empty() && !is_sep(out.back())) out.push_back(separator());
    out += leaf;
    return out;
}

// Reads a whole file into `out`. Returns false when it cannot be opened.
inline bool read_file(const std::string& path, std::string& out) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;
    std::ostringstream buffer;
    buffer << input.rdbuf();
    out = buffer.str();
    return true;
}

// Reads an object member that must be a finite number. Returns false on a wrong
// type or a non-finite value.
inline bool number_member(const json::Value& obj, const char* key, double& out) {
    const json::Value* v = obj.find(key);
    if (v == nullptr || !v->is_number()) return false;
    if (!(v->number == v->number)) return false;  // reject NaN without <cmath>
    out = v->number;
    return true;
}

// Reads an object member that must be an exact integer.
inline bool integer_member(const json::Value& obj, const char* key, std::int64_t& out) {
    const json::Value* v = obj.find(key);
    if (v == nullptr || !v->is_integral()) return false;
    out = static_cast<std::int64_t>(v->number);
    return true;
}

}  // namespace detail

// The directories searched for bridge output, in order:
//   1. <exe_dir>/../../bridge/mt5_python/out   (packaged app next to the repo)
//   2. $AURA_BRIDGE_DIR                        (explicit override)
//   3. ./bridge/mt5_python/out                 (working-directory relative)
// Empty entries are omitted. The order is deterministic.
inline std::vector<std::string> candidate_dirs(const std::string& exe_dir) {
    std::vector<std::string> dirs;
    if (!exe_dir.empty()) {
        const std::string up_up =
            detail::join_path(detail::join_path(exe_dir, ".."), "..");
        dirs.push_back(detail::join_path(detail::join_path(up_up, "bridge"),
                                         detail::join_path("mt5_python", "out")));
    }
    const char* env = std::getenv("AURA_BRIDGE_DIR");
    if (env != nullptr && env[0] != '\0') dirs.emplace_back(env);
    dirs.emplace_back(detail::join_path("bridge", detail::join_path("mt5_python", "out")));
    return dirs;
}

// Loads one timeframe's candles from `dir`/candles_<label>.json. Never throws.
inline LoadResult load_candles(const std::string& dir, const std::string& label) {
    LoadResult result;

    const runtime::Timeframe tf = runtime::timeframe_from_string(label);
    if (tf == runtime::Timeframe::UNKNOWN) {
        result.reason = "unknown timeframe label '" + label + "'";
        return result;
    }

    const std::string path = detail::join_path(dir, "candles_" + label + ".json");
    result.path_tried = path;

    std::string text;
    if (!detail::read_file(path, text)) {
        result.reason = "candles file not found";
        return result;
    }

    const json::ParseResult parsed = json::parse(text);
    if (!parsed.ok) {
        result.reason = "json parse error: " + parsed.reason;
        return result;
    }
    const json::Value& root = parsed.root;
    if (!root.is_object()) {
        result.reason = "root is not a JSON object";
        return result;
    }

    // Explicit identity: a file whose timeframe disagrees with the requested
    // label is refused rather than silently relabelled.
    if (const json::Value* tf_field = root.find("timeframe")) {
        if (!tf_field->is_string() || tf_field->string != label) {
            result.reason = "timeframe mismatch in file";
            return result;
        }
    }
    if (const json::Value* sym = root.find("symbol")) {
        if (sym->is_string()) result.symbol = sym->string;
    }
    if (const json::Value* broker = root.find("broker")) {
        if (broker->is_string()) result.broker = broker->string;
    }
    detail::integer_member(root, "account_login", result.account_login);
    detail::integer_member(root, "generated_at_utc", result.generated_at_utc);

    const json::Value* candles = root.find("candles");
    if (candles == nullptr || !candles->is_array()) {
        result.reason = "missing 'candles' array";
        return result;
    }

    // If a count is declared it must agree with the array, so a truncated or
    // concatenated payload is rejected rather than partially trusted.
    std::int64_t declared_count = 0;
    if (detail::integer_member(root, "count", declared_count)) {
        if (declared_count < 0 ||
            static_cast<std::size_t>(declared_count) != candles->array.size()) {
            result.reason = "declared count does not match candles array";
            return result;
        }
    }

    std::vector<runtime::MarketBar> bars;
    bars.reserve(candles->array.size());
    for (const json::Value& item : candles->array) {
        if (!item.is_object()) {
            result.reason = "candle entry is not an object";
            return result;
        }
        std::int64_t t = 0;
        double o = 0.0, h = 0.0, l = 0.0, c = 0.0;
        if (!detail::integer_member(item, "time", t) ||
            !detail::number_member(item, "open", o) ||
            !detail::number_member(item, "high", h) ||
            !detail::number_member(item, "low", l) ||
            !detail::number_member(item, "close", c)) {
            result.reason = "candle entry has a missing or non-numeric field";
            return result;
        }
        runtime::MarketBar bar;
        bar.timeframe = tf;
        bar.closed = true;  // bridge exports closed bars only
        bar.close_time = foundation::Timestamp::from_seconds(t);
        bar.open = o;
        bar.high = h;
        bar.low = l;
        bar.close = c;
        bars.push_back(bar);
    }

    // Project through the same pure path the runtime uses: explicit timeframe,
    // closed bars only, non-finite OHLC dropped.
    result.series = make_candle_series(tf, bars);
    result.count = result.series.candles.size();
    if (!result.series.available) {
        result.reason = "no closed candles for " + label;
        return result;
    }
    result.loaded = true;
    result.reason = "ok";
    return result;
}

// Tries each candidate directory in order and returns the first successful load.
// When none succeeds the returned reason names the last directory attempted.
inline LoadResult load_candles_any(const std::vector<std::string>& dirs,
                                   const std::string& label) {
    LoadResult last;
    last.reason = "no candidate directory";
    for (const std::string& dir : dirs) {
        LoadResult r = load_candles(dir, label);
        if (r.loaded) return r;
        last = r;
    }
    return last;
}

}  // namespace mt5
}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_MT5CANDLES_H
