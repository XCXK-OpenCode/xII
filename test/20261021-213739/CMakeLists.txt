cmake_minimum_required(VERSION 3.22)
project(test_20261021_213739)

set(CMAKE_CXX_STANDARD 20)

add_executable(test_20261021_213739 main.cpp)

target_link_libraries(test_20261021_213739 SDL3::SDL3-shared xII)
