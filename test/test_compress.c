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

static int
usage(void) {
    fprintf(
        stderr,
        "usage: test_compress"
        " [-l level] [-i bytes] [-o bytes] [-r code] file expected_file\n"

        "  -l level  compression level\n"
        "  -i bytes  input chunk size\n"
        "  -o bytes  output chunk size\n"
        "  -r code   expected result code \n"
    );

    return EXIT_FAILURE;
}

int main(int argc, char** argv) {
    struct compress_fixture fixture = {0};
    if (!parse_compress_fixture(argc, argv, &fixture)) {
        return usage();
    }

    struct run_result const run = run_compress(
        fixture.test.data,
        fixture.test.size,
        fixture.compression_level
    );

    bool passed = assert_run(run, fixture.test.expected_result);

    if (!assert_total_in(run, fixture.test.size, 0)) {
        passed = false;
    }
    if (!assert_total_out(run, fixture.test.expected_size, 16)) {
        passed = false;
    }
    if (!assert_out(run, fixture.test.expected, fixture.test.expected_size)) {
        passed = false;
    }

    free(run.out);
    free_test_fixture(&fixture.test);
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
