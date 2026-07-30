/* decompress.h: internal decompression API
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

#ifndef LIBYAZ0_DECOMPRESS_H
#define LIBYAZ0_DECOMPRESS_H

#include "yaz0/yaz0.h"

enum yaz0_decompress_mode {
   YAZ0_DECOMPRESS_DONE,
   YAZ0_DECOMPRESS_ERROR,
};

struct yaz0_decompress_state {
   struct yaz0_common_state common;
   enum yaz0_decompress_mode mode;
   enum yaz0_result error;
};

#endif //LIBYAZ0_DECOMPRESS_H
