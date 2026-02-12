cmake_minimum_required(VERSION 3.15)

# 设置Vcpkg工具链（如果需要，请取消注释并填写路径）
# set(CMAKE_TOOLCHAIN_FILE "your/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

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

message(STATUS "构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "源文件: ${SOURCES}")

# 1. 查找并链接 yaml-cpp
find_package(yaml-cpp CONFIG QUIET)
if(yaml-cpp_FOUND)
    message(STATUS "找到 yaml-cpp 库 (CONFIG模式)")
    if(TARGET yaml-cpp::yaml-cpp)
        set(YAMLCPP_TARGET yaml-cpp::yaml-cpp)
    elseif(TARGET yaml-cpp)
        set(YAMLCPP_TARGET yaml-cpp)
    endif()
else()
    # 备用方案：使用pkg-config
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(YAML_CPP QUIET yaml-cpp)
        if(YAML_CPP_FOUND)
            message(STATUS "找到 yaml-cpp 库 (pkg-config模式)")
        endif()
    endif()
    if(NOT YAML_CPP_FOUND)
        message(WARNING "未找到 yaml-cpp 库。如果需要YAML功能，请安装该库。")
    endif()
endif()

# 2. 查找并链接 CURL
find_package(CURL REQUIRED)
message(STATUS "找到 CURL 库: ${CURL_FOUND}")

# 添加可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 设置目标属性
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# 添加编译选项
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

# 收集要链接的库
set(LINK_LIBS)

# 链接 yaml-cpp
if(DEFINED YAMLCPP_TARGET)
    message(STATUS "链接 yaml-cpp 目标: ${YAMLCPP_TARGET}")
    list(APPEND LINK_LIBS ${YAMLCPP_TARGET})
elseif(YAML_CPP_FOUND)
    message(STATUS "链接 yaml-cpp 库 (pkg-config)")
    list(APPEND LINK_LIBS ${YAML_CPP_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PRIVATE ${YAML_CPP_INCLUDE_DIRS})
else()
    message(STATUS "未链接 yaml-cpp 库")
endif()

# 链接 CURL (必需)
list(APPEND LINK_LIBS CURL::libcurl)

# 链接所有库
target_link_libraries(${PROJECT_NAME} PRIVATE ${LINK_LIBS})

# 创建运行目标
add_custom_target(run
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    COMMENT "Running ${PROJECT_NAME}"
)
