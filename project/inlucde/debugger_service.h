#pragma once
#include "debugger_config.h"
#include <string>
#include <vector>

class DebuggerService {
public:
    static DebuggerConfig get_debugger_config();

private:
    static std::string find_debugger_path(DebuggerConfig& config);
    static std::vector<std::string> find_debugger_in_path(const std::string& executable_name);
    static std::vector<std::string> find_vs_debugger_paths();
};