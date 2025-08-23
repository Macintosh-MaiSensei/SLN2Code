cmake_minimum_required(VERSION 3.31)
project(FarewellSLN-Cpp-Starter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(FarewellSLN-Cpp-Starter LANGUAGES CXX)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)
add_executable(src main.cpp)
