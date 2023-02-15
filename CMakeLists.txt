cmake_minimum_required(VERSION 3.24)
project(mobycpp)

set(CMAKE_CXX_STANDARD 17)
file(GLOB add
        "*.h"
        "*.cpp"
        )

add_executable(mobycpp ${add})
#add_executable(mobycpp main.cpp config.cpp)
