cmake_minimum_required(VERSION 3.20)
project(libyaz0
        VERSION 0.1.0
        DESCRIPTION "Yaz0 compression library"
        HOMEPAGE_URL "https://github.com/jessebrands/libyaz0"
        LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(${PROJECT_NAME} SHARED)
target_sources(${PROJECT_NAME} PRIVATE
        src/yaz0.c
        src/compress.c
        src/decompress.c)

target_include_directories(${PROJECT_NAME}
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_NAME "yaz0"
        DEBUG_POSTFIX "d")
