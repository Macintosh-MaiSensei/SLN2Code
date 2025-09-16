#pragma once
#include "constants.h"
#include <string>

class DebuggerConfig {
public:
    std::string name;
    std::string path;
    DebuggerType type = DebuggerType::UNKNOWN;
};