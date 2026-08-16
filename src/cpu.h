/*
 * cpu.h: CPU intrinsics and capability probes
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

#ifndef LIBYAZ0_CPU_H
#define LIBYAZ0_CPU_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#if defined _MSC_VER
#  include <intrin.h>
#endif

#if defined __BYTE_ORDER__ && defined __ORDER_LITTLE_ENDIAN__
#  define YAZ0_LITTLE_ENDIAN (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#elif defined _MSC_VER
   /* MSVC defines no byte-order macro and targets no big-endian platform. */
#  define YAZ0_LITTLE_ENDIAN 1
#else
#  define YAZ0_LITTLE_ENDIAN 0
#endif

#if defined __x86_64__ || defined _M_X64 || defined __i386__ || defined _M_IX86
#  define YAZ0_TARGET_X86 1
#else
#  define YAZ0_TARGET_X86 0
#endif

#if defined __aarch64__ || defined _M_ARM64
#  define YAZ0_TARGET_AARCH64 1
#else
#  define YAZ0_TARGET_AARCH64 0
#endif

#if defined __wasm__
#  define YAZ0_TARGET_WASM 1
#else
#  define YAZ0_TARGET_WASM 0
#endif

#if YAZ0_TARGET_X86 && !defined YAZ0_DISABLE_SSE2
#  define YAZ0_HAVE_SSE2 1
#else
#  define YAZ0_HAVE_SSE2 0
#endif

#if YAZ0_TARGET_X86 && !defined YAZ0_DISABLE_AVX2
#  define YAZ0_HAVE_AVX2 1
#else
#  define YAZ0_HAVE_AVX2 0
#endif

#if YAZ0_TARGET_AARCH64 && !defined YAZ0_DISABLE_NEON
#  define YAZ0_HAVE_NEON 1
#else
#  define YAZ0_HAVE_NEON 0
#endif

#if YAZ0_TARGET_WASM && !defined YAZ0_DISABLE_SIMD128
#  define YAZ0_HAVE_SIMD128 1
#else
#  define YAZ0_HAVE_SIMD128 0
#endif

/*!
 * \brief Index of the lowest set bit. Undefined for a zero mask.
 */
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
    while (((mask >> i) & 1u) == 0) {
        ++i;
    }
    return i;
#endif
}

/*!
 * \brief Index of the lowest set bit. Undefined for a zero mask.
 */
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
    /* _BitScanForward64 does not exist on 32-bit MSVC. */
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

bool
yaz0_little_endian(void);

bool
yaz0_sse2_supported(void);

bool
yaz0_avx2_supported(void);

bool
yaz0_neon_supported(void);

bool
yaz0_simd128_supported(void);

#endif //LIBYAZ0_CPU_H
