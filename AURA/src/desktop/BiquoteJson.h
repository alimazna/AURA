#ifndef AURA_DESKTOP_BIQUOTEJSON_H
#define AURA_DESKTOP_BIQUOTEJSON_H

// Minimal, dependency-free JSON reader for the Biquote bridge files.
//
// The project vendors no JSON library (AURA/third_party holds only stb_image.h)
// and no network-fetched third-party header is introduced here on purpose: the
// desktop app must build offline, on Windows and on Linux, with the single
// Foundation translation unit it already has.
//
// This is therefore a deliberately small reader that handles exactly what the
// bridge emits - objects, arrays, strings, numbers, true/false/null - and
// nothing else. It is NOT a general-purpose JSON library: it refuses anything
// malformed rather than guessing, and every failure surfaces as JsonError so
// the caller can fall back safely. It never throws.

#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace aura {
namespace desktop {
namespace json {

enum class Type { Null, Bool, Number, String, Array, Object };

class JsonError {  // value-semantics, no allocation of its own
public:
    explicit JsonError(std::string message) : message_(std::move(message)) {}
    const std::string& message() const noexcept { return message_; }

private:
    std::string message_;
};

class Value {
public:
    Value() = default;

    Type type() const noexcept { return type_; }
    bool is_null() const noexcept { return type_ == Type::Null; }
    bool is_number() const noexcept { return type_ == Type::Number; }
    bool is_string() const noexcept { return type_ == Type::String; }
    bool is_array() const noexcept { return type_ == Type::Array; }
    bool is_object() const noexcept { return type_ == Type::Object; }

    // Scalars. Each returns the fallback when the value is a different type, so
    // a caller never has to type-check before reading.
    double number(double fallback = 0.0) const noexcept {
        return type_ == Type::Number ? number_ : fallback;
    }
    bool boolean(bool fallback = false) const noexcept {
        return type_ == Type::Bool ? bool_ : fallback;
    }
    const std::string& string() const noexcept { return string_; }

    // Containers. Both are empty when the value is not that container.
    const std::vector<Value>& array() const noexcept { return array_; }
    const std::map<std::string, Value>& object() const noexcept { return object_; }

    // Object lookup by key. Returns a null Value when absent.
    const Value& at(const std::string& key) const noexcept {
        static const Value kNull;
        if (type_ != Type::Object) return kNull;
        const auto it = object_.find(key);
        return it == object_.end() ? kNull : it->second;
    }
    bool has(const std::string& key) const noexcept {
        return type_ == Type::Object && object_.find(key) != object_.end();
    }

    // Populated by the parser; used by the public parse() only.
    void set_null() noexcept { type_ = Type::Null; }
    void set_bool(bool v) noexcept { type_ = Type::Bool; bool_ = v; }
    void set_number(double v) noexcept { type_ = Type::Number; number_ = v; }
    void set_string(std::string v) noexcept { type_ = Type::String; string_ = std::move(v); }
    void set_array(std::vector<Value> v) noexcept { type_ = Type::Array; array_ = std::move(v); }
    void set_object(std::map<std::string, Value> v) noexcept {
        type_ = Type::Object;
        object_ = std::move(v);
    }

private:
    Type type_{Type::Null};
    bool bool_{false};
    double number_{0.0};
    std::string string_{};
    std::vector<Value> array_{};
    std::map<std::string, Value> object_{};
};

class Parser {
public:
    explicit Parser(const std::string& text) : text_(text) {}

    Value parse() {
        skip_space();
        Value root = parse_value();
        skip_space();
        if (pos_ != text_.size()) {
            throw JsonError("trailing content at offset " + std::to_string(pos_));
        }
        return root;
    }

private:
    const std::string& text_;
    std::size_t pos_{0};

    bool eof() const noexcept { return pos_ >= text_.size(); }
    char peek() const noexcept { return text_[pos_]; }

    void skip_space() noexcept {
        while (!eof()) {
            const char c = peek();
            // Only ASCII whitespace; JSON forbids anything else between tokens.
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                ++pos_;
            } else {
                break;
            }
        }
    }

    [[noreturn]] void fail(const std::string& what) const {
        throw JsonError(what + " at offset " + std::to_string(pos_));
    }

    void expect(char c) {
        if (eof() || peek() != c) fail(std::string("expected '") + c + "'");
        ++pos_;
    }

    Value parse_value() {
        if (eof()) fail("unexpected end of input");
        switch (peek()) {
            case '{': return parse_object();
            case '[': return parse_array();
            case '"': {
                Value v;
                v.set_string(parse_string());
                return v;
            }
            case 't': return parse_literal("true", true);
            case 'f': return parse_literal("false", false);
            case 'n': return parse_literal("null", false, true);
            default: return parse_number();
        }
    }

    Value parse_literal(const char* word, bool bool_value, bool is_null = false) {
        const std::size_t n = std::char_traits<char>::length(word);
        if (text_.compare(pos_, n, word) != 0) fail(std::string("expected ") + word);
        pos_ += n;
        Value v;
        if (is_null) {
            v.set_null();
        } else {
            v.set_bool(bool_value);
        }
        return v;
    }

    Value parse_object() {
        expect('{');
        std::map<std::string, Value> members;
        skip_space();
        if (!eof() && peek() == '}') {
            ++pos_;
            Value v;
            v.set_object(std::move(members));
            return v;
        }
        for (;;) {
            skip_space();
            if (eof() || peek() != '"') fail("expected object key");
            const std::string key = parse_string();
            skip_space();
            expect(':');
            skip_space();
            members[key] = parse_value();  // a duplicate key keeps the last value
            skip_space();
            if (eof()) fail("unterminated object");
            if (peek() == ',') {
                ++pos_;
                continue;
            }
            if (peek() == '}') {
                ++pos_;
                break;
            }
            fail("expected ',' or '}'");
        }
        Value v;
        v.set_object(std::move(members));
        return v;
    }

    Value parse_array() {
        expect('[');
        std::vector<Value> items;
        skip_space();
        if (!eof() && peek() == ']') {
            ++pos_;
            Value v;
            v.set_array(std::move(items));
            return v;
        }
        for (;;) {
            skip_space();
            items.push_back(parse_value());
            skip_space();
            if (eof()) fail("unterminated array");
            if (peek() == ',') {
                ++pos_;
                continue;
            }
            if (peek() == ']') {
                ++pos_;
                break;
            }
            fail("expected ',' or ']'");
        }
        Value v;
        v.set_array(std::move(items));
        return v;
    }

    std::string parse_string() {
        expect('"');
        std::string out;
        while (true) {
            if (eof()) fail("unterminated string");
            const char c = text_[pos_++];
            if (c == '"') break;
            if (c != '\\') {
                out.push_back(c);  // raw control characters are not emitted by the bridge
                continue;
            }
            if (eof()) fail("unterminated escape");
            const char esc = text_[pos_++];
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
                    // Only BMP \uXXXX is handled; surrogate pairs are joined when
                    // they are well formed. News text can legitimately contain them.
                    const unsigned cp = parse_hex4();
                    if (cp >= 0xD800u && cp <= 0xDBFFu && pos_ + 1 < text_.size() &&
                        text_[pos_] == '\\' && text_[pos_ + 1] == 'u') {
                        const std::size_t save = pos_;
                        pos_ += 2;
                        const unsigned low = parse_hex4();
                        if (low >= 0xDC00u && low <= 0xDFFFu) {
                            const unsigned combined =
                                0x10000u + ((cp - 0xD800u) << 10) + (low - 0xDC00u);
                            append_utf8(out, combined);
                            break;
                        }
                        pos_ = save;  // not a valid pair: fall through to a lone unit
                    }
                    append_utf8(out, cp);
                    break;
                }
                default: fail("invalid escape");
            }
        }
        return out;
    }

    unsigned parse_hex4() {
        if (pos_ + 4 > text_.size()) fail("truncated \\u escape");
        unsigned value = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = text_[pos_++];
            value <<= 4;
            if (c >= '0' && c <= '9') {
                value |= static_cast<unsigned>(c - '0');
            } else if (c >= 'a' && c <= 'f') {
                value |= static_cast<unsigned>(c - 'a' + 10);
            } else if (c >= 'A' && c <= 'F') {
                value |= static_cast<unsigned>(c - 'A' + 10);
            } else {
                fail("invalid hex digit");
            }
        }
        return value;
    }

    static void append_utf8(std::string& out, unsigned cp) {
        if (cp < 0x80u) {
            out.push_back(static_cast<char>(cp));
        } else if (cp < 0x800u) {
            out.push_back(static_cast<char>(0xC0u | (cp >> 6)));
            out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
        } else {
            out.push_back(static_cast<char>(0xE0u | (cp >> 12)));
            out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
            out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
        }
    }

    Value parse_number() {
        const std::size_t start = pos_;
        if (!eof() && (peek() == '-' || peek() == '+')) ++pos_;
        bool any_digit = false;
        while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) {
            ++pos_;
            any_digit = true;
        }
        if (!eof() && peek() == '.') {
            ++pos_;
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) {
                ++pos_;
                any_digit = true;
            }
        }
        if (!eof() && (peek() == 'e' || peek() == 'E')) {
            ++pos_;
            if (!eof() && (peek() == '-' || peek() == '+')) ++pos_;
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek()))) ++pos_;
        }
        if (!any_digit) fail("invalid number");

        const std::string token = text_.substr(start, pos_ - start);
        // std::strtod is locale-sensitive in principle; the bridge writes plain
        // '.' decimals and the process never calls setlocale, so this is exact.
        Value v;
        v.set_number(std::strtod(token.c_str(), nullptr));
        return v;
    }
};

// Parses a complete document. Throws JsonError on malformed input.
inline Value parse(const std::string& text) { return Parser(text).parse(); }

}  // namespace json
}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_BIQUOTEJSON_H
