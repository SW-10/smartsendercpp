cmake_minimum_required(VERSION 3.24)
project(mobycpp)

set(CMAKE_CXX_STANDARD 17)
add_subdirectory(config)
add_executable(mobycpp main.cpp)
