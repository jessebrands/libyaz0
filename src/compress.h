/* compress.h: internal compression API
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

#ifndef LIBYAZ0_COMPRESS_H
#define LIBYAZ0_COMPRESS_H

#include "common.h"
#include "yaz0/yaz0.h"

enum yaz0_compress_mode {
   YAZ0_COMPRESS_HEADER,
   YAZ0_COMPRESS_ERROR,
   YAZ0_COMPRESS_DONE,
};

struct yaz0_compress_state {
   struct yaz0_common_state common;
   enum yaz0_compress_mode mode;
   enum yaz0_result error;

   int level;
   uint32_t uncompressed_size;

   uint8_t window[YAZ0_MAX_DISTANCE * 2 + YAZ0_MAX_MATCH];
   size_t window_pos;
};

#endif //LIBYAZ0_COMPRESS_H
