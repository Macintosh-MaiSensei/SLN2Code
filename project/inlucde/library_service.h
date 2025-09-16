#pragma once
#include "library_info_provider.h"
#include "constants.h"
#include <filesystem>
#include <string>
#include <vector>
#include <memory>
#include <set>
#include <mutex>
#include <future>

namespace fs = std::filesystem;

class LibraryService {
private:
    static std::unique_ptr<ILibraryInfoProvider> provider_;
    static std::string get_unzip_command(const fs::path& zip_file, const fs::path& output_dir);
    static void create_third_party_dir(const fs::path& project_path);
    static void generate_library_guide(const fs::path& project_path, const Constants::ThirdPartyLibrary& lib);
    static ILibraryInfoProvider& get_provider();

public:
    static void set_provider(std::unique_ptr<ILibraryInfoProvider> provider);
    static void offer_library_installation(const fs::path& project_path);
    static bool download_and_extract_library(const fs::path& project_path, const Constants::ThirdPartyLibrary& lib);
    static void add_third_party_library(const fs::path& project_path, const std::string& lib_name);
};