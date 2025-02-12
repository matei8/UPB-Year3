cmake_minimum_required(VERSION 3.29)
project(tema1a)

set(CMAKE_CXX_STANDARD 14)

include_directories(src)

add_executable(tema1a
        src/main.cpp
        src/main.h)
