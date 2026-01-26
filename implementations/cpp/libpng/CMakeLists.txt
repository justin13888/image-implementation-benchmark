cmake_minimum_required(VERSION 3.24)
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
find_library(MIMALLOC_LIB mimalloc REQUIRED)

# Decoder binary
add_executable(bench-libpng-decode decode.cpp)
target_link_libraries(bench-libpng-decode PNG::PNG ZLIB::ZLIB ${MIMALLOC_LIB})

# Encoder binary
add_executable(bench-libpng-encode encode.cpp)
target_link_libraries(bench-libpng-encode PNG::PNG ZLIB::ZLIB ${MIMALLOC_LIB})
