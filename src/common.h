/*
 * common.h: common library utilities and allocator
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

#ifndef LIBYAZ0_COMMON_H
#define LIBYAZ0_COMMON_H

#include <stddef.h>

#include "yaz0/yaz0.h"

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

#if YAZ0_LITTLE_ENDIAN
#  define YAZ0_HAVE_SWAR64 1
#else
#  define YAZ0_HAVE_SWAR64 0
#endif

#define YAZ0_MAGIC "Yaz0"
#define YAZ0_HEADER_SIZE 16
#define YAZ0_MAX_DISTANCE 4096
#define YAZ0_TOKENS_PER_BLOCK 8

#define YAZ0_DISTANCE_BIAS 1
#define YAZ0_SHORT_LENGTH_BIAS 2
#define YAZ0_LONG_LENGTH_BIAS 18
#define YAZ0_MIN_MATCH 3
#define YAZ0_MAX_MATCH 273

#define YAZ0_MAX_BLOCK_SIZE 25

#define YAZ0_WINDOW_SIZE    (YAZ0_MAX_DISTANCE * 2 + YAZ0_MAX_MATCH)
#define YAZ0_WINDOW_PADDING 64

enum yaz0_step {
    YAZ0_STEP_CONTINUE,
    YAZ0_STEP_RETURN,
};

enum yaz0_kind {
    YAZ0_KIND_COMPRESSOR = 1,
    YAZ0_KIND_DECOMPRESSOR = 2,
};

struct yaz0_common_state {
    struct yaz0_stream* stream;
    enum yaz0_kind kind;

    void* opaque;
    yaz0_alloc_func alloc;
    yaz0_free_func free;
};

void* yaz0_alloc(struct yaz0_stream const* stream, size_t size);

void yaz0_free(struct yaz0_stream const* stream, void* ptr);

#endif //LIBYAZ0_COMMON_H
