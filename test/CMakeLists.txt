add_library(yaz0test STATIC)
target_link_libraries(yaz0test PUBLIC yaz0-static)
target_sources(yaz0test PRIVATE
        test_driver.c)

set_target_properties(yaz0test PROPERTIES DEBUG_POSTFIX "d")

add_executable(test_compress test_compress.c)
target_link_libraries(test_compress PRIVATE yaz0test)

add_executable(test_decompress test_decompress.c)
target_link_libraries(test_decompress PRIVATE yaz0test)

add_test(
        NAME Compress
        COMMAND test_compress
)

add_test(
        NAME Decompress
        COMMAND test_decompress
)
