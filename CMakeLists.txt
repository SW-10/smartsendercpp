cmake_minimum_required(VERSION 3.24)
project(smartsendercpp)

set(CMAKE_CXX_STANDARD 17)

add_executable(smartsendercpp "src/CMakeLists.txt" "src/models/CMakeLists.txt" "src/utils/CMakeLists.txt" "src/managers/CMakeLists.txt")

add_subdirectory(src)
add_subdirectory(src/models)
add_subdirectory(src/utils)
add_subdirectory(src/managers)
