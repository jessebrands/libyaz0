cmake_minimum_required(VERSION 3.10)
project(yaz0
        VERSION 0.1.0
        DESCRIPTION "Library for (de)compressing Yaz0 data and files."
        HOMEPAGE_URL "https://github.com/jessebrands/libyaz0"
        LANGUAGES C)

option(YAZ0_BUILD_TESTS "Build tests (requires GoogleTest and C++20)" ON)

#
# File listings.
#
set(YAZ0_PUBLIC_API
        "include/yaz0.h")

set(YAZ0_SOURCES
        "src/byteswap.h"
        "src/header.c"
        "src/deflate.c"
        "src/inflate.c")

set(YAZ0_TEST_SOURCES
        "tests/header_test.cpp")

#
# Library target.
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
    enable_testing()

    #
    # Download GoogleTest.
    #
    include(FetchContent)
    FetchContent_Declare(googletest
            URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    #
    # Testing target.
    #
    add_executable(yaz0_test "${YAZ0_TEST_SOURCES}")

    set_target_properties(yaz0_test PROPERTIES
            CXX_STANDARD 20
            CXX_STANDARD_REQUIRED ON)

    target_link_libraries(yaz0_test
            yaz0
            GTest::gtest_main)
endif ()
