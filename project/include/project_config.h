#pragma once
#include "compiler_config.h"
#include "debugger_config.h"
#include <vector>
#include <string>

class ProjectConfig {
public:
    std::string name;
    std::string path;
    CompilerConfig compiler;
    DebuggerConfig debugger;
    std::vector<std::string> libraries;
};