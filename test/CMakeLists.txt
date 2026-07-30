set(YAZ0_TEST_DATA_DIR "" CACHE PATH
        "Directory of extracted Yaz0 test data (scanned recursively)")

add_library(yaz0test STATIC)
target_link_libraries(yaz0test PUBLIC yaz0-static)
target_sources(yaz0test PRIVATE
        test_driver.c)

set_target_properties(yaz0test PROPERTIES DEBUG_POSTFIX "d")

add_executable(test_compress test_compress.c)
target_link_libraries(test_compress PRIVATE yaz0test)

add_executable(test_decompress test_decompress.c)
target_link_libraries(test_decompress PRIVATE yaz0test)

function(yaz0_add_test name compressed_file decompressed_file)
    add_test(NAME "CompressFile.${name}"
            COMMAND test_compress "${decompressed_file}" "${compressed_file}")

    add_test(NAME "DecompressFile.${name}"
            COMMAND test_decompress "${compressed_file}" "${decompressed_file}")
endfunction()

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
            yaz0_add_test("${TEST_NAME}"
                    "${YAZ0_TEST_DATA_DIR}/${TEST_BASE}.szs"
                    "${YAZ0_TEST_DATA_DIR}/${TEST_BASE}.bin")
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
