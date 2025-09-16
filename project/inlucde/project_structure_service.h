#pragma once
#include "directory_node.h"
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

class ProjectStructureService {
public:
    static DirectoryNode get_project_structure(const std::string& project_name);
    static bool create_directory_recursive(const fs::path& base_path, const DirectoryNode& node,
                                           int depth = 0, const std::string& prefix = "");

#ifdef _WIN32
    static bool use_ascii_output;
#endif
};