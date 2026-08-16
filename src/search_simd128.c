/*
 * search_simd128.c: longest substring match on WebAssembly using SIMD128
 * Copyright (C) 2026 Jesse Gerard Brands
 *
 * This file is part of libyaz0.
 *
 * libyaz0 is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * libyaz0 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libyaz0. If not, see <https://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <stdbool.h>

#include "cpu.h"
#include "search.h"

#if YAZ0_HAVE_SIMD128

#include <wasm_simd128.h>

static inline size_t
yaz0_length_simd128(uint8_t const* a, uint8_t const* b, size_t const max_lookahead) {
    size_t i = 0;
    while (max_lookahead - i >= 16) {
        v128_t const x = wasm_v128_load(&a[i]);
        v128_t const y = wasm_v128_load(&b[i]);
        uint32_t const diff = (~wasm_i8x16_bitmask(wasm_i8x16_eq(x, y))) & 0xFFFFu;
        if (diff != 0) {
            return i + yaz0_ctz32(diff);
        }
        i += 16;
    }

    while (i < max_lookahead && a[i] == b[i]) {
        ++i;
    }

    return i;
}

size_t
yaz0_search_simd128(uint8_t const* data, size_t const start_pos, size_t const offset,
                    size_t const max_lookahead, size_t* match_pos) {
    assert(data != NULL);
    assert(start_pos <= offset);
    assert(match_pos != NULL);
    assert(max_lookahead >= YAZ0_MIN_MATCH);

    size_t longest_run = 0;

    // Every candidate must at minimum match the first byte at the offset.
    v128_t const first = wasm_i8x16_splat((char) data[offset]);

    // A candidate must reach MIN_MATCH to be encodable at all, so the second
    // test starts there rather than duplicating the first.
    size_t want_offset = YAZ0_MIN_MATCH - 1;
    v128_t want = wasm_i8x16_splat((char) data[offset + want_offset]);

    size_t const tail_start = offset - ((offset - start_pos) & (size_t) 31);

    size_t i = start_pos;
    for (; i < tail_start; i += 16) {
        v128_t const head = wasm_v128_load(&data[i]);
        v128_t const tail = wasm_v128_load(&data[i + want_offset]);
        v128_t const hit = wasm_v128_and(wasm_i8x16_eq(head, first), wasm_i8x16_eq(tail, want));
        uint32_t mask = wasm_i8x16_bitmask(hit);

        // Bit k is the candidate at data[i + k], so taking the lowest set bit
        // first keeps the earliest position winning ties.
        while (mask != 0) {
            size_t const pos = i + yaz0_ctz32(mask);
            mask &= mask - 1;

            size_t const run_length = yaz0_length_simd128(&data[pos], &data[offset], max_lookahead);
            if (run_length > longest_run) {
                *match_pos = pos;
                longest_run = run_length;

                if (run_length == max_lookahead) {
                    return longest_run;
                }

                want_offset = (longest_run < YAZ0_MIN_MATCH - 1)
                                  ? (size_t) (YAZ0_MIN_MATCH - 1)
                                  : longest_run;

                want = wasm_i8x16_splat((char) data[offset + want_offset]);
            }
        }
    }

    for (; i < offset; ++i) {
        if (data[i] != data[offset]) {
            continue;
        }
        if (longest_run > 0 && data[i + longest_run] != data[offset + longest_run]) {
            continue;
        }

        size_t const run_length = yaz0_length_simd128(&data[i], &data[offset], max_lookahead);
        if (run_length > longest_run) {
            *match_pos = i;
            longest_run = run_length;

            if (run_length == max_lookahead) {
                return longest_run;
            }
        }
    }

    return (longest_run >= YAZ0_MIN_MATCH) ? longest_run : 0;
}

#endif // YAZ0_HAVE_SIMD128
