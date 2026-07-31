cmake_minimum_required(VERSION 3.20)
project(libyaz0
        VERSION 0.1.0
        DESCRIPTION "Yaz0 compression library"
        HOMEPAGE_URL "https://github.com/jessebrands/libyaz0"
        LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(YAZ0_TESTS "Enable testing and build testing suite" ON)

set(YAZ0_SOURCES
        src/yaz0.c
        src/common.c
        src/compress.c
        src/decompress.c
        src/search.c
        src/stream.c)

add_library(yaz0 SHARED)
target_sources(yaz0 PRIVATE ${YAZ0_SOURCES})

target_include_directories(yaz0
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

set_target_properties(yaz0 PROPERTIES
        OUTPUT_NAME "yaz0"
        DEBUG_POSTFIX "d")

add_library(yaz0-static STATIC)
target_sources(yaz0-static PRIVATE ${YAZ0_SOURCES})

if (MSVC)
    target_compile_options(yaz0 PRIVATE /W4)
    target_compile_options(yaz0-static PRIVATE /W4)
else ()
    target_compile_options(yaz0 PRIVATE -Wall -Wextra -Wconversion)
    target_compile_options(yaz0-static PRIVATE -Wall -Wextra -Wconversion)
endif ()

target_include_directories(yaz0-static
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

set_target_properties(yaz0-static PROPERTIES
        OUTPUT_NAME "yaz0"
        DEBUG_POSTFIX "d")

if (YAZ0_TESTS)
    enable_testing()
    add_subdirectory("test")
endif ()
