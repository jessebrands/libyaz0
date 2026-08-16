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
        " [-l level] [-i bytes] [-o bytes] [-r code] [-p bytes] [-m matcher] file [expected_file]\n"

        "  -m matcher  matcher algorithm\n"
        "  -l level    compression level\n"
        "  -i bytes    input chunk size\n"
        "  -o bytes    output chunk size\n"
        "  -r code     expected result code \n"
        "  -p bytes    allowed alignment padding on output size\n"
    );

    return EXIT_FAILURE;
}

int main(int argc, char** argv) {
    struct compress_fixture fixture = {0};
    if (!parse_compress_fixture(argc, argv, &fixture)) {
        return usage();
    }

    struct run_result const run = run_compress_chunked_with_matcher(
        fixture.test.data,
        fixture.test.size,
        fixture.compression_level,
        fixture.test.in_chunk,
        fixture.test.out_chunk,
        fixture.matcher
    );

    if (run.result == YAZ0_UNSUPPORTED && fixture.test.expected_result != YAZ0_UNSUPPORTED) {
        fprintf(stderr, "SKIP:   matcher '%s' is unavailable in this build\n",
                yaz0_matcher_name(fixture.matcher));
        free(run.out);
        free_test_fixture(&fixture.test);
        return 2;
    }

    bool passed = assert_run(run, fixture.test.expected_result);

    if (!assert_total_in(run, fixture.test.size, 0)) {
        passed = false;
    }

    if (fixture.test.expected != NULL) {
        // A reference file was supplied, so the matcher must reproduce it.
        if (!assert_total_out(run, fixture.test.expected_size, fixture.output_padding)) {
            passed = false;
        }
        if (!assert_out(run, fixture.test.expected, fixture.test.expected_size)) {
            passed = false;
        }
    } else {
        // So long as the result can be decompressed to the original data, that's a pass.
        struct run_result const restored = run_decompress_chunked(
            run.out,
            run.total_out,
            fixture.test.in_chunk,
            fixture.test.out_chunk
        );

        if (!assert_run(restored, YAZ0_STREAM_END)) {
            passed = false;
        }
        if (!assert_total_out(restored, fixture.test.size, 0)) {
            passed = false;
        }
        if (!assert_out(restored, fixture.test.data, fixture.test.size)) {
            passed = false;
        }

        free(restored.out);
    }

    free(run.out);
    free_test_fixture(&fixture.test);
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
