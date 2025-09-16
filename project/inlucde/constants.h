#pragma once

#include <string>
#include <unordered_map>
#include <vector>
enum class CompilerType {
    GCC,
    GXX,
    CLANG,
    CLANGXX,
    MSVC,
    UNKNOWN
};

enum class DebuggerType {
    GDB,
    LLDB,
    CPPVSDBG,
    UNKNOWN
};

namespace Constants {
    const std::string VERSION = "1.0.3";
    const std::string DEFAULT_PROJECT_NAME = "project";
    const std::string DEFAULT_LIBRARY_MIRROR = "https://github.com/Macintosh-MaiSensei/SLN2Code";

    const std::unordered_map<std::string, CompilerType> COMPILER_TYPE_MAP = {
            {"gcc", CompilerType::GCC},
            {"g++", CompilerType::GXX},
            {"clang", CompilerType::CLANG},
            {"clang++", CompilerType::CLANGXX},
            {"cl", CompilerType::MSVC},
            {"msvc", CompilerType::MSVC}
    };

    const std::unordered_map<std::string, DebuggerType> DEBUGGER_TYPE_MAP = {
            {"gdb", DebuggerType::GDB},
            {"lldb", DebuggerType::LLDB},
            {"cppvsdbg", DebuggerType::CPPVSDBG}
    };

    struct ThirdPartyLibrary {
        std::string name;
        std::string downloadUrl;
        std::string includePath;
        std::string libPath;
        std::vector<std::string> dependencies;
        std::string configInstructions;
        std::string sha256;
    };

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