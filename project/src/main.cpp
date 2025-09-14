/*Created by Macintosh-MaiSensei on 2025/9/2.*/
/*Version 1.01 RC*/    
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <array>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <set>
#include <cstdlib>
#include <regex>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <optional>
#include <cctype>
#include <functional>
#ifdef _WIN32
    #define popen _popen
    #define pclose _pclose
    #include <io.h>
#else
    #include <cstdio>
#endif
namespace fs = std::filesystem;
// 编译器类型枚举
enum class CompilerType {
    GCC,
    GXX,
    CLANG,
    CLANGXX,
    MSVC,
    UNKNOWN
};

// 调试器类型枚举
enum class DebuggerType {
    GDB,
    LLDB,
    CPPVSDBG,
    UNKNOWN
};

// 目录节点类
class DirectoryNode {
public:
    std::string name;
    std::vector<DirectoryNode> children;
};

// 编译器配置结构
class CompilerConfig {
public:
    std::string name;
    std::string path;
    std::string cppStandard;
    std::string cStandard;
    CompilerType type = CompilerType::UNKNOWN;
    std::vector<std::string> extraArgs;
};

// 调试器配置结构
class DebuggerConfig {
public:
    std::string name;
    std::string path;
    DebuggerType type = DebuggerType::UNKNOWN;
};

// 项目配置结构
class ProjectConfig {
public:
    std::string name;
    std::string path;
    CompilerConfig compiler;
    DebuggerConfig debugger;
    std::vector<std::string> libraries;
};

// 第三方库信息结构
class ThirdPartyLibrary {
public:
    std::string name;
    std::string downloadUrl;
    std::string includePath;
    std::string libPath;
    std::vector<std::string> dependencies;
    std::string configInstructions;
    std::string sha256;
};

// 全局常量
namespace Constants {
    const std::string VERSION = "1.0.1";
    const std::string DEFAULT_PROJECT_NAME = "project";
    const std::string DEFAULT_LIBRARY_MIRROR = "https://raw.githubusercontent.com/Macintosh-MaiSensei/SLN2Code/main/libraries/";
    
    // 支持的编译器类型映射
    const std::unordered_map<std::string, CompilerType> COMPILER_TYPE_MAP = {
        {"gcc", CompilerType::GCC},
        {"g++", CompilerType::GXX},
        {"clang", CompilerType::CLANG},
        {"clang++", CompilerType::CLANGXX},
        {"cl", CompilerType::MSVC},
        {"msvc", CompilerType::MSVC}
    };
    
    // 支持的调试器类型映射
    const std::unordered_map<std::string, DebuggerType> DEBUGGER_TYPE_MAP = {
        {"gdb", DebuggerType::GDB},
        {"lldb", DebuggerType::LLDB},
        {"cppvsdbg", DebuggerType::CPPVSDBG}
    };
    
    // 内置支持的第三方库
    const std::unordered_map<std::string, ThirdPartyLibrary> BUILTIN_LIBRARIES = {
        {"glfw", {
            "GLFW",
            "https://github.com/glfw/glfw/releases/download/3.3.8/glfw-3.3.8.zip",
            "glfw-3.3.8/include",
            "glfw-3.3.8/lib",
            {},
            R"(在CMakeLists.txt中添加:
target_include_directories(${PROJECT_NAME} PRIVATE "third_party/glfw-3.3.8/include")
target_link_directories(${PROJECT_NAME} PRIVATE "third_party/glfw-3.3.8/lib")
target_link_libraries(${PROJECT_NAME} glfw3)
)",
            "4d025083cc4a3dd1f91ab9b9ba4f5807193823e565a5bcf4be202669d9911ea6"
        }},
        {"boost", {
            "Boost",
            "https://archives.boost.io/release/1.89.0/source/boost_1_89_0.zip",
            "boost_1_89_0",
            "",
            {},
            R"(在CMakeLists.txt中添加:
set(BOOST_ROOT "third_party/boost_1_89_0")
find_package(Boost REQUIRED COMPONENTS system filesystem)
target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES})
)",
            "77bee48e32cabab96a3fd2589ec3ab9a17798d330220fdd8bde6ff5611b4ccde"
        }},
        {"sdl2", {
            "SDL2",
            "https://github.com/libsdl-org/SDL/releases/download/release-2.28.5/SDL2-devel-2.28.5-VC.zip",
            "SDL2-2.28.5/include",
            "SDL2-2.28.5/lib/x64",
            {},
            R"(在CMakeLists.txt中添加:
target_include_directories(${PROJECT_NAME} PRIVATE "third_party/SDL2-2.28.5/include")
target_link_directories(${PROJECT_NAME} PRIVATE "third_party/SDL2-2.28.5/lib/x64")
target_link_libraries(${PROJECT_NAME} SDL2 SDL2main)
)",
            "4ac4ba2208410b7b984759ee12e13e0606bd62032b5ddc36fb7d96b9ade78871"
        }}
    };
}

// 实用工具类
class Utils {
public:
    // 清理路径输入
    static std::string clean_path(const std::string& input) {
        std::string cleaned = input;
        
        // 替换反斜杠为正斜杠
        std::replace(cleaned.begin(), cleaned.end(), '\\', '/');
        
        // 移除末尾的斜杠
        if (!cleaned.empty() && cleaned.back() == '/' && cleaned.size() > 1) {
            cleaned.pop_back();
        }
        
        return cleaned;
    }
    
    // 获取有效项目名称
    static std::string get_valid_name(const std::string& input) {
        std::string name = input;
        
        // 移除非字母数字字符（允许连字符和下划线）
        name.erase(std::remove_if(name.begin(), name.end(),
                                  [](char c) { return !(std::isalnum(c) || c == '-' || c == '_' || c == ' '); }),
                   name.end());
        
        // 替换空格为连字符
        std::replace(name.begin(), name.end(), ' ', '-');
        
        // 转换为小写
        std::transform(name.begin(), name.end(), name.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        
        return name;
    }
    
    // 检查编译器是否支持指定标准
    static bool is_standard_supported(const CompilerConfig& compiler, const std::string& standard) {
        // 简化实现：实际项目中应调用编译器获取支持的标准列表
        return true;
    }
    
    // 安全执行系统命令
    static int execute_command(const std::string& command) {
        std::cout << "Executing: " << command << std::endl;
        return std::system(command.c_str());
    }
    
    // 执行命令并获取输出
    static std::string execute_command_with_output(const std::string& command) {
#ifdef _WIN32
        std::string cmd = "cmd /c \"" + command + "\"";
#else
        std::string cmd = command;
#endif
        
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
        
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }
        
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        
        return result;
    }
    
    // 跨平台路径拼接
    static fs::path join_paths(const fs::path& base, const std::string& part) {
        return base / part;
    }
    
    // 安全创建目录
    static bool safe_create_directory(const fs::path& path) {
        try {
            if (!fs::exists(path)) {
                return fs::create_directories(path);
            }
            return true;
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error creating directory: " << e.what() << std::endl;
            return false;
        }
    }
    
    // 安全写入文件
    static bool safe_write_file(const fs::path& path, const std::string& content) {
        try {
            std::ofstream file(path);
            if (!file) return false;
            file << content;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error writing file: " << e.what() << std::endl;
            return false;
        }
    }
    
    // 安全读取文件
    static std::string safe_read_file(const fs::path& path) {
        try {
            std::ifstream file(path);
            if (!file) return "";
            std::string content((std::istreambuf_iterator<char>(file)), 
                                std::istreambuf_iterator<char>());
            return content;
        } catch (const std::exception& e) {
            std::cerr << "Error reading file: " << e.what() << std::endl;
            return "";
        }
    }
    
    // 下载文件
    static bool download_file(const std::string& url, const fs::path& output_path) {
        std::string command;
#ifdef _WIN32
        command = "curl -L -o \"" + output_path.string() + "\" \"" + url + "\"";
#else
        command = "curl -L -o \"" + output_path.string() + "\" \"" + url + "\"";
#endif
        
        int result = execute_command(command);
        return result == 0;
    }
    
    // 简单字符串修剪
    static std::string trim(const std::string& str) {
        auto start = std::find_if_not(str.begin(), str.end(), [](int c) {
            return std::isspace(c);
        });
        auto end = std::find_if_not(str.rbegin(), str.rend(), [](int c) {
            return std::isspace(c);
        }).base();
        return (start < end) ? std::string(start, end) : std::string();
    }
    
    // 简单JSON解析（仅用于库信息）
    static std::unordered_map<std::string, std::string> parse_simple_json(const std::string& json) {
        std::unordered_map<std::string, std::string> result;
        std::istringstream stream(json);
        std::string line;
        
        while (std::getline(stream, line)) {
            line = trim(line);
            if (line.empty() || line == "{" || line == "}") continue;
            
            // 移除逗号和引号
            line.erase(std::remove(line.begin(), line.end(), '"'), line.end());
            line.erase(std::remove(line.begin(), line.end(), ','), line.end());
            
            // 分割键值对
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string key = trim(line.substr(0, colon_pos));
                std::string value = trim(line.substr(colon_pos + 1));
                result[key] = value;
            }
        }
        
        return result;
    }
};

// 库信息提供者接口
class ILibraryInfoProvider {
public:
    virtual ~ILibraryInfoProvider() = default;
    
    // 获取所有可用库的名称列表
    virtual std::vector<std::string> get_available_libraries() = 0;
    
    // 获取指定库的详细信息
    virtual std::optional<ThirdPartyLibrary> get_library_info(const std::string& lib_name) = 0;
    
    // 刷新库信息（从远程源更新）
    virtual bool refresh_library_info() = 0;
    
    // 获取提供者名称
    virtual std::string get_provider_name() const = 0;
};

// 内置库信息提供者
class BuiltinLibraryProvider : public ILibraryInfoProvider {
public:
    std::vector<std::string> get_available_libraries() override {
        std::vector<std::string> libs;
        for (const auto& pair : Constants::BUILTIN_LIBRARIES) {
            libs.push_back(pair.first);
        }
        return libs;
    }
    
    std::optional<ThirdPartyLibrary> get_library_info(const std::string& lib_name) override {
        auto it = Constants::BUILTIN_LIBRARIES.find(lib_name);
        if (it != Constants::BUILTIN_LIBRARIES.end()) {
            return it->second;
        }
        return std::nullopt;
    }
    
    bool refresh_library_info() override {
        // 内置库不需要刷新
        return true;
    }
    
    std::string get_provider_name() const override {
        return "Built-in Library Provider";
    }
};

// 远程库信息提供者
class RemoteLibraryProvider : public ILibraryInfoProvider {
public:
    RemoteLibraryProvider(const std::string& mirror_url) 
        : mirror_url_(ensure_trailing_slash(mirror_url)) {}
    
    std::vector<std::string> get_available_libraries() override {
        if (available_libraries_.empty()) {
            refresh_library_info();
        }
        return available_libraries_;
    }
    
    std::optional<ThirdPartyLibrary> get_library_info(const std::string& lib_name) override {
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
        ThirdPartyLibrary lib_info;
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
    
    bool refresh_library_info() override {
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
    
    std::string get_provider_name() const override {
        return "Remote Library Provider (" + mirror_url_ + ")";
    }

private:
    std::string mirror_url_;
    std::vector<std::string> available_libraries_;
    std::unordered_map<std::string, ThirdPartyLibrary> library_cache_;
    
    static std::string ensure_trailing_slash(const std::string& url) {
        if (url.empty() || url.back() != '/') {
            return url + '/';
        }
        return url;
    }
};

// 编译器服务类
class CompilerService {
public:
    // 获取编译器配置
    static CompilerConfig get_compiler_config() {
        CompilerConfig config;
        
        std::cout << "\n=== Compiler Configuration ===\n";
        
        // 选择编译器类型
        std::cout << "Select compiler type:\n";
        std::cout << "1. gcc\n";
        std::cout << "2. g++\n";
        std::cout << "3. clang\n";
        std::cout << "4. clang++\n";
        std::cout << "5. MSVC (Visual Studio)\n";
        std::cout << "Enter choice (1-5, default 1): ";
        
        std::string choice;
        std::getline(std::cin, choice);
        
        if (choice == "2") {
            config.type = CompilerType::GXX;
            config.name = "g++";
        } else if (choice == "3") {
            config.type = CompilerType::CLANG;
            config.name = "clang";
        } else if (choice == "4") {
            config.type = CompilerType::CLANGXX;
            config.name = "clang++";
        } else if (choice == "5") {
            config.type = CompilerType::MSVC;
            config.name = "cl";
        } else {
            config.type = CompilerType::GXX;
            config.name = "g++";
        }
        
        // 获取编译器路径
        config.path = find_compiler_path(config);
        
        // 选择C++标准
        config.cppStandard = select_cpp_standard(config);
        
        // 选择C标准
        config.cStandard = select_c_standard(config);
        
        // 添加额外编译选项
        std::cout << "Enter any additional compiler flags (space separated, or press Enter for none): ";
        std::string extra_flags;
        std::getline(std::cin, extra_flags);
        
        if (!extra_flags.empty()) {
            std::istringstream iss(extra_flags);
            std::string flag;
            while (iss >> flag) {
                config.extraArgs.push_back(flag);
            }
        }
        
        return config;
    }

private:
    // 查找编译器路径
    static std::string find_compiler_path(CompilerConfig& config) {
        std::vector<std::string> compiler_paths;
        
        if (config.type == CompilerType::MSVC) {
            compiler_paths = find_msvc_paths();
        } else {
            compiler_paths = find_compiler_in_path(config.name);
        }
        
        // 处理找到的编译器路径
        if (compiler_paths.empty()) {
            std::cout << "No " << config.name << " compiler found.\n";
            std::cout << "Enter " << config.name << " compiler path: ";
            std::string path;
            std::getline(std::cin, path);
            return Utils::clean_path(path);
        }
        
        if (compiler_paths.size() == 1) {
            std::cout << "Using " << config.name << " compiler found at: " << compiler_paths[0] << "\n";
            return compiler_paths[0];
        }
        
        std::cout << "Multiple " << config.name << " compilers found:\n";
        for (size_t i = 0; i < compiler_paths.size(); ++i) {
            std::cout << "  " << i + 1 << ". " << compiler_paths[i] << "\n";
        }
        std::cout << "Select compiler (1-" << compiler_paths.size() << "): ";
        std::string selection;
        std::getline(std::cin, selection);
        
        try {
            size_t idx = std::stoi(selection) - 1;
            if (idx < compiler_paths.size()) {
                return compiler_paths[idx];
            }
        } catch (...) {
            // 无效选择，使用第一个
        }
        
        return compiler_paths[0];
    }
    
    // 在PATH环境变量中查找编译器
    static std::vector<std::string> find_compiler_in_path(const std::string& executable_name) {
        std::vector<std::string> found_paths;
        
        // 获取PATH环境变量
        const char* path_env = std::getenv("PATH");
        if (!path_env) {
            return found_paths;
        }
        
        std::string path_str(path_env);
        
        // 分割PATH字符串
#ifdef _WIN32
        const char delimiter = ';';
        std::string exe_name = executable_name + ".exe";
#else
        const char delimiter = ':';
        std::string exe_name = executable_name;
#endif
        
        std::istringstream path_stream(path_str);
        std::string directory;
        
        while (std::getline(path_stream, directory, delimiter)) {
            fs::path dir_path(directory);
            if (fs::exists(dir_path) && fs::is_directory(dir_path)) {
                fs::path executable_path = dir_path / exe_name;
                if (fs::exists(executable_path) && fs::is_regular_file(executable_path)) {
                    found_paths.push_back(Utils::clean_path(executable_path.string()));
                }
            }
        }
        
        return found_paths;
    }
    
    // 查找MSVC路径
    static std::vector<std::string> find_msvc_paths() {
        std::vector<std::string> paths;
        
#ifdef _WIN32
        // 常见的Visual Studio安装路径
        std::vector<std::string> common_paths = {
            "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC",
            "C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Tools/MSVC",
            "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/MSVC",
            "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC",
            "C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional/VC/Tools/MSVC",
            "C:/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise/VC/Tools/MSVC"
        };
        
        for (const auto& base_path : common_paths) {
            if (fs::exists(base_path)) {
                for (const auto& entry : fs::directory_iterator(base_path)) {
                    if (entry.is_directory()) {
                        fs::path compiler_path = entry.path() / "bin/Hostx64/x64/cl.exe";
                        if (fs::exists(compiler_path)) {
                            paths.push_back(Utils::clean_path(compiler_path.string()));
                        }
                    }
                }
            }
        }
#endif
        
        return paths;
    }
    
    // 选择C++标准
    static std::string select_cpp_standard(const CompilerConfig& compiler) {
        std::cout << "Select C++ standard:\n";
        std::cout << "1. C++98\n";
        std::cout << "2. C++11\n";
        std::cout << "3. C++14\n";
        std::cout << "4. C++17\n";
        std::cout << "5. C++20\n";
        std::cout << "6. C++23\n";
        std::cout << "Enter choice (1-6, default 4): ";
        
        std::string choice;
        std::getline(std::cin, choice);
        
        if (choice == "1" && Utils::is_standard_supported(compiler, "c++98")) {
            return "c++98";
        } else if (choice == "2" && Utils::is_standard_supported(compiler, "c++11")) {
            return "c++11";
        } else if (choice == "3" && Utils::is_standard_supported(compiler, "c++14")) {
            return "c++14";
        } else if (choice == "5" && Utils::is_standard_supported(compiler, "c++20")) {
            return "c++20";
        } else if (choice == "6" && Utils::is_standard_supported(compiler, "c++23")) {
            return "c++23";
        }
        return "c++17"; // 默认
    }
    
    // 选择C标准
    static std::string select_c_standard(const CompilerConfig& compiler) {
        std::cout << "Select C standard:\n";
        std::cout << "1. C99\n";
        std::cout << "2. C11\n";
        std::cout << "3. C17\n";
        std::cout << "4. C20\n";
        std::cout << "5. C23\n";
        std::cout << "Enter choice (1-5, default 3): ";
        
        std::string choice;
        std::getline(std::cin, choice);
        
        if (choice == "1" && Utils::is_standard_supported(compiler, "c99")) {
            return "c99";
        } else if (choice == "2" && Utils::is_standard_supported(compiler, "c11")) {
            return "c11";
        } else if (choice == "4" && Utils::is_standard_supported(compiler, "c20")) {
            return "c20";
        } else if (choice == "5" && Utils::is_standard_supported(compiler, "c23")) {
            return "c23";
        }
        return "c17"; // 默认
    }
};

// 调试器服务类
class DebuggerService {
public:
    // 获取调试器配置
    static DebuggerConfig get_debugger_config() {
        DebuggerConfig config;
        
        std::cout << "\n=== Debugger Configuration ===\n";
        
        // 选择调试器类型
        std::cout << "Select debugger type:\n";
        std::cout << "1. GDB\n";
        std::cout << "2. LLDB\n";
        std::cout << "3. Visual Studio Debugger\n";
        std::cout << "Enter choice (1-3, default 1): ";
        
        std::string choice;
        std::getline(std::cin, choice);
        
        if (choice == "2") {
            config.type = DebuggerType::LLDB;
            config.name = "lldb";
        } else if (choice == "3") {
            config.type = DebuggerType::CPPVSDBG;
            config.name = "Visual Studio Debugger";
        } else {
            config.type = DebuggerType::GDB;
            config.name = "gdb";
        }
        
        // 获取调试器路径
        config.path = find_debugger_path(config);
        
        return config;
    }

private:
    // 查找调试器路径
    static std::string find_debugger_path(DebuggerConfig& config) {
        std::vector<std::string> debugger_paths;
        
        if (config.type == DebuggerType::CPPVSDBG) {
            debugger_paths = find_vs_debugger_paths();
        } else {
            debugger_paths = find_debugger_in_path(config.name);
        }
        
        // 处理找到的调试器路径
        if (debugger_paths.empty()) {
            std::cout << "No " << config.name << " debugger found.\n";
            std::cout << "Enter " << config.name << " debugger path: ";
            std::string path;
            std::getline(std::cin, path);
            return Utils::clean_path(path);
        }
        
        if (debugger_paths.size() == 1) {
            std::cout << "Using " << config.name << " debugger found at: " << debugger_paths[0] << "\n";
            return debugger_paths[0];
        }
        
        std::cout << "Multiple " << config.name << " debuggers found:\n";
        for (size_t i = 0; i < debugger_paths.size(); ++i) {
            std::cout << "  " << i + 1 << ". " << debugger_paths[i] << "\n";
        }
        std::cout << "Select debugger (1-" << debugger_paths.size() << "): ";
        std::string selection;
        std::getline(std::cin, selection);
        
        try {
            size_t idx = std::stoi(selection) - 1;
            if (idx < debugger_paths.size()) {
                return debugger_paths[idx];
            }
        } catch (...) {
            // 无效选择，使用第一个
        }
        
        return debugger_paths[0];
    }
    
    // 在PATH环境变量中查找调试器
    static std::vector<std::string> find_debugger_in_path(const std::string& executable_name) {
        std::vector<std::string> found_paths;
        
        // 获取PATH环境变量
        const char* path_env = std::getenv("PATH");
        if (!path_env) {
            return found_paths;
        }
        
        std::string path_str(path_env);
        
        // 分割PATH字符串
#ifdef _WIN32
        const char delimiter = ';';
        std::string exe_name = executable_name + ".exe";
#else
        const char delimiter = ':';
        std::string exe_name = executable_name;
#endif
        
        std::istringstream path_stream(path_str);
        std::string directory;
        
        while (std::getline(path_stream, directory, delimiter)) {
            fs::path dir_path(directory);
            if (fs::exists(dir_path) && fs::is_directory(dir_path)) {
                fs::path executable_path = dir_path / exe_name;
                if (fs::exists(executable_path) && fs::is_regular_file(executable_path)) {
                    found_paths.push_back(Utils::clean_path(executable_path.string()));
                }
            }
        }
        
        return found_paths;
    }
    
    // 查找VS调试器路径
    static std::vector<std::string> find_vs_debugger_paths() {
        std::vector<std::string> paths;
        
#ifdef _WIN32
        // 尝试查找VS调试器
        std::vector<std::string> common_paths = {
            "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE",
            "C:/Program Files/Microsoft Visual Studio/2022/Professional/Common7/IDE",
            "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/Common7/IDE",
            "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE",
            "C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional/Common7/IDE",
            "C:/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise/Common7/IDE"
        };
        
        for (const auto& path : common_paths) {
            fs::path debugger_path = fs::path(path) / "Debugger/vsdbg.exe";
            if (fs::exists(debugger_path)) {
                paths.push_back(Utils::clean_path(debugger_path.string()));
            }
        }
#endif
        
        return paths;
    }
};

// 项目结构服务
class ProjectStructureService {
public:
    // 生成项目结构定义
    static DirectoryNode get_project_structure(const std::string& project_name) {
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
    
    // 递归创建目录结构
    static bool create_directory_recursive(const fs::path& base_path, const DirectoryNode& node, int depth = 0) {
        const fs::path current_path = Utils::join_paths(base_path, node.name);
        
        // 缩进显示层级
        const std::string indent(depth * 2, ' ');
        std::cout << indent << node.name;
        
        try {
            if (!fs::exists(current_path)) {
                if (Utils::safe_create_directory(current_path)) {
                    std::cout << "  ......done" << std::endl;
                } else {
                    std::cout << "  ......FAILED" << std::endl;
                    return false;
                }
            } else {
                std::cout << "  ......already exists" << std::endl;
            }
            
            // 递归创建子目录
            for (const auto& child : node.children) {
                if (!create_directory_recursive(current_path, child, depth + 1)) {
                    return false;
                }
            }
            return true;
        } catch (const fs::filesystem_error& e) {
            std::cerr << indent << "Error: " << e.what() << std::endl;
            return false;
        }
    }
};

// SHA256计算类
class SHA256 {
private:
    static constexpr std::array<uint32_t, 64> K = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    std::array<uint32_t, 8> state = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    static uint32_t rotr(uint32_t x, uint32_t n) {
        return (x >> n) | (x << (32 - n));
    }

    static uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) ^ (~x & z);
    }

    static uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) ^ (x & z) ^ (y & z);
    }

    static uint32_t sigma0(uint32_t x) {
        return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
    }

    static uint32_t sigma1(uint32_t x) {
        return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
    }

    static uint32_t gamma0(uint32_t x) {
        return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
    }

    static uint32_t gamma1(uint32_t x) {
        return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
    }

    void transform(const uint8_t* data) {
        std::array<uint32_t, 64> W = {0};

        // 将数据分成16个32位字
        for (int i = 0; i < 16; i++) {
            W[i] = (static_cast<uint32_t>(data[i*4]) << 24) |
                   (static_cast<uint32_t>(data[i*4+1]) << 16) |
                   (static_cast<uint32_t>(data[i*4+2]) << 8) |
                   (static_cast<uint32_t>(data[i*4+3]));
        }

        // 扩展消息
        for (int i = 16; i < 64; i++) {
            W[i] = gamma1(W[i-2]) + W[i-7] + gamma0(W[i-15]) + W[i-16];
        }

        auto a = state[0];
        auto b = state[1];
        auto c = state[2];
        auto d = state[3];
        auto e = state[4];
        auto f = state[5];
        auto g = state[6];
        auto h = state[7];

        // 主循环
        for (int i = 0; i < 64; i++) {
            uint32_t T1 = h + sigma1(e) + ch(e, f, g) + K[i] + W[i];
            uint32_t T2 = sigma0(a) + maj(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + T1;
            d = c;
            c = b;
            b = a;
            a = T1 + T2;
        }

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }

public:
    static std::string hash_file(const fs::path& file_path) {
        std::ifstream file(file_path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Can't open the file: " + file_path.string());
        }

        SHA256 sha;
        std::array<uint8_t, 64> buffer = {0};
        uint64_t total_bytes = 0;

        while (file) {
            file.read(reinterpret_cast<char*>(buffer.data()), 64);
            size_t bytes_read = file.gcount();

            if (bytes_read == 0) break;

            total_bytes += bytes_read;

            // 如果是最后一块
            if (bytes_read < 64) {
                // 填充
                buffer[bytes_read++] = 0x80;
                while (bytes_read < 56) {
                    buffer[bytes_read++] = 0x00;
                }

                // 添加长度（以位为单位）
                uint64_t bit_length = total_bytes * 8;
                buffer[63] = static_cast<uint8_t>(bit_length);
                buffer[62] = static_cast<uint8_t>(bit_length >> 8);
                buffer[61] = static_cast<uint8_t>(bit_length >> 16);
                buffer[60] = static_cast<uint8_t>(bit_length >> 24);
                buffer[59] = static_cast<uint8_t>(bit_length >> 32);
                buffer[58] = static_cast<uint8_t>(bit_length >> 40);
                buffer[57] = static_cast<uint8_t>(bit_length >> 48);
                buffer[56] = static_cast<uint8_t>(bit_length >> 56);

                sha.transform(buffer.data());
                break;
            }

            sha.transform(buffer.data());
        }

        // 转换为十六进制字符串
        std::ostringstream result;
        result << std::hex << std::setfill('0');
        for (uint32_t val : sha.state) {
            result << std::setw(8) << val;
        }

        return result.str();
    }
};

// 第三方库服务
class LibraryService {
public:
    // 设置库信息提供者（默认为内置）
    static void set_provider(std::unique_ptr<ILibraryInfoProvider> provider) {
        provider_ = std::move(provider);
    }
    
    // 获取当前提供者
    static ILibraryInfoProvider& get_provider() {
        if (!provider_) {
            provider_ = std::make_unique<BuiltinLibraryProvider>();
        }
        return *provider_;
    }
    
    // 创建第三方库目录
    static void create_third_party_dir(const fs::path& project_path) {
        const fs::path third_party_dir = project_path / "third_party";
        Utils::safe_create_directory(third_party_dir);
    }

    // 下载并解压库文件
    static bool download_and_extract_library(const fs::path& project_path, const ThirdPartyLibrary& lib) {
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

        // 删除压缩包
        fs::remove(zip_file);

        return true;
    }

    // 生成库使用指南
    static void generate_library_guide(const fs::path& project_path, const ThirdPartyLibrary& lib) {
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

    // 添加第三方库到项目
    static void add_third_party_library(const fs::path& project_path, const std::string& lib_name) {
        static std::set<std::string> installed_libs;
        if (installed_libs.find(lib_name) != installed_libs.end()) {
            std::cout << lib_name << " already installed. Skipping.\n";
            return;
        }
        installed_libs.insert(lib_name);
        
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

    // 在生成项目文件后添加库安装选项
    static void offer_library_installation(const fs::path& project_path) {
        std::cout << "\nWould you like to add any third-party libraries? [y/N]: ";
        std::string response;
        std::getline(std::cin, response);

        if (response.empty() || (response[0] != 'y' && response[0] != 'Y')) {
            return;
        }

        auto& provider = get_provider();
        auto available_libs = provider.get_available_libraries();
        
        if (available_libs.empty()) {
            std::cout << "No libraries available from the current provider.\n";
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

private:
    static std::unique_ptr<ILibraryInfoProvider> provider_;
    
    // 获取解压命令
    static std::string get_unzip_command(const fs::path& zip_file, const fs::path& output_dir) {
#ifdef _WIN32
        return "powershell -command \"Expand-Archive -Path '" + zip_file.string() + "' -DestinationPath '" + output_dir.string() + "'\"";
#else
        return "unzip -o \"" + zip_file.string() + "\" -d \"" + output_dir.string() + "\"";
#endif
    }
};

// 初始化静态成员
std::unique_ptr<ILibraryInfoProvider> LibraryService::provider_ = nullptr;

// 项目生成服务
class ProjectGenerator {
public:
    // 生成项目文件
    static void generate_project_files(const fs::path& project_path, const std::string& project_name,
                                      const CompilerConfig& compiler, const DebuggerConfig& debugger) {
        const fs::path vscode_dir = project_path / ".vscode";
        Utils::safe_create_directory(vscode_dir);

        // 生成.vscode配置文件
        generate_c_cpp_properties(vscode_dir, compiler);
        generate_tasks_json(vscode_dir, project_name, compiler);
        generate_launch_json(vscode_dir, project_name, compiler, debugger);
        generate_settings_json(vscode_dir);

        // 创建CMakeLists.txt（备用）
        generate_cmake_file(project_path, project_name, compiler);

        // 创建main.cpp
        generate_main_cpp(project_path, project_name);

        // 创建基本.gitignore
        generate_gitignore(project_path);
    }

private:
    // 生成c_cpp_properties.json
    static void generate_c_cpp_properties(const fs::path& vscode_dir, const CompilerConfig& compiler) {
        std::string intelliSenseMode = get_intellisense_mode(compiler.type);
        std::string compilerPath = Utils::clean_path(compiler.path);
        
        std::string content = R"({
    "configurations": [
        {
            "name": "Win32",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/include"
            ],
            "defines": [
                "_DEBUG",
                "UNICODE",
                "_UNICODE"
            ],
            "compilerPath": ")" + compilerPath + R"(",
            "cStandard": ")" + compiler.cStandard + R"(",
            "cppStandard": ")" + compiler.cppStandard + R"(",
            "intelliSenseMode": ")" + intelliSenseMode + R"("
        },
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/include"
            ],
            "defines": [],
            "compilerPath": ")" + compilerPath + R"(",
            "cStandard": ")" + compiler.cStandard + R"(",
            "cppStandard": ")" + compiler.cppStandard + R"(",
            "intelliSenseMode": ")" + intelliSenseMode + R"("
        }
    ],
    "version": 4
})";
        
        Utils::safe_write_file(vscode_dir / "c_cpp_properties.json", content);
    }
    
static std::string get_intellisense_mode(CompilerType type) {
    // 确定平台和架构
    std::string platform;
    std::string architecture;
    
    #if defined(_WIN32) || defined(_WIN64)
        platform = "windows";
        #if defined(_M_AMD64) || defined(__x86_64__)
            architecture = "x64";
        #elif defined(_M_IX86)
            architecture = "x86";
        #elif defined(_M_ARM)
            architecture = "arm";
        #elif defined(_M_ARM64) || defined(__aarch64__)
            architecture = "arm64";
        #else
            architecture = "x64"; // 默认
        #endif
    #elif defined(__APPLE__)
        platform = "macos";
        #if defined(__x86_64__)
            architecture = "x64";
        #elif defined(__aarch64__) || defined(__arm64__)
            architecture = "arm64";
        #else
            architecture = "arm64"; // Apple Silicon默认
        #endif
    #elif defined(__linux__)
        platform = "linux";
        #if defined(__x86_64__)
            architecture = "x64";
        #elif defined(__i386__)
            architecture = "x86";
        #elif defined(__aarch64__)
            architecture = "arm64";
        #elif defined(__arm__)
            architecture = "arm";
        #else
            architecture = "x64"; // 默认
        #endif
    #else
        platform = "linux";
        architecture = "x64"; // 其他平台默认
    #endif

    // 根据编译器类型和架构组合模式字符串
    switch (type) {
        case CompilerType::CLANG:
        case CompilerType::CLANGXX:
            return platform + "-clang-" + architecture;
        
        case CompilerType::MSVC:
            return platform + "-msvc-" + architecture;
        
        case CompilerType::GCC:
        case CompilerType::GXX:
        default:
            return platform + "-gcc-" + architecture;
    }
}
    
    // 生成tasks.json
    static void generate_tasks_json(const fs::path& vscode_dir, const std::string& project_name, 
                                   const CompilerConfig& compiler) {
        std::string output_ext = (compiler.type == CompilerType::MSVC) ? ".exe" : "";
        std::string compilerPath = Utils::clean_path(compiler.path);
        
        std::string args;
        if (compiler.type == CompilerType::MSVC) {
            args = R"(                "/std:)" + compiler.cppStandard.substr(2) + R"(",
                "/I${workspaceFolder}/include",)";
        } else {
            args = R"(                "-std=)" + compiler.cppStandard + R"(",
                "-I${workspaceFolder}/include",)";
        }
        
        // 添加额外编译选项
        for (const auto& arg : compiler.extraArgs) {
            args += "\n                \"" + arg + "\",";
        }
        
        std::string problemMatcher = (compiler.type == CompilerType::MSVC) ? 
            R"("$msCompile")" : R"("$gcc")";
        
        std::string content = R"({
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build",
            "type": "shell",
            "command": ")" + compilerPath + R"(",
            "args": [
)" + args + R"(
                "${workspaceFolder}/src/*.cpp",
                "-o",
                "${workspaceFolder}/build/bin/Debug/)" + project_name + output_ext + R"("
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": [)" + problemMatcher + R"(],
            "detail": "Built with )" + compiler.name + R"("
        },
        {
            "label": "Clean",
            "type": "shell",
            "command": "rm",
            "args": [
                "-rf",
                "${workspaceFolder}/build/bin/Debug/*"
            ]
        }
    ]
})";
        
        Utils::safe_write_file(vscode_dir / "tasks.json", content);
    }
    
    // 生成launch.json
    static void generate_launch_json(const fs::path& vscode_dir, const std::string& project_name, 
                                    const CompilerConfig& compiler, const DebuggerConfig& debugger) {
        std::string output_ext = (compiler.type == CompilerType::MSVC) ? ".exe" : "";
        std::string debuggerPath = Utils::clean_path(debugger.path);
        std::string debuggerType;
        
        switch (debugger.type) {
            case DebuggerType::GDB: debuggerType = "gdb"; break;
            case DebuggerType::LLDB: debuggerType = "lldb"; break;
            case DebuggerType::CPPVSDBG: debuggerType = "cppvsdbg"; break;
            default: debuggerType = "cppdbg";
        }
        
        std::string content = R"({
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug Launch",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/bin/Debug/)" + project_name + output_ext + R"(",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": true,
            "MIMode": ")" + debuggerType + R"(",
            "miDebuggerPath": ")" + debuggerPath + R"(",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ],
            "preLaunchTask": "Build"
        }
    ]
})";
        
        Utils::safe_write_file(vscode_dir / "launch.json", content);
    }
    
    // 生成settings.json
    static void generate_settings_json(const fs::path& vscode_dir) {
        std::string content = R"({
    "files.associations": {
        "*.h": "c",
        "*.hpp": "cpp",
        "*.ipp": "cpp"
    },
    "editor.formatOnSave": true,
    "C_Cpp.default.configurationProvider": "ms-vscode.cpptools",
    "explorer.confirmDragAndDrop": false
})";
        
        Utils::safe_write_file(vscode_dir / "settings.json", content);
    }
    
    // 生成CMakeLists.txt
    static void generate_cmake_file(const fs::path& project_path, const std::string& project_name,
                                   const CompilerConfig& compiler) {
        std::string content = "cmake_minimum_required(VERSION 3.20)\n"
              "project(" + project_name + " VERSION 1.0 LANGUAGES CXX)\n\n"
              "set(CMAKE_CXX_STANDARD " + compiler.cppStandard.substr(2) + ")\n"
              "set(CMAKE_C_STANDARD " + compiler.cStandard.substr(1) + ")\n\n"
              "include(FetchContent)\n"
              "include_directories(include)\n"
              "add_executable(" + project_name + " src/main.cpp)\n"
              "target_include_directories(" + project_name + " PUBLIC include)\n"
              "install(TARGETS " + project_name + " DESTINATION bin)\n"
              "install(DIRECTORY include/ DESTINATION include)\n";

        // 添加编译器特定选项
        if (!compiler.extraArgs.empty()) {
            content += "\n# Additional compiler flags\n";
            content += "target_compile_options(" + project_name + " PRIVATE";
            for (const auto& arg : compiler.extraArgs) {
                content += " " + arg;
            }
            content += ")\n";
        }
        
        Utils::safe_write_file(project_path / "CMakeLists.txt", content);
    }
    
    // 生成main.cpp
    static void generate_main_cpp(const fs::path& project_path, const std::string& project_name) {
        const fs::path src_dir = project_path / "src";
        Utils::safe_create_directory(src_dir);
        
        std::string content = R"(#include <iostream>

int main() {
    std::cout << "Hello, )" + project_name + R"(!\n";
    std::cout << "Project created successfully!\n";
    return 0;
}
)";
        
        Utils::safe_write_file(src_dir / "main.cpp", content);
    }
    
    // 生成.gitignore
    static void generate_gitignore(const fs::path& project_path) {
        std::string content = R"(# Build artifacts
build/
*.exe
*.out
*.o
*.obj

# Editor files
.vscode/
!.vscode/tasks.json
!.vscode/launch.json
!.vscode/c_cpp_properties.json
!.vscode/settings.json

# System files
.DS_Store
Thumbs.db
)";
        
        Utils::safe_write_file(project_path / ".gitignore", content);
    }
};
void Logo(){
    std::cout <<"  ____  _     _   _ ____   ____          _      "<<"\n"
              <<" / ___|| |   | \\ | |___ \\ / ___|___   __| | ___ "<<"\n"
              <<" \\___ \\| |   |  \\| | __) | |   / _ \\ / _` |/ _ \\ "<<"\n"
              <<"  ___) | |___| |\\  |/ __/| |__| (_) | (_| |  __/"<<"\n"
              <<" |____/|_____|_| \\_|_____|\\____\\___/ \\__,_|\\___|"<<"\n";
}
// 命令行解析器
class CommandLineParser {
public:
    struct Options {
        std::string project_name = Constants::DEFAULT_PROJECT_NAME;
        std::string base_path = fs::current_path().string();
        std::vector<std::string> libraries_to_install;
        std::string library_mirror_url;
        CompilerConfig compiler;
        DebuggerConfig debugger;
        bool show_version = false;
        bool show_help = false;
    };

    static Options parse(int argc, char* argv[]) {
        Options options;
        
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "--name" || arg == "-n") {
                if (i + 1 < argc) {
                    options.project_name = Utils::get_valid_name(argv[++i]);
                } else {
                    throw std::runtime_error("Missing project name after " + arg);
                }
            } else if (arg == "--path" || arg == "-p") {
                if (i + 1 < argc) {
                    options.base_path = Utils::clean_path(argv[++i]);
                } else {
                    throw std::runtime_error("Missing path after " + arg);
                }
            } else if (arg == "-v" || arg == "--version") {
                options.show_version = true;
            } else if (arg == "-I" || arg == "--install") {
                if (i + 1 < argc) {
                    options.libraries_to_install.push_back(argv[++i]);
                } else {
                    throw std::runtime_error("Missing library name after " + arg);
                }
            } else if (arg == "--library-mirror" || arg == "-lm") {
                if (i + 1 < argc) {
                    options.library_mirror_url = argv[++i];
                } else {
                    throw std::runtime_error("Missing mirror URL after " + arg);
                }
            } else if (arg == "-c" || arg == "--compiler") {
                if (i + 1 < argc) {
                    std::string compiler_name = argv[++i];
                    auto it = Constants::COMPILER_TYPE_MAP.find(compiler_name);
                    if (it != Constants::COMPILER_TYPE_MAP.end()) {
                        options.compiler.type = it->second;
                        options.compiler.name = compiler_name;
                    }
                } else {
                    throw std::runtime_error("Missing compiler name after " + arg);
                }
            } else if (arg == "-cp" || arg == "--compiler-path") {
                if (i + 1 < argc) {
                    options.compiler.path = Utils::clean_path(argv[++i]);
                } else {
                    throw std::runtime_error("Missing compiler path after " + arg);
                }
            } else if (arg == "-cppstd" || arg == "--cpp-standard") {
                if (i + 1 < argc) {
                    options.compiler.cppStandard = argv[++i];
                } else {
                    throw std::runtime_error("Missing C++ standard after " + arg);
                }
            } else if (arg == "-cstd" || arg == "--c-standard") {
                if (i + 1 < argc) {
                    options.compiler.cStandard = argv[++i];
                } else {
                    throw std::runtime_error("Missing C standard after " + arg);
                }
            } else if (arg == "-d" || arg == "--debugger") {
                if (i + 1 < argc) {
                    std::string debugger_name = argv[++i];
                    auto it = Constants::DEBUGGER_TYPE_MAP.find(debugger_name);
                    if (it != Constants::DEBUGGER_TYPE_MAP.end()) {
                        options.debugger.type = it->second;
                        options.debugger.name = debugger_name;
                    }
                } else {
                    throw std::runtime_error("Missing debugger name after " + arg);
                }
            } else if (arg == "-dp"|| arg == "--debugger-path") {
                if (i + 1 < argc) {
                    options.debugger.path = Utils::clean_path(argv[++i]);
                } else {
                    throw std::runtime_error("Missing debugger path after " + arg);
                }
            } else if (arg == "-ea" || arg == "--extra-args") {
                if (i + 1 < argc) {
                    std::string extra_args = argv[++i];
                    std::istringstream iss(extra_args);
                    std::string flag;
                    while (iss >> flag) {
                        options.compiler.extraArgs.push_back(flag);
                    }
                } else {
                    throw std::runtime_error("Missing extra arguments after " + arg);
                }
            } else if (arg == "--help" || arg == "-h") {
                options.show_help = true;
            } else {
                throw std::runtime_error("Unknown option: " + arg);
            }
        }
        
        return options;
    }
    
    static void print_help(const std::string& program_name) {
        std::cout << "Usage: " << program_name << " [options]\n"
                  << "Options:\n"
                  << "  -n, --name NAME           Set project name\n"
                  << "  -p, --path PATH           Set project path\n"
                  << "  -I, --install LIB         Install third-party library\n"
                  << "  -lm, --library-mirror URL Use custom library mirror\n"
                  << "  -c, --compiler COMPILER    Set compiler (gcc, clang, cl)\n"
                  << "  -cp, --compiler-path PATH   Set compiler path\n"
                  << "  -cppstd, --cpp-standard STD     Set C++ standard (c++98 c++11 c++14 c++17 c++20 c++23)\n"
                  << "  -cstd, --c-standard STD       Set C standard (c99 c11 c17 c23)\n"
                  << "  -d, --debugger DEBUGGER    Set debugger (gdb, lldb)\n"
                  << "  -dp, --debugger-path PATH   Set debugger path\n"
                  << "  -ea, --extra-args ARGS      Set additional compiler flags\n"
                  << "  -v, --version             Output the version of the program\n"
                  << "  -h, --help                Show this help message\n";
    }
    
    static void print_version() {
        Logo();
        std::cout << "SLN2Code\n"
                  << "Version: " << Constants::VERSION << "\n"
                  << "Maintainer Macintosh-Maisensei\n"
                  << "SLN2Code is libre and open-source software\n";
    }
};

int main(int argc, char* argv[]) {
    try {
        // 解析命令行参数
        CommandLineParser::Options options = CommandLineParser::parse(argc, argv);
        
        if (options.show_help) {
            CommandLineParser::print_help(argv[0]);
            return 0;
        }
        
        if (options.show_version) {
            CommandLineParser::print_version();
            return 0;
        }
        
        // 配置库信息提供者
        if (!options.library_mirror_url.empty()) {
            LibraryService::set_provider(
                std::make_unique<RemoteLibraryProvider>(options.library_mirror_url)
            );
            std::cout << "Using library mirror: " << options.library_mirror_url << "\n";
        } else {
            // 使用默认提供者
            LibraryService::set_provider(std::make_unique<BuiltinLibraryProvider>());
        }
        
        // 交互式输入（如果没有通过命令行指定）
        if (options.project_name == Constants::DEFAULT_PROJECT_NAME) {
            Logo();
            std::cout << "v1.0.1 " << "https://github.com/Macintosh-MaiSensei/SLN2Code " << "Maintainer Macintosh-Maisensei\n";
            std::cout << "SLN2Code is libre and open-source software\n";
            std::cout << "Enter project name (default: " << Constants::DEFAULT_PROJECT_NAME << "): ";
            std::string input_name;
            std::getline(std::cin, input_name);
            
            if (!input_name.empty()) {
                options.project_name = Utils::get_valid_name(input_name);
            }
            
            std::cout << "Enter project path (default: current directory): ";
            std::string input_path;
            std::getline(std::cin, input_path);
            
            if (!input_path.empty()) {
                options.base_path = Utils::clean_path(input_path);
            }
        }
        
        // 获取完整的项目路径
        const fs::path project_full_path = fs::path(options.base_path) / options.project_name;
        
        // 如果目录已存在，提示用户确认
        if (fs::exists(project_full_path)) {
            std::cout << "Warning: Directory already exists: " << project_full_path << "\n";
            std::cout << "Do you want to continue? Existing files may be overwritten. [y/N]: ";
            std::string response;
            std::getline(std::cin, response);
            
            if (response.empty() || (response[0] != 'y' && response[0] != 'Y')) {
                std::cout << "Project creation canceled.\n";
                return 0;
            }
        }
        
        std::cout << "\nCreating project \"" << options.project_name << "\" at:\n"
                  << "  " << project_full_path << "\n\n";
        
        // 获取编译器配置（如果没有通过命令行指定）
        if (options.compiler.name.empty()) {
            options.compiler = CompilerService::get_compiler_config();
        } else {
            std::cout << "Compiler: " << options.compiler.name 
                      << " (" << options.compiler.path << ")\n";
        }
        
        // 获取调试器配置（如果没有通过命令行指定）
        if (options.debugger.name.empty()) {
            options.debugger = DebuggerService::get_debugger_config();
        } else {
            std::cout << "Debugger: " << options.debugger.name 
                      << " (" << options.debugger.path << ")\n";
        }
        
        std::cout << "C++ Standard: " << options.compiler.cppStandard << "\n";
        std::cout << "C Standard: " << options.compiler.cStandard << "\n\n";
        
        // 创建项目目录结构
        DirectoryNode project_structure = ProjectStructureService::get_project_structure(options.project_name);
        bool success = ProjectStructureService::create_directory_recursive(options.base_path, project_structure);
        
        if (!success) {
            std::cerr << "\nFailed to create project directories!\n";
            return 1;
        }
        
        // 处理命令行指定的库安装
        if (!options.libraries_to_install.empty()) {
            for (const auto& lib_name : options.libraries_to_install) {
                LibraryService::add_third_party_library(project_full_path, lib_name);
            }
        } else {
            LibraryService::offer_library_installation(project_full_path);
        }
        
        // 生成基础文件
        ProjectGenerator::generate_project_files(project_full_path, options.project_name, 
                                                options.compiler, options.debugger);
        
        std::cout << "\nProject \"" << options.project_name << "\" created successfully!\n";
        std::cout << "Configuration files generated in .vscode directory:\n";
        std::cout << "  - c_cpp_properties.json\n";
        std::cout << "  - tasks.json\n";
        std::cout << "  - launch.json\n";
        std::cout << "  - settings.json\n\n";
        
        if (!options.libraries_to_install.empty() || 
            fs::exists(project_full_path / "third_party")) {
            std::cout << "Third-party libraries installed in:\n";
            std::cout << "  - third_party/\n";
            std::cout << "  - docs/ (usage guides)\n\n";
        }
        
        std::cout << "Next steps:\n"
                  << "  1. cd " << project_full_path << "\n"
                  << "  2. code . (to open in VS Code)\n"
                  << "  3. Review and update configuration files if needed\n"
                  << "  4. Press F5 to build and debug!\n\n";
        
        std::cout << "Press Enter to exit...\n";
        std::cin.get();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
