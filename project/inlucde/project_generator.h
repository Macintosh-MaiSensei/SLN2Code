#pragma once
#include "compiler_config.h"
#include "debugger_config.h"
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

class ProjectGenerator {
public:
    static void generate_project_files(const fs::path& project_path, const std::string& project_name,
                                       const CompilerConfig& compiler, const DebuggerConfig& debugger);

private:
    static void generate_c_cpp_properties(const fs::path& vscode_dir, const CompilerConfig& compiler);
    static std::string get_intellisense_mode(CompilerType type);
    static void generate_tasks_json(const fs::path& vscode_dir, const std::string& project_name,
                                    const CompilerConfig& compiler);
    static void generate_launch_json(const fs::path& vscode_dir, const std::string& project_name,
                                     const CompilerConfig& compiler, const DebuggerConfig& debugger);
    static void generate_settings_json(const fs::path& vscode_dir);
    static void generate_cmake_file(const fs::path& project_path, const std::string& project_name,
                                    const CompilerConfig& compiler);
    static void generate_main_cpp(const fs::path& project_path, const std::string& project_name);
    static void generate_gitignore(const fs::path& project_path);
};