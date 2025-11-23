cmake_minimum_required(VERSION 3.14)
project(bench-spng)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

include_directories(../../../harness/cpp)

include(FetchContent)
FetchContent_Declare(
  spng
  GIT_REPOSITORY https://github.com/randy408/libspng.git
  GIT_TAG        v0.7.4
)
FetchContent_MakeAvailable(spng)

add_executable(bench-spng main.cpp)
target_link_libraries(bench-spng spng)
