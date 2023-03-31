cmake_minimum_required(VERSION 3.24)
project(smartsendercpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_FLAGS "${CMAKE_CFLAGS} -fopenmp")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG")

add_executable(smartsendercpp "src/CMakeLists.txt" "src/models/CMakeLists.txt" "src/utils/CMakeLists.txt" src/utils/Timekeeper.cpp src/utils/Timekeeper.h src/utils/ObserverPatternInterfaces.h)

add_subdirectory(src)
add_subdirectory(src/models)
add_subdirectory(src/utils)
add_subdirectory(src/managers)
