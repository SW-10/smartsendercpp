cmake_minimum_required(VERSION 3.24)
project(smartsendercpp)

set(CMAKE_CXX_STANDARD 20)

add_executable(smartsendercpp "src/CMakeLists.txt" "src/models/CMakeLists.txt" "src/utils/CMakeLists.txt" src/models/TextCompressor.cpp src/models/TextCompressor.h)

add_subdirectory(src)
add_subdirectory(src/models)
add_subdirectory(src/utils)
