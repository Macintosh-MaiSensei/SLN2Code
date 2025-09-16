#pragma once
#include "library_info_provider.h"
#include "constants.h"
#include <string>
#include <vector>
#include <unordered_map>

class RemoteLibraryProvider : public ILibraryInfoProvider {
public:
    RemoteLibraryProvider(const std::string& mirror_url);

    std::vector<std::string> get_available_libraries() override;
    std::optional<Constants::ThirdPartyLibrary> get_library_info(const std::string& lib_name) override;
    bool refresh_library_info() override;
    std::string get_provider_name() const override;

private:
    std::string mirror_url_;
    std::vector<std::string> available_libraries_;
    std::unordered_map<std::string, Constants::ThirdPartyLibrary> library_cache_;

    static std::string ensure_trailing_slash(const std::string& url);
};