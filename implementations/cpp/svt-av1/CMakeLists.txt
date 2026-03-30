cmake_minimum_required(VERSION 3.24)
project(bench-svt-av1)

set(CMAKE_CXX_STANDARD 17)

# Aggressive optimization flags per README
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fstrict-aliasing -fomit-frame-pointer -march=native -DNDEBUG")
set(CMAKE_BUILD_TYPE Release)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fwhole-program-vtables")
endif()

include_directories(../../../harness/cpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SVT_AV1 REQUIRED IMPORTED_TARGET SvtAv1Enc)
find_package(libavif REQUIRED)
find_package(ZLIB REQUIRED)
find_package(mimalloc REQUIRED)

# Encoder binary
add_executable(bench-svt-av1-encode encode.cpp)
target_link_libraries(bench-svt-av1-encode PkgConfig::SVT_AV1 avif ZLIB::ZLIB mimalloc-static)
