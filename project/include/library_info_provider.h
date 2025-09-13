#pragma once
#include "constants.h"
#include <vector>
#include <string>
#include <optional>

class ILibraryInfoProvider {
public:
    virtual ~ILibraryInfoProvider() = default;
    virtual std::vector<std::string> get_available_libraries() = 0;
    virtual std::optional<Constants::ThirdPartyLibrary> get_library_info(const std::string& lib_name) = 0;
    virtual bool refresh_library_info() = 0;
    virtual std::string get_provider_name() const = 0;
};