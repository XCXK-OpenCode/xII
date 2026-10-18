cmake_minimum_required(VERSION 3.22)
project(example_rainbow_renderer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(example_rainbow_renderer main.cpp)

target_link_libraries(example_rainbow_renderer PRIVATE SDL3::SDL3-shared xII)