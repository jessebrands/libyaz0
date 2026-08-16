/*
 * matcher.h: Yaz0 match finder
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

#ifndef LIBYAZ0_MATCHER_H
#define LIBYAZ0_MATCHER_H

#include "search.h"

enum yaz0_matcher {
    YAZ0_MATCHER_STANDARD = 0,
};

struct yaz0_matcher_config {
    size_t max_distance;
    uint32_t uncompressed_size;
    enum yaz0_search search;
};

struct yaz0_matcher_impl {
    enum yaz0_matcher id;
    char const* name;
    bool (*supported)(void);
    size_t (*state_size)(void);
    size_t (*state_align)(void);
    void (*init)(void* self, struct yaz0_matcher_config const* cfg);
    void (*slide)(void* self, size_t drop);

    struct yaz0_token (*find)(void* self, uint8_t const* window,
                              size_t window_size, size_t position);
};

/*!
 * \brief Generic matcher: greedy with one-position lazy lookahead, delegating
 *        the scan to whichever search implementation was selected.
 */
extern struct yaz0_matcher_impl const yaz0_matcher_standard;

/*!
 * \brief Picks the best matcher for the configuration.
 */
struct yaz0_matcher_impl const*
yaz0_matcher_select(struct yaz0_matcher_config const* cfg);

#endif //LIBYAZ0_MATCHER_H
