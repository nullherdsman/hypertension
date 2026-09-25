cmake_minimum_required(VERSION 3.25)
project(hypertension VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(hypertension
    src/main.cpp
    src/cli/cli.cpp
    src/runtime/mode.cpp
    src/forth/forth.cpp
)

target_include_directories(hypertension PRIVATE src)
target_compile_options(hypertension PRIVATE -Wall -Wextra -Wpedantic)
