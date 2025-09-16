#pragma once
#include "library_info_provider.h"
#include "constants.h"

class BuiltinLibraryProvider : public ILibraryInfoProvider {
public:
    std::vector<std::string> get_available_libraries() override;
    std::optional<Constants::ThirdPartyLibrary> get_library_info(const std::string& lib_name) override;
    bool refresh_library_info() override;
    std::string get_provider_name() const override;
};