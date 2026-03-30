cmake_minimum_required(VERSION 3.24)
project(bench-rav1d)

set(CMAKE_CXX_STANDARD 17)

# Aggressive optimization flags per README
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fstrict-aliasing -fomit-frame-pointer -march=native -DNDEBUG")
set(CMAKE_BUILD_TYPE Release)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fwhole-program-vtables")
endif()

include_directories(../../../harness/cpp)

# Find rav1d (Rust-built static lib with dav1d-compatible symbols)
find_library(RAV1D_LIB NAMES rav1d REQUIRED)
find_package(libavif REQUIRED)
find_package(ZLIB REQUIRED)
find_package(mimalloc REQUIRED)

# Decoder binary
add_executable(bench-rav1d-decode decode.cpp)
# Link rav1d BEFORE avif so its dav1d_* symbols satisfy avif's references
target_link_libraries(bench-rav1d-decode ${RAV1D_LIB} avif ZLIB::ZLIB mimalloc-static pthread dl m)
