cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME lockdep)

project(${PROJECT_NAME} VERSION 1.0)

set(CMAKE_C_COMPILER /usr/bin/clang)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(COMPILE_OPTIONS
    "-Wall"
    "-Werror"
    "-Wextra"
    "-fsanitize=address,undefined"
    "-fno-omit-frame-pointer"
    "-g3")

set(LINK_OPTIONS "-fsanitize=address,undefined")

file(GLOB SOURCES "src/*.c")
include_directories(src/include)

add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE ${COMPILE_OPTIONS})

target_link_options(${PROJECT_NAME} PRIVATE ${LINK_OPTIONS})
