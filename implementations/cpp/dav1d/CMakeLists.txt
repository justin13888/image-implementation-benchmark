cmake_minimum_required(VERSION 3.10)
project(bench-dav1d)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

include_directories(../../../harness/cpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DAV1D REQUIRED dav1d)
pkg_check_modules(AVIF REQUIRED libavif)

include_directories(${DAV1D_INCLUDE_DIRS} ${AVIF_INCLUDE_DIRS})
link_directories(${DAV1D_LIBRARY_DIRS} ${AVIF_LIBRARY_DIRS})

add_executable(bench-dav1d main.cpp)
target_link_libraries(bench-dav1d ${DAV1D_LIBRARIES} ${AVIF_LIBRARIES})
