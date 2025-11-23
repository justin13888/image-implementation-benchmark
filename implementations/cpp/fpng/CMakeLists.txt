cmake_minimum_required(VERSION 3.14)
project(bench-fpng)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

include_directories(../../../harness/cpp)

include(FetchContent)
FetchContent_Declare(
  fpng
  GIT_REPOSITORY https://github.com/richgel999/fpng.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(fpng)

# fpng doesn't have a CMakeLists.txt usually, or it might.
# If it doesn't, we need to add source files manually.
# Let's check if FetchContent works. If not, we might need to define the library.
# Checking fpng repo... it has no CMakeLists.txt in root.
# So we need to add sources.

add_library(fpng_lib ${fpng_SOURCE_DIR}/src/fpng.cpp)
target_include_directories(fpng_lib PUBLIC ${fpng_SOURCE_DIR}/src)

find_package(PNG REQUIRED)

add_executable(bench-fpng main.cpp)
target_link_libraries(bench-fpng fpng_lib PNG::PNG)
