/* yaz0.h: public API header
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

#ifndef LIBYAZ0_YAZ0_H
#define LIBYAZ0_YAZ0_H

#include <stddef.h>
#include <stdint.h>

#if defined __cplusplus
extern "C" {
#endif

typedef void* (* yaz0_alloc_func)(void* opaque, size_t size);

typedef void (* yaz0_free_func)(void* opaque, void* ptr);

enum yaz0_result {
    YAZ0_OK = 0,
    YAZ0_STREAM_END = 1,
    YAZ0_ERRNO = -1,
    YAZ0_STREAM_ERROR = -2,
    YAZ0_DATA_ERROR = -3,
    YAZ0_MEMORY_ERROR = -4,
    YAZ0_BUFFER_ERROR = -5,
    YAZ0_TRUNCATED = -6,
    YAZ0_BAD_HEADER = -7,
    YAZ0_SIZE_MISMATCH = -8,
};

enum yaz0_flush {
    YAZ0_NO_FLUSH = 0,
    YAZ0_FINISH = 1,
};

enum yaz0_level {
    YAZ0_DEFAULT_COMPRESSION = -1,
    YAZ0_NO_COMPRESSION = 0,
    YAZ0_BEST_COMPRESSION = 9,
};

struct yaz0_header {
    char magic[4];
    uint32_t uncompressed_size;
    uint32_t alignment;
    char reserved[4];
};

struct yaz0_stream {
    uint8_t const* next_in;
    size_t avail_in;
    size_t total_in;

    uint8_t* next_out;
    size_t avail_out;
    size_t total_out;

    void* opaque;
    yaz0_alloc_func alloc;
    yaz0_free_func free;

    void* state;
};

enum yaz0_result
yaz0_compress_init(struct yaz0_stream* stream, int level,
                   uint32_t uncompressed_size);

enum yaz0_result
yaz0_compress(struct yaz0_stream* stream, enum yaz0_flush flush);

void
yaz0_compress_end(struct yaz0_stream* stream);

enum yaz0_result
yaz0_decompress_init(struct yaz0_stream* stream);

enum yaz0_result
yaz0_decompress(struct yaz0_stream* stream, enum yaz0_flush flush);

void
yaz0_decompress_end(struct yaz0_stream* stream);

enum yaz0_result
yaz0_read_header(uint8_t const* data, size_t size, struct yaz0_header* header);

enum yaz0_result
yaz0_write_header(struct yaz0_header const* header, uint8_t* dst, size_t size);

char const*
yaz0_result_name(enum yaz0_result result);

#if defined __cplusplus
}
#endif

#endif // LIBYAZ0_YAZ0_H
