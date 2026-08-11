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

#include "search.h"

#if defined _MSC_VER
#  include <intrin.h>
#  include <immintrin.h>
#else
#  include <cpuid.h>
#endif

static bool
yaz0_cpuid_count(uint32_t const leaf, uint32_t const subleaf,
                 uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
#if defined _MSC_VER
    int regs[4];
    __cpuidex(regs, (int) leaf, (int) subleaf);
    *eax = (uint32_t) regs[0];
    *ebx = (uint32_t) regs[1];
    *ecx = (uint32_t) regs[2];
    *edx = (uint32_t) regs[3];
    return true;
#else
    return __get_cpuid_count(leaf, subleaf, eax, ebx, ecx, edx) != 0;
#endif
}

static uint64_t
yaz0_xgetbv0(void) {
#if defined _MSC_VER
    return _xgetbv(0);
#else
    // YUMMY INLINE ASSEMBLY
    uint32_t eax, edx;
    __asm__ __volatile__("xgetbv" : "=a"(eax), "=d"(edx): "c"(0));
    return ((uint64_t) edx << 32) | eax;
#endif
}

bool
yaz0_search_avx2_supported(void) {
#if !YAZ0_HAVE_AVX2
    return false;
#else
    uint32_t eax, ebx, ecx, edx;

    // Check leaf 7 for AVX2 bit
    if (!yaz0_cpuid_count(0, 0, &eax, &ebx, &ecx, &edx) || eax < 7) {
        return false;
    }
    if (!yaz0_cpuid_count(1, 0, &eax, &ebx, &ecx, &edx)) {
        return false;
    }

    // Test that the OS enabled XSAVE, or XGETBV is bad bad bad
    if ((ecx & (1u << 27)) == 0) {
        return false;
    }

    // Do we have AVX?
    if ((ecx & (1u << 28)) == 0) {
        return false;
    }

    // XCR0 bits 1 and 2 are the SSE and YMM state. Without both, the OS does
    // not preserve ymm registers across a context switch and using them
    // corrupts state rather than faulting cleanly.
    if ((yaz0_xgetbv0() & 0x6u) != 0x6u) {
        return false;
    }
    if (!yaz0_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
        return false;
    }

    // If this passes, we have AVX2 \o/
    return (ebx & (1u << 5)) != 0;
#endif
}

static inline unsigned
yaz0_ctz32(uint32_t const mask) {
    assert(mask != 0);
#if defined __GNUC__ || defined __clang__
    return (unsigned) __builtin_ctz(mask);
#elif defined _MSC_VER
    unsigned long index;
    _BitScanForward(&index, mask);
    return (unsigned) index;
#else
    unsigned i = 0;
    while ((mask & (1u << i)) == 0) {
        ++i;
    }
    return i;
#endif
}

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
