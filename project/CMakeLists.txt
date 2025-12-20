cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE ""#写自己的
    CACHE STRING "Vcpkg toolchain file")

project(sln2code)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})

include_directories(include)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c" "src/*.cc")

file(GLOB_RECURSE HEADERS "include/*.h" "include/*.hpp" "include/*.hxx")

# 打印文件信息
message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "源文件: ${SOURCES}")

# 查找 ftxui 库
find_package(ftxui CONFIG REQUIRED)
message(STATUS "找到 ftxui 库: ${ftxui_FOUND}")

# 查找 yaml-cpp 库
find_package(yaml-cpp CONFIG QUIET)
if(yaml-cpp_FOUND)
    message(STATUS "找到 yaml-cpp 库 (CONFIG模式)")
    # 检查可用的目标名称
    if(TARGET yaml-cpp::yaml-cpp)
        message(STATUS "yaml-cpp 目标名称: yaml-cpp::yaml-cpp")
        set(YAMLCPP_TARGET yaml-cpp::yaml-cpp)
    elseif(TARGET yaml-cpp)
        message(STATUS "yaml-cpp 目标名称: yaml-cpp (已弃用)")
        set(YAMLCPP_TARGET yaml-cpp)
    endif()
else()
    # 尝试通过其他方式查找
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(YAML_CPP QUIET yaml-cpp)
        if(YAML_CPP_FOUND)
            message(STATUS "找到 yaml-cpp 库 (pkg-config模式)")
        endif()
    endif()
    if(NOT YAML_CPP_FOUND)
        message(WARNING "未找到 yaml-cpp 库，如果需要使用YAML功能，请安装: vcpkg install yaml-cpp")
    endif()
endif()

# 尝试不同的 simdjson 目标名称
find_package(simdjson CONFIG QUIET)
if(simdjson_FOUND)
    message(STATUS "找到 simdjson 库 (CONFIG模式)")
    if(TARGET simdjson::simdjson)
        set(SIMDJSON_TARGET simdjson::simdjson)
    elseif(TARGET simdjson)
        set(SIMDJSON_TARGET simdjson)
    endif()
else()
    # 尝试通过其他方式查找
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(SIMDJSON QUIET simdjson)
        if(SIMDJSON_FOUND)
            message(STATUS "找到 simdjson 库 (pkg-config模式)")
        endif()
    endif()
endif()

# 手动查找 GMP
message(STATUS "手动查找 GMP 库...")

set(GMP_POSSIBLE_INCLUDE_PATHS
    "/Users/guanfeng/vcpkg/installed/arm64-osx/include"
    "/usr/local/include"
    "/usr/include"
)

set(GMP_POSSIBLE_LIB_PATHS
    "/Users/guanfeng/vcpkg/installed/arm64-osx/lib"
    "/usr/local/lib"
    "/usr/lib"
)

find_path(GMP_INCLUDE_DIR gmp.h
    PATHS ${GMP_POSSIBLE_INCLUDE_PATHS}
    NO_DEFAULT_PATH
)

find_library(GMP_LIBRARY
    NAMES gmp
    PATHS ${GMP_POSSIBLE_LIB_PATHS}
    NO_DEFAULT_PATH
)

if(GMP_INCLUDE_DIR AND GMP_LIBRARY)
    set(GMP_FOUND TRUE)
    message(STATUS "找到 GMP 头文件: ${GMP_INCLUDE_DIR}")
    message(STATUS "找到 GMP 库: ${GMP_LIBRARY}")
else()
    find_path(GMP_INCLUDE_DIR gmp.h)
    find_library(GMP_LIBRARY NAMES gmp)
    
    if(GMP_INCLUDE_DIR AND GMP_LIBRARY)
        set(GMP_FOUND TRUE)
        message(STATUS "找到 GMP 头文件: ${GMP_INCLUDE_DIR}")
        message(STATUS "找到 GMP 库: ${GMP_LIBRARY}")
    else()
        message(FATAL_ERROR "未找到 GMP 库。请使用以下命令安装: /Users/guanfeng/vcpkg/vcpkg install gmp:arm64-osx")
    endif()
endif()

# 添加可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 设置目标属性
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GMP_INCLUDE_DIR}
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# 添加编译选项
target_compile_options(${PROJECT_NAME} PRIVATE 
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

# 收集要链接的库
set(LINK_LIBS ftxui::component ${GMP_LIBRARY})

# 链接 yaml-cpp
if(DEFINED YAMLCPP_TARGET)
    message(STATUS "链接 yaml-cpp 目标: ${YAMLCPP_TARGET}")
    list(APPEND LINK_LIBS ${YAMLCPP_TARGET})
elseif(YAML_CPP_FOUND)
    message(STATUS "链接 yaml-cpp 库 (pkg-config)")
    list(APPEND LINK_LIBS ${YAML_CPP_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PRIVATE ${YAML_CPP_INCLUDE_DIRS})
else()
    message(STATUS "未找到 yaml-cpp 库，跳过链接")
endif()

# 链接 simdjson
if(DEFINED SIMDJSON_TARGET)
    message(STATUS "链接 simdjson 目标: ${SIMDJSON_TARGET}")
    list(APPEND LINK_LIBS ${SIMDJSON_TARGET})
elseif(SIMDJSON_FOUND)
    message(STATUS "链接 simdjson 库 (pkg-config)")
    list(APPEND LINK_LIBS ${SIMDJSON_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PRIVATE ${SIMDJSON_INCLUDE_DIRS})
else()
    # 如果都找不到，使用 FetchContent
    message(STATUS "使用 FetchContent 下载 simdjson")
    include(FetchContent)
    FetchContent_Declare(
        simdjson
        GIT_REPOSITORY https://github.com/simdjson/simdjson.git
        GIT_TAG v3.1.8
    )
    FetchContent_MakeAvailable(simdjson)
    list(APPEND LINK_LIBS simdjson)
endif()

# 链接所有库
target_link_libraries(${PROJECT_NAME} PRIVATE ${LINK_LIBS})

# 创建运行目标
add_custom_target(run
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    COMMENT "Running ${PROJECT_NAME}"
)
