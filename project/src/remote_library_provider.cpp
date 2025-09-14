#include "remote_library_provider.h"
#include "utils.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

RemoteLibraryProvider::RemoteLibraryProvider(const std::string& mirror_url)
        : mirror_url_(ensure_trailing_slash(mirror_url)) {}

std::vector<std::string> RemoteLibraryProvider::get_available_libraries() {
    if (available_libraries_.empty()) {
        refresh_library_info();
    }
    return available_libraries_;
}

std::optional<Constants::ThirdPartyLibrary> RemoteLibraryProvider::get_library_info(const std::string& lib_name) {
    // 首先检查缓存
    auto it = library_cache_.find(lib_name);
    if (it != library_cache_.end()) {
        return it->second;
    }

    // 从远程获取库信息
    std::string lib_info_url = mirror_url_ + lib_name + ".json";
    fs::path temp_file = fs::temp_directory_path() / (lib_name + "_info.json");

    if (!Utils::download_file(lib_info_url, temp_file)) {
        std::cerr << "Failed to download library info for " << lib_name << std::endl;
        return std::nullopt;
    }

    std::string json_content = Utils::safe_read_file(temp_file);
    if (json_content.empty()) {
        std::cerr << "Failed to read library info for " << lib_name << std::endl;
        return std::nullopt;
    }

    // 解析JSON
    auto json_map = Utils::parse_simple_json(json_content);
    Constants::ThirdPartyLibrary lib_info;
    lib_info.name = json_map["name"];
    lib_info.downloadUrl = json_map["downloadUrl"];
    lib_info.includePath = json_map["includePath"];
    lib_info.libPath = json_map["libPath"];
    lib_info.configInstructions = json_map["configInstructions"];
    lib_info.sha256 = json_map["sha256"];

    // 解析依赖项
    std::istringstream deps_stream(json_map["dependencies"]);
    std::string dep;
    while (std::getline(deps_stream, dep, ',')) {
        lib_info.dependencies.push_back(Utils::trim(dep));
    }

    // 添加到缓存
    library_cache_[lib_name] = lib_info;

    // 删除临时文件
    fs::remove(temp_file);

    return lib_info;
}

bool RemoteLibraryProvider::refresh_library_info() {
    // 下载库列表
    std::string list_url = mirror_url_ + "libraries.json";
    fs::path temp_file = fs::temp_directory_path() / "library_list.json";

    if (!Utils::download_file(list_url, temp_file)) {
        std::cerr << "Failed to download library list" << std::endl;
        return false;
    }

    std::string json_content = Utils::safe_read_file(temp_file);
    if (json_content.empty()) {
        std::cerr << "Failed to read library list" << std::endl;
        return false;
    }

    // 解析库列表
    available_libraries_.clear();
    std::istringstream stream(json_content);
    std::string line;

    while (std::getline(stream, line)) {
        line = Utils::trim(line);
        if (line.empty() || line == "[" || line == "]") continue;

        // 移除引号和逗号
        line.erase(std::remove(line.begin(), line.end(), '"'), line.end());
        line.erase(std::remove(line.begin(), line.end(), ','), line.end());

        if (!line.empty()) {
            available_libraries_.push_back(line);
        }
    }

    // 删除临时文件
    fs::remove(temp_file);

    return true;
}

std::string RemoteLibraryProvider::get_provider_name() const {
    return "Remote Library Provider (" + mirror_url_ + ")";
}

std::string RemoteLibraryProvider::ensure_trailing_slash(const std::string& url) {
    if (url.empty() || url.back() != '/') {
        return url + '/';
    }
    return url;
}