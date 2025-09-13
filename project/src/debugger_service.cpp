#include "debugger_service.h"
#include "utils.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <cstdlib>

namespace fs = std::filesystem;

DebuggerConfig DebuggerService::get_debugger_config() {
    DebuggerConfig config;

    std::cout << "\n=== Debugger Configuration ===\n";

    // 选择调试器类型
    std::cout << "Select debugger type:\n";
    std::cout << "1. GDB\n";
    std::cout << "2. LLDB\n";
    std::cout << "3. Visual Studio Debugger\n";
    std::cout << "Enter choice (1-3, default 1): ";

    std::string choice;
    std::getline(std::cin, choice);

    if (choice == "2") {
        config.type = DebuggerType::LLDB;
        config.name = "lldb";
    } else if (choice == "3") {
        config.type = DebuggerType::CPPVSDBG;
        config.name = "Visual Studio Debugger";
    } else {
        config.type = DebuggerType::GDB;
        config.name = "gdb";
    }

    // 获取调试器路径
    config.path = find_debugger_path(config);

    return config;
}

std::string DebuggerService::find_debugger_path(DebuggerConfig& config) {
    std::vector<std::string> debugger_paths;

    if (config.type == DebuggerType::CPPVSDBG) {
        debugger_paths = find_vs_debugger_paths();
    } else {
        debugger_paths = find_debugger_in_path(config.name);
    }

    // 处理找到的调试器路径
    if (debugger_paths.empty()) {
        std::cout << "No " << config.name << " debugger found.\n";
        std::cout << "Enter " << config.name << " debugger path: ";
        std::string path;
        std::getline(std::cin, path);
        return Utils::clean_path(path);
    }

    if (debugger_paths.size() == 1) {
        std::cout << "Using " << config.name << " debugger found at: " << debugger_paths[0] << "\n";
        return debugger_paths[0];
    }

    std::cout << "Multiple " << config.name << " debuggers found:\n";
    for (size_t i = 0; i < debugger_paths.size(); ++i) {
        std::cout << "  " << i + 1 << ". " << debugger_paths[i] << "\n";
    }
    std::cout << "Select debugger (1-" << debugger_paths.size() << "): ";
    std::string selection;
    std::getline(std::cin, selection);

    try {
        size_t idx = std::stoi(selection) - 1;
        if (idx < debugger_paths.size()) {
            return debugger_paths[idx];
        }
    } catch (...) {
        // 无效选择，使用第一个
    }

    return debugger_paths[0];
}

std::vector<std::string> DebuggerService::find_debugger_in_path(const std::string& executable_name) {
    std::vector<std::string> found_paths;

    // 获取PATH环境变量
    const char* path_env = std::getenv("PATH");
    if (!path_env) {
        return found_paths;
    }

    std::string path_str(path_env);

    // 分割PATH字符串
#ifdef _WIN32
    const char delimiter = ';';
    std::string exe_name = executable_name + ".exe";
#else
    const char delimiter = ':';
    std::string exe_name = executable_name;
#endif

    std::istringstream path_stream(path_str);
    std::string directory;

    while (std::getline(path_stream, directory, delimiter)) {
        fs::path dir_path(directory);
        if (fs::exists(dir_path) && fs::is_directory(dir_path)) {
            fs::path executable_path = dir_path / exe_name;
            if (fs::exists(executable_path) && fs::is_regular_file(executable_path)) {
                found_paths.push_back(Utils::clean_path(executable_path.string()));
            }
        }
    }

    return found_paths;
}

std::vector<std::string> DebuggerService::find_vs_debugger_paths() {
    std::vector<std::string> paths;

#ifdef _WIN32
    // 常见的Visual Studio安装路径
    std::vector<std::string> common_paths = {
        "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE",
        "C:/Program Files/Microsoft Visual Studio/2022/Professional/Common7/IDE",
        "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/Common7/IDE",
        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE",
        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional/Common7/IDE",
        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise/Common7/IDE"
    };

    for (const auto& path : common_paths) {
        fs::path debugger_path = fs::path(path) / "Debugger/vsdbg.exe";
        if (fs::exists(debugger_path)) {
            paths.push_back(Utils::clean_path(debugger_path.string()));
        }
    }
#endif

    return paths;
}