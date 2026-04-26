cmake_minimum_required(VERSION 3.30)

project(titan)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(deps/toolkit)
add_subdirectory(core)
add_subdirectory(game)
