#pragma once
#include "project_config.h"
#include <string>
#include <vector>
#include <iostream>

class CommandLineParser {
public:
    struct Options {
        std::string project_name = Constants::DEFAULT_PROJECT_NAME;
        std::string base_path;
        std::vector<std::string> libraries_to_install;
        std::string library_mirror_url;
        CompilerConfig compiler;
        DebuggerConfig debugger;
        bool show_version = false;
        bool show_help = false;
    };

    static Options parse(int argc, char* argv[]);
    static void print_help(const std::string& program_name);
    static void print_version();
};