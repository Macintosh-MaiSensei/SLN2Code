#include "library_service.h"
#include "builtin_library_provider.h"
#include "remote_library_provider.h"
#include "utils.h"
#include "sha256.h"
#include <fstream>
#include <sstream>
#include <thread>
#include <future>
#include <iostream>

using namespace Constants;

std::unique_ptr<ILibraryInfoProvider> LibraryService::provider_ = nullptr;

void LibraryService::set_provider(std::unique_ptr<ILibraryInfoProvider> provider) {
    provider_ = std::move(provider);
}

void LibraryService::offer_library_installation(const fs::path& project_path) {
    auto& provider = get_provider();
    auto available_libs = provider.get_available_libraries();

    if (available_libs.empty()) {
        std::cout << "No libraries available from the current provider.\n";
        return;
    }

    std::cout << "\nWould you like to add any third-party libraries? [y/N]: ";
    std::string response;
    std::getline(std::cin, response);

    if (response.empty() || (response[0] != 'y' && response[0] != 'Y')) {
        return;
    }

    std::cout << "\nAvailable libraries:\n";
    for (const auto& lib : available_libs) {
        std::cout << "  - " << lib << "\n";
    }

    while (true) {
        std::cout << "\nEnter library name (or 'done' to finish): ";
        std::string lib_name;
        std::getline(std::cin, lib_name);

        if (lib_name == "done") {
            break;
        }

        add_third_party_library(project_path, lib_name);
    }
}

void LibraryService::add_third_party_library(const fs::path& project_path, const std::string& lib_name) {
    static std::set<std::string> installed_libs;
    static std::mutex mtx;

    {
        std::lock_guard<std::mutex> lock(mtx);
        if (installed_libs.find(lib_name) != installed_libs.end()) {
            std::cout << lib_name << " already installed. Skipping.\n";
            return;
        }
        installed_libs.insert(lib_name);
    }

    create_third_party_dir(project_path);

    auto& provider = get_provider();
    auto lib_info = provider.get_library_info(lib_name);

    if (!lib_info) {
        std::cerr << "Library not found: " << lib_name << std::endl;
        return;
    }

    std::cout << "\nAdding " << lib_info->name << " to project...\n";

    // 下载并解压库
    if (!download_and_extract_library(project_path, *lib_info)) {
        return;
    }

    // 生成使用指南
    generate_library_guide(project_path, *lib_info);

    // 更新CMakeLists.txt
    const fs::path cmake_path = project_path / "CMakeLists.txt";
    if (fs::exists(cmake_path)) {
        std::ofstream cmake(cmake_path, std::ios::app);
        cmake << "\n# " << lib_info->name << " Configuration\n";
        cmake << lib_info->configInstructions << "\n";
    }

    // 安装依赖项
    for (const auto& dep : lib_info->dependencies) {
        std::cout << "Installing dependency: " << dep << "\n";
        add_third_party_library(project_path, dep);
    }

    std::cout << lib_info->name << " added successfully!\n";
    std::cout << "See docs/" << lib_info->name << "_GUIDE.md for usage instructions\n";
}

bool LibraryService::download_and_extract_library(const fs::path& project_path, const ThirdPartyLibrary& lib) {
    const fs::path third_party_dir = project_path / "third_party";
    const fs::path zip_file = third_party_dir / (lib.name + ".zip");

    std::cout << "\nDownloading " << lib.name << "...\n";

    // 下载文件
    if (!Utils::download_file(lib.downloadUrl, zip_file)) {
        std::cerr << "Failed to download " << lib.name << std::endl;
        return false;
    }

    std::cout << "Extracting " << lib.name << "...\n";

    // 验证SHA256
    if (!lib.sha256.empty()) {
        std::cout << "Verifying SHA256 checksum...\n";
        try {
            std::string calculated_sha = SHA256::hash_file(zip_file);
            std::transform(calculated_sha.begin(), calculated_sha.end(), calculated_sha.begin(), ::tolower);

            if (calculated_sha != lib.sha256) {
                std::cerr << "SHA256 verification failed!\n";
                std::cerr << "Expected: " << lib.sha256 << "\n";
                std::cerr << "Actual:   " << calculated_sha << "\n";
                fs::remove(zip_file);
                return false;
            }
            std::cout << "SHA256 verification passed.\n";
        } catch (const std::exception& e) {
            std::cerr << "Error during SHA256 calculation: " << e.what() << std::endl;
            fs::remove(zip_file);
            return false;
        }
    } else {
        std::cout << "Warning: No SHA256 provided for " << lib.name << ", skipping verification.\n";
    }

    // 解压文件
    std::string unzip_cmd = get_unzip_command(zip_file, third_party_dir);
    int result = Utils::execute_command(unzip_cmd);

    if (result != 0) {
        std::cerr << "Failed to extract " << lib.name << std::endl;
        return false;
    }

    // 删除压缩包（异步执行）
    std::thread([zip_file]() {
        try {
            fs::remove(zip_file);
        } catch (const std::exception& e) {
            std::cerr << "Error removing zip file: " << e.what() << std::endl;
        }
    }).detach();

    return true;
}

void LibraryService::create_third_party_dir(const fs::path& project_path) {
    const fs::path third_party_dir = project_path / "third_party";
    Utils::safe_create_directory(third_party_dir);
}

void LibraryService::generate_library_guide(const fs::path& project_path, const ThirdPartyLibrary& lib) {
    const fs::path docs_dir = project_path / "docs";
    Utils::safe_create_directory(docs_dir);

    const fs::path guide_path = docs_dir / (lib.name + "_GUIDE.md");

    std::string content = "# " + lib.name + " Integration Guide\n\n";
    content += "## Installation\n";
    content += "The library has been downloaded to: `third_party/" + fs::path(lib.downloadUrl).stem().string() + "`\n\n";
    content += "## Configuration\n";
    content += "### CMake Configuration\n";
    content += "```cmake\n" + lib.configInstructions + "\n```\n\n";
    content += "### Include in Code\n";

    if (lib.name == "GLFW") {
        content += "```cpp\n#include <GLFW/glfw3.h>\n```\n\n";
    } else if (lib.name == "Boost") {
        content += "```cpp\n#include <boost/filesystem.hpp>\n```\n\n";
    } else if (lib.name == "SDL2") {
        content += "```cpp\n#include <SDL.h>\n```\n\n";
    } else {
        content += "```cpp\n#include <" + lib.name + ".h>\n```\n\n";
    }

    content += "## Official Documentation\n";
    if (lib.name == "GLFW") {
        content += "- [GLFW Documentation](https://www.glfw.org/docs/latest/)\n";
    } else if (lib.name == "Boost") {
        content += "- [Boost Documentation](https://www.boost.org/doc/libs/)\n";
    } else if (lib.name == "SDL2") {
        content += "- [SDL2 Documentation](https://wiki.libsdl.org/)\n";
    } else {
        content += "- Check the official website for " + lib.name + "\n";
    }

    Utils::safe_write_file(guide_path, content);
}

std::string LibraryService::get_unzip_command(const fs::path& zip_file, const fs::path& output_dir) {
#ifdef _WIN32
    return "powershell -command \"Expand-Archive -Path '" + zip_file.string() + "' -DestinationPath '" + output_dir.string() + "'\"";
#else
    return "unzip -o \"" + zip_file.string() + "\" -d \"" + output_dir.string() + "\"";
#endif
}

ILibraryInfoProvider& LibraryService::get_provider() {
    if (!provider_) {
        provider_ = std::make_unique<BuiltinLibraryProvider>();
    }
    return *provider_;
}