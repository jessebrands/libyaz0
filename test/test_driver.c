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
run_compress_chunked(uint8_t const* data, size_t const size, int const level,
                     size_t const in_chunk, size_t const out_chunk) {
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
    size_t const window_in = in_chunk > 0 ? in_chunk : size;
    size_t const window_out = out_chunk > 0 ? out_chunk : out_size;

    run.out = malloc(out_size);
    if (run.out == NULL) {
        fprintf(stderr, "ERROR:  Failed to allocate output buffer\n");
        run.result = YAZ0_MEMORY_ERROR;
        goto cleanup_stream;
    }

    stream.next_in = data;
    stream.next_out = run.out;

    size_t const max_iterations = size + out_size + 64;

    for (size_t i = 0; i < max_iterations; ++i) {
        size_t const remaining_in = size - stream.total_in;
        size_t const remaining_out = out_size - stream.total_out;
        size_t const give = window_in < remaining_in ? window_in : remaining_in;
        size_t const want = window_out < remaining_out ? window_out : remaining_out;

        stream.avail_in = give;
        stream.avail_out = want;

        enum yaz0_flush const flush = stream.total_in + give >= size
                                          ? YAZ0_FINISH
                                          : YAZ0_NO_FLUSH;

        run.result = yaz0_compress(&stream, flush);

        if (run.result != YAZ0_OK) {
            break;
        }
    }

    run.total_in = stream.total_in;
    run.total_out = stream.total_out;

cleanup_stream:
    yaz0_compress_end(&stream);
    return run;
}

struct run_result run_decompress_chunked(uint8_t const* data, size_t const size,
                                         size_t const in_chunk, size_t const out_chunk) {
    struct run_result run = {0};

    struct yaz0_header header = {0};
    run.result = yaz0_read_header(data, size, &header);
    if (run.result != YAZ0_OK) {
        fprintf(stderr, "FAILED: Input is not a Yaz0 file\n");
        return run;
    }

    if (memcmp(header.magic, "Yaz0", 4) != 0) {
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

    size_t const out_size = header.uncompressed_size;
    size_t const window_in = in_chunk > 0 ? in_chunk : size;
    size_t const window_out = out_chunk > 0 ? out_chunk : out_size;

    run.out = malloc(out_size > 0 ? out_size : 1);
    if (run.out == NULL) {
        fprintf(stderr, "ERROR:  Failed to allocate output buffer\n");
        run.result = YAZ0_MEMORY_ERROR;
        goto cleanup_stream;
    }

    stream.next_in = data;
    stream.next_out = run.out;

    size_t const max_iterations = size + out_size + 64;

    for (size_t i = 0; i < max_iterations; ++i) {
        size_t const remaining_in = size - stream.total_in;
        size_t const remaining_out = out_size - stream.total_out;
        size_t const give = window_in < remaining_in ? window_in : remaining_in;
        size_t const want = window_out < remaining_out ? window_out : remaining_out;

        stream.avail_in = give;
        stream.avail_out = want;

        enum yaz0_flush const flush = stream.total_in + give >= size
                                          ? YAZ0_FINISH
                                          : YAZ0_NO_FLUSH;

        run.result = yaz0_decompress(&stream, flush);

        if (run.result != YAZ0_OK) {
            break;
        }
    }

    run.total_in = stream.total_in;
    run.total_out = stream.total_out;

cleanup_stream:
    yaz0_decompress_end(&stream);
    return run;
}

bool
assert_run(struct run_result const run, enum yaz0_result const expected) {
    if (run.result != expected) {
        fprintf(
            stderr,
            "FAILED: Expected run.result = %s (%d)\n"
            "          Actual run.result = %s (%d)\n",
            yaz0_result_name(expected), expected,
            yaz0_result_name(run.result), run.result
        );

        return false;
    }

    return true;
}

bool
assert_total_in(struct run_result const run, size_t const expected, size_t const lenience) {
    if (lenience > 0) {
        if (run.total_in > expected || expected - run.total_in >= lenience) {
            fprintf(
                stderr,
                "FAILED: Expected run.total_in <= %zu and %zu - run.total_in < %zu\n"
                "          Actual run.total_in  = %zu\n",
                expected, expected, lenience,
                run.total_in
            );
            return false;
        }
    } else {
        if (run.total_in != expected) {
            fprintf(
                stderr,
                "FAILED: Expected run.total_in = %zu\n"
                "          Actual run.total_in = %zu\n",
                expected,
                run.total_in
            );
            return false;
        }
    }

    return true;
}

bool
assert_total_out(struct run_result const run, size_t const expected, size_t const lenience) {
    if (lenience > 0) {
        if (run.total_out > expected || expected - run.total_out >= lenience) {
            fprintf(
                stderr,
                "FAILED: Expected run.total_out <= %zu and %zu - run.total_out < %zu\n"
                "          Actual run.total_out  = %zu\n",
                expected, expected, lenience,
                run.total_out
            );
            return false;
        }
    } else {
        if (run.total_out != expected) {
            fprintf(
                stderr,
                "FAILED: Expected run.total_out = %zu\n"
                "          Actual run.total_out = %zu\n",
                expected,
                run.total_out
            );
            return false;
        }
    }

    return true;
}

bool
assert_out(struct run_result const run, uint8_t const* expected, size_t const expected_size) {
    if (run.out == NULL || expected == NULL) {
        return true;
    }

    size_t const count = run.total_out < expected_size
                             ? run.total_out
                             : expected_size;

    for (size_t i = 0; i < count; ++i) {
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
