cmake_minimum_required(VERSION 3.10)
project(bench-libjpeg-turbo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

include_directories(../../../harness/cpp)

find_package(JPEG REQUIRED)

add_executable(bench-libjpeg-turbo main.cpp)
target_link_libraries(bench-libjpeg-turbo JPEG::JPEG)
