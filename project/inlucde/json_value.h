#pragma once

#include <variant>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <sstream>
#include <cctype>
#include <cstdio>
#include <utility>
#include <optional>

// 前置声明
class JsonValue;

// 类型别名
using JsonObject = std::map<std::string, JsonValue>;
using JsonArray = std::vector<JsonValue>;

class JsonValue {
public:
    // 构造函数
    JsonValue();
    JsonValue(std::nullptr_t);
    JsonValue(bool value);
    JsonValue(int value);
    JsonValue(double value);
    JsonValue(const char* value);
    JsonValue(const std::string& value);
    JsonValue(const JsonObject& value);
    JsonValue(const JsonArray& value);

    // 类型检查
    bool is_null() const;
    bool is_boolean() const;
    bool is_integer() const;
    bool is_double() const;
    bool is_number() const;
    bool is_string() const;
    bool is_object() const;
    bool is_array() const;

    // 值获取
    bool as_boolean() const;
    int as_integer() const;
    double as_double() const;
    const std::string& as_string() const;
    const JsonObject& as_object() const;
    JsonArray& as_array();
    const JsonArray& as_array() const;

    // 对象访问
    bool has_key(const std::string& key) const;
    const JsonValue& operator[](const std::string& key) const;
    JsonValue& operator[](const std::string& key);

    // 数组访问
    const JsonValue& operator[](size_t index) const;
    JsonValue& operator[](size_t index);

    // 转换为字符串表示
    std::string to_string() const;

private:
    static std::string escape_string(const std::string& str);

    using JsonValueBase = std::variant<
            std::nullptr_t,
            bool,
            int,
            double,
            std::string,
            JsonObject,
            JsonArray
    >;

    JsonValueBase data;
};

class JsonParser {
public:
    JsonParser(const std::string& json);
    JsonValue parse();

private:
    char current() const;
    char next();
    void skip_whitespace();
    void expect(char c);
    JsonValue parse_value();
    JsonValue parse_null();
    JsonValue parse_true();
    JsonValue parse_false();
    JsonValue parse_string();
    JsonValue parse_number();
    JsonValue parse_array();
    JsonValue parse_object();
    std::runtime_error parse_error(const std::string& msg) const;

    const std::string& input;
    size_t pos;
};