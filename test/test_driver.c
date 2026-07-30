/* test_driver.c: test helper functions
   Copyright (C) 2026 Jesse Gerard Brands

   This file is part of libyaz0.

   libyaz0 is free software: you can redistribute it and/or modify it under
   the terms of the GNU Lesser General Public License as published by the Free
   Software Foundation, either version 3 of the License, or (at your option)
   any later version.

   libyaz0 is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
   more details.

   You should have received a copy of the GNU Lesser General Public License
   along with libyaz0. If not, see <https://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_driver.h"

struct run_result
run_compress(uint8_t const* data, size_t const size, int const level) {
    struct run_result run = {0};

    if (size > UINT32_MAX) {
        fprintf(
            stderr,
            "ERROR:  Input data is greater than UINT32_MAX!\n"
            "        Yaz0 cannot encode sizes above UINT32_MAX!\n"
        );
        run.result = YAZ0_STREAM_ERROR;
        return run;
    }

    struct yaz0_stream stream = {0};
    run.result = yaz0_compress_init(&stream, level, size);
    if (run.result != YAZ0_OK) {
        fprintf(stderr, "FAILED: Could not initialize compressor\n");
        return run;
    }

    /* The literal worst case scenario is all literals. In such a case we'd
       add an extra byte for every 8 bytes, resulting in a compression ratio
       of 1.125.

       The formula to calculate the size of an incompressible file is:
         16 + size + ceil(size/8)
     */
    size_t const out_size = 16 + size + (size + 7) / 8;

    run.out = malloc(out_size);
    if (run.out == NULL) {
        fprintf(stderr, "ERROR:  Failed to allocate output buffer\n");
        run.result = YAZ0_MEMORY_ERROR;
        goto cleanup_stream;
    }

    stream.next_in = data;
    stream.avail_in = size;
    stream.next_out = run.out;
    stream.avail_out = out_size;

    run.result = yaz0_compress(&stream, YAZ0_FINISH);
    run.total_in = stream.total_in;
    run.total_out = stream.total_out;

cleanup_stream:
    yaz0_compress_end(&stream);
    return run;
}

struct run_result run_decompress(uint8_t const* data, size_t size) {
    struct run_result run = {0};

    if (size < 16) {
        fprintf(stderr, "FAILED: Input is too small to be a Yaz0 stream\n");
        run.result = YAZ0_DATA_ERROR;
        return run;
    }

    if (memcmp(data, "Yaz0", 4) != 0) {
        fprintf(stderr, "FAILED: Input is not a Yaz0 stream\n");
        run.result = YAZ0_DATA_ERROR;
        return run;
    }

    struct yaz0_stream stream = {0};
    run.result = yaz0_decompress_init(&stream);
    if (run.result != YAZ0_OK) {
        fprintf(stderr, "FAILED: Could not initialize compressor\n");
        return run;
    }

    // TODO: Implement proper library API for this at some point.
    size_t const out_size =
            ((size_t) data[4] << 24) | ((size_t) data[5] << 16) |
            ((size_t) data[6] << 8) | ((size_t) data[7]);

    run.out = malloc(out_size > 0 ? out_size : 1);
    if (run.out == NULL) {
        fprintf(stderr, "ERROR:  Failed to allocate output buffer\n");
        run.result = YAZ0_MEMORY_ERROR;
        goto cleanup_stream;
    }

    stream.next_in = data;
    stream.avail_in = size;
    stream.next_out = run.out;
    stream.avail_out = out_size;

    run.result = yaz0_decompress(&stream, YAZ0_FINISH);
    run.total_in = stream.total_in;
    run.total_out = stream.total_out;

cleanup_stream:
    yaz0_decompress_end(&stream);
    return run;
}

bool
assert_run(struct run_result run, size_t const expected_in, uint8_t const* expected, size_t const expected_size) {
    if (run.result < YAZ0_OK) {
        fprintf(
            stderr,
            "FAILED: Expected run.result >= YAZ0_OK (0)\n"
            "          Actual run.result  = %d\n",
            run.result
        );
        return false;
    }

    // Files extracted from Nintendo 64 ROMs are padded to a multiple of 16
    // bytes. Besides, we really only care that we don't exceed expected_in.
    if (run.total_in > expected_in) {
        fprintf(
            stderr,
            "FAILED: Expected run.total_in <= %zu\n"
            "          Actual run.total_in  = %zu\n",
            expected_in,
            run.total_in
        );
        return false;
    }

    if (run.total_out != expected_size) {
        fprintf(
            stderr,
            "FAILED: Expected run.total_out = %zu\n"
            "          Actual run.total_out = %zu\n",
            expected_size,
            run.total_out
        );
        return false;
    }

    for (size_t i = 0; i < expected_size; ++i) {
        if (expected[i] == run.out[i]) {
            continue;
        }
        fprintf(
            stderr,
            "FAILED: Expected run.out[%zu] = 0x%02X\n"
            "          Actual run.out[%zu] = 0x%02X\n",
            i, expected[i],
            i, run.out[i]
        );
        return false;
    }

    return true;
}

bool
assert_compress(uint8_t const* data, size_t const data_size, int const level,
                uint8_t const* expected, size_t const expected_size) {
    struct run_result const run = run_compress(data, data_size, level);
    bool const passed = assert_run(run, data_size, expected, expected_size);
    free(run.out);
    return passed;
}

bool
assert_decompress(uint8_t const* data, size_t const data_size,
                  uint8_t const* expected, size_t const expected_size) {
    struct run_result const run = run_decompress(data, data_size);
    bool const passed = assert_run(run, data_size, expected, expected_size);
    free(run.out);
    return passed;
}

static bool
load_file(char const* filename, uint8_t** data, size_t* size) {
    assert(filename != NULL);
    assert(data != NULL);
    assert(size != NULL);

    *data = NULL;
    *size = 0;

    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        return false;
    }

    bool passed = false;
    uint8_t* buffer = NULL;

    fseek(file, 0, SEEK_END);
    long const file_size = ftell(file);
    if (file_size < 0) {
        goto cleanup_file;
    }

    // Empty files are valid.
    if (file_size == 0) {
        passed = true;
        goto cleanup_file;
    }

    rewind(file);
    buffer = malloc((size_t) file_size);
    if (buffer == NULL) {
        goto cleanup_buffer;
    }

    if (fread(buffer, 1, (size_t) file_size, file) != (size_t) file_size) {
        goto cleanup_buffer;
    }

    *data = buffer;
    *size = (size_t) file_size;
    buffer = NULL;
    passed = true;

cleanup_buffer:
    free(buffer);
cleanup_file:
    fclose(file);
    return passed;
}

bool
assert_compress_file(char const* filename, int const level, char const* expected_filename) {
    assert(filename != NULL);
    assert(expected_filename != NULL);

    bool passed = false;
    uint8_t* data = NULL;
    size_t data_size = 0;

    if (!load_file(filename, &data, &data_size)) {
        fprintf(stderr, "ERROR: Could not open input file '%s'\n", filename);
        goto cleanup_input;
    }

    uint8_t* expected = NULL;
    size_t expected_size = 0;

    if (!load_file(expected_filename, &expected, &expected_size)) {
        fprintf(stderr, "ERROR: Could not open verification file '%s'\n", expected_filename);
        goto cleanup_input;
    }

    passed = assert_compress(data, data_size, level, expected, expected_size);
    free(expected);

cleanup_input:
    free(data);
    return passed;
}

bool
assert_decompress_file(char const* filename, char const* expected_filename) {
    assert(filename != NULL);
    assert(expected_filename != NULL);

    bool passed = false;

    uint8_t* data = NULL;
    size_t data_size = 0;

    if (!load_file(filename, &data, &data_size)) {
        fprintf(stderr, "ERROR: Could not open input file '%s'\n", filename);
        goto cleanup_input;
    }

    uint8_t* expected = NULL;
    size_t expected_size = 0;

    if (!load_file(expected_filename, &expected, &expected_size)) {
        fprintf(stderr, "ERROR: Could not open verification file '%s'\n", expected_filename);
        goto cleanup_input;
    }

    passed = assert_decompress(data, data_size, expected, expected_size);
    free(expected);

cleanup_input:
    free(data);
    return passed;
}
