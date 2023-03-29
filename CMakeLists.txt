cmake_minimum_required(VERSION 3.24)
project(smartsendercpp)

set(CMAKE_CXX_STANDARD 20)
find_package(Arrow REQUIRED)
find_package(ArrowFlight REQUIRED)
find_package(ArrowFlightSql REQUIRED)
list(APPEND CMAKE_PREFIX_PATH "./lib")


add_executable(smartsendercpp "src/CMakeLists.txt" "src/models/CMakeLists.txt" "src/utils/CMakeLists.txt")

add_subdirectory(src)
add_subdirectory(src/models)
add_subdirectory(src/utils)
add_subdirectory(src/managers)

target_link_libraries(smartsendercpp PUBLIC Arrow::arrow_shared ArrowFlight::arrow_flight_shared ArrowFlightSql::arrow_flight_sql_shared parquet)
