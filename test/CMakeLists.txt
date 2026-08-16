set(YAZ0_CORPUS_GENERATORS
        "Zeroes"
        "Alternating"
        "Binary"
        "Periodic"
        "Structured"
        "Incompressible")

set(YAZ0_TEST_DATA_DIR "" CACHE PATH
        "Directory of extracted Yaz0 test data (scanned recursively)")

set(YAZ0_MATCHERS "standard")
set(YAZ0_GOLDEN_MATCHERS "standard")

add_library(yaz0test STATIC test_driver.c test_utility.c test_corpus.c)
add_library(yaz0::yaz0test ALIAS yaz0test)
target_link_libraries(yaz0test PUBLIC yaz0::yaz0_static)
target_include_directories(yaz0test PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_compile_options(yaz0test PRIVATE ${YAZ0_WARNING_FLAGS})

add_executable(test_compress test_compress.c)
target_link_libraries(test_compress PRIVATE yaz0::yaz0test)
target_compile_options(test_compress PRIVATE ${YAZ0_WARNING_FLAGS})

add_executable(test_decompress test_decompress.c)
target_link_libraries(test_decompress PRIVATE yaz0::yaz0test)
target_compile_options(test_decompress PRIVATE ${YAZ0_WARNING_FLAGS})

function(yaz0_add_test YAZ0_TEST_NAME)
    cmake_parse_arguments(ARG "" "COMPRESSED_FILE;DECOMPRESSED_FILE;PADDING;IN_CHUNK;OUT_CHUNK" "" ${ARGN})

    if (ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "yaz0_add_test(${YAZ0_TEST_NAME}): unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif ()
    if (ARG_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR "yaz0_add_test(${YAZ0_TEST_NAME}): missing values for: ${ARG_KEYWORDS_MISSING_VALUES}")
    endif ()
    if (NOT ARG_COMPRESSED_FILE OR NOT ARG_DECOMPRESSED_FILE)
        message(FATAL_ERROR "yaz0_add_test(${YAZ0_TEST_NAME}): COMPRESSED_FILE and DECOMPRESSED_FILE are required")
    endif ()

    if (NOT DEFINED ARG_PADDING)
        set(ARG_PADDING 0)
    endif ()
    if (NOT DEFINED ARG_IN_CHUNK)
        set(ARG_IN_CHUNK 0)
    endif ()
    if (NOT DEFINED ARG_OUT_CHUNK)
        set(ARG_OUT_CHUNK 0)
    endif ()

    set(YAZ0_TEST_OPTIONS
            -p ${ARG_PADDING} -i ${ARG_IN_CHUNK} -o ${ARG_OUT_CHUNK})

    foreach (YAZ0_MATCHER IN LISTS YAZ0_MATCHERS)
        set(YAZ0_CASE "Compress.${YAZ0_MATCHER}.${YAZ0_TEST_NAME}")

        if (YAZ0_MATCHER IN_LIST YAZ0_GOLDEN_MATCHERS)
            add_test(NAME "${YAZ0_CASE}"
                    COMMAND test_compress ${YAZ0_TEST_OPTIONS} -m ${YAZ0_MATCHER}
                    "${ARG_DECOMPRESSED_FILE}" "${ARG_COMPRESSED_FILE}")
        else ()
            add_test(NAME "${YAZ0_CASE}"
                    COMMAND test_compress ${YAZ0_TEST_OPTIONS} -m ${YAZ0_MATCHER}
                    "${ARG_DECOMPRESSED_FILE}")
        endif ()

        set_tests_properties("${YAZ0_CASE}" PROPERTIES
                LABELS "compress"
                SKIP_RETURN_CODE 2)
    endforeach ()

    add_test(NAME "Decompress.${YAZ0_TEST_NAME}"
            COMMAND test_decompress ${YAZ0_TEST_OPTIONS}
            "${ARG_COMPRESSED_FILE}" "${ARG_DECOMPRESSED_FILE}")

    set_tests_properties("Decompress.${YAZ0_TEST_NAME}" PROPERTIES LABELS "decompress")
endfunction()

function(yaz0_add_synthetic_test name case)
    yaz0_add_test("Synthetic.${name}"
            COMPRESSED_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${case}.szs"
            DECOMPRESSED_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${case}.bin")

    # Test chunked paths too.
    set(chunk_sizes 1 2 7 13)
    foreach (in_chunk IN ITEMS ${chunk_sizes})
        foreach (out_chunk IN ITEMS ${chunk_sizes})
            yaz0_add_test("Synthetic.${name}.In${in_chunk}Out${out_chunk}"
                    COMPRESSED_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${case}.szs"
                    DECOMPRESSED_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${case}.bin"
                    IN_CHUNK ${in_chunk}
                    OUT_CHUNK ${out_chunk})
        endforeach ()
    endforeach ()
endfunction()

yaz0_add_synthetic_test("HeaderOnly" "cases/header_only")
yaz0_add_synthetic_test("SingleLiteralGroup" "cases/single_literal_group")
yaz0_add_synthetic_test("ABCRepeat" "cases/abc_repeat")

if (YAZ0_TEST_DATA_DIR)
    if (NOT IS_DIRECTORY "${YAZ0_TEST_DATA_DIR}")
        message(FATAL_ERROR "${YAZ0_TEST_DATA_DIR} is not a directory")
    endif ()

    message(STATUS "Scanning for Yaz0 files in: ${YAZ0_TEST_DATA_DIR}")

    file(GLOB_RECURSE YAZ0_COMPRESSED_TEST_FILES
            RELATIVE "${YAZ0_TEST_DATA_DIR}"
            "${YAZ0_TEST_DATA_DIR}/*.szs")

    foreach (TEST_FILENAME IN LISTS YAZ0_COMPRESSED_TEST_FILES)
        string(REGEX REPLACE "\\.szs$" "" TEST_BASE "${TEST_FILENAME}")
        string(REPLACE "/" "." TEST_NAME "${TEST_BASE}")
        string(REPLACE " " "_" TEST_NAME "${TEST_NAME}")

        if (EXISTS "${YAZ0_TEST_DATA_DIR}/${TEST_BASE}.bin")
            yaz0_add_test("RomFile.${TEST_NAME}"
                    COMPRESSED_FILE "${YAZ0_TEST_DATA_DIR}/${TEST_BASE}.szs"
                    DECOMPRESSED_FILE "${YAZ0_TEST_DATA_DIR}/${TEST_BASE}.bin"
                    IN_CHUNK 13
                    OUT_CHUNK 11
                    PADDING 16)
        else ()
            list(APPEND YAZ0_ORPHANED_TEST_FILES "${TEST_FILENAME}")
        endif ()
    endforeach ()

    if (YAZ0_ORPHANED_TEST_FILES)
        list(LENGTH YAZ0_ORPHANED_TEST_FILES YAZ0_ORPHAN_COUNT)
        message(WARNING
                "${YAZ0_ORPHAN_COUNT} Yaz0 file(s) have no matching .bin and "
                "were skipped")
    endif ()
endif ()

#
# Testing suite for search algorithms
#
add_executable(test_search
        test_search.c)

target_link_libraries(test_search
        PRIVATE yaz0::yaz0_static
        PRIVATE yaz0::yaz0test)

target_include_directories(test_search
        PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_compile_options(test_search PRIVATE ${YAZ0_WARNING_FLAGS})

function(yaz0_add_search_test name algorithm)
    foreach (corpus_name IN ITEMS ${YAZ0_CORPUS_GENERATORS})
        string(TOLOWER ${corpus_name} corpus)

        add_test(NAME "Search.${name}.${corpus_name}" COMMAND test_search ${algorithm} ${corpus})
        set_tests_properties("Search.${name}.${corpus_name}" PROPERTIES
                LABELS "search"
                SKIP_RETURN_CODE 2)
    endforeach ()
endfunction()

yaz0_add_search_test("Reference" reference)
yaz0_add_search_test("Scalar" scalar)
yaz0_add_search_test("SSE2" sse2)
yaz0_add_search_test("AVX2" avx2)
yaz0_add_search_test("AVX512" avx512)
yaz0_add_search_test("Neon" neon)
yaz0_add_search_test("SIMD128" simd128)
yaz0_add_search_test("SWAR64" swar64)

#
# Benchmarking suite
#
add_executable(yaz0bench yaz0bench.c)
target_link_libraries(yaz0bench PRIVATE yaz0::yaz0test)
target_compile_options(yaz0bench PRIVATE ${YAZ0_WARNING_FLAGS})
