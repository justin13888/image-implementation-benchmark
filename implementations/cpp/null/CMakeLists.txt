cmake_minimum_required(VERSION 3.10)
project(bench-null)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -flto -march=native")
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

find_package(ZLIB REQUIRED)
find_library(MIMALLOC_LIB mimalloc REQUIRED)

# Decoder binary
add_executable(bench-null-decode decode.cpp)
target_link_libraries(bench-null-decode ZLIB::ZLIB ${MIMALLOC_LIB})

# Encoder binary
add_executable(bench-null-encode encode.cpp)
target_link_libraries(bench-null-encode ZLIB::ZLIB ${MIMALLOC_LIB})
