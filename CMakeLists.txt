cmake_minimum_required(VERSION 3.31)
project(yaz0
        VERSION 0.1.0
        DESCRIPTION "Library for (de)compressing Yaz0 data and files."
        HOMEPAGE_URL "https://github.com/jessebrands/libyaz0"
        LANGUAGES C)

option(YAZ0_BUILD_TESTS "Build tests (requires GoogleTest and C++11)" OFF)

#
# File listings.
#
set(YAZ0_PUBLIC_API
        "include/yaz0.h")

set(YAZ0_SOURCES
        "src/deflate.c"
        src/inflate.c)

set(YAZ0_TEST_SOURCES)

#
# Library target
#
add_library(yaz0
        "${YAZ0_SOURCES}"
        "${YAZ0_PUBLIC_API}")

set_target_properties(yaz0 PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED ON)

target_include_directories(yaz0 PUBLIC "${PROJECT_SOURCE_DIR}/include")

if (YAZ0_BUILD_TESTS)
    message("Tests are enabled. Enabling C++ language and downloading GoogleTest.")
    enable_language(CXX)
endif ()
