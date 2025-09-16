#include "utils.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>
#include <regex>
#include <array>
#include <cstdlib>
#include <unordered_map>
#include <filesystem>
#include <memory>  // 添加unique_ptr支持

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#include <io.h>
#else
#include <cstdio>
#endif

namespace fs = std::filesystem;

std::string Utils::clean_path(const std::string& input) {
    std::string cleaned = input;
    std::replace(cleaned.begin(), cleaned.end(), '\\', '/');
    if (!cleaned.empty() && cleaned.back() == '/' && cleaned.size() > 1) {
        cleaned.pop_back();
    }
    return cleaned;
}

std::string Utils::get_valid_name(const std::string& input) {
    std::string name = input;
    name.erase(std::remove_if(name.begin(), name.end(),
                              [](char c) { return !(std::isalnum(c) || c == '-' || c == '_' || c == ' '); }),
               name.end());
    std::replace(name.begin(), name.end(), ' ', '-');
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return name;
}

bool Utils::is_standard_supported(const CompilerConfig& compiler, const std::string& standard) {
    return true;
}

int Utils::execute_command(const std::string& command) {
    std::cout << "Executing: " << command << std::endl;
    return std::system(command.c_str());
}

std::string Utils::execute_command_with_output(const std::string& command) {
#ifdef _WIN32
    std::string cmd = "cmd /c \"" + command + "\"";
#else
    std::string cmd = command;
#endif

    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);

    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    return result;
}

fs::path Utils::join_paths(const fs::path& base, const std::string& part) {
    return base / part;
}

bool Utils::safe_create_directory(const fs::path& path) {
    try {
        if (!fs::exists(path)) {
            return fs::create_directories(path);
        }
        return true;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error creating directory: " << e.what() << std::endl;
        return false;
    }
}

bool Utils::safe_write_file(const fs::path& path, const std::string& content) {
    try {
        std::ofstream file(path);
        if (!file) return false;
        file << content;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error writing file: " << e.what() << std::endl;
        return false;
    }
}

std::string Utils::safe_read_file(const fs::path& path) {
    try {
        std::ifstream file(path);
        if (!file) return "";
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        return content;
    } catch (const std::exception& e) {
        std::cerr << "Error reading file: " << e.what() << std::endl;
        return "";
    }
}

bool Utils::download_file(const std::string& url, const fs::path& output_path) {
    std::string command;
#ifdef _WIN32
    command = "curl -L -o \"" + output_path.string() + "\" \"" + url + "\"";
#else
    command = "curl -L -o \"" + output_path.string() + "\" \"" + url + "\"";
#endif

    int result = execute_command(command);
    return result == 0;
}

std::string Utils::trim(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), [](int c) {
        return std::isspace(c);
    });
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](int c) {
        return std::isspace(c);
    }).base();
    return (start < end) ? std::string(start, end) : std::string();
}

// 使用JsonParser实现的parse_simple_json
std::unordered_map<std::string, std::string> Utils::parse_simple_json(const std::string& json) {
    try {
        JsonParser parser(json);
        JsonValue value = parser.parse();

        if (!value.is_object()) {
            throw std::runtime_error("Expected JSON object");
        }

        std::unordered_map<std::string, std::string> result;
        for (const auto& [key, val] : value.as_object()) {
            if (val.is_string()) {
                result[key] = val.as_string();
            } else {
                result[key] = val.to_string();
            }
        }
        return result;
    } catch (const std::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return {};
    }
}

// 实现parse_json方法
JsonValue Utils::parse_json(const std::string& json) {
    JsonParser parser(json);
    return parser.parse();
}