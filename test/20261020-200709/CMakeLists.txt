cmake_minimum_required(VERSION 3.22)
project(test_20261020_200709)

set(CMAKE_CXX_STANDARD 20)

add_executable(test_20261020_200709 main.cpp)

target_link_libraries(test_20261020_200709 SDL3::SDL3-shared xII)
