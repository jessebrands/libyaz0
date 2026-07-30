/* yaz0.c
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

#include <string.h>

#include "common.h"
#include "yaz0/yaz0.h"

static inline uint32_t read_be32(uint8_t const* p) {
    return ((uint32_t) p[0] << 24)
           | ((uint32_t) p[1] << 16)
           | ((uint32_t) p[2] << 8)
           | (uint32_t) p[3];
}

static inline void write_be32(uint8_t* p, uint32_t const v) {
    p[0] = (uint8_t) (v >> 24);
    p[1] = (uint8_t) (v >> 16);
    p[2] = (uint8_t) (v >> 8);
    p[3] = (uint8_t) v;
}

enum yaz0_result
yaz0_read_header(uint8_t const* data, size_t const size, struct yaz0_header* header) {
    if (size < 16) {
        return YAZ0_BUFFER_ERROR;
    }

    if (memcmp(data, YAZ0_MAGIC, 4) != 0) {
        return YAZ0_BAD_HEADER;
    }

    memcpy(header->magic, data, 4);
    header->uncompressed_size = read_be32(data + 4);
    header->alignment = read_be32(data + 8);
    memcpy(header->reserved, data + 12, 4);

    return YAZ0_OK;
}

enum yaz0_result
yaz0_write_header(struct yaz0_header const* header, uint8_t* dst, size_t const size) {
    if (size < 16) {
        return YAZ0_BUFFER_ERROR;
    }

    memcpy(dst, YAZ0_MAGIC, 4);
    write_be32(dst + 4, header->uncompressed_size);
    write_be32(dst + 8, header->alignment);
    memcpy(dst + 12, header->reserved, 4);
    return YAZ0_OK;
}
