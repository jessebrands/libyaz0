/* test_utility.c: test utility functions
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
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_driver.h"

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

static bool
parse_result_arg(char const* text, enum yaz0_result* out) {
    if (text == NULL || *text == '\0') {
        return false;
    }

    errno = 0;
    char* end = NULL;
    long const value = strtol(text, &end, 10);

    if (*end != '\0' || errno == ERANGE || value < INT_MIN || value > INT_MAX) {
        return false;
    }

    *out = (enum yaz0_result) value;
    return true;
}

static bool
parse_size_arg(char const* arg, size_t* out) {
    if (arg == NULL || *arg == '\0' || *arg == '-') {
        return false;
    }

    errno = 0;
    char* end = NULL;
    unsigned long long const value = strtoull(arg, &end, 10);

    if (*end != '\0' || errno == ERANGE || value > SIZE_MAX) {
        return false;
    }

    *out = (size_t) value;
    return true;
}

static bool
parse_level_arg(char const* arg, int* out) {
    if (arg == NULL || *arg == '\0') {
        return false;
    }

    errno = 0;
    char* end = NULL;
    long const value = strtol(arg, &end, 10);

    if (*end != '\0' || errno == ERANGE) {
        return false;
    }

    *out = (int) value;

    if (*out < 0 || *out > 9) {
        return false;
    }

    return true;
}

static bool
parse_common_option(char const* arg, char const* value,
                    struct test_fixture* fixture, bool* handled) {
    *handled = true;
    if (strcmp(arg, "-i") == 0) return parse_size_arg(value, &fixture->in_chunk);
    if (strcmp(arg, "-o") == 0) return parse_size_arg(value, &fixture->out_chunk);
    if (strcmp(arg, "-r") == 0) return parse_result_arg(value, &fixture->expected_result);
    *handled = false;
    return false;
}

bool
parse_file_args(int argc, char** argv, struct test_fixture* fixture) {
    char const* in_filename = NULL;
    char const* expected_filename = NULL;

    for (int i = 1; i < argc; ++i) {
        char const* const arg = argv[i];
        if (arg[0] == '-' && arg[1] != '\0') {
            ++i;
            continue;
        }

        if (in_filename == NULL) {
            in_filename = arg;
        } else if (expected_filename == NULL) {
            expected_filename = arg;
        } else {
            return false;
        }
    }

    if (in_filename == NULL || expected_filename == NULL) {
        return false;
    }

    if (!load_file(in_filename, &fixture->data, &fixture->size)) {
        fprintf(stderr, "ERROR: Could not load input file '%s'\n", in_filename);
        return false;
    }

    if (!load_file(expected_filename, &fixture->expected, &fixture->expected_size)) {
        fprintf(stderr, "ERROR: Could not load verification file '%s'\n", expected_filename);
        free(fixture->data);
        fixture->data = NULL;
        fixture->size = 0;
        return false;
    }

    return true;
}

bool
parse_compress_fixture(int argc, char** argv, struct compress_fixture* fixture) {
    fixture->test.expected_result = YAZ0_STREAM_END;
    fixture->compression_level = YAZ0_DEFAULT_COMPRESSION;

    for (int i = 1; i < argc; ++i) {
        char const* const arg = argv[i];

        if (arg[0] != '-' || arg[1] == '\0') {
            continue;
        }

        if (i + 1 >= argc) {
            return false;
        }

        char const* const value = argv[++i];
        bool handled = false;
        bool const ok = parse_common_option(arg, value, &fixture->test, &handled);

        if (handled) {
            if (!ok) {
                return false;
            }
            continue;
        }

        if (strcmp(arg, "-l") == 0) {
            if (!parse_level_arg(value, &fixture->compression_level)) {
                return false;
            }
            continue;
        }

        return false;
    }

    if (!parse_file_args(argc, argv, &fixture->test)) {
        return false;
    }

    return true;
}

bool
parse_decompress_fixture(int argc, char** argv, struct decompress_fixture* fixture) {
    fixture->test.expected_result = YAZ0_STREAM_END;

    for (int i = 1; i < argc; ++i) {
        char const* const arg = argv[i];

        if (arg[0] != '-' || arg[1] == '\0') {
            continue;
        }

        if (i + 1 >= argc) {
            return false;
        }

        char const* const value = argv[++i];
        bool handled = false;
        bool const ok = parse_common_option(arg, value, &fixture->test, &handled);

        if (handled) {
            if (!ok) {
                return false;
            }
            continue;
        }

        return false;
    }

    if (!parse_file_args(argc, argv, &fixture->test)) {
        return false;
    }

    return true;
}

void free_test_fixture(struct test_fixture* fixture) {
    free(fixture->data);
    free(fixture->expected);
}
