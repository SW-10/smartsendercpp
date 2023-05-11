cmake_minimum_required(VERSION 3.24)
project(smartsendercpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_FLAGS "${CMAKE_CFLAGS} -fopenmp")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG")
if(UNIX)
    find_package(Arrow REQUIRED)
    find_package(ArrowFlight REQUIRED)
    find_package(ArrowFlightSql REQUIRED)
    list(APPEND CMAKE_PREFIX_PATH "./lib")
endif(UNIX)

option(HUFFMAN "Enable/Disable HUFFMAN" ON)

if(HUFFMAN)
    add_definitions(-DHUFFMAN_ENABLED)
endif()


add_executable(smartsendercpp "src/CMakeLists.txt" "src/models/CMakeLists.txt" "src/utils/CMakeLists.txt")

add_subdirectory(src)
add_subdirectory(src/models)
add_subdirectory(src/utils)
add_subdirectory(src/managers)

if(UNIX)
    target_link_libraries(smartsendercpp PUBLIC Arrow::arrow_shared ArrowFlight::arrow_flight_shared ArrowFlightSql::arrow_flight_sql_shared parquet)
endif(UNIX)