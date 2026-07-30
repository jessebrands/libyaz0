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

#include <stdio.h>
#include <stdlib.h>

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

    struct yaz0_stream stream = {0};
    run.result = yaz0_decompress_init(&stream);
    if (run.result != YAZ0_OK) {
        fprintf(stderr, "FAILED: Could not initialize compressor\n");
        return run;
    }

    // TODO: There are better ways to do this, but the output can never
    //       be smaller than the input, so this is ok.
    size_t const out_size = size;

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

    run.result = yaz0_decompress(&stream, YAZ0_FINISH);
    run.total_in = stream.total_in;
    run.total_out = stream.total_out;

cleanup_stream:
    yaz0_decompress_end(&stream);
    return run;
}
