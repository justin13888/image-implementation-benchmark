cmake_minimum_required(VERSION 3.24)
project(bench-libgav1)

set(CMAKE_CXX_STANDARD 17)

# Aggressive optimization flags per README
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fstrict-aliasing -fomit-frame-pointer -march=native -DNDEBUG")
set(CMAKE_BUILD_TYPE Release)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fwhole-program-vtables")
endif()

include_directories(../../../harness/cpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGAV1 REQUIRED IMPORTED_TARGET libgav1)
find_package(libavif REQUIRED)
find_package(ZLIB REQUIRED)
find_package(mimalloc REQUIRED)

# Decoder binary
add_executable(bench-libgav1-decode decode.cpp)
target_link_libraries(bench-libgav1-decode PkgConfig::LIBGAV1 avif ZLIB::ZLIB mimalloc-static)
