/* test_compress.c: decompression test fixture
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

int main(void) {
   uint8_t const data[] = {
      0x59, 0x61, 0x7A, 0x30, // Yaz0 magic
      0x00, 0x00, 0x00, 0x00, // Uncompressed size
      0x00, 0x00, 0x00, 0x00, // Alignment
      0x00, 0x00, 0x00, 0x00, // Reserved
  };

   struct run_result const run = run_decompress(data, sizeof data);

   if (run.result < YAZ0_OK) {
      fprintf(
          stderr,
          "FAILED: Expected run.result >= YAZ0_OK (0)\n"
          "          Actual run.result  = %d\n",
          run.result
      );
      return EXIT_FAILURE;
   }

   if (run.total_in != sizeof data) {
      fprintf(
          stderr,
          "FAILED: Expected run.total_in = %zu\n"
          "          Actual run.total_in = %zu\n",
          sizeof data,
          run.total_in
      );
      return EXIT_FAILURE;
   }

   if (run.total_out != 0) {
      fprintf(
          stderr,
          "FAILED: Expected run.total_out = %zu\n"
          "          Actual run.total_out = %zu\n",
          0,
          run.total_out
      );
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}
