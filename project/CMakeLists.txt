cmake_minimum_required(VERSION 3.31)
project(SLN2Code_Modular)

set(CMAKE_CXX_STANDARD 17)

add_executable(SLN2Code_Modular utils.cpp
        compiler_service.cpp
        debugger_service.cpp
        project_structure_service.cpp
        sha256.cpp
        library_service.cpp
        project_generator.cpp
        command_line_parser.cpp
        builtin_library_provider.cpp
        remote_library_provider.cpp
        main.cpp
        json_value.cpp)

include_directories(include)
add_executable(SLN2Code ${SOURCES})
if(WIN32)
    target_link_libraries(SLN2Code ws2_32)
    add_definitions(-D_WIN32)
endif()
install(TARGETS SLN2Code DESTINATION bin)
