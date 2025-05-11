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

#include "yaz0.h"

#include <stdbool.h>
#include <string.h>
#include <tgmath.h>

#define MAX_LOOKBACK_WINDOW 0x111
#define MAX_SEARCH_DISTANCE 0x1000

//
// 16-byte header that precedes Yaz0 compressed data.
//
struct z64_yaz0_header {
        char magic[4];              // Should be equal to 'Yaz0'.
        uint32_t uncompressed_size; // Size of the uncompressed data in bytes.
        uint32_t alignment;         // Unknown value, must be 0.
        uint32_t unknown;           // Unknown value, must be 0.
};

//
// Counts the amount of consecutive matching bytes between two strings.
//
size_t yaz0_count_iterative(uint8_t const* a, uint8_t const* b, size_t const window) {
        size_t i = 0;
        for (; i < window; ++i) {
                if (a[i] != b[i]) {
                        break;
                }
        }
        return i;
}

//
// Compares two strings and returns the amount of consecutive matching bytes.
//
size_t yaz0_count(uint8_t const* a, uint8_t const* b, size_t const window) {
        return yaz0_count_iterative(a, b, window);
}

//
// Iteratively searches the longest match.
//
void yaz0_search_iterative(size_t const in_size, uint8_t const in[], size_t const offset, size_t const start,
                           size_t const window, size_t* match_offset, size_t* match_size) {
        for (size_t i = start; i < offset; ++i) {
                if (in[i] != in[offset]) {
                        continue;
                }
                size_t const sz = yaz0_count(in + i, in + offset, window);
                if (sz > *match_size) {
                        *match_size = sz;
                        *match_offset = i;
                        if (*match_size == window) {
                                break;
                        }
                }
        }
}

//
// Searches for the largest match.
//
enum yaz0_result yaz0_search(size_t const in_size, uint8_t const in[], size_t const offset, size_t const lookback,
                             size_t* match_offset, size_t* match_size) {
        *match_offset = 0;
        *match_size = 0;

        // Calculate the search window.
        size_t const window = in_size - offset > MAX_LOOKBACK_WINDOW ? MAX_LOOKBACK_WINDOW : in_size - offset;
        if (window < 3) {
                // We need at least 3 bytes to search in.
                *match_size = 0;
                return YAZ0_OUT_OF_RANGE;
        }

        // Calculate our starting search position.
        size_t const start = offset > lookback ? offset - lookback : 0;

        // And go through our search algorithms (256->128->8)
        // position = avx2_search(in_size, in, offset, position, window, match_offset, match_size);
        yaz0_search_iterative(in_size, in, offset, start, window, match_offset, match_size);
        return YAZ0_OK;
}

//
// Performs a search and a lookahead search.
//
enum yaz0_result yaz0_lookahead_search(size_t const in_size, uint8_t const* in, size_t const offset,
                                       size_t const lookback, size_t* match_cursor, size_t* match_size,
                                       size_t* lookahead_cursor, size_t* lookahead_size, bool* lookahead) {
        // If we've already found a match through a lookahead, we just return here.
        if (*lookahead == true) {
                *lookahead = false;
                *match_size = *lookahead_size;
                return YAZ0_OK;
        }

        // Lookbehind search.
        enum yaz0_result result = yaz0_search(in_size, in, offset, lookback, match_cursor, match_size);
        if (result != YAZ0_OK) {
                return result;
        }

        // Did we find a match and is it large enough?
        if (*match_size >= 3) {
                // Will a lookahead search outperform the lookbehind?
                result = yaz0_search(in_size, in, offset + 1, lookback, lookahead_cursor, lookahead_size);
                if (result != YAZ0_OK) {
                        return result;
                }
                if (*lookahead_size >= *match_size + 2) {
                        *lookahead = true;
                        *match_size = 1;
                        *match_cursor = *lookahead_cursor;
                }
        }

        return YAZ0_OK;
}

enum yaz0_result yaz0_deflate(uint8_t* out, size_t const out_size, uint8_t const* in, size_t const in_size,
                              int const level, size_t* compressed_size) {
        uint8_t packet = 0;
        uint8_t bit_mask = 0x80;

        size_t in_cursor = 0;
        size_t packet_cursor = 0;
        size_t out_cursor = packet_cursor + 1;

        size_t match_size = 0;
        size_t match_cursor = 0;

        size_t lookahead_cursor = 0;
        size_t lookahead_size = 0;
        bool lookahead = false;

        // Calculate the lookback distance.
        if (level < 0 || level > 9) {
                return YAZ0_STREAM_ERROR;
        }
        double const level_factor = (double)level / 9.0;
        size_t lookback = floor((double)MAX_SEARCH_DISTANCE * level_factor);

        // Run through the buffers.
        while (in_cursor < in_size && out_cursor < out_size) {
                // Find the best matching byte span.
                yaz0_lookahead_search(in_size, in, in_cursor, lookback, &match_cursor, &match_size, &lookahead_cursor,
                                      &lookahead_size, &lookahead);

                if (match_size < 3) {
                        out[out_cursor++] = in[in_cursor++];
                        packet |= bit_mask;
                } else if (match_size > 0x11) {
                        size_t const distance = in_cursor - match_cursor - 1;
                        out[out_cursor++] = distance >> 8;
                        out[out_cursor++] = distance & 0xFF;
                        out[out_cursor++] = match_size - 0x12 & 0xFF;
                        in_cursor += match_size;
                } else {
                        size_t const distance = in_cursor - match_cursor - 1;
                        out[out_cursor++] = match_size - 2 << 4 | distance >> 8;
                        out[out_cursor++] = distance & 0xFF;
                        in_cursor += match_size;
                }

                // Move forwards to the next byte.
                bit_mask = bit_mask >> 1;

                // All bytes are written, so finalize the group.
                if (bit_mask == 0) {
                        out[packet_cursor] = packet;
                        packet_cursor = out_cursor;
                        if (in_cursor < in_size) {
                                out_cursor++;
                        }
                        packet = 0;
                        bit_mask = 0x80;
                }
        }

        // Is the current group unfinalized? (this is always true)
        if (bit_mask != 0) {
                out[packet_cursor] = packet;
        }

        // Calculate the compressed size of the file.
        if (compressed_size != NULL) {
                *compressed_size = out_cursor;
        }
        return YAZ0_OK;
}
