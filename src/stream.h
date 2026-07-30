/* stream.h: yaz0_stream convenience functions
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

#ifndef LIBYAZ0_STREAM_H
#define LIBYAZ0_STREAM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "yaz0/yaz0.h"

enum yaz0_io_result {
    YAZ0_IO_OK = 0,
    YAZ0_IO_READ_ERROR = -101,
    YAZ0_IO_WRITE_ERROR = -102,
};

#define YAZ0_IO_SUCCESS(x) ((x) == YAZ0_IO_OK)

static inline bool yaz0_stream_read_available(struct yaz0_stream const* stream) {
    return stream->avail_in > 0;
}

static inline bool yaz0_stream_write_available(struct yaz0_stream const* stream) {
    return stream->avail_out > 0;
}

size_t yaz0_stream_read(struct yaz0_stream* stream, uint8_t* dest, size_t length);

size_t yaz0_stream_write(struct yaz0_stream* stream, uint8_t const* src, size_t length);

enum yaz0_io_result yaz0_stream_read_byte(struct yaz0_stream* stream, uint8_t* b);

uint8_t yaz0_stream_read_byte_unsafe(struct yaz0_stream* stream);

enum yaz0_io_result yaz0_stream_write_byte(struct yaz0_stream* stream, uint8_t b);

void yaz0_stream_write_byte_unsafe(struct yaz0_stream* stream, uint8_t b);

uint8_t yaz0_stream_copy_byte_unsafe(struct yaz0_stream* stream);

enum yaz0_io_result yaz0_stream_copy_byte(struct yaz0_stream* stream, uint8_t* b);

#endif //LIBYAZ0_STREAM_H
