/* stream.c: yaz0_stream convenience functions
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

#include <assert.h>
#include <string.h>

#include "stream.h"

bool yaz0_stream_valid(struct yaz0_stream const* stream) {
    return stream != NULL &&
           !((stream->avail_in != 0 && stream->next_in == NULL) ||
             (stream->avail_out != 0 && stream->next_out == NULL));
}

size_t yaz0_stream_read(struct yaz0_stream* stream, uint8_t* dest, size_t const length) {
    assert(dest != NULL);

    size_t const have = length > stream->avail_in ? stream->avail_in : length;

    // memcpy is undefined on a NULL pointer, even at zero length.
    if (have == 0) {
        return 0;
    }

    memcpy(dest, stream->next_in, have);
    stream->next_in += have;
    stream->avail_in -= have;
    stream->total_in += have;
    return have;
}

uint8_t yaz0_stream_read_byte_unsafe(struct yaz0_stream* stream) {
    assert(stream->avail_in > 0 && stream->next_in != NULL);

    uint8_t const b = *stream->next_in;
    stream->next_in++;
    stream->avail_in--;
    stream->total_in++;
    return b;
}

enum yaz0_io_result yaz0_stream_read_byte(struct yaz0_stream* stream, uint8_t* b) {
    if (!yaz0_stream_read_available(stream)) {
        return YAZ0_IO_READ_ERROR;
    }
    *b = yaz0_stream_read_byte_unsafe(stream);
    return YAZ0_IO_OK;
}

size_t yaz0_stream_write(struct yaz0_stream* stream, uint8_t const* src, size_t const length) {
    assert(src != NULL);

    size_t const have = length > stream->avail_out ? stream->avail_out : length;

    // memcpy is undefined on a NULL pointer, even at zero length.
    if (have == 0) {
        return 0;
    }

    memcpy(stream->next_out, src, have);
    stream->next_out += have;
    stream->avail_out -= have;
    stream->total_out += have;
    return have;
}

void yaz0_stream_write_byte_unsafe(struct yaz0_stream* stream, uint8_t const b) {
    assert(stream->avail_out > 0 && stream->next_out != NULL);

    *stream->next_out = b;
    stream->next_out++;
    stream->avail_out--;
    stream->total_out++;
}

enum yaz0_io_result yaz0_stream_write_byte(struct yaz0_stream* stream, uint8_t const b) {
    if (!yaz0_stream_write_available(stream)) {
        return YAZ0_IO_WRITE_ERROR;
    }
    yaz0_stream_write_byte_unsafe(stream, b);
    return YAZ0_IO_OK;
}

uint8_t yaz0_stream_copy_byte_unsafe(struct yaz0_stream* stream) {
    assert(stream->avail_in > 0 && stream->next_in != NULL);
    assert(stream->avail_out > 0 && stream->next_out != NULL);

    uint8_t const b = yaz0_stream_read_byte_unsafe(stream);
    yaz0_stream_write_byte_unsafe(stream, b);
    return b;
}

enum yaz0_io_result yaz0_stream_copy_byte(struct yaz0_stream* stream, uint8_t* b) {
    if (!yaz0_stream_read_available(stream)) {
        return YAZ0_IO_READ_ERROR;
    }
    if (!yaz0_stream_write_available(stream)) {
        return YAZ0_IO_WRITE_ERROR;
    }
    *b = yaz0_stream_copy_byte_unsafe(stream);
    return YAZ0_IO_OK;
}
