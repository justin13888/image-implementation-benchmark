cmake_minimum_required(VERSION 3.10)
project(bench-dav1d)

set(CMAKE_CXX_STANDARD 17)

# Aggressive optimization flags per README
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -fstrict-aliasing -fomit-frame-pointer -march=native -DNDEBUG")
set(CMAKE_BUILD_TYPE Release)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fwhole-program-vtables")
endif()

include_directories(../../../harness/cpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DAV1D REQUIRED IMPORTED_TARGET dav1d)
find_package(libavif REQUIRED)
find_package(ZLIB REQUIRED)
find_library(MIMALLOC_LIB mimalloc REQUIRED)

# Decoder binary
add_executable(bench-dav1d-decode decode.cpp)
target_link_libraries(bench-dav1d-decode PkgConfig::DAV1D avif ZLIB::ZLIB ${MIMALLOC_LIB})
