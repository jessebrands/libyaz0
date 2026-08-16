/*
 * search_avx2.c: longest substring match AVX2 implementation
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
#include <stdint.h>

#include "cpu.h"
#include "search.h"

#if YAZ0_HAVE_AVX2

#include <immintrin.h>

static inline size_t
yaz0_length_avx2(uint8_t const* a, uint8_t const* b, size_t const max_lookahead) {
    size_t i = 0;
    while (max_lookahead - i >= 32) {
        __m256i const x = _mm256_loadu_si256((__m256i const*) (a + i));
        __m256i const y = _mm256_loadu_si256((__m256i const*) (b + i));
        uint32_t const diff = ~(uint32_t) _mm256_movemask_epi8(_mm256_cmpeq_epi8(x, y));
        if (diff != 0) {
            return i + yaz0_ctz32(diff);
        }
        i += 32;
    }

    while (i < max_lookahead && a[i] == b[i]) {
        ++i;
    }

    return i;
}

size_t
yaz0_search_avx2(uint8_t const* data, size_t const start_pos,
                 size_t const offset, size_t const max_lookahead, size_t* match_pos) {
    assert(data != NULL);
    assert(start_pos <= offset);
    assert(match_pos != NULL);
    assert(max_lookahead >= YAZ0_MIN_MATCH);

    size_t longest_run = 0;

    __m256i const first = _mm256_set1_epi8((char) data[offset]);

    // Anything less than the minimum match is pointless.
    size_t want_offset = YAZ0_MIN_MATCH - 1;
    __m256i want = _mm256_set1_epi8((char) data[offset + want_offset]);

    size_t const tail_start = offset - ((offset - start_pos) & (size_t) 31);

    size_t i = start_pos;
    for (; i < tail_start; i += 32) {
        __m256i const head = _mm256_loadu_si256((__m256i const*) &data[i]);
        __m256i const tail = _mm256_loadu_si256((__m256i const*) &data[i + want_offset]);
        __m256i const hit = _mm256_and_si256(_mm256_cmpeq_epi8(head, first), _mm256_cmpeq_epi8(tail, want));

        uint32_t mask = (uint32_t) _mm256_movemask_epi8(hit);
        while (mask != 0) {
            size_t const pos = i + yaz0_ctz32(mask);
            size_t const run_length = yaz0_length_avx2(&data[pos], &data[offset], max_lookahead);
            if (run_length > longest_run) {
                *match_pos = pos;
                longest_run = run_length;

                if (run_length == max_lookahead) {
                    return longest_run;
                }

                want_offset = (longest_run < YAZ0_MIN_MATCH - 1)
                                  ? (size_t) (YAZ0_MIN_MATCH - 1)
                                  : longest_run;

                want = _mm256_set1_epi8((char) data[offset + want_offset]);
            }

            mask &= mask - 1;
        }
    }

    // The remaining candidates are scanned one at a time.
    for (; i < offset; ++i) {
        if (data[i] != data[offset]) {
            continue;
        }
        if (longest_run > 0 && data[i + longest_run] != data[offset + longest_run]) {
            continue;
        }

        size_t const run_length = yaz0_length_avx2(&data[i], &data[offset], max_lookahead);
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

#endif // YAZ0_HAVE_AVX2
