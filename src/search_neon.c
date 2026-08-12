/*
 * search_neon.c: longest substring match on ARM Neon
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

#include "search.h"

#if YAZ0_HAVE_NEON

#include <arm_neon.h>

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

// The brilliant folks at ARM decided in all their wisdom that PMOVMASKB, probably the
// the most popular SIMD instruction of all time, is just not cool enough for them.
//
// So we're resorting to this. Will this kill performance? I don't know. I don't understand
// ARM Neon, I don't know what I'm doing. It seems to work?
static inline uint64_t
yaz0_move_mask(uint8x16_t const cmp) {
    uint8x8_t const narrowed = vshrn_n_u16(vreinterpretq_u16_u8(cmp), 4);
    return vget_lane_u64(vreinterpret_u64_u8(narrowed), 0);
}

static inline size_t
yaz0_length_neon(uint8_t const *a, uint8_t const *b, size_t const max_lookahead) {
    size_t i = 0;
    while (max_lookahead - i >= 16) {
        uint8x16_t const x = vld1q_u8(&a[i]);
        uint8x16_t const y = vld1q_u8(&b[i]);

        // PMOVMSKB, my precious... :(
        uint64_t const diff = ~yaz0_move_mask(vceqq_u8(x, y));

        if (diff != 0) {
            return i + (size_t) (yaz0_ctz64(diff) >> 2);
        }
        i += 16;
    }

    while (i < max_lookahead && a[i] == b[i]) {
        ++i;
    }

    return i;
}

// Port of the SIMD128/SSE2 kernel but now it's ARM Neon. x86 is dead! IT'S DEAD!!
// Or is it? I don't know what I am doing honestly, I am pretty good at x86 asm but
// despite having had a M1 Mac for over 5 years, I still never touched ARM before.
//
// Finding information about it is pretty hard and the ARM intrinsics guide website
// is a little janky, but we got there in the end.
//
// Seems to be about twice as fast as the SWAR64 kernel which is already pretty fast
// on ARM (compiler must really like it!), so I'm happy with this. It's not quite
// SSE2 levels of fast but my M1 Mac is a lot less powerful than my Core Ultra 285K
// so maybe it's pretty much equal?
size_t
yaz0_search_neon(uint8_t const *data, size_t const start_pos, size_t const offset,
                 size_t const max_lookahead, size_t *match_pos) {
    assert(data != NULL);
    assert(start_pos <= offset);
    assert(match_pos != NULL);
    assert(max_lookahead >= YAZ0_MIN_MATCH);

    size_t longest_run = 0;

    // Every candidate must at minimum match the first byte at the offset.
    uint8x16_t const first = vdupq_n_u8(data[offset]);

    // A candidate must reach MIN_MATCH to be encodable at all, so the second
    // test starts there rather than duplicating the first.
    size_t want_offset = YAZ0_MIN_MATCH - 1;
    uint8x16_t want = vdupq_n_u8(data[offset + want_offset]);

    size_t const tail_start = offset - ((offset - start_pos) & (size_t) 15);
    size_t const wide_end = offset - ((offset - start_pos) & (size_t) 63);

    size_t i = start_pos;
    for (; i < wide_end; i += 64) {
        // Hypothesis: vget_lane is expensive, what about amortizing it?
        //
        // it could work but my concern is that I'm just gonna pay for 4 expensive
        // transfers instead since the odds of finding a hit in 64 bytes seems high.
        // I guess future me will find out.
        //
        // Future me here: it works.
        // Of course this works. I'm gonna pay for these transfers one way or
        // another. LOL We're just checking if we can skip this at all.
        //
        // also mandatory joke:
        //   me: mom, can we have AVX512?
        //  mom: we have AVX512 at home
        //  AVX512 at home:
        uint8x16x4_t const head = vld1q_u8_x4(&data[i]);
        uint8x16x4_t const tail = vld1q_u8_x4(&data[i + want_offset]);
        uint8x16_t const hit0 = vandq_u8(vceqq_u8(head.val[0], first), vceqq_u8(tail.val[0], want));
        uint8x16_t const hit1 = vandq_u8(vceqq_u8(head.val[1], first), vceqq_u8(tail.val[1], want));
        uint8x16_t const hit2 = vandq_u8(vceqq_u8(head.val[2], first), vceqq_u8(tail.val[2], want));
        uint8x16_t const hit3 = vandq_u8(vceqq_u8(head.val[3], first), vceqq_u8(tail.val[3], want));

        // OR everything together. If the result is zero, we can skip these 64 bytes.
        // This is the best case scenario, cause it means we can avoid paying for
        // 4 expensive register transfers.
        uint8x16_t const any = vorrq_u8(vorrq_u8(hit0, hit1), vorrq_u8(hit2, hit3));
        if (vmaxvq_u8(any) == 0) {
            continue;
        }

        // Now we pay the cost of 4 loads at once, but hopefully we can avoid that most of the time?
        uint8x16_t const hits[4] = {hit0, hit1, hit2, hit3};
        for (unsigned b = 0; b < 4; ++b) {
            size_t const base = i + b * 16u;
            uint64_t mask = yaz0_move_mask(hits[b]);

            while (mask != 0) {
                unsigned const bit = yaz0_ctz64(mask);
                size_t const pos = base + (bit >> 2);

                size_t const run_length = yaz0_length_neon(&data[pos], &data[offset], max_lookahead);
                if (run_length > longest_run) {
                    *match_pos = pos;
                    longest_run = run_length;
                    if (run_length == max_lookahead) {
                        return longest_run;
                    }
                    want_offset = (longest_run < YAZ0_MIN_MATCH - 1)
                                      ? (size_t) (YAZ0_MIN_MATCH - 1)
                                      : longest_run;

                    want = vdupq_n_u8(data[offset + want_offset]);
                }

                mask &= ~(UINT64_C(0xF) << bit);
            }
        }
    }

    // Bit k is the candidate at data[i + k], so taking the lowest set bit
    // first keeps the earliest position winning ties.
    for (; i < tail_start; i += 16) {
        // Ok, ok, we can do this. This is just like SSE2! :D
        uint8x16_t const head = vld1q_u8(&data[i]);
        uint8x16_t const tail = vld1q_u8(&data[i + want_offset]);
        uint8x16_t const hit = vandq_u8(vceqq_u8(head, first), vceqq_u8(tail, want));

        // ahh... fuck
        uint64_t mask = yaz0_move_mask(hit);
        while (mask != 0) {
            unsigned const bit = yaz0_ctz64(mask);
            size_t const pos = i + (bit >> 2);

            size_t const run_length = yaz0_length_neon(&data[pos], &data[offset], max_lookahead);
            if (run_length > longest_run) {
                *match_pos = pos;
                longest_run = run_length;

                if (run_length == max_lookahead) {
                    return longest_run;
                }

                want_offset = (longest_run < YAZ0_MIN_MATCH - 1)
                                  ? (size_t) (YAZ0_MIN_MATCH - 1)
                                  : longest_run;

                want = vdupq_n_u8(data[offset + want_offset]);
            }

            // We're using a nibble mask so this is a little different.
            // Can you believe I lost all my gains here and spent hours
            // wondering why my ARM neon code is so slow? Yep.
            mask &= ~(UINT64_C(0xF) << bit);

            // Learning a new instruction set is hard. :(
        }
    }

    for (; i < offset; ++i) {
        if (data[i] != data[offset]) {
            continue;
        }
        if (longest_run > 0 && data[i + longest_run] != data[offset + longest_run]) {
            continue;
        }

        size_t const run_length = yaz0_length_neon(&data[i], &data[offset], max_lookahead);
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

#endif // YAZ0_HAVE_NEON

bool
yaz0_search_neon_supported(void) {
    return YAZ0_HAVE_NEON;
}
