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

#include <string.h>

enum yaz0_result yaz0_inflate(uint8_t* dst, size_t const dst_size, uint8_t const* src, size_t const src_size) {
        size_t in_cursor = 0;
        size_t out_cursor = 0;
        int bit_count = 0;
        uint8_t packet = 0;

        // Is the input data even large enough to hold the header?
        if (src_size < sizeof(struct yaz0_header)) {
                return YAZ0_OUT_OF_RANGE;
        }

        // Read out the header and do some quick sanity checking.
        struct yaz0_header header = {0};
        enum yaz0_result const result = yaz0_read_header(src, src_size, &header);
        if (result != YAZ0_OK) {
                return result;
        }
        if (memcmp(header.magic, YAZ0_MAGIC, sizeof(header.magic)) != 0) {
                return YAZ0_INVALID_DATA;
        }
        if (header.uncompressed_size > dst_size) {
                return YAZ0_DESTINATION_TOO_SMALL;
        }

        // Advance past the header and begin decompressing.
        in_cursor += sizeof(struct yaz0_header);

        while (out_cursor < dst_size) {
                // If we ran out of bits to process, grab the next byte.
                if (!bit_count) {
                        packet = src[in_cursor++];
                        bit_count = 8;
                }

                // Perform a whole byte copy?
                if (packet & 0x80) {
                        dst[out_cursor++] = src[in_cursor++];
                } else {
                        // We cannot just copy the byte. We must look-behind in the buffer.
                        uint8_t const b1 = src[in_cursor++];
                        uint8_t const b2 = src[in_cursor++];

                        // Calculate look-behind position and length.
                        uint16_t const distance = (b1 & 0xF) << 8 | b2;
                        size_t lb_cursor = out_cursor - (distance + 1);
                        size_t length = b1 >> 4;

                        if (length == 0) {
                                // The length is too long to store in 4 bits, so it's located in the next input byte.
                                length = src[in_cursor++] + 0x12;
                        } else {
                                length += 2;
                        }

                        // Copy bytes from a previous spot.
                        if (lb_cursor + length >= out_cursor) {
                                // This look-behind will look into data written by this copy.
                                for (int i = 0; i < length; ++i) {
                                        dst[out_cursor++] = dst[lb_cursor++];
                                }
                        } else {
                                // The look-behind does not write past the current cursor.
                                memcpy(&dst[out_cursor], &dst[lb_cursor], length);
                                out_cursor += length;
                        }
                }

                // Advance to the next bit.
                packet = packet << 1;
                --bit_count;
        }

        return YAZ0_OK;
}
