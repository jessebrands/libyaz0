/*
 * This file is part of Zelda64.
 *
 * Zelda64 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Zelda64 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Zelda64. If not, see <https://www.gnu.org/licenses/>.
 */

#include "byteswap.h"
#include "yaz0.h"

#include <assert.h>
#include <string.h>

enum yaz0_result yaz0_read_header(uint8_t const* restrict data, size_t const size,
                                  struct yaz0_header* restrict header) {
        static_assert(sizeof(struct yaz0_header) == 16);
        if (size < sizeof(struct yaz0_header)) {
                return YAZ0_OUT_OF_RANGE;
        }
        memcpy(header, data, sizeof(*header));
        return YAZ0_OK;
}

enum yaz0_result yaz0_write_header(uint8_t* data, size_t const size, uint32_t const uncompressed_size,
                                   uint32_t const alignment) {
        static_assert(sizeof(struct yaz0_header) == 16);
        if (size < sizeof(struct yaz0_header)) {
                return YAZ0_DESTINATION_TOO_SMALL;
        }
        struct yaz0_header const header = {
            .magic = YAZ0_MAGIC,
            .uncompressed_size = yaz0_to_be_u32(uncompressed_size),
            .alignment = yaz0_to_be_u32(alignment),
            .reserved = 0,
        };
        memcpy(data, &header, sizeof(header));
        return YAZ0_OK;
}
