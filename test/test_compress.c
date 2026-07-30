/* test_compress.c: compression test fixture
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

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: test_compress [file] [expected_file]\n");
        return EXIT_FAILURE;
    }

    char const* in_filename = argv[1];
    char const* expected_filename = argv[2];

    return assert_compress_file(in_filename, YAZ0_DEFAULT_COMPRESSION, expected_filename)
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
