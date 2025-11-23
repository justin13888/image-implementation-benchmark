cmake_minimum_required(VERSION 3.10)
project(bench-libwebp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

include_directories(../../../harness/cpp)

# Try to find WebP
find_package(PkgConfig REQUIRED)
pkg_check_modules(WEBP REQUIRED libwebp)

include_directories(${WEBP_INCLUDE_DIRS})
link_directories(${WEBP_LIBRARY_DIRS})

add_executable(bench-libwebp main.cpp)
target_link_libraries(bench-libwebp ${WEBP_LIBRARIES})
