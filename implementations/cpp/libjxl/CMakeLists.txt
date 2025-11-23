cmake_minimum_required(VERSION 3.10)
project(bench-libjxl)

set(CMAKE_CXX_STANDARD 17)

# Aggressive optimization flags per README
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -fstrict-aliasing -fomit-frame-pointer -march=native -DNDEBUG")
set(CMAKE_BUILD_TYPE Release)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fwhole-program-vtables")
endif()

include_directories(../../../harness/cpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JXL REQUIRED IMPORTED_TARGET libjxl)
pkg_check_modules(JXL_THREADS REQUIRED IMPORTED_TARGET libjxl_threads)
find_package(ZLIB REQUIRED)

add_executable(bench-libjxl decode.cpp)
target_link_libraries(bench-libjxl PkgConfig::JXL PkgConfig::JXL_THREADS ZLIB::ZLIB)

# Encoder binary
add_executable(bench-libjxl-encode encode.cpp)
target_link_libraries(bench-libjxl-encode PkgConfig::JXL PkgConfig::JXL_THREADS ZLIB::ZLIB)

# Link mimalloc for consistent allocator
find_library(MIMALLOC_LIB mimalloc REQUIRED)
target_link_libraries(bench-libjxl ${MIMALLOC_LIB})
target_link_libraries(bench-libjxl-encode ${MIMALLOC_LIB})
