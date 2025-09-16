#include "project_structure_service.h"
#include "utils.h"
#include <iostream>
#include <algorithm>

namespace fs = std::filesystem;

#ifdef _WIN32
bool ProjectStructureService::use_ascii_output = true;
#endif

DirectoryNode ProjectStructureService::get_project_structure(const std::string& project_name) {
    return {
            project_name,
            {
                    {".vscode", {}},
                    {"include", {}},
                    {"lib", {}},
                    {"lib64", {}},
                    {"src", {}},
                    {"build", {
                            {"bin", {
                                    {"Debug", {}},
                                    {"Release", {}}
                            }},
                            {"obj", {}}
                    }},
                    {"third_party", {}},
                    {"docs", {}}
            }
    };
}

#ifdef _WIN32
bool ProjectStructureService::create_directory_recursive(const fs::path& base_path, const DirectoryNode& node,
                                                         int depth, const std::string& prefix) {
    const fs::path current_path = Utils::join_paths(base_path, node.name);
    const std::string indent(depth * 2, ' ');

    // 定义替代字符
    const std::string vertical_line = use_ascii_output ? "|" : "\u2502";
    const std::string branch_line = use_ascii_output ? "|-- " : "\u251c\u2500\u2500 ";
    const std::string space_fill = use_ascii_output ? "    " : "    ";

    // 使用树状结构前缀
    std::string tree_prefix;
    if (depth > 0) {
        tree_prefix = prefix + (depth > 1 ? vertical_line + "   " : "") + (depth > 0 ? branch_line : "");
    }

    // 显示当前目录
    std::cout << tree_prefix << node.name;

    try {
        if (!fs::exists(current_path)) {
            if (Utils::safe_create_directory(current_path)) {
                std::cout << " ...done" << "\n";
            } else {
                std::cout << " ...failed" << "\n";
                return false;
            }
        } else {
            std::cout << " ...done" << "\n";
        }

        // 处理子目录
        std::string child_prefix = prefix + (depth > 0 ? vertical_line + "   " : "");
        for (size_t i = 0; i < node.children.size(); i++) {
            const auto& child = node.children[i];

            // 判断是否是最后一个子节点
            bool is_last = (i == node.children.size() - 1);
            std::string new_prefix = is_last ? space_fill : vertical_line + "   ";

            if (!create_directory_recursive(current_path, child, depth + 1,
                                            child_prefix + new_prefix)) {
                return false;
            }
        }

        return true;
    } catch (const fs::filesystem_error& e) {
        std::cout << " ...failed (" << e.what() << ")" << std::endl;
        return false;
    }
}
#endif

#if defined(__linux__) || defined(__APPLE__)
bool ProjectStructureService::create_directory_recursive(const fs::path& base_path, const DirectoryNode& node,
                                                         int depth, const std::string& prefix) {
    const fs::path current_path = Utils::join_paths(base_path, node.name);
    const std::string indent(depth * 2, ' ');

    // 使用树状结构前缀
    std::string tree_prefix;
    if (depth > 0) {
        tree_prefix = prefix + (depth > 1 ? "│   " : "") + (depth > 0 ? "├── " : "");
    }

    // 显示当前目录
    std::cout << tree_prefix << node.name;

    try {
        if (!fs::exists(current_path)) {
            if (Utils::safe_create_directory(current_path)) {
                std::cout << " ✓" << "\n";
            } else {
                std::cout << " ✗" << "\n";
                return false;
            }
        } else {
            std::cout << " ✓" << "\n";
        }

        // 处理子目录
        std::string child_prefix = prefix + (depth > 0 ? "│   " : "");
        for (size_t i = 0; i < node.children.size(); i++) {
            const auto& child = node.children[i];

            // 判断是否是最后一个子节点
            bool is_last = (i == node.children.size() - 1);
            std::string new_prefix = is_last ? "    " : "│   ";

            if (!create_directory_recursive(current_path, child, depth + 1,
                                            child_prefix + new_prefix)) {
                return false;
            }
        }

        return true;
    } catch (const fs::filesystem_error& e) {
        std::cout << " ✗ (" << e.what() << ")" << std::endl;
        return false;
    }
}
#endif