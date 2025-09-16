#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <array>
#include <cctype>
#include <algorithm>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <regex>
#include <functional>
#include <optional>
#include "compiler_config.h"
#include "json_value.h"
namespace fs = std::filesystem;

class Utils {
public:
    static std::string clean_path(const std::string& input);
    static std::string get_valid_name(const std::string& input);
    static bool is_standard_supported(const CompilerConfig& compiler, const std::string& standard);
    static int execute_command(const std::string& command);
    static std::string execute_command_with_output(const std::string& command);
    static fs::path join_paths(const fs::path& base, const std::string& part);
    static bool safe_create_directory(const fs::path& path);
    static bool safe_write_file(const fs::path& path, const std::string& content);
    static std::string safe_read_file(const fs::path& path);
    static bool download_file(const std::string& url, const fs::path& output_path);
    static std::string trim(const std::string& str);
    static std::unordered_map<std::string, std::string> parse_simple_json(const std::string& json);
    static JsonValue parse_json(const std::string& json);
};