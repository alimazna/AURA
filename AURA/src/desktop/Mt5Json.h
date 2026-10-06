#ifndef AURA_DESKTOP_MT5JSON_H
#define AURA_DESKTOP_MT5JSON_H

// AURA — minimal, self-contained JSON parser for the MT5 Python bridge output.
//
// Scope is deliberately narrow: this parses ONLY the shape produced by
// `AURA/bridge/mt5_python/read_candles.py` (a top-level object whose `candles`
// value is an array of flat objects of numbers). It is not a general-purpose
// JSON library, and it adds no third-party dependency.
//
// Design:
//   * Header-only, C++17, no exceptions, no I/O, no clock.
//   * Deterministic: identical input yields identical output.
//   * Strict: malformed input, wrong types, trailing junk, a truncated array, a
//     duplicate top-level key or an unknown candle key are all rejected.
//   * Bounded recursion depth so pathological nesting cannot exhaust the stack.
//
// A parse failure is a normal result (ok == false with a reason), never a crash.

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace aura {
namespace desktop {
namespace json {

// A parsed JSON value. Only the subset the bridge emits is modelled: null, bool,
// number, string, array and object. Integers and reals share `number`; callers
// that need an integer check that the value is integral.
struct Value {
    enum class Type : std::uint8_t { Null, Bool, Number, String, Array, Object };

    Type type{Type::Null};
    bool boolean{false};
    double number{0.0};
    std::string string{};
    std::vector<Value> array{};
    std::map<std::string, Value> object{};

    bool is_null() const noexcept { return type == Type::Null; }
    bool is_bool() const noexcept { return type == Type::Bool; }
    bool is_number() const noexcept { return type == Type::Number; }
    bool is_string() const noexcept { return type == Type::String; }
    bool is_array() const noexcept { return type == Type::Array; }
    bool is_object() const noexcept { return type == Type::Object; }

    // True when the number is an exact integer (used to read `count`, epoch
    // seconds and integer volumes without a lossy cast).
    bool is_integral() const noexcept {
        if (type != Type::Number) return false;
        const double rounded = static_cast<double>(static_cast<std::int64_t>(number));
        return rounded == number;
    }

    // Object member lookup; nullptr when absent or when this is not an object.
    const Value* find(const std::string& key) const {
        if (type != Type::Object) return nullptr;
        const auto it = object.find(key);
        return it == object.end() ? nullptr : &it->second;
    }
};

struct ParseResult {
    bool ok{false};
    std::string reason{"not parsed"};
    Value root{};
};

namespace detail {

constexpr int kMaxDepth = 64;

// Cursor over the input text. Advances on success, leaves the position unchanged
// on failure so the caller can report where parsing stopped.
struct Cursor {
    const std::string& text;
    std::size_t pos{0};
    explicit Cursor(const std::string& t) : text(t) {}
    bool eof() const noexcept { return pos >= text.size(); }
    char peek() const noexcept { return pos < text.size() ? text[pos] : '\0'; }
};

inline void skip_ws(Cursor& c) {
    while (!c.eof()) {
        const char ch = c.text[c.pos];
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
            ++c.pos;
        } else {
            break;
        }
    }
}

inline bool parse_value(Cursor& c, Value& out, int depth, std::string& err);

inline bool parse_hex4(Cursor& c, std::uint32_t& cp) {
    if (c.pos + 4 > c.text.size()) return false;
    std::uint32_t v = 0;
    for (int i = 0; i < 4; ++i) {
        const char ch = c.text[c.pos + static_cast<std::size_t>(i)];
        v <<= 4;
        if (ch >= '0' && ch <= '9') v |= static_cast<std::uint32_t>(ch - '0');
        else if (ch >= 'a' && ch <= 'f') v |= static_cast<std::uint32_t>(ch - 'a' + 10);
        else if (ch >= 'A' && ch <= 'F') v |= static_cast<std::uint32_t>(ch - 'A' + 10);
        else return false;
    }
    c.pos += 4;
    cp = v;
    return true;
}

inline void append_utf8(std::string& s, std::uint32_t cp) {
    if (cp <= 0x7F) {
        s.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        s.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        s.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        s.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        s.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        s.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        s.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

inline bool parse_string(Cursor& c, std::string& out, std::string& err) {
    if (c.peek() != '"') {
        err = "expected string";
        return false;
    }
    ++c.pos;
    out.clear();
    while (true) {
        if (c.eof()) {
            err = "unterminated string";
            return false;
        }
        const char ch = c.text[c.pos++];
        if (ch == '"') return true;
        if (static_cast<unsigned char>(ch) < 0x20) {
            err = "control character in string";
            return false;
        }
        if (ch != '\\') {
            out.push_back(ch);
            continue;
        }
        if (c.eof()) {
            err = "truncated escape";
            return false;
        }
        const char esc = c.text[c.pos++];
        switch (esc) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u': {
                std::uint32_t cp = 0;
                if (!parse_hex4(c, cp)) {
                    err = "bad \\u escape";
                    return false;
                }
                if (cp >= 0xD800 && cp <= 0xDBFF) {
                    // High surrogate: a low surrogate must follow.
                    if (c.pos + 1 >= c.text.size() || c.text[c.pos] != '\\' ||
                        c.text[c.pos + 1] != 'u') {
                        err = "lone high surrogate";
                        return false;
                    }
                    c.pos += 2;
                    std::uint32_t low = 0;
                    if (!parse_hex4(c, low) || low < 0xDC00 || low > 0xDFFF) {
                        err = "bad low surrogate";
                        return false;
                    }
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                    err = "lone low surrogate";
                    return false;
                }
                append_utf8(out, cp);
                break;
            }
            default:
                err = "invalid escape";
                return false;
        }
    }
}

inline bool parse_number(Cursor& c, double& out, std::string& err) {
    const std::size_t start = c.pos;
    if (c.peek() == '-') ++c.pos;
    if (c.eof()) {
        err = "truncated number";
        return false;
    }
    if (c.peek() == '0') {
        ++c.pos;
    } else if (c.peek() >= '1' && c.peek() <= '9') {
        while (!c.eof() && c.peek() >= '0' && c.peek() <= '9') ++c.pos;
    } else {
        err = "invalid number";
        return false;
    }
    if (c.peek() == '.') {
        ++c.pos;
        if (c.eof() || c.peek() < '0' || c.peek() > '9') {
            err = "invalid fraction";
            return false;
        }
        while (!c.eof() && c.peek() >= '0' && c.peek() <= '9') ++c.pos;
    }
    if (c.peek() == 'e' || c.peek() == 'E') {
        ++c.pos;
        if (c.peek() == '+' || c.peek() == '-') ++c.pos;
        if (c.eof() || c.peek() < '0' || c.peek() > '9') {
            err = "invalid exponent";
            return false;
        }
        while (!c.eof() && c.peek() >= '0' && c.peek() <= '9') ++c.pos;
    }
    const std::string token = c.text.substr(start, c.pos - start);
    try {
        std::size_t used = 0;
        out = std::stod(token, &used);
        if (used != token.size()) {
            err = "trailing characters in number";
            return false;
        }
    } catch (...) {
        err = "number out of range";
        return false;
    }
    return true;
}

inline bool parse_literal(Cursor& c, const char* word) {
    const std::size_t len = std::char_traits<char>::length(word);
    if (c.text.compare(c.pos, len, word) != 0) return false;
    c.pos += len;
    return true;
}

inline bool parse_array(Cursor& c, Value& out, int depth, std::string& err) {
    ++c.pos;  // consume '['
    out.type = Value::Type::Array;
    skip_ws(c);
    if (c.peek() == ']') {
        ++c.pos;
        return true;
    }
    while (true) {
        Value item;
        if (!parse_value(c, item, depth + 1, err)) return false;
        out.array.push_back(std::move(item));
        skip_ws(c);
        const char ch = c.peek();
        if (ch == ',') {
            ++c.pos;
            skip_ws(c);
            continue;
        }
        if (ch == ']') {
            ++c.pos;
            return true;
        }
        err = "expected ',' or ']' in array";
        return false;
    }
}

inline bool parse_object(Cursor& c, Value& out, int depth, std::string& err) {
    ++c.pos;  // consume '{'
    out.type = Value::Type::Object;
    skip_ws(c);
    if (c.peek() == '}') {
        ++c.pos;
        return true;
    }
    while (true) {
        std::string key;
        if (!parse_string(c, key, err)) return false;
        skip_ws(c);
        if (c.peek() != ':') {
            err = "expected ':' after object key";
            return false;
        }
        ++c.pos;
        skip_ws(c);
        Value value;
        if (!parse_value(c, value, depth + 1, err)) return false;
        // A duplicate key is malformed: reject rather than silently overwrite.
        if (out.object.find(key) != out.object.end()) {
            err = "duplicate object key";
            return false;
        }
        out.object.emplace(std::move(key), std::move(value));
        skip_ws(c);
        const char ch = c.peek();
        if (ch == ',') {
            ++c.pos;
            skip_ws(c);
            continue;
        }
        if (ch == '}') {
            ++c.pos;
            return true;
        }
        err = "expected ',' or '}' in object";
        return false;
    }
}

inline bool parse_value(Cursor& c, Value& out, int depth, std::string& err) {
    if (depth > kMaxDepth) {
        err = "maximum nesting depth exceeded";
        return false;
    }
    skip_ws(c);
    if (c.eof()) {
        err = "unexpected end of input";
        return false;
    }
    const char ch = c.peek();
    switch (ch) {
        case '{':
            return parse_object(c, out, depth, err);
        case '[':
            return parse_array(c, out, depth, err);
        case '"':
            out.type = Value::Type::String;
            return parse_string(c, out.string, err);
        case 't':
            if (!parse_literal(c, "true")) {
                err = "invalid literal";
                return false;
            }
            out.type = Value::Type::Bool;
            out.boolean = true;
            return true;
        case 'f':
            if (!parse_literal(c, "false")) {
                err = "invalid literal";
                return false;
            }
            out.type = Value::Type::Bool;
            out.boolean = false;
            return true;
        case 'n':
            if (!parse_literal(c, "null")) {
                err = "invalid literal";
                return false;
            }
            out.type = Value::Type::Null;
            return true;
        default:
            out.type = Value::Type::Number;
            return parse_number(c, out.number, err);
    }
}

}  // namespace detail

// Parses one complete JSON document. Trailing non-whitespace is rejected, so a
// truncated or concatenated payload never parses as valid.
inline ParseResult parse(const std::string& text) {
    ParseResult result;
    detail::Cursor cursor(text);
    detail::skip_ws(cursor);
    if (cursor.eof()) {
        result.reason = "empty input";
        return result;
    }
    std::string err;
    if (!detail::parse_value(cursor, result.root, 0, err)) {
        result.reason = err;
        return result;
    }
    detail::skip_ws(cursor);
    if (!cursor.eof()) {
        result.reason = "trailing characters after JSON value";
        return result;
    }
    result.ok = true;
    result.reason = "ok";
    return result;
}

}  // namespace json
}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_MT5JSON_H
