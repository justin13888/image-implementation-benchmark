cmake_minimum_required(VERSION 3.10)
project(bench-libavif)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

include_directories(../../../harness/cpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(AVIF REQUIRED libavif)

include_directories(${AVIF_INCLUDE_DIRS})
link_directories(${AVIF_LIBRARY_DIRS})

add_executable(bench-libavif main.cpp)
target_link_libraries(bench-libavif ${AVIF_LIBRARIES})
