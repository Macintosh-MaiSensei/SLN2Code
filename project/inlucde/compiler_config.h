#pragma once
#include "constants.h"
#include <vector>
#include <string>

class CompilerConfig {
public:
    std::string name;
    std::string path;
    std::string cppStandard;
    std::string cStandard;
    CompilerType type = CompilerType::UNKNOWN;
    std::vector<std::string> extraArgs;
};