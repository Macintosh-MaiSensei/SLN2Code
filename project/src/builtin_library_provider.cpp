#include "builtin_library_provider.h"
#include "constants.h"

std::vector<std::string> BuiltinLibraryProvider::get_available_libraries() {
    std::vector<std::string> libs;
    for (const auto& pair : Constants::BUILTIN_LIBRARIES) {
        libs.push_back(pair.first);
    }
    return libs;
}

std::optional<Constants::ThirdPartyLibrary> BuiltinLibraryProvider::get_library_info(const std::string& lib_name) {
    auto it = Constants::BUILTIN_LIBRARIES.find(lib_name);
    if (it != Constants::BUILTIN_LIBRARIES.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool BuiltinLibraryProvider::refresh_library_info() {
    // 内置库不需要刷新
    return true;
}

std::string BuiltinLibraryProvider::get_provider_name() const {
    return "Built-in Library Provider";
}