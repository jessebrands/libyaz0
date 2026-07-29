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

#include <stddef.h>
#include <stdint.h>

#include "yaz0/yaz0.h"

struct run_result {
    enum yaz0_result result;
    uint8_t* out;
    size_t total_in;
    size_t total_out;
};

/*
   Compresses data at the given compression level.
 */
struct run_result
run_compress(uint8_t const* data, size_t size, int level);

#endif //LIBYAZ0_TEST_DRIVER_H
