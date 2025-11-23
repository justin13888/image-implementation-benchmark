cmake_minimum_required(VERSION 3.10)
project(bench-libavif)

set(CMAKE_CXX_STANDARD 17)

# Aggressive optimization flags per README
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -fstrict-aliasing -fomit-frame-pointer -march=native -DNDEBUG")
set(CMAKE_BUILD_TYPE Release)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fwhole-program-vtables")
endif()

include_directories(../../../harness/cpp)

find_package(libavif REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(bench-libavif decode.cpp)
target_link_libraries(bench-libavif avif ZLIB::ZLIB)

# Encoder binary
add_executable(bench-libavif-encode encode.cpp)
target_link_libraries(bench-libavif-encode avif ZLIB::ZLIB)

# Link mimalloc for consistent allocator
find_library(MIMALLOC_LIB mimalloc REQUIRED)
target_link_libraries(bench-libavif ${MIMALLOC_LIB})
target_link_libraries(bench-libavif-encode ${MIMALLOC_LIB})
