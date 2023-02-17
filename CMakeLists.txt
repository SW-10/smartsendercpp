cmake_minimum_required(VERSION 3.24)
project(smartsendercpp)

set(CMAKE_CXX_STANDARD 17)
file(GLOB_RECURSE add
        "src/*.h"
        "src/*.cpp"
        "src/*.c"
        )

add_executable(smartsendercpp ${add})
#add_executable(mobycpp main.cpp config.cpp)
