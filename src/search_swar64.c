/*
 * search_swar.c: longest substring match using SWAR techniques
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
#include <string.h>

#include "search.h"

#if YAZ0_HAVE_SWAR64

#if defined _MSC_VER && (defined _M_X64 || defined _M_ARM64)
#include <intrin.h>
#elif defined _MSC_VER
#include <intrin.h>
#endif

static inline uint64_t
yaz0_load64(uint8_t const* const p) {
    uint64_t v;
    memcpy(&v, p, sizeof v);
    return v;
}

static inline uint64_t
yaz0_splat64(uint8_t const b) {
    return UINT64_C(0x0101010101010101) * b;
}

static inline unsigned
yaz0_ctz64(uint64_t const mask) {
    assert(mask != 0);
#if defined __GNUC__ || defined __clang__
    return (unsigned) __builtin_ctzll(mask);
#elif defined _MSC_VER && (defined _M_X64 || defined _M_ARM64)
    unsigned long index;
    _BitScanForward64(&index, mask);
    return (unsigned) index;
#elif defined _MSC_VER
    // _BitScanForward64 does not exist on 32-bit MSVC.
    unsigned long index;
    if (_BitScanForward(&index, (unsigned long) (mask & UINT32_C(0xFFFFFFFF)))) {
        return (unsigned) index;
    }
    _BitScanForward(&index, (unsigned long) (mask >> 32));
    return (unsigned) index + 32;
#else
    unsigned i = 0;
    while (((mask >> i) & 1) == 0) {
        ++i;
    }
    return i;
#endif
}

static inline size_t
yaz0_length_swar64(uint8_t const* const a, uint8_t const* const b, size_t const max_lookahead) {
    size_t i = 0;
    while (max_lookahead - i >= 8) {
        uint64_t const diff = yaz0_load64(a + i) ^ yaz0_load64(b + i);
        if (diff != 0) {
            return i + (yaz0_ctz64(diff) >> 3);
        }
        i += 8;
    }

    while (i < max_lookahead && a[i] == b[i]) {
        ++i;
    }

    return i;
}

size_t yaz0_search_swar64(uint8_t const* data, size_t start_pos, size_t offset, size_t max_lookahead,
    size_t* match_pos) {
    assert(data != NULL);
    assert(start_pos <= offset);
    assert(match_pos != NULL);
    assert(max_lookahead >= YAZ0_MIN_MATCH);

    size_t longest_run = 0;

    uint64_t const first = yaz0_splat64(data[offset]);

    size_t want_offset = YAZ0_MIN_MATCH - 1;
    uint64_t want = yaz0_splat64(data[offset + want_offset]);

    size_t const tail_start = offset - ((offset - start_pos) & (size_t) 7);

    size_t i = start_pos;
    for (; i < tail_start; i += 8) {
        // XOR yields a zero byte where a lane matches.
        uint64_t const diff = (yaz0_load64(&data[i]) ^ first)
                              | (yaz0_load64(&data[i + want_offset]) ^ want);

        uint64_t const t0 = (~diff & UINT64_C(0x7f7f7f7f7f7f7f7f)) + UINT64_C(0x0101010101010101);
        uint64_t const t1 = ~diff & UINT64_C(0x8080808080808080);

        uint64_t mask = t0 & t1;

        while (mask != 0) {
            size_t const pos = i + (yaz0_ctz64(mask) >> 3);
            mask &= mask - 1;

            size_t const run_length = yaz0_length_swar64(&data[pos], &data[offset], max_lookahead);
            if (run_length > longest_run) {
                *match_pos = pos;
                longest_run = run_length;

                if (run_length == max_lookahead) {
                    return longest_run;
                }

                want_offset = (longest_run < YAZ0_MIN_MATCH - 1)
                                  ? (size_t) (YAZ0_MIN_MATCH - 1)
                                  : longest_run;

                want = yaz0_splat64(data[offset + want_offset]);
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

        size_t const run_length = yaz0_length_swar64(&data[i], &data[offset], max_lookahead);
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

#endif // YAZ0_HAVE_SWAR64

bool
yaz0_search_swar64_supported(void) {
    return YAZ0_HAVE_SWAR64;
}
