#include "json_value.h"
#include <cctype>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>

// ======================= JsonValue 实现 =======================

// 构造函数
JsonValue::JsonValue() : data(nullptr) {}
JsonValue::JsonValue(std::nullptr_t) : data(nullptr) {}
JsonValue::JsonValue(bool value) : data(value) {}
JsonValue::JsonValue(int value) : data(value) {}
JsonValue::JsonValue(double value) : data(value) {}
JsonValue::JsonValue(const char* value) : data(std::string(value)) {}
JsonValue::JsonValue(const std::string& value) : data(value) {}
JsonValue::JsonValue(const JsonObject& value) : data(value) {}
JsonValue::JsonValue(const JsonArray& value) : data(value) {}

// 类型检查
bool JsonValue::is_null() const { return std::holds_alternative<std::nullptr_t>(data); }
bool JsonValue::is_boolean() const { return std::holds_alternative<bool>(data); }
bool JsonValue::is_integer() const { return std::holds_alternative<int>(data); }
bool JsonValue::is_double() const { return std::holds_alternative<double>(data); }
bool JsonValue::is_number() const { return is_integer() || is_double(); }
bool JsonValue::is_string() const { return std::holds_alternative<std::string>(data); }
bool JsonValue::is_object() const { return std::holds_alternative<JsonObject>(data); }
bool JsonValue::is_array() const { return std::holds_alternative<JsonArray>(data); }

// 值获取
bool JsonValue::as_boolean() const {
    if (is_boolean()) return std::get<bool>(data);
    throw std::runtime_error("Not a boolean");
}

int JsonValue::as_integer() const {
    if (is_integer()) return std::get<int>(data);
    if (is_double()) return static_cast<int>(std::get<double>(data));
    throw std::runtime_error("Not a number");
}

double JsonValue::as_double() const {
    if (is_double()) return std::get<double>(data);
    if (is_integer()) return static_cast<double>(std::get<int>(data));
    throw std::runtime_error("Not a number");
}

const std::string& JsonValue::as_string() const {
    if (is_string()) return std::get<std::string>(data);
    throw std::runtime_error("Not a string");
}

const JsonObject& JsonValue::as_object() const {
    if (is_object()) return std::get<JsonObject>(data);
    throw std::runtime_error("Not an object");
}

JsonArray& JsonValue::as_array() {
    if (is_array()) return std::get<JsonArray>(data);
    throw std::runtime_error("Not an array");
}

const JsonArray& JsonValue::as_array() const {
    if (is_array()) return std::get<JsonArray>(data);
    throw std::runtime_error("Not an array");
}

// 对象访问
bool JsonValue::has_key(const std::string& key) const {
    if (!is_object()) return false;
    const auto& obj = as_object();
    return obj.find(key) != obj.end();
}

const JsonValue& JsonValue::operator[](const std::string& key) const {
    if (!is_object()) throw std::runtime_error("Not an object");
    const auto& obj = as_object();
    auto it = obj.find(key);
    if (it == obj.end()) throw std::runtime_error("Key not found");
    return it->second;
}

JsonValue& JsonValue::operator[](const std::string& key) {
    if (!is_object()) throw std::runtime_error("Not an object");
    auto& obj = std::get<JsonObject>(data);
    return obj[key];
}

// 数组访问
const JsonValue& JsonValue::operator[](size_t index) const {
    if (!is_array()) throw std::runtime_error("Not an array");
    const auto& arr = as_array();
    if (index >= arr.size()) throw std::runtime_error("Index out of range");
    return arr[index];
}

JsonValue& JsonValue::operator[](size_t index) {
    if (!is_array()) throw std::runtime_error("Not an array");
    auto& arr = as_array();
    if (index >= arr.size()) throw std::runtime_error("Index out of range");
    return arr[index];
}

// 转换为字符串表示
std::string JsonValue::to_string() const {
    if (is_null()) return "null";
    if (is_boolean()) return as_boolean() ? "true" : "false";
    if (is_integer()) return std::to_string(as_integer());
    if (is_double()) {
        std::ostringstream oss;
        oss << as_double();
        return oss.str();
    }
    if (is_string()) return "\"" + escape_string(as_string()) + "\"";
    if (is_object()) {
        std::string result = "{";
        bool first = true;
        for (const auto& [key, value] : as_object()) {
            if (!first) result += ", ";
            first = false;
            result += "\"" + escape_string(key) + "\": " + value.to_string();
        }
        return result + "}";
    }
    if (is_array()) {
        std::string result = "[";
        bool first = true;
        for (const auto& value : as_array()) {
            if (!first) result += ", ";
            first = false;
            result += value.to_string();
        }
        return result + "]";
    }
    return "unknown";
}

std::string JsonValue::escape_string(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    result += buf;
                } else {
                    result += c;
                }
        }
    }
    return result;
}

// ======================= JsonParser 实现 =======================

JsonParser::JsonParser(const std::string& json) : input(json), pos(0) {}

JsonValue JsonParser::parse() {
    skip_whitespace();
    return parse_value();
}

char JsonParser::current() const {
    if (pos >= input.size()) return '\0';
    return input[pos];
}

char JsonParser::next() {
    if (pos >= input.size()) return '\0';
    return input[++pos];
}

void JsonParser::skip_whitespace() {
    while (pos < input.size() && std::isspace(static_cast<unsigned char>(input[pos]))) {
        pos++;
    }
}

void JsonParser::expect(char c) {
    if (current() != c) {
        throw parse_error("Expected '" + std::string(1, c) + "'");
    }
    next();
}

JsonValue JsonParser::parse_value() {
    skip_whitespace();
    char c = current();
    switch (c) {
        case 'n': return parse_null();
        case 't': return parse_true();
        case 'f': return parse_false();
        case '"': return parse_string();
        case '[': return parse_array();
        case '{': return parse_object();
        case '-': case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return parse_number();
        default:
            throw parse_error("Unexpected character: " + std::string(1, c));
    }
}

JsonValue JsonParser::parse_null() {
    expect('n');
    expect('u');
    expect('l');
    expect('l');
    return JsonValue(nullptr);
}

JsonValue JsonParser::parse_true() {
    expect('t');
    expect('r');
    expect('u');
    expect('e');
    return JsonValue(true);
}

JsonValue JsonParser::parse_false() {
    expect('f');
    expect('a');
    expect('l');
    expect('s');
    expect('e');
    return JsonValue(false);
}

JsonValue JsonParser::parse_string() {
    expect('"');
    std::string result;
    bool escape = false;

    while (pos < input.size()) {
        char c = current();

        if (escape) {
            escape = false;
            switch (c) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'u': {
                    if (pos + 4 >= input.size()) {
                        throw parse_error("Incomplete unicode escape");
                    }
                    std::string hex = input.substr(pos + 1, 4);
                    pos += 4;
                    try {
                        unsigned int code = std::stoul(hex, nullptr, 16);
                        if (code <= 0x7F) {
                            result += static_cast<char>(code);
                        } else if (code <= 0x7FF) {
                            result += static_cast<char>(0xC0 | (code >> 6));
                            result += static_cast<char>(0x80 | (code & 0x3F));
                        } else {
                            result += static_cast<char>(0xE0 | (code >> 12));
                            result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                            result += static_cast<char>(0x80 | (code & 0x3F));
                        }
                    } catch (...) {
                        throw parse_error("Invalid unicode escape: \\u" + hex);
                    }
                    break;
                }
                default:
                    throw parse_error("Invalid escape sequence: \\" + std::string(1, c));
            }
        } else if (c == '\\') {
            escape = true;
        } else if (c == '"') {
            next();
            return JsonValue(result);
        } else {
            result += c;
        }

        next();
    }

    throw parse_error("Unterminated string");
}

JsonValue JsonParser::parse_number() {
    size_t start = pos;
    bool is_double = false;

    if (current() == '-') {
        next();
    }

    if (current() == '0') {
        next();
    } else if (std::isdigit(static_cast<unsigned char>(current()))) {
        while (std::isdigit(static_cast<unsigned char>(current()))) {
            next();
        }
    } else {
        throw parse_error("Invalid number");
    }

    if (current() == '.') {
        is_double = true;
        next();
        if (!std::isdigit(static_cast<unsigned char>(current()))) {
            throw parse_error("Invalid number after decimal point");
        }
        while (std::isdigit(static_cast<unsigned char>(current()))) {
            next();
        }
    }

    if (current() == 'e' || current() == 'E') {
        is_double = true;
        next();
        if (current() == '+' || current() == '-') {
            next();
        }
        if (!std::isdigit(static_cast<unsigned char>(current()))) {
            throw parse_error("Invalid exponent");
        }
        while (std::isdigit(static_cast<unsigned char>(current()))) {
            next();
        }
    }

    std::string num_str = input.substr(start, pos - start);

    try {
        if (is_double) {
            return JsonValue(std::stod(num_str));
        } else {
            return JsonValue(std::stoi(num_str));
        }
    } catch (...) {
        throw parse_error("Invalid number format: " + num_str);
    }
}

JsonValue JsonParser::parse_array() {
    expect('[');
    skip_whitespace();

    JsonArray result;

    if (current() == ']') {
        next();
        return JsonValue(result);
    }

    while (true) {
        result.push_back(parse_value());
        skip_whitespace();

        if (current() == ']') {
            next();
            break;
        }

        expect(',');
        skip_whitespace();
    }

    return JsonValue(result);
}

JsonValue JsonParser::parse_object() {
    expect('{');
    skip_whitespace();

    JsonObject result;

    if (current() == '}') {
        next();
        return JsonValue(result);
    }

    while (true) {
        skip_whitespace();
        std::string key = parse_string().as_string();
        skip_whitespace();
        expect(':');
        JsonValue value = parse_value();
        result[key] = value;
        skip_whitespace();

        if (current() == '}') {
            next();
            break;
        }

        expect(',');
        skip_whitespace();
    }

    return JsonValue(result);
}

std::runtime_error JsonParser::parse_error(const std::string& msg) const {
    size_t line = 1;
    size_t column = 1;
    size_t context_start = pos;

    while (context_start > 0 && input[context_start - 1] != '\n') {
        context_start--;
    }

    size_t context_end = pos;
    while (context_end < input.size() && input[context_end] != '\n') {
        context_end++;
    }

    for (size_t i = 0; i < pos; i++) {
        if (input[i] == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
    }

    std::string context = input.substr(context_start, context_end - context_start);
    std::string indicator = std::string(pos - context_start, ' ') + "^";

    return std::runtime_error(
            "JSON parse error at line " + std::to_string(line) +
            ", column " + std::to_string(column) + ": " + msg +
            "\n" + context + "\n" + indicator
    );
}