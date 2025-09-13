#include "command_line_parser.h"
#include "library_service.h"
#include "builtin_library_provider.h"
#include "remote_library_provider.h"
#include "compiler_service.h"
#include "debugger_service.h"
#include "project_structure_service.h"
#include "project_generator.h"
#include "utils.h"
#include "Logo.h"
#include <iostream>
#include <filesystem>
#include <set>
#include <mutex>

namespace fs = std::filesystem;


int main(int argc, char* argv[]) {
    try {
        CommandLineParser::Options options = CommandLineParser::parse(argc, argv);

        if (options.show_help) {
            CommandLineParser::print_help(argv[0]);
            return 0;
        }

        if (options.show_version) {
            CommandLineParser::print_version();
            return 0;
        }

        // 配置库信息提供者
        if (!options.library_mirror_url.empty()) {
            LibraryService::set_provider(
                    std::make_unique<RemoteLibraryProvider>(options.library_mirror_url)
            );
            std::cout << "Using library mirror: " << options.library_mirror_url << "\n";
        } else {
            LibraryService::set_provider(std::make_unique<BuiltinLibraryProvider>());
        }

        // 交互式输入（如果没有通过命令行指定）
        if (options.project_name == Constants::DEFAULT_PROJECT_NAME) {
            Logo();
            std::cout << Constants::VERSION <<" Modular"<< " Alpha|"
                      << "https://github.com/Macintosh-MaiSensei/SLN2Code|"
                      << "Maintainer Macintosh-Maisensei\n";
            std::cout << "SLN2Code is libre and open-source software\n";

            std::cout << "Enter project name (default: " << Constants::DEFAULT_PROJECT_NAME << "): ";
            std::string input_name;
            std::getline(std::cin, input_name);

            if (!input_name.empty()) {
                options.project_name = Utils::get_valid_name(input_name);
            }

            std::cout << "Enter project path (default: current directory): ";
            std::string input_path;
            std::getline(std::cin, input_path);

            if (!input_path.empty()) {
                options.base_path = Utils::clean_path(input_path);
            } else {
                options.base_path = fs::current_path().string();
            }
        }

        // 获取完整的项目路径
        const fs::path project_full_path = fs::path(options.base_path) / options.project_name;

        // 如果目录已存在，提示用户确认
        if (fs::exists(project_full_path)) {
            std::cout << "Warning: Directory already exists: " << project_full_path << "\n";
            std::cout << "Do you want to continue? Existing files may be overwritten. [y/N]: ";
            std::string response;
            std::getline(std::cin, response);

            if (response.empty() || (response[0] != 'y' && response[0] != 'Y')) {
                std::cout << "Project creation canceled.\n";
                return 0;
            }
        }

        std::cout << "\nCreating project \"" << options.project_name << "\" at:\n"
                  << "  " << project_full_path << "\n\n";

        // 获取编译器配置（如果没有通过命令行指定）
        if (options.compiler.name.empty()) {
            options.compiler = CompilerService::get_compiler_config();
        } else {
            std::cout << "Compiler: " << options.compiler.name
                      << " (" << options.compiler.path << ")\n";
        }

        // 获取调试器配置（如果没有通过命令行指定）
        if (options.debugger.name.empty()) {
            options.debugger = DebuggerService::get_debugger_config();
        } else {
            std::cout << "Debugger: " << options.debugger.name
                      << " (" << options.debugger.path << ")\n";
        }

        std::cout << "C++ Standard: " << options.compiler.cppStandard << "\n";
        std::cout << "C Standard: " << options.compiler.cStandard << "\n\n";

        // 创建项目目录结构
        DirectoryNode project_structure = ProjectStructureService::get_project_structure(options.project_name);
        bool success = ProjectStructureService::create_directory_recursive(options.base_path, project_structure);

        if (!success) {
            std::cerr << "\nFailed to create project directories!\n";
            return 1;
        }

        // 处理命令行指定的库安装
        if (!options.libraries_to_install.empty()) {
            for (const auto& lib_name : options.libraries_to_install) {
                LibraryService::add_third_party_library(project_full_path, lib_name);
            }
        } else {
            LibraryService::offer_library_installation(project_full_path);
        }

        // 生成基础文件
        ProjectGenerator::generate_project_files(project_full_path, options.project_name,
                                                 options.compiler, options.debugger);

        std::cout << "\nProject \"" << options.project_name << "\" created successfully!\n";
        std::cout << "Configuration files generated in .vscode directory:\n";
        std::cout << "  - c_cpp_properties.json\n";
        std::cout << "  - tasks.json\n";
        std::cout << "  - launch.json\n";
        std::cout << "  - settings.json\n\n";

        if (!options.libraries_to_install.empty() ||
            fs::exists(project_full_path / "third_party")) {
            std::cout << "Third-party libraries installed in:\n";
            std::cout << "  - third_party/\n";
            std::cout << "  - docs/ (usage guides)\n\n";
        }

        std::cout << "Next steps:\n"
                  << "  1. cd " << project_full_path << "\n"
                  << "  2. code . (to open in VS Code)\n"
                  << "  3. Review and update configuration files if needed\n"
                  << "  4. Press F5 to build and debug!\n\n";

        std::cout << "Press Enter to exit...\n";
        std::cin.get();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}