cmake_minimum_required(VERSION 3.24)
project(bench-libjpeg-turbo)

set(CMAKE_CXX_STANDARD 17)

# Aggressive optimization flags per README
set(CMAKE_C_FLAGS_RELEASE "-O3 -flto -march=native -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -fstrict-aliasing -fomit-frame-pointer -march=native -DNDEBUG")
set(CMAKE_BUILD_TYPE Release)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fwhole-program-vtables")
endif()

include_directories(../../../harness/cpp)

include(FetchContent)

find_package(JPEG REQUIRED)
find_package(ZLIB REQUIRED)
find_library(MIMALLOC_LIB mimalloc REQUIRED)

# Decoder binary
add_executable(bench-libjpeg-turbo-decode decode.cpp)
target_link_libraries(bench-libjpeg-turbo-decode JPEG::JPEG ZLIB::ZLIB ${MIMALLOC_LIB})

# Encoder binary
add_executable(bench-libjpeg-turbo-encode encode.cpp)
target_link_libraries(bench-libjpeg-turbo-encode JPEG::JPEG ZLIB::ZLIB ${MIMALLOC_LIB})
