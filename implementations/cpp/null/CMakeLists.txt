cmake_minimum_required(VERSION 3.24)
project(bench-null)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

find_package(ZLIB REQUIRED)
find_package(mimalloc REQUIRED)

# Decoder binary
add_executable(bench-null-decode decode.cpp)
target_link_libraries(bench-null-decode ZLIB::ZLIB mimalloc-static)

# Encoder binary
add_executable(bench-null-encode encode.cpp)
target_link_libraries(bench-null-encode ZLIB::ZLIB mimalloc-static)
