cmake_minimum_required(VERSION 3.31)
project(SLN2Code_Modular)

set(CMAKE_CXX_STANDARD 17)

add_executable(SLN2Code_Modular src/utils.cpp
        src/compiler_service.cpp
        src/debugger_service.cpp
        src/project_structure_service.cpp
        src/sha256.cpp
        src/library_service.cpp
        src/project_generator.cpp
        src/command_line_parser.cpp
        src/builtin_library_provider.cpp
        src/remote_library_provider.cpp
        src/main.cpp)
include_directories(include)
add_executable(SLN2Code ${SOURCES})
if(WIN32)
    target_link_libraries(SLN2Code ws2_32)
    add_definitions(-D_WIN32)
endif()
install(TARGETS SLN2Code DESTINATION bin)
