#include "command_line_parser.h"
#include "constants.h"
#include "utils.h"
#include "Logo.h"
#include <iostream>
#include <sstream>
#include <algorithm>

using namespace Constants;

CommandLineParser::Options CommandLineParser::parse(int argc, char* argv[]) {
    Options options;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--name" || arg == "-n") {
            if (i + 1 < argc) {
                options.project_name = Utils::get_valid_name(argv[++i]);
            } else {
                throw std::runtime_error("Missing project name after " + arg);
            }
        } else if (arg == "--path" || arg == "-p") {
            if (i + 1 < argc) {
                options.base_path = Utils::clean_path(argv[++i]);
            } else {
                throw std::runtime_error("Missing path after " + arg);
            }
        } else if (arg == "-v" || arg == "--version") {
            options.show_version = true;
        } else if (arg == "-I" || arg == "--install") {
            if (i + 1 < argc) {
                options.libraries_to_install.push_back(argv[++i]);
            } else {
                throw std::runtime_error("Missing library name after " + arg);
            }
        } else if (arg == "--library-mirror" || arg == "-lm") {
            if (i + 1 < argc) {
                options.library_mirror_url = argv[++i];
            } else {
                throw std::runtime_error("Missing mirror URL after " + arg);
            }
        } else if (arg == "-c" || arg == "--compiler") {
            if (i + 1 < argc) {
                std::string compiler_name = argv[++i];
                auto it = COMPILER_TYPE_MAP.find(compiler_name);
                if (it != COMPILER_TYPE_MAP.end()) {
                    options.compiler.type = it->second;
                    options.compiler.name = compiler_name;
                }
            } else {
                throw std::runtime_error("Missing compiler name after " + arg);
            }
        } else if (arg == "-cp" || arg == "--compiler-path") {
            if (i + 1 < argc) {
                options.compiler.path = Utils::clean_path(argv[++i]);
            } else {
                throw std::runtime_error("Missing compiler path after " + arg);
            }
        } else if (arg == "-cppstd" || arg == "--cpp-standard") {
            if (i + 1 < argc) {
                options.compiler.cppStandard = argv[++i];
            } else {
                throw std::runtime_error("Missing C++ standard after " + arg);
            }
        } else if (arg == "-cstd" || arg == "--c-standard") {
            if (i + 1 < argc) {
                options.compiler.cStandard = argv[++i];
            } else {
                throw std::runtime_error("Missing C standard after " + arg);
            }
        } else if (arg == "-d" || arg == "--debugger") {
            if (i + 1 < argc) {
                std::string debugger_name = argv[++i];
                auto it = DEBUGGER_TYPE_MAP.find(debugger_name);
                if (it != DEBUGGER_TYPE_MAP.end()) {
                    options.debugger.type = it->second;
                    options.debugger.name = debugger_name;
                }
            } else {
                throw std::runtime_error("Missing debugger name after " + arg);
            }
        } else if (arg == "-dp"|| arg == "--debugger-path") {
            if (i + 1 < argc) {
                options.debugger.path = Utils::clean_path(argv[++i]);
            } else {
                throw std::runtime_error("Missing debugger path after " + arg);
            }
        } else if (arg == "-ea" || arg == "--extra-args") {
            if (i + 1 < argc) {
                std::string extra_args = argv[++i];
                std::istringstream iss(extra_args);
                std::string flag;
                while (iss >> flag) {
                    options.compiler.extraArgs.push_back(flag);
                }
            } else {
                throw std::runtime_error("Missing extra arguments after " + arg);
            }
        } else if (arg == "--help" || arg == "-h") {
            options.show_help = true;
        } else {
            throw std::runtime_error("Unknown option: " + arg);
        }
    }

    return options;
}

void CommandLineParser::print_help(const std::string& program_name) {
    std::cout << "Usage: " << program_name << " [options]\n"
              << "Options:\n"
              << "  -n, --name NAME           Set project name\n"
              << "  -p, --path PATH           Set project path\n"
              << "  -I, --install LIB         Install third-party library\n"
              << "  -lm, --library-mirror URL Use custom library mirror\n"
              << "  -c, --compiler COMPILER    Set compiler (gcc, clang, cl)\n"
              << "  -cp, --compiler-path PATH   Set compiler path\n"
              << "  -cppstd, --cpp-standard STD     Set C++ standard (c++98 c++11 c++14 c++17 c++20 c++23)\n"
              << "  -cstd, --c-standard STD       Set C standard (c99 c11 c17 c23)\n"
              << "  -d, --debugger DEBUGGER    Set debugger (gdb, lldb)\n"
              << "  -dp, --debugger-path PATH   Set debugger path\n"
              << "  -ea, --extra-args ARGS      Set additional compiler flags\n"
              << "  -v, --version             Output the version of the program\n"
              << "  -h, --help                Show this help message\n";
}

void CommandLineParser::print_version() {
    Logo();
    std::cout << "SLN2Code\n"
              << "Version: " << Constants::VERSION << " Modular" << " Alpha\n"
              << "Maintainer: Macintosh-Maisensei\n"
              << "Contributors: None\n"
              << "https://github.com/Macintosh-MaiSensei/SLN2Code\n"
              << "SLN2Code is libre and open-source software, distributed under the MIT License.\n";
}