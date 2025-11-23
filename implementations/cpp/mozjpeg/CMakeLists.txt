cmake_minimum_required(VERSION 3.10)
project(bench-mozjpeg)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

include_directories(../../../harness/cpp)

include(FetchContent)
FetchContent_Declare(
  mozjpeg
  GIT_REPOSITORY https://github.com/mozilla/mozjpeg.git
  GIT_TAG        v4.1.1
)
# mozjpeg options
set(PNG_SUPPORTED OFF CACHE BOOL "" FORCE)
set(ENABLE_STATIC ON CACHE BOOL "" FORCE)
set(ENABLE_SHARED OFF CACHE BOOL "" FORCE)
set(WITH_TURBOJPEG OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(mozjpeg)

add_executable(bench-mozjpeg main.cpp)
target_link_libraries(bench-mozjpeg jpeg-static)
target_include_directories(bench-mozjpeg PRIVATE ${mozjpeg_SOURCE_DIR} ${mozjpeg_BINARY_DIR})
