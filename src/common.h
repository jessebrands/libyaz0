/* common.h: common library utilities and allocator
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

#ifndef LIBYAZ0_COMMON_H
#define LIBYAZ0_COMMON_H

#include <stddef.h>

#include "yaz0/yaz0.h"

#define YAZ0_MAGIC "Yaz0"
#define YAZ0_MAX_DISTANCE 4096
#define YAZ0_TOKENS_PER_BLOCK 8

#define YAZ0_DISTANCE_BIAS 1
#define YAZ0_SHORT_LENGTH_BIAS 2
#define YAZ0_LONG_LENGTH_BIAS 18

enum yaz0_step {
    YAZ0_STEP_CONTINUE,
    YAZ0_STEP_RETURN,
};

enum yaz0_kind {
    YAZ0_KIND_COMPRESSOR = 1,
    YAZ0_KIND_DECOMPRESSOR = 2,
};

struct yaz0_common_state {
    struct yaz0_stream* stream;
    enum yaz0_kind kind;

    void* opaque;
    yaz0_alloc_func alloc;
    yaz0_free_func free;
};

void* yaz0_alloc(struct yaz0_stream const* stream, size_t size);

void yaz0_free(struct yaz0_stream const* stream, void* ptr);

#endif //LIBYAZ0_COMMON_H
