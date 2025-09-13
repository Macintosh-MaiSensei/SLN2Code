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

    // 替换反斜杠为正斜杠
    std::replace(cleaned.begin(), cleaned.end(), '\\', '/');

    // 移除末尾的斜杠
    if (!cleaned.empty() && cleaned.back() == '/' && cleaned.size() > 1) {
        cleaned.pop_back();
    }

    return cleaned;
}

std::string Utils::get_valid_name(const std::string& input) {
    std::string name = input;

    // 移除非字母数字字符（允许连字符和下划线）
    name.erase(std::remove_if(name.begin(), name.end(),
                              [](char c) { return !(std::isalnum(c) || c == '-' || c == '_' || c == ' '); }),
               name.end());

    // 替换空格为连字符
    std::replace(name.begin(), name.end(), ' ', '-');

    // 转换为小写
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    return name;
}

bool Utils::is_standard_supported(const CompilerConfig& compiler, const std::string& standard) {
    // 简化实现：实际项目中应调用编译器获取支持的标准列表
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

std::unordered_map<std::string, std::string> Utils::parse_simple_json(const std::string& json) {
    std::unordered_map<std::string, std::string> result;
    std::istringstream stream(json);
    std::string line;

    while (std::getline(stream, line)) {
        line = trim(line);
        if (line.empty() || line == "{" || line == "}") continue;

        // 移除逗号和引号
        line.erase(std::remove(line.begin(), line.end(), '"'), line.end());
        line.erase(std::remove(line.begin(), line.end(), ','), line.end());

        // 分割键值对
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = trim(line.substr(0, colon_pos));
            std::string value = trim(line.substr(colon_pos + 1));
            result[key] = value;
        }
    }

    return result;
}