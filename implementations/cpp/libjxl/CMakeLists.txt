cmake_minimum_required(VERSION 3.10)
project(bench-libjxl)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

include_directories(../../../harness/cpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JXL REQUIRED libjxl)
pkg_check_modules(JXL_THREADS REQUIRED libjxl_threads)

include_directories(${JXL_INCLUDE_DIRS} ${JXL_THREADS_INCLUDE_DIRS})
link_directories(${JXL_LIBRARY_DIRS} ${JXL_THREADS_LIBRARY_DIRS})

add_executable(bench-libjxl main.cpp)
target_link_libraries(bench-libjxl ${JXL_LIBRARIES} ${JXL_THREADS_LIBRARIES})

add_executable(encode-jxl encode.cpp)
target_link_libraries(encode-jxl ${JXL_LIBRARIES} ${JXL_THREADS_LIBRARIES})
