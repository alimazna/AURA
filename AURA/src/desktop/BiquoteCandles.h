#ifndef AURA_DESKTOP_BIQUOTECANDLES_H
#define AURA_DESKTOP_BIQUOTECANDLES_H

// Optional real-candle loader for the Biquote bridge.
//
// Reads AURA/bridge/biquote/out/candles_<LABEL>.json when that file exists and
// projects it into the SAME CandleSeries the existing synthetic path produces.
// When the file is absent, unreadable, empty or malformed, the loader reports
// `available == false` and the caller falls back to the pre-existing behaviour
// unchanged. Nothing here throws, and no value is ever invented.
//
// This is purely additive: it does not modify the runtime's closed-bar store, the
// synthetic frame generator, or any trading, signal or risk behaviour.

#include "desktop/BiquoteJson.h"
#include "desktop/CandleChart.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace aura {
namespace desktop {
namespace biquote {

// Why a real-candle load succeeded or did not. Never contains invented data:
// it is either an empty string (loaded) or a human-readable reason.
struct LoadResult {
    CandleSeries series{};      // valid only when `loaded` is true
    bool loaded{false};         // true only when at least one real bar was parsed
    std::string path_tried{};   // the file that was actually read
    std::string reason{};       // empty when loaded, otherwise why not
};

// Convenience alias so callers outside this namespace do not have to qualify it.
using BiquoteLoad = LoadResult;

// Timeframe label <-> the runtime enum, resolved by explicit identity so a label
// can never be mapped by position.
inline bool timeframe_from_label(const std::string& label, runtime::Timeframe& out) {
    static const runtime::Timeframe kAll[] = {
        runtime::Timeframe::M1,  runtime::Timeframe::M5,  runtime::Timeframe::M15,
        runtime::Timeframe::M30, runtime::Timeframe::H1,  runtime::Timeframe::H4,
        runtime::Timeframe::D1,  runtime::Timeframe::W1,  runtime::Timeframe::MN1};
    for (const runtime::Timeframe tf : kAll) {
        if (label == runtime::to_string(tf)) {
            out = tf;
            return true;
        }
    }
    return false;
}

inline const char* label_from_timeframe(runtime::Timeframe tf) {
    // runtime::to_string is a string_view over a canonical literal.
    return runtime::to_string(tf).data();
}

// Candidate directories for the bridge output, in priority order.
//
//   1. $AURA_BRIDGE_DIR        - explicit operator override; AUTHORITATIVE, so
//                                when it is set nothing else is searched and a
//                                typo can never silently fall back to a
//                                different directory's data.
//   2. <exe_dir>/../../bridge/biquote/out  - the packaged layout: the exe is
//                                AURA/build/Release/aura_gui.exe, so ../../ is
//                                AURA/ itself. (../../../ would climb past AURA
//                                to the repository root, which is why it is
//                                tried later rather than first.)
//      <exe_dir>/../bridge/biquote/out     - single-config builds, exe in build/
//      <exe_dir>/../../../bridge/biquote/out - deeper nested build trees
//      <exe_dir>/bridge/biquote/out       - exe sitting next to the bridge
//   3. ./bridge/biquote/out    - only when the exe directory is unknown, i.e. a
//                                bare invocation; relative paths are not mixed
//                                into the exe-relative search because that
//                                would make the result depend on the current
//                                working directory.
inline std::vector<std::string> candidate_dirs(const std::string& exe_dir) {
    std::vector<std::string> dirs;
    if (const char* env = std::getenv("AURA_BRIDGE_DIR")) {
        if (*env != '\0') {
            dirs.push_back(std::string(env) + "/out");
            return dirs;  // authoritative
        }
    }
    if (!exe_dir.empty()) {
        dirs.push_back(exe_dir + "/../../bridge/biquote/out");
        dirs.push_back(exe_dir + "/../bridge/biquote/out");
        dirs.push_back(exe_dir + "/../../../bridge/biquote/out");
        dirs.push_back(exe_dir + "/bridge/biquote/out");
        return dirs;
    }
    dirs.push_back("bridge/biquote/out");
    dirs.push_back("../bridge/biquote/out");
    dirs.push_back("../../bridge/biquote/out");
    return dirs;
}

// Reads one bridge file and projects it onto the existing CandleSeries shape.
// `tf` is authoritative: a file whose "timeframe" field disagrees is refused, so
// a mislabelled artifact can never be drawn as the wrong series.
inline LoadResult load_candles_file(const std::string& path, runtime::Timeframe tf) {
    LoadResult out;
    out.path_tried = path;

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        out.reason = "file not readable";
        return out;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();

    json::Value doc;
    try {
        doc = json::parse(buffer.str());
    } catch (const json::JsonError& err) {
        out.reason = std::string("malformed JSON: ") + err.message();
        return out;
    }

    if (!doc.is_object()) {
        out.reason = "top level is not an object";
        return out;
    }

    const std::string expected = label_from_timeframe(tf);
    const std::string declared = doc.at("timeframe").string();
    if (!declared.empty() && declared != expected) {
        out.reason = "file declares timeframe '" + declared + "', expected '" + expected + "'";
        return out;
    }

    const json::Value& candles = doc.at("candles");
    if (!candles.is_array()) {
        out.reason = "no candles array";
        return out;
    }

    CandleSeries series;
    series.timeframe = tf;
    series.label = expected;

    for (const json::Value& item : candles.array()) {
        if (!item.is_object()) continue;
        const json::Value& t = item.at("time");
        if (!t.is_number()) continue;  // a bar without an identity is not a bar

        Candle c;
        // The bridge writes epoch seconds; AURA stores nanoseconds since epoch.
        c.close_time = foundation::Timestamp::from_seconds(t.number());
        c.open = item.at("open").number();
        c.high = item.at("high").number();
        c.low = item.at("low").number();
        c.close = item.at("close").number();

        // Non-finite OHLC is dropped rather than drawn, matching the existing
        // synthetic projection exactly.
        if (!std::isfinite(c.open) || !std::isfinite(c.high) || !std::isfinite(c.low) ||
            !std::isfinite(c.close))
            continue;

        series.candles.push_back(c);
    }

    if (series.candles.empty()) {
        out.reason = "no usable closed bars";
        return out;
    }

    // The bridge writes oldest-first; sort defensively by bar identity so the
    // chart can never see an out-of-order series.
    std::sort(series.candles.begin(), series.candles.end(),
              [](const Candle& a, const Candle& b) { return a.close_time < b.close_time; });

    series.available = true;
    series.first_close = series.candles.front().close_time;
    series.last_close = series.candles.back().close_time;
    series.low = series.candles.front().low;
    series.high = series.candles.front().high;
    for (const Candle& c : series.candles) {
        if (c.low < series.low) series.low = c.low;
        if (c.high > series.high) series.high = c.high;
    }

    out.series = series;
    out.loaded = true;
    return out;
}

// Loads from one explicit directory. Used by the default path search and
// directly by tests, which must not depend on the process working directory.
inline LoadResult load_candles_from_dir(const std::string& dir, runtime::Timeframe tf) {
    const std::string file = std::string("candles_") + label_from_timeframe(tf) + ".json";
    return load_candles_file(dir + "/" + file, tf);
}

// Tries every candidate directory and returns the first file that yields real
// bars. When none does, the reason reports how many paths were tried.
inline LoadResult load_candles(runtime::Timeframe tf, const std::string& exe_dir) {
    LoadResult out;
    const std::string file = std::string("candles_") + label_from_timeframe(tf) + ".json";
    const std::vector<std::string> dirs = candidate_dirs(exe_dir);

    std::string first_reason;
    for (const std::string& dir : dirs) {
        const std::string path = dir + "/" + file;
        LoadResult attempt = load_candles_file(path, tf);
        if (attempt.loaded) return attempt;
        if (first_reason.empty()) {
            first_reason = path + " -> " + attempt.reason;
            out.path_tried = path;
        }
    }
    out.reason = dirs.empty() ? "no candidate directories"
                              : ("no real candles found across " +
                                 std::to_string(dirs.size()) + " path(s); first: " + first_reason);
    return out;
}

// Startup-time probe: true when at least the operational timeframe is available.
// Used only to pick the honest UI label; it never gates the synthetic path.
inline bool real_data_available(runtime::Timeframe tf, const std::string& exe_dir) {
    return load_candles(tf, exe_dir).loaded;
}

}  // namespace biquote
}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_BIQUOTECANDLES_H
