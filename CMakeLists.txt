cmake_minimum_required(VERSION 3.21)
project(libyaz0
        VERSION 1.0.0
        DESCRIPTION "Yaz0 compression library"
        HOMEPAGE_URL "https://github.com/jessebrands/libyaz0"
        LANGUAGES C)

include(CheckCCompilerFlag)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(YAZ0_BUILD_SHARED "Enable building libyaz0 as shared library" ON)
option(YAZ0_BUILD_STATIC "Enable building libyaz0 as static library" ON)
option(YAZ0_BUILD_TESTS "Enable building the testing suite" ${PROJECT_IS_TOP_LEVEL})
option(YAZ0_INSTALL "Enable installation of libyaz0" ${PROJECT_IS_TOP_LEVEL})

option(YAZ0_ENABLE_SSE2 "Enable the SSE2 search implementation on x86" ON)

set(YAZ0_PUBLIC_HEADERS
        include/yaz0/yaz0.h
        ${CMAKE_CURRENT_BINARY_DIR}/include/yaz0/config.h)

set(YAZ0_PRIVATE_HEADERS
        src/common.h
        src/compress.h
        src/decompress.h
        src/search.h
        src/stream.h)

set(YAZ0_SOURCES
        src/yaz0.c
        src/common.c
        src/compress.c
        src/decompress.c
        src/search.c
        src/search_sse2.c
        src/stream.c)

if (YAZ0_ENABLE_SSE2)
    check_c_compiler_flag("-msse2" YAZ0_HAVE_MSSE2_FLAG)
    if (YAZ0_HAVE_MSSE2_FLAG)
        set_source_files_properties(src/search_sse2.c PROPERTIES COMPILE_OPTIONS "-msse2")
    endif ()
endif ()

configure_file(
        include/yaz0/config.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/include/yaz0/config.h
        @ONLY)

if (YAZ0_BUILD_SHARED)
    add_library(yaz0 SHARED
            ${YAZ0_SOURCES}
            ${YAZ0_PUBLIC_HEADERS}
            ${YAZ0_PRIVATE_HEADERS})

    add_library(yaz0::yaz0 ALIAS yaz0)

    target_include_directories(yaz0
            PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
            PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

    set_target_properties(yaz0 PROPERTIES
            OUTPUT_NAME "yaz0"
            VERSION ${PROJECT_VERSION}
            SOVERSION "${PROJECT_VERSION_MAJOR}")

    target_compile_definitions(yaz0 PRIVATE YAZ0_SHARED)
    set_target_properties(yaz0 PROPERTIES C_VISIBILITY_PRESET hidden)
    target_compile_features(yaz0 PUBLIC c_std_99)

    if (MSVC)
        target_compile_options(yaz0 PRIVATE /W4)
    else ()
        target_compile_options(yaz0 PRIVATE -Wall -Wextra -Wconversion)
    endif ()

    if (NOT YAZ0_ENABLE_SSE2)
        target_compile_definitions(yaz0 PRIVATE YAZ0_DISABLE_SSE2)
    endif ()
endif ()

if (YAZ0_BUILD_STATIC OR YAZ0_BUILD_TESTS)
    add_library(yaz0_static STATIC
            ${YAZ0_SOURCES}
            ${YAZ0_PUBLIC_HEADERS}
            ${YAZ0_PRIVATE_HEADERS})

    add_library(yaz0::yaz0_static ALIAS yaz0_static)

    target_include_directories(yaz0_static
            PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
            PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

    if (MSVC)
        # MSVC will create a yaz0.lib for both static and shared targets.
        # We prefix the static library with 's' on MSVC to prevent that.
        set_target_properties(yaz0_static PROPERTIES OUTPUT_NAME "yaz0s")
    else ()
        set_target_properties(yaz0_static PROPERTIES OUTPUT_NAME "yaz0")
    endif ()

    target_compile_definitions(yaz0_static PUBLIC YAZ0_STATIC)
    target_compile_features(yaz0_static PUBLIC c_std_99)

    if (MSVC)
        target_compile_options(yaz0_static PRIVATE /W4)
    else ()
        target_compile_options(yaz0_static PRIVATE -Wall -Wextra -Wconversion)
    endif ()

    if (NOT YAZ0_ENABLE_SSE2)
        target_compile_definitions(yaz0_static PRIVATE YAZ0_DISABLE_SSE2)
    endif ()
endif ()

if (YAZ0_BUILD_TESTS)
    enable_testing()
    add_subdirectory("test")
endif ()

set(YAZ0_INSTALL_TARGETS)
if (YAZ0_BUILD_SHARED)
    list(APPEND YAZ0_INSTALL_TARGETS yaz0)
endif ()
if (YAZ0_BUILD_STATIC)
    list(APPEND YAZ0_INSTALL_TARGETS yaz0_static)
endif ()

if (YAZ0_INSTALL AND YAZ0_INSTALL_TARGETS)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(TARGETS ${YAZ0_INSTALL_TARGETS}
            EXPORT yaz0Targets
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

    install(FILES ${YAZ0_PUBLIC_HEADERS}
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/yaz0")

    install(EXPORT yaz0Targets
            FILE yaz0Targets.cmake
            NAMESPACE yaz0::
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/yaz0")

    configure_package_config_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/yaz0Config.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/yaz0Config.cmake"
            INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/yaz0")

    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/yaz0ConfigVersion.cmake"
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY SameMajorVersion)

    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/yaz0Config.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/yaz0ConfigVersion.cmake"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/yaz0")

    install(FILES COPYING COPYING.LESSER
            DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/libyaz0")

    if (PROJECT_IS_TOP_LEVEL)
        set(CPACK_PACKAGE_NAME "libyaz0")
        set(CPACK_PACKAGE_VENDOR "Jesse Gerard Brands")
        set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
        set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_HOMEPAGE_URL}")
        set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING.LESSER")
        set(CPACK_VERBATIM_VARIABLES ON)

        set(CPACK_GENERATOR "TGZ;ZIP")

        set(CPACK_SOURCE_GENERATOR "TGZ")
        set(CPACK_SOURCE_PACKAGE_FILE_NAME "libyaz0-${PROJECT_VERSION}")
        set(CPACK_SOURCE_IGNORE_FILES
                "/\\.git"
                "/\\.idea/"
                "/cmake-build-[^/]*/"
                "/build[^/]*/"
                "/install/"
                "/__pycache__/"
                "\\.pyc$")

        include(CPack)
    endif ()
endif ()
