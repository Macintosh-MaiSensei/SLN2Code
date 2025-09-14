#pragma once
#include "compiler_config.h"
#include <string>
#include <vector>

class CompilerService {
public:
    static CompilerConfig get_compiler_config();

private:
    static std::string find_compiler_path(CompilerConfig& config);
    static std::vector<std::string> find_compiler_in_path(const std::string& executable_name);
    static std::vector<std::string> find_msvc_paths();
    static std::string select_cpp_standard(const CompilerConfig& compiler);
    static std::string select_c_standard(const CompilerConfig& compiler);
};