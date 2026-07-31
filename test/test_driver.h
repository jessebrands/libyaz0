/* test_driver.h: test helper functions
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

#ifndef LIBYAZ0_TEST_DRIVER_H
#define LIBYAZ0_TEST_DRIVER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "yaz0/yaz0.h"

struct run_result {
    enum yaz0_result result;
    uint8_t* out;
    size_t total_in;
    size_t total_out;
};

struct test_fixture {
    enum yaz0_result expected_result;
    size_t in_chunk;
    size_t out_chunk;

    uint8_t* data;
    size_t size;
    uint8_t* expected;
    size_t expected_size;
};

struct compress_fixture {
    struct test_fixture test;
    int compression_level;
    size_t output_padding;
};

struct decompress_fixture {
    struct test_fixture test;
    size_t input_padding;
};

bool
parse_compress_fixture(int argc, char** argv, struct compress_fixture* fixture);

bool
parse_decompress_fixture(int argc, char** argv, struct decompress_fixture* fixture);

void
free_test_fixture(struct test_fixture* fixture);

/*
   Compresses data at the given compression level.
 */
struct run_result
run_compress(uint8_t const* data, size_t size, int level);

struct run_result
run_decompress_chunked(uint8_t const* data, size_t size, size_t in_chunk, size_t out_chunk);

bool
assert_run(struct run_result run, enum yaz0_result expected);

bool
assert_total_in(struct run_result run, size_t expected, size_t lenience);

bool
assert_total_out(struct run_result run, size_t expected, size_t lenience);

bool
assert_out(struct run_result run, uint8_t const* expected, size_t expected_size);

#endif //LIBYAZ0_TEST_DRIVER_H
