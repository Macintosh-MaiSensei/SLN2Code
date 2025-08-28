/*Created by Macintosh-MaiSensei on 2025/8/22.*/
/*Version 1.01 Beta*/                                                                                                                                                                                                                                                                                                                                                                                              
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
#include <unordered_set>

namespace fs = std::filesystem;
// 目录节点类
class DirectoryNode {
public:
    std::string name;
    std::vector<DirectoryNode> children;
};

// 编译器配置结构
struct CompilerConfig {
    std::string name;
    std::string path;
    std::string cppStandard;
    std::string cStandard;
    std::string type; // "msvc", "gcc", "clang"
    std::vector<std::string> extraArgs;
};

// 调试器配置结构
struct DebuggerConfig {
    std::string name;
    std::string path;
    std::string type; // "gdb", "lldb", "cppvsdbg"
};

// 项目配置结构
struct ProjectConfig {
    std::string name;
    std::string path;
    CompilerConfig compiler;
    DebuggerConfig debugger;
    std::vector<std::string> libraries;
};

// 生成项目结构定义
DirectoryNode get_project_structure(const std::string& project_name) {
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
                    {"third_party", {}},  // 新增第三方库目录
                    {"docs", {}}           // 新增文档目录
            }
    };
}

// 清理路径输入
std::string clean_path(const std::string& input) {
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
std::string get_valid_name(const std::string& input) {
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

// 递归创建目录结构
bool create_directory_recursive(const fs::path& base_path, const DirectoryNode& node, int depth = 0) {
    const fs::path current_path = base_path / node.name;

    // 缩进显示层级
    const std::string indent(depth * 2, ' ');
    std::cout << indent << node.name;

    try {
        if (!fs::exists(current_path)) {
            if (fs::create_directory(current_path)) {
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

// 从PATH环境变量中查找可执行文件
std::vector<std::string> find_executables_in_path(const std::string& executable_name) {
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
#else
    const char delimiter = ':';
#endif
    
    std::istringstream path_stream(path_str);
    std::string directory;
    
    while (std::getline(path_stream, directory, delimiter)) {
        fs::path dir_path(directory);
        if (fs::exists(dir_path) && fs::is_directory(dir_path)) {
            fs::path executable_path = dir_path / executable_name;
#ifdef _WIN32
            executable_path.replace_extension(".exe");
#endif
            if (fs::exists(executable_path) && fs::is_regular_file(executable_path)) {
                found_paths.push_back(clean_path(executable_path.string()));
            }
        }
    }
    
    return found_paths;
}

// 获取用户选择的编译器配置
CompilerConfig get_compiler_config() {
    CompilerConfig config;
    
    std::cout << "\n=== Compiler Configuration ===\n";
    
    // 选择编译器类型
    std::cout << "Select compiler type:\n";
    std::cout << "1. GCC\n";
    std::cout << "2. Clang\n";
    std::cout << "3. MSVC (Visual Studio)\n";
    std::cout << "Enter choice (1-3, default 1): ";
    
    std::string choice;
    std::getline(std::cin, choice);
    
    if (choice == "2") {
        config.type = "clang";
        config.name = "clang";
    } else if (choice == "3") {
        config.type = "msvc";
        config.name = "cl";
    } else {
        config.type = "gcc";
        config.name = "gcc";
    }
    
    // 从PATH中查找编译器
    std::vector<std::string> compiler_paths;
    if (config.type == "msvc") {
        // 对于MSVC，我们可能需要查找特定的路径
        compiler_paths = find_executables_in_path("cl");
        
        // 如果在PATH中找不到，尝试常见的VS安装路径
        if (compiler_paths.empty()) {
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
                                compiler_paths.push_back(clean_path(compiler_path.string()));
                            }
                        }
                    }
                }
            }
#endif
        }
    } else {
        // 对于GCC和Clang，直接从PATH中查找
        compiler_paths = find_executables_in_path(config.name);
    }
    
    // 处理找到的编译器路径
    if (compiler_paths.empty()) {
        std::cout << "No " << config.name << " compiler found in PATH.\n";
        std::cout << "Enter " << config.name << " compiler path: ";
        std::getline(std::cin, config.path);
        config.path = clean_path(config.path);
    } else if (compiler_paths.size() == 1) {
        config.path = compiler_paths[0];
        std::cout << "Using " << config.name << " compiler found at: " << config.path << "\n";
    } else {
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
                config.path = compiler_paths[idx];
            } else {
                config.path = compiler_paths[0];
            }
        } catch (...) {
            config.path = compiler_paths[0];
        }
        
        std::cout << "Using " << config.name << " compiler at: " << config.path << "\n";
    }
    
    // 选择C++标准
    std::cout << "Select C++ standard:\n";
    std::cout << "1. C++98\n";
    std::cout << "2. C++11\n";
    std::cout << "3. C++14\n";
    std::cout << "4. C++17\n";
    std::cout << "5. C++20\n";
    std::cout << "6. C++23\n";
    std::cout << "Enter choice (1-6, default 3): ";

    std::string cpp_choice;
    std::getline(std::cin, cpp_choice);
    if (cpp_choice == "1") {
        config.cppStandard = "c++98";
    } else if (cpp_choice == "2") {
        config.cppStandard = "c++11";
    } else if (cpp_choice == "3") {
        config.cppStandard = "c++14";
    } else if (cpp_choice == "4") {
        config.cppStandard = "c++17";
    } else if (cpp_choice == "5") {
        config.cppStandard = "c++20";
    } else if (cpp_choice == "6") {
        config.cppStandard = "c++23";
    } else {
        config.cppStandard = "c++17";
    }
    
    // 选择C标准
    std::cout << "Select C standard:\n";
    std::cout << "1. C99\n";
    std::cout << "2. C11\n";
    std::cout << "3. C17\n";
    std::cout << "4. C20\n";
    std::cout << "5. C23\n";
    std::cout << "Enter choice (1-5, default 3): ";

    std::string c_choice;
    std::getline(std::cin, c_choice);
    
    if (c_choice == "1") {
        config.cStandard = "c99";
    } else if (c_choice == "2") {
        config.cStandard = "c11";
    } else if (c_choice == "3") {
        config.cStandard = "c17";
    } else if (c_choice == "4") {
        config.cStandard = "c20";
    } else if (c_choice == "5") {
        config.cStandard = "c23";
    } else {
        config.cStandard = "c17";
    }
    
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

// 获取用户选择的调试器配置
DebuggerConfig get_debugger_config() {
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
        config.type = "lldb";
        config.name = "lldb";
    } else if (choice == "3") {
        config.type = "cppvsdbg";
        config.name = "Visual Studio Debugger";
    } else {
        config.type = "gdb";
        config.name = "gdb";
    }
    
    // 从PATH中查找调试器
    std::vector<std::string> debugger_paths;
    if (config.type == "cppvsdbg") {
        // 对于VS调试器，可能需要特殊处理
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
            fs::path debugger_path = fs::path(path) / "Debugger";
            if (fs::exists(debugger_path)) {
                debugger_paths.push_back(clean_path(debugger_path.string()));
            }
        }
#endif
    } else {
        // 对于GDB和LLDB，直接从PATH中查找
        debugger_paths = find_executables_in_path(config.name);
    }
    
    // 处理找到的调试器路径
    if (debugger_paths.empty()) {
        std::cout << "No " << config.name << " debugger found in PATH.\n";
        std::cout << "Enter " << config.name << " debugger path: ";
        std::getline(std::cin, config.path);
        config.path = clean_path(config.path);
    } else if (debugger_paths.size() == 1) {
        config.path = debugger_paths[0];
        std::cout << "Using " << config.name << " debugger found at: " << config.path << "\n";
    } else {
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
                config.path = debugger_paths[idx];
            } else {
                config.path = debugger_paths[0];
            }
        } catch (...) {
            config.path = debugger_paths[0];
        }

        std::cout << "Using " << config.name << " debugger at: " << config.path << "\n";
    }
    
    return config;
}

// 生成基础的 c_cpp_properties.json
void generate_c_cpp_properties(const fs::path& vscode_dir, const CompilerConfig& compiler) {
    std::ofstream file(vscode_dir / "c_cpp_properties.json");
    
    std::string intelliSenseMode;
    if (compiler.type == "msvc") {
        intelliSenseMode = "windows-msvc-x64";
    } else if (compiler.type == "clang") {
        intelliSenseMode = "linux-clang-x64";
    } else {
        intelliSenseMode = "linux-gcc-x64";
    }
    
    // 确保编译器路径使用正斜杠
    std::string compilerPath = clean_path(compiler.path);
    
    file << R"({
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
            "compilerPath": ")" << compilerPath << R"(",
            "cStandard": ")" << compiler.cStandard << R"(",
            "cppStandard": ")" << compiler.cppStandard << R"(",
            "intelliSenseMode": ")" << intelliSenseMode << R"("
        },
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/include"
            ],
            "defines": [],
            "compilerPath": ")" << compilerPath << R"(",
            "cStandard": ")" << compiler.cStandard << R"(",
            "cppStandard": ")" << compiler.cppStandard << R"(",
            "intelliSenseMode": ")" << intelliSenseMode << R"("
        }
    ],
    "version": 4
})";
}

// 生成基础的 tasks.json
void generate_tasks_json(const fs::path& vscode_dir, const std::string& project_name, const CompilerConfig& compiler) {
    std::ofstream file(vscode_dir / "tasks.json");
    
    std::string output_ext = "";
    if (compiler.type == "msvc") {
        output_ext = ".exe";
    }
    
    // 确保编译器路径使用正斜杠
    std::string compilerPath = clean_path(compiler.path);
    
    file << R"({
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build",
            "type": "shell",
            "command": ")" << compilerPath << R"(",
            "args": [
                "-std=)" << compiler.cppStandard << R"(",
                "-I${workspaceFolder}/include",)";
    
    // 添加额外编译选项
    for (const auto& arg : compiler.extraArgs) {
        file << "\n                \"" << arg << "\",";
    }
    
    file << R"(
                "${workspaceFolder}/src/*.cpp",
                "-o",
                "${workspaceFolder}/build/bin/Debug/)" << project_name << output_ext << R"("
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": [)";
    
    if (compiler.type == "msvc") {
        file << R"(
                "$msCompile")";
    } else {
        file << R"(
                "$gcc")";
    }
    
    file << R"(
            ],
            "detail": "Built with )" << compiler.name << R"("
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
}

// 生成基础的 launch.json
void generate_launch_json(const fs::path& vscode_dir, const std::string& project_name, 
                         const CompilerConfig& compiler, const DebuggerConfig& debugger) {
    std::ofstream file(vscode_dir / "launch.json");
    
    std::string output_ext = "";
    if (compiler.type == "msvc") {
        output_ext = ".exe";
    }
    
    // 确保调试器路径使用正斜杠
    std::string debuggerPath = clean_path(debugger.path);
    
    file << R"({
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug Launch",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/bin/Debug/)" << project_name << output_ext << R"(",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": true,
            "MIMode": ")" << debugger.type << R"(",
            "miDebuggerPath": ")" << debuggerPath << R"(",
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
}

// 生成基础的 settings.json
void generate_settings_json(const fs::path& vscode_dir) {
    std::ofstream file(vscode_dir / "settings.json");
    file << R"({
    "files.associations": {
        "*.h": "c",
        "*.hpp": "cpp",
        "*.ipp": "cpp"
    },
    "editor.formatOnSave": true,
    "C_Cpp.default.configurationProvider": "ms-vscode.cpptools",
    "explorer.confirmDragAndDrop": false
})";
}

// ======================== SHA256计算功能 ========================
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
            throw std::runtime_error("Can't open the file:" + file_path.string());
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

// ======================== 第三方库集成模块 ========================
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

// 支持的第三方库
std::unordered_map<std::string, ThirdPartyLibrary> SUPPORTED_LIBRARIES = {
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

// 创建第三方库目录
void create_third_party_dir(const fs::path& project_path) {
    const fs::path third_party_dir = project_path / "third_party";
    if (!fs::exists(third_party_dir)) {
        fs::create_directory(third_party_dir);
    }
}

// 下载并解压库文件
bool download_and_extract_library(const fs::path& project_path, const ThirdPartyLibrary& lib) {
    const fs::path third_party_dir = project_path / "third_party";
    const fs::path zip_file = third_party_dir / (lib.name + ".zip");

    std::cout << "\nDownloading " << lib.name << "...\n";

    // 使用系统命令下载
    std::string download_cmd = "curl -L -o \"" + zip_file.string() + "\" \"" + lib.downloadUrl + "\"";
    int result = std::system(download_cmd.c_str());

    if (result != 0) {
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
#ifdef _WIN32
    std::string unzip_cmd = "tar -xf \"" + zip_file.string() + "\" -C \"" + third_party_dir.string() + "\"";
#else
    std::string unzip_cmd = "unzip -o \"" + zip_file.string() + "\" -d \"" + third_party_dir.string() + "\"";
#endif

    result = std::system(unzip_cmd.c_str());

    if (result != 0) {
        std::cerr << "Failed to extract " << lib.name << std::endl;
        return false;
    }

    // 删除压缩包
    fs::remove(zip_file);

    return true;
}

// 生成库使用指南
void generate_library_guide(const fs::path& project_path, const ThirdPartyLibrary& lib) {
    const fs::path docs_dir = project_path / "docs";
    if (!fs::exists(docs_dir)) {
        fs::create_directory(docs_dir);
    }

    const fs::path guide_path = docs_dir / (lib.name + "_GUIDE.md");

    std::ofstream guide(guide_path);
    guide << "# " << lib.name << " Integration Guide\n\n";
    guide << "## Installation\n";
    guide << "The library has been downloaded to: `third_party/" << fs::path(lib.downloadUrl).stem().string() << "`\n\n";
    guide << "## Configuration\n";
    guide << "### CMake Configuration\n";
    guide << "```cmake\n" << lib.configInstructions << "\n```\n\n";
    guide << "### Include in Code\n";

    if (lib.name == "GLFW") {
        guide << "```cpp\n#include <GLFW/glfw3.h>\n```\n\n";
    } else if (lib.name == "Boost") {
        guide << "```cpp\n#include <boost/filesystem.hpp>\n```\n\n";
    } else if (lib.name == "SDL2") {
        guide << "```cpp\n#include <SDL.h>\n```\n\n";
    }

    guide << "## Official Documentation\n";
    if (lib.name == "GLFW") {
        guide << "- [GLFW Documentation](https://www.glfw.org/docs/latest/)\n";
    } else if (lib.name == "Boost") {
        guide << "- [Boost Documentation](https://www.boost.org/doc/libs/)\n";
    } else if (lib.name == "SDL2") {
        guide << "- [SDL2 Documentation](https://wiki.libsdl.org/)\n";
    }
}

// 添加第三方库到项目
void add_third_party_library(const fs::path& project_path, const std::string& lib_name) {
    static std::set<std::string> installed_libs; // ✅ 静态变量（线程安全）
    if (installed_libs.find(lib_name) != installed_libs.end()) {
        std::cout << lib_name << " already installed. Skipping.\n";
        return;
    }
    installed_libs.insert(lib_name);
    create_third_party_dir(project_path);

    // 转换为小写以匹配键
    std::string lower_lib_name = lib_name;
    std::transform(lower_lib_name.begin(), lower_lib_name.end(), lower_lib_name.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    auto it = SUPPORTED_LIBRARIES.find(lower_lib_name);
    if (it == SUPPORTED_LIBRARIES.end()) {
        std::cerr << "Unsupported library: " << lib_name << std::endl;
        std::cout << "Available libraries: ";
        for (const auto& lib : SUPPORTED_LIBRARIES) {
            std::cout << lib.first << " ";
        }
        std::cout << std::endl;
        return;
    }

    const ThirdPartyLibrary& lib = it->second;

    std::cout << "\nAdding " << lib.name << " to project...\n";

    // 下载并解压库
    if (!download_and_extract_library(project_path, lib)) {
        return;
    }

    // 生成使用指南
    generate_library_guide(project_path, lib);

    // 更新CMakeLists.txt
    const fs::path cmake_path = project_path / "CMakeLists.txt";
    if (fs::exists(cmake_path)) {
        std::ofstream cmake(cmake_path, std::ios::app);
        cmake << "\n# " << lib.name << " Configuration\n";
        cmake << lib.configInstructions << "\n";
    }

    std::cout << lib.name << " added successfully!\n";
    std::cout << "See docs/" << lib.name << "_GUIDE.md for usage instructions\n";
}

void install_libraries(const fs::path& project_path, const std::vector<std::string>& libs) {
    for (const auto& lib : libs) {
        add_third_party_library(project_path, lib); // 复用单库安装逻辑
        try {
            add_third_party_library(project_path, lib);
        } catch (const std::exception& e) {
            std::cerr << "Failed to install " << lib << ": " << e.what() << "\n";
        }
    }
}

// 在生成项目文件后添加库安装选项
void offer_library_installation(const fs::path& project_path) {
    std::cout << "\nWould you like to add any third-party libraries? [y/N]: ";
    std::string response;
    std::getline(std::cin, response);

    if (response.empty() || (response[0] != 'y' && response[0] != 'Y')) {
        return;
    }

    std::cout << "\nAvailable libraries:\n";
    for (const auto& lib : SUPPORTED_LIBRARIES) {
        std::cout << "  - " << lib.first << "\n";
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

// 生成文件模板
void generate_project_files(const fs::path& project_path, const std::string& project_name,
                           const CompilerConfig& compiler, const DebuggerConfig& debugger) {
    const fs::path vscode_dir = project_path / ".vscode";

    // 确保.vscode目录存在
    if (!fs::exists(vscode_dir)) {
        fs::create_directories(vscode_dir);
    }

    // 生成.vscode配置文件
    generate_c_cpp_properties(vscode_dir, compiler);
    generate_tasks_json(vscode_dir, project_name, compiler);
    generate_launch_json(vscode_dir, project_name, compiler, debugger);
    generate_settings_json(vscode_dir);

    // 创建CMakeLists.txt（备用）
    {
        std::ofstream cmake(project_path / "CMakeLists.txt");
        cmake << "cmake_minimum_required(VERSION 3.20)\n"
              << "project(" << project_name << " VERSION 1.0 LANGUAGES CXX)\n\n"
              << "set(CMAKE_CXX_STANDARD " << compiler.cppStandard.substr(2) << ")\n"
              << "set(CMAKE_C_STANDARD " << compiler.cStandard.substr(1) << ")\n\n"
              << "include(FetchContent)\n"
              << "include_directories(include)\n"
              << "add_library(${PROJECT_NAME}_core STATIC src/main.cpp)\n"
              << "target_include_directories(${PROJECT_NAME}_core PUBLIC include)\n"
              << "add_executable(${PROJECT_NAME} src/main.cpp)\n"
              << "target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_core)\n"
              << "install(TARGETS ${PROJECT_NAME} DESTINATION bin)\n"
              << "install(DIRECTORY include/ DESTINATION include)\n";

        // 添加编译器特定选项
        if (!compiler.extraArgs.empty()) {
            cmake << "\n# Additional compiler flags\n";
            cmake << "target_compile_options(${PROJECT_NAME} PRIVATE";
            for (const auto& arg : compiler.extraArgs) {
                cmake << " " << arg;
            }
            cmake << ")\n";
        }
    }

    // 创建main.cpp
    {
        const fs::path src_dir = project_path / "src";
        if (!fs::exists(src_dir)) fs::create_directories(src_dir);

        std::ofstream main_cpp(src_dir / "main.cpp");
        main_cpp << "#include <iostream>\n\n"
                 << "int main() {\n"
                 << "    std::cout << \"Hello, " << project_name << "!\\n\";\n"
                 << "    std::cout << \"Did you know? More useful task.json and launch.json can be found in my GitHub project\\n\";\n"
                 << "    return 0;\n"
                 << "}\n";
    }

    // 创建基本.gitignore
    {
        std::ofstream gitignore(project_path / ".gitignore");
        gitignore << "# Build artifacts\n"
                  << "build/\n"
                  << "*.exe\n"
                  << "*.out\n"
                  << "*.o\n"
                  << "*.obj\n\n"
                  << "# Editor files\n"
                  << ".vscode/\n"
                  << "!.vscode/tasks.json\n"
                  << "!.vscode/launch.json\n"
                  << "!.vscode/c_cpp_properties.json\n"
                  << "!.vscode/settings.json\n\n"
                  << "# System files\n"
                  << ".DS_Store\n"
                  << "Thumbs.db\n";
    }
}

int main(int argc, char* argv[]) {
    std::string project_name = "project";
    std::string base_path = fs::current_path().string();
    const std::string VERSION = "1.0.1";
    std::vector<std::string> libraries_to_install;
    
    // 默认配置
    CompilerConfig compilerConfig;
    DebuggerConfig debuggerConfig;

    // 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--name" || arg == "-n") {
            if (i + 1 < argc) {
                project_name = get_valid_name(argv[++i]);
            } else {
                std::cerr << "Error: Missing project name after " << arg << std::endl;
                return 1;
            }
        } else if (arg == "--path" || arg == "-p") {
            if (i + 1 < argc) {
                base_path = clean_path(argv[++i]);
            } else {
                std::cerr << "Error: Missing path after " << arg << std::endl;
                return 1;
            }
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "FarewellSLN-Cpp-Starter\n"
                      << "Version: " << VERSION << " Beta1\n"
                      << "Maintainer Macintosh-Maisensei\n"
                      << "FarewellSLN-Cpp-Starter is libre and open-source software\n";
            return 0;
        } else if (arg == "-I" || arg == "--install") {
            if (i + 1 < argc) {
                libraries_to_install.push_back(argv[++i]);
            } else {
                std::cerr << "Error: Missing library name after " << arg << std::endl;
                return 1;
            }
        } else if (arg == "--compiler") {
            if (i + 1 < argc) {
                compilerConfig.name = argv[++i];
            } else {
                std::cerr << "Error: Missing compiler name after " << arg << std::endl;
                return 1;
            }
        } else if (arg == "--compiler-path") {
            if (i + 1 < argc) {
                compilerConfig.path = argv[++i];
            } else {
                std::cerr << "Error: Missing compiler path after " << arg << std::endl;
                return 1;
            }
        } else if (arg == "--cpp-standard") {
            if (i + 1 < argc) {
                compilerConfig.cppStandard = argv[++i];
            } else {
                std::cerr << "Error: Missing C++ standard after " << arg << std::endl;
                return 1;
            }
        } else if (arg == "--c-standard") {
            if (i + 1 < argc) {
                compilerConfig.cStandard = argv[++i];
            } else {
                std::cerr << "Error: Missing C standard after " << arg << std::endl;
                return 1;
            }
        } else if (arg == "--debugger") {
            if (i + 1 < argc) {
                debuggerConfig.name = argv[++i];
            } else {
                std::cerr << "Error: Missing debugger name after " << arg << std::endl;
                return 1;
            }
        } else if (arg == "--debugger-path") {
            if (i + 1 < argc) {
                debuggerConfig.path = argv[++i];
            } else {
                std::cerr << "Error: Missing debugger path after " << arg << std::endl;
                return 1;
            }
        } else if (arg == "--extra-args") {
            if (i + 1 < argc) {
                std::string extra_args = argv[++i];
                std::istringstream iss(extra_args);
                std::string flag;
                while (iss >> flag) {
                    compilerConfig.extraArgs.push_back(flag);
                }
            } else {
                std::cerr << "Error: Missing extra arguments after " << arg << std::endl;
                return 1;
            }
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << "  -n, --name NAME           Set project name\n"
                      << "  -p, --path PATH           Set project path\n"
                      << "  -I, --install LIB         Install third-party library\n"
                      << "      --compiler COMPILER    Set compiler (gcc, clang, cl)\n"
                      << "      --compiler-path PATH   Set compiler path\n"
                      << "      --cpp-standard STD     Set C++ standard (c++11, c++14, c++17, c++20)\n"
                      << "      --c-standard STD       Set C standard (c11, c17)\n"
                      << "      --debugger DEBUGGER    Set debugger (gdb, lldb)\n"
                      << "      --debugger-path PATH   Set debugger path\n"
                      << "      --extra-args ARGS      Set additional compiler flags\n"
                      << "  -v, --version             Output the version of the program\n"
                      << "  -h, --help                Show this help message\n";
            return 0;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            return 1;
        }
    }

    if (project_name != "project" && base_path == fs::current_path().string()) {
        std::cout << "Project will be created in current directory: " << base_path << "\n";
        std::cout << "Press Enter to continue, or Ctrl+C to cancel...";
        std::cin.get();
    }

    // 交互式输入（如果没有通过命令行指定）
    if (project_name == "project") {
        std::cout << "Enter project name (default: project): ";
        std::string input_name;
        std::getline(std::cin, input_name);

        if (!input_name.empty()) {
            project_name = get_valid_name(input_name);
        }

        std::cout << "Enter project path (default: current directory): ";
        std::string input_path;
        std::getline(std::cin, input_path);

        if (!input_path.empty()) {
            base_path = clean_path(input_path);
        }
    }

    // 获取编译器配置（如果没有通过命令行指定）
    if (compilerConfig.name.empty()) {
        compilerConfig = get_compiler_config();
    }
    
    // 获取调试器配置（如果没有通过命令行指定）
    if (debuggerConfig.name.empty()) {
        debuggerConfig = get_debugger_config();
    }

    // 获取完整的项目路径
    const fs::path project_full_path = fs::path(base_path) / project_name;

    std::cout << "\nCreating project \"" << project_name << "\" at:\n"
              << "  " << project_full_path << "\n\n";
    std::cout << "Compiler: " << compilerConfig.name << " (" << compilerConfig.path << ")\n";
    std::cout << "C++ Standard: " << compilerConfig.cppStandard << "\n";
    std::cout << "C Standard: " << compilerConfig.cStandard << "\n";
    std::cout << "Debugger: " << debuggerConfig.name << " (" << debuggerConfig.path << ")\n\n";

    // 创建项目目录结构
    DirectoryNode project_structure = get_project_structure(project_name);
    bool success = create_directory_recursive(base_path, project_structure);

    if (!success) {
        std::cerr << "\nFailed to create project directories!\n";
        return 1;
    }

    // 处理命令行指定的库安装
    if (!libraries_to_install.empty()) {
        for (const auto& lib_name : libraries_to_install) {
            add_third_party_library(project_full_path, lib_name);
        }
    } else { // ✅ 仅当未通过命令行安装库时，才触发交互式安装
        offer_library_installation(project_full_path);
    }

    // 生成基础文件
    generate_project_files(project_full_path, project_name, compilerConfig, debuggerConfig);

    std::cout << "\nProject \"" << project_name << "\" created successfully!\n";
    std::cout << "Configuration files generated in .vscode directory:\n";
    std::cout << "  - c_cpp_properties.json\n";
    std::cout << "  - tasks.json\n";
    std::cout << "  - launch.json\n";
    std::cout << "  - settings.json\n\n";
    std::cout << "Next steps:\n"
              << "  1. cd " << project_full_path << "\n"
              << "  2. code . (to open in VS Code)\n"
              << "  3. Review and update configuration files if needed\n"
              << "  4. Press F5 to build and debug!\n\n";
    std::cout << "Press Enter to exit...\n";
    std::cout.flush();
    std::cin.get();
    return 0;
}
