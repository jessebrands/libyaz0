/*
 * cpu.c: CPU intrinsics and capability probes
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

#include "cpu.h"

#if YAZ0_TARGET_X86 && !defined _MSC_VER
#  include <cpuid.h>
#endif

#if YAZ0_TARGET_X86
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
#endif // YAZ0_TARGET_X86

bool
yaz0_sse2_supported(void) {
#if !YAZ0_HAVE_SSE2
    return false;
#elif defined __x86_64__ || defined _M_X64
    return true;
#elif defined _MSC_VER
    int regs[4];
    __cpuid(regs, 1);
    return (regs[3] & (1 << 26)) != 0;
#else
    unsigned eax, ebx, ecx, edx;
    if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        return false;
    }
    return (edx & (1u << 26)) != 0;
#endif
}

bool
yaz0_avx2_supported(void) {
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

bool
yaz0_simd128_supported(void) {
    return YAZ0_HAVE_SIMD128;
}

bool
yaz0_neon_supported(void) {
    return YAZ0_HAVE_NEON;
}

bool
yaz0_little_endian(void) {
    return YAZ0_LITTLE_ENDIAN;
}
