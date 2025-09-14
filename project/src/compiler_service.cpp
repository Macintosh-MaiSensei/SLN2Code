#include "compiler_service.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <cstdlib>
#include <algorithm>

namespace fs = std::filesystem;

CompilerConfig CompilerService::get_compiler_config() {
    CompilerConfig config;

    std::cout << "\n=== Compiler Configuration ===\n";

    // 选择编译器类型
    std::cout << "Select compiler type:\n";
    std::cout << "1. gcc\n";
    std::cout << "2. g++\n";
    std::cout << "3. clang\n";
    std::cout << "4. clang++\n";
    std::cout << "5. MSVC (Visual Studio)\n";
    std::cout << "Enter choice (1-5, default 1): ";

    std::string choice;
    std::getline(std::cin, choice);

    if (choice == "2") {
        config.type = CompilerType::GXX;
        config.name = "g++";
    } else if (choice == "3") {
        config.type = CompilerType::CLANG;
        config.name = "clang";
    } else if (choice == "4") {
        config.type = CompilerType::CLANGXX;
        config.name = "clang++";
    } else if (choice == "5") {
        config.type = CompilerType::MSVC;
        config.name = "cl";
    } else {
        config.type = CompilerType::GXX;
        config.name = "g++";
    }

    // 获取编译器路径
    config.path = find_compiler_path(config);

    // 选择C++标准
    config.cppStandard = select_cpp_standard(config);

    // 选择C标准
    config.cStandard = select_c_standard(config);

    // 添加额外编译选项
    std::cout << "Enter any additional compiler flags (space separated, or press Enter for none): ";
    std::string extra_flags;
    std::getline(std::cin, extra_flags);

    if (!extra_flags.empty()) {
        std::istringstream iss(extra_flags);
        std::string flag;
        while (iss >> flag) {
            config.extraArgs.push_back(flag);
        }
    }

    return config;
}

std::string CompilerService::find_compiler_path(CompilerConfig& config) {
    std::vector<std::string> compiler_paths;

    if (config.type == CompilerType::MSVC) {
        compiler_paths = find_msvc_paths();
    } else {
        compiler_paths = find_compiler_in_path(config.name);
    }

    // 处理找到的编译器路径
    if (compiler_paths.empty()) {
        std::cout << "No " << config.name << " compiler found.\n";
        std::cout << "Enter " << config.name << " compiler path: ";
        std::string path;
        std::getline(std::cin, path);
        return Utils::clean_path(path);
    }

    if (compiler_paths.size() == 1) {
        std::cout << "Using " << config.name << " compiler found at: " << compiler_paths[0] << "\n";
        return compiler_paths[0];
    }

    std::cout << "Multiple " << config.name << " compilers found:\n";
    for (size_t i = 0; i < compiler_paths.size(); ++i) {
        std::cout << "  " << i + 1 << ". " << compiler_paths[i] << "\n";
    }
    std::cout << "Select compiler (1-" << compiler_paths.size() << "): ";
    std::string selection;
    std::getline(std::cin, selection);

    try {
        size_t idx = std::stoi(selection) - 1;
        if (idx < compiler_paths.size()) {
            return compiler_paths[idx];
        }
    } catch (...) {
        // 无效选择，使用第一个
    }

    return compiler_paths[0];
}

std::vector<std::string> CompilerService::find_compiler_in_path(const std::string& executable_name) {
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

std::vector<std::string> CompilerService::find_msvc_paths() {
    std::vector<std::string> paths;

#ifdef _WIN32
    // 常见的Visual Studio安装路径
    std::vector<std::string> common_paths = {
        "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC",
        "C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Tools/MSVC",
        "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/MSVC",
        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC",
        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional/VC/Tools/MSVC",
        "C:/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise/VC/Tools/MSVC"
    };

    for (const auto& base_path : common_paths) {
        if (fs::exists(base_path)) {
            for (const auto& entry : fs::directory_iterator(base_path)) {
                if (entry.is_directory()) {
                    fs::path compiler_path = entry.path() / "bin/Hostx64/x64/cl.exe";
                    if (fs::exists(compiler_path)) {
                        paths.push_back(Utils::clean_path(compiler_path.string()));
                    }
                }
            }
        }
    }
#endif

    return paths;
}

std::string CompilerService::select_cpp_standard(const CompilerConfig& compiler) {
    std::cout << "Select C++ standard:\n";
    std::cout << "1. C++98\n";
    std::cout << "2. C++11\n";
    std::cout << "3. C++14\n";
    std::cout << "4. C++17\n";
    std::cout << "5. C++20\n";
    std::cout << "6. C++23\n";
    std::cout << "Enter choice (1-6, default 4): ";

    std::string choice;
    std::getline(std::cin, choice);

    if (choice == "1" && Utils::is_standard_supported(compiler, "c++98")) {
        return "c++98";
    } else if (choice == "2" && Utils::is_standard_supported(compiler, "c++11")) {
        return "c++11";
    } else if (choice == "3" && Utils::is_standard_supported(compiler, "c++14")) {
        return "c++14";
    } else if (choice == "5" && Utils::is_standard_supported(compiler, "c++20")) {
        return "c++20";
    } else if (choice == "6" && Utils::is_standard_supported(compiler, "c++23")) {
        return "c++23";
    }
    return "c++17"; // 默认
}

std::string CompilerService::select_c_standard(const CompilerConfig& compiler) {
    std::cout << "Select C standard:\n";
    std::cout << "1. C99\n";
    std::cout << "2. C11\n";
    std::cout << "3. C17\n";
    std::cout << "4. C20\n";
    std::cout << "5. C23\n";
    std::cout << "Enter choice (1-5, default 3): ";

    std::string choice;
    std::getline(std::cin, choice);

    if (choice == "1" && Utils::is_standard_supported(compiler, "c99")) {
        return "c99";
    } else if (choice == "2" && Utils::is_standard_supported(compiler, "c11")) {
        return "c11";
    } else if (choice == "4" && Utils::is_standard_supported(compiler, "c20")) {
        return "c20";
    } else if (choice == "5" && Utils::is_standard_supported(compiler, "c23")) {
        return "c23";
    }
    return "c17"; // 默认
}