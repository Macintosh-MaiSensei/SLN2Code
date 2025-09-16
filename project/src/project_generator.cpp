#include "project_generator.h"
#include "utils.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>

namespace fs = std::filesystem;

void ProjectGenerator::generate_project_files(const fs::path& project_path, const std::string& project_name,
                                              const CompilerConfig& compiler, const DebuggerConfig& debugger) {
    const fs::path vscode_dir = project_path / ".vscode";
    Utils::safe_create_directory(vscode_dir);

    generate_c_cpp_properties(vscode_dir, compiler);
    generate_tasks_json(vscode_dir, project_name, compiler);
    generate_launch_json(vscode_dir, project_name, compiler, debugger);
    generate_settings_json(vscode_dir);
    generate_cmake_file(project_path, project_name, compiler);
    generate_main_cpp(project_path, project_name);
    generate_gitignore(project_path);
}

void ProjectGenerator::generate_c_cpp_properties(const fs::path& vscode_dir, const CompilerConfig& compiler) {
    std::string intelliSenseMode = get_intellisense_mode(compiler.type);
    std::string compilerPath = Utils::clean_path(compiler.path);

    std::string content = R"({
    "configurations": [
        {
            "name": "Win32",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/include"
            ],
            "defines": [
                "_DEBUG",
                "UNICODE",
                "_UNICODE"
            ],
            "compilerPath": ")" + compilerPath + R"(",
            "cStandard": ")" + compiler.cStandard + R"(",
            "cppStandard": ")" + compiler.cppStandard + R"(",
            "intelliSenseMode": ")" + intelliSenseMode + R"("
        },
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/include"
            ],
            "defines": [],
            "compilerPath": ")" + compilerPath + R"(",
            "cStandard": ")" + compiler.cStandard + R"(",
            "cppStandard": ")" + compiler.cppStandard + R"(",
            "intelliSenseMode": ")" + intelliSenseMode + R"("
        }
    ],
    "version": 4
})";

    Utils::safe_write_file(vscode_dir / "c_cpp_properties.json", content);
}

std::string ProjectGenerator::get_intellisense_mode(CompilerType type) {
    // 确定平台和架构
    std::string platform;
    std::string architecture;

#if defined(_WIN32) || defined(_WIN64)
    platform = "windows";
    #if defined(_M_AMD64) || defined(__x86_64__)
        architecture = "x64";
    #elif defined(_M_IX86)
        architecture = "x86";
    #elif defined(_M_ARM)
        architecture = "arm";
    #elif defined(_M_ARM64) || defined(__aarch64__)
        architecture = "arm64";
    #else
        architecture = "x64"; // 默认
    #endif
#elif defined(__APPLE__)
    platform = "macos";
#if defined(__x86_64__)
    architecture = "x64";
#elif defined(__aarch64__) || defined(__arm64__)
    architecture = "arm64";
    #else
        architecture = "arm64"; // Apple Silicon默认
#endif
#elif defined(__linux__)
    platform = "linux";
    #if defined(__x86_64__)
        architecture = "x64";
    #elif defined(__i386__)
        architecture = "x86";
    #elif defined(__aarch64__)
        architecture = "arm64";
    #elif defined(__arm__)
        architecture = "arm";
    #else
        architecture = "x64"; // 默认
    #endif
#else
    platform = "linux";
    architecture = "x64"; // 其他平台默认
#endif

    // 根据编译器类型和架构组合模式字符串
    switch (type) {
        case CompilerType::CLANG:
        case CompilerType::CLANGXX:
            return platform + "-clang-" + architecture;

        case CompilerType::MSVC:
            return platform + "-msvc-" + architecture;

        case CompilerType::GCC:
        case CompilerType::GXX:
        default:
            return platform + "-gcc-" + architecture;
    }
}

void ProjectGenerator::generate_tasks_json(const fs::path& vscode_dir, const std::string& project_name,
                                           const CompilerConfig& compiler) {
    std::string output_ext = (compiler.type == CompilerType::MSVC) ? ".exe" : "";
    std::string compilerPath = Utils::clean_path(compiler.path);

    std::string args;
    if (compiler.type == CompilerType::MSVC) {
        args = R"(                "/std:)" + compiler.cppStandard.substr(2) + R"(",
                "/I${workspaceFolder}/include",)";
    } else {
        args = R"(                "-std=)" + compiler.cppStandard + R"(",
                "-I${workspaceFolder}/include",)";
    }

    // 添加额外编译选项
    for (const auto& arg : compiler.extraArgs) {
        args += "\n                \"" + arg + "\",";
    }

    std::string problemMatcher = (compiler.type == CompilerType::MSVC) ?
                                 R"("$msCompile")" : R"("$gcc")";

    std::string content = R"({
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build",
            "type": "shell",
            "command": ")" + compilerPath + R"(",
            "args": [
)" + args + R"(
                "${workspaceFolder}/src/*.cpp",
                "-o",
                "${workspaceFolder}/build/bin/Debug/)" + project_name + output_ext + R"("
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": [)" + problemMatcher + R"(],
            "detail": "Built with )" + compiler.name + R"("
        },
        {
            "label": "Clean",
            "type": "shell",
            "command": "rm",
            "args": [
                "-rf",
                "${workspaceFolder}/build/bin/Debug/*"
            ]
        }
    ]
})";

    Utils::safe_write_file(vscode_dir / "tasks.json", content);
}

void ProjectGenerator::generate_launch_json(const fs::path& vscode_dir, const std::string& project_name,
                                            const CompilerConfig& compiler, const DebuggerConfig& debugger) {
    std::string output_ext = (compiler.type == CompilerType::MSVC) ? ".exe" : "";
    std::string debuggerPath = Utils::clean_path(debugger.path);
    std::string debuggerType;

    switch (debugger.type) {
        case DebuggerType::GDB: debuggerType = "gdb"; break;
        case DebuggerType::LLDB: debuggerType = "lldb"; break;
        case DebuggerType::CPPVSDBG: debuggerType = "cppvsdbg"; break;
        default: debuggerType = "cppdbg";
    }

    std::string content = R"({
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug Launch",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/bin/Debug/)" + project_name + output_ext + R"(",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": true,
            "MIMode": ")" + debuggerType + R"(",
            "miDebuggerPath": ")" + debuggerPath + R"(",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ],
            "preLaunchTask": "Build"
        }
    ]
})";

    Utils::safe_write_file(vscode_dir / "launch.json", content);
}

void ProjectGenerator::generate_settings_json(const fs::path& vscode_dir) {
    std::string content = R"({
    "files.associations": {
        "*.h": "c",
        "*.hpp": "cpp",
        "*.ipp": "cpp"
    },
    "editor.formatOnSave": true,
    "C_Cpp.default.configurationProvider": "ms-vscode.cpptools",
    "explorer.confirmDragAndDrop": false
})";

    Utils::safe_write_file(vscode_dir / "settings.json", content);
}

void ProjectGenerator::generate_cmake_file(const fs::path& project_path, const std::string& project_name,
                                           const CompilerConfig& compiler) {
    std::string content = "cmake_minimum_required(VERSION 3.20)\n"
                          "project(" + project_name + " VERSION 1.0 LANGUAGES CXX)\n\n"
                                                      "set(CMAKE_CXX_STANDARD " + compiler.cppStandard.substr(2) + ")\n"
                                                                                                                   "set(CMAKE_C_STANDARD " + compiler.cStandard.substr(1) + ")\n\n"
                                                                                                                                                                            "include(FetchContent)\n"
                                                                                                                                                                            "include_directories(include)\n"
                                                                                                                                                                            "add_executable(" + project_name + " src/main.cpp)\n"
                                                                                                                                                                                                               "target_include_directories(" + project_name + " PUBLIC include)\n"
                                                                                                                                                                                                                                                              "install(TARGETS " + project_name + " DESTINATION bin)\n"
                                                                                                                                                                                                                                                                                                  "install(DIRECTORY include/ DESTINATION include)\n";

    // 添加编译器特定选项
    if (!compiler.extraArgs.empty()) {
        content += "\n# Additional compiler flags\n";
        content += "target_compile_options(" + project_name + " PRIVATE";
        for (const auto& arg : compiler.extraArgs) {
            content += " " + arg;
        }
        content += ")\n";
    }

    Utils::safe_write_file(project_path / "CMakeLists.txt", content);
}

void ProjectGenerator::generate_main_cpp(const fs::path& project_path, const std::string& project_name) {
    const fs::path src_dir = project_path / "src";
    Utils::safe_create_directory(src_dir);

    std::string content = R"(#include <iostream>

int main() {
    std::cout << "Hello, )" + project_name + R"(!\n";
    std::cout << "Project created successfully!\n";
    return 0;
}
)";

    Utils::safe_write_file(src_dir / "main.cpp", content);
}

void ProjectGenerator::generate_gitignore(const fs::path& project_path) {
    std::string content = R"(# Build artifacts
build/
*.exe
*.out
*.o
*.obj

# Editor files
.vscode/
!.vscode/tasks.json
!.vscode/launch.json
!.vscode/c_cpp_properties.json
!.vscode/settings.json

# System files
.DS_Store
Thumbs.db
)";

    Utils::safe_write_file(project_path / ".gitignore", content);
}