cmake_minimum_required(VERSION 3.22)
project(example_fadein_renderer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(example_fadein_renderer main.cpp)

target_link_libraries(example_fadein_renderer PRIVATE SDL3::SDL3-shared xII)