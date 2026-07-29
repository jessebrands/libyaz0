/* decompress.c: Yaz0 decompressor implementation
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

#include "decompress.h"

enum yaz0_result
yaz0_decompress_init(struct yaz0_stream* stream) {
    return YAZ0_STREAM_ERROR;
}

enum yaz0_result
yaz0_decompress(struct yaz0_stream* stream, enum yaz0_flush const flush) {
    return YAZ0_STREAM_ERROR;
}

void
yaz0_decompress_end(struct yaz0_stream* stream) {
}
