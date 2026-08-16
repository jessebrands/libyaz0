/*
 * search_sse2.c: longest substring match SSE2 implementation
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

#include "cpu.h"
#include "search.h"

#if YAZ0_HAVE_SSE2

#include <emmintrin.h>

static inline size_t
yaz0_length_sse2(uint8_t const* a, uint8_t const* b, size_t const max_lookahead) {
    size_t i = 0;
    while (max_lookahead - i >= 16) {
        __m128i const x = _mm_loadu_si128((__m128i const*) (a + i));
        __m128i const y = _mm_loadu_si128((__m128i const*) (b + i));
        uint32_t const diff = (~(uint32_t) _mm_movemask_epi8(_mm_cmpeq_epi8(x, y))) & 0xFFFFu;
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
yaz0_search_sse2(uint8_t const* data, size_t const start_pos,
                 size_t const offset, size_t const max_lookahead, size_t* match_pos) {
    assert(data != NULL);
    assert(start_pos <= offset);
    assert(match_pos != NULL);
    assert(max_lookahead >= YAZ0_MIN_MATCH);

    size_t longest_run = 0;

    // Every candidate must at minimum match the first byte at the offset.
    __m128i const first = _mm_set1_epi8((char) data[offset]);

    // A candidate must reach MIN_MATCH to be encodable at all, so the second
    // test starts there rather than duplicating the first.
    size_t want_offset = YAZ0_MIN_MATCH - 1;
    __m128i want = _mm_set1_epi8((char) data[offset + want_offset]);

    size_t const tail_start = offset - ((offset - start_pos) & (size_t) 31);

    size_t i = start_pos;
    for (; i < tail_start; i += 32) {
        __m128i const head_lo = _mm_loadu_si128((__m128i const*) &data[i]);
        __m128i const head_hi = _mm_loadu_si128((__m128i const*) &data[i + 16]);
        __m128i const tail_lo = _mm_loadu_si128((__m128i const*) &data[i + want_offset]);
        __m128i const tail_hi = _mm_loadu_si128((__m128i const*) &data[i + 16 + want_offset]);

        __m128i const hit_lo = _mm_and_si128(_mm_cmpeq_epi8(head_lo, first),
                                             _mm_cmpeq_epi8(tail_lo, want));
        __m128i const hit_hi = _mm_and_si128(_mm_cmpeq_epi8(head_hi, first),
                                             _mm_cmpeq_epi8(tail_hi, want));

        // Bits 0-15 are the candidates at data[i], bits 16-31 those at
        // data[i + 16], so one mask covers both halves in position order.
        uint32_t mask = (uint32_t) _mm_movemask_epi8(hit_lo)
                        | ((uint32_t) _mm_movemask_epi8(hit_hi) << 16);

        // Bit k is the candidate at data[i + k], so taking the lowest set bit
        // first keeps the earliest position winning ties.
        while (mask != 0) {
            size_t const pos = i + yaz0_ctz32(mask);
            mask &= mask - 1;

            size_t const run_length = yaz0_length_sse2(&data[pos], &data[offset], max_lookahead);
            if (run_length > longest_run) {
                *match_pos = pos;
                longest_run = run_length;

                if (run_length == max_lookahead) {
                    return longest_run;
                }

                want_offset = (longest_run < YAZ0_MIN_MATCH - 1)
                                  ? (size_t) (YAZ0_MIN_MATCH - 1)
                                  : longest_run;

                want = _mm_set1_epi8((char) data[offset + want_offset]);
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

        size_t const run_length = yaz0_length_sse2(&data[i], &data[offset], max_lookahead);
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

#endif // YAZ0_HAVE_SSE2
