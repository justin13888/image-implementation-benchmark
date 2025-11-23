cmake_minimum_required(VERSION 3.10)
project(bench-libpng)

set(CMAKE_CXX_STANDARD 17)

# Aggressive optimization flags per README
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -fstrict-aliasing -fomit-frame-pointer -march=native -DNDEBUG")
set(CMAKE_BUILD_TYPE Release)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fwhole-program-vtables")
endif()

include_directories(../../../harness/cpp)

find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(bench-libpng decode.cpp)
target_link_libraries(bench-libpng PNG::PNG ZLIB::ZLIB)

# Encoder binary
add_executable(bench-libpng-encode encode.cpp)
target_link_libraries(bench-libpng-encode PNG::PNG ZLIB::ZLIB)

# Link mimalloc for consistent allocator
find_library(MIMALLOC_LIB mimalloc REQUIRED)
target_link_libraries(bench-libpng ${MIMALLOC_LIB})
target_link_libraries(bench-libpng-encode ${MIMALLOC_LIB})
