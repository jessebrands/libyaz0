/*
 * search.h: longest substring match search
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

#ifndef LIBYAZ0_SEARCH_H
#define LIBYAZ0_SEARCH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "common.h"
#include "yaz0/yaz0.h"

/*!
 * Function pointer type for a longest matching substring search function.
 */
typedef size_t (* yaz0_search_func)(uint8_t const* data, size_t start_pos, size_t offset,
                                    size_t max_lookahead, size_t* match_pos);

struct yaz0_search_impl {
    enum yaz0_search id;
    char const* name;
    yaz0_search_func search;
    bool (* supported)(void);
};

/*!
 * \brief Reference implementation for substring match length. VERY SLOW!
 */
size_t
yaz0_length_reference(uint8_t const* a, uint8_t const* b, size_t max_lookahead);

/*!
 * \brief Reference implementation for the search algorithm. Very, very slow!
 */
size_t
yaz0_search_reference(uint8_t const* data, size_t start_pos, size_t offset,
                      size_t max_lookahead, size_t* match_pos);

/*!
 * \brief Fast, portable matching length using memchr.
 */
size_t
yaz0_length_scalar(uint8_t const* a, uint8_t const* b, size_t max_lookahead);

/*!
 * \brief Fast, portable longest matching substring search using memchr and a filter.
 * \see yaz0_search_func
 */
size_t
yaz0_search_scalar(uint8_t const* data, size_t start_pos, size_t offset,
                   size_t max_lookahead, size_t* match_pos);

#if YAZ0_HAVE_SWAR64
/*!
 * \brief Fast, portable search implementation making use of SWAR techniques.
 * \see yaz0_search_func
 * \note Available on little-endian only.
 */
size_t
yaz0_search_swar64(uint8_t const* data, size_t start_pos, size_t offset,
                 size_t max_lookahead, size_t* match_pos);
#endif

bool
yaz0_search_swar64_supported(void);

#if YAZ0_HAVE_SSE2
size_t
yaz0_search_sse2(uint8_t const* data, size_t start_pos, size_t offset,
                 size_t max_lookahead, size_t* match_pos);
#endif

bool
yaz0_search_sse2_supported(void);

#if YAZ0_HAVE_AVX2
size_t
yaz0_search_avx2(uint8_t const* data, size_t start_pos,
                 size_t offset, size_t max_lookahead, size_t* match_pos);
#endif

bool
yaz0_search_avx2_supported(void);

struct yaz0_search_impl const*
yaz0_search_select(enum yaz0_search search);

#endif //LIBYAZ0_SEARCH_H
