/*
 * matcher.c: matcher selection
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

#include <stdbool.h>
#include <stddef.h>

#include "matcher.h"

static struct yaz0_matcher const* const matchers[] = {
    &yaz0_matcher_std,
};

#define YAZ0_MATCHER_COUNT (sizeof matchers / sizeof matchers[0])

static bool
is_matcher_supported(struct yaz0_matcher const* const matcher) {
    return matcher->supported == NULL || matcher->supported();
}

struct yaz0_matcher const*
yaz0_matcher_select(struct yaz0_matcher_config const* cfg) {
    (void) cfg;

    for (size_t i = 0; i < YAZ0_MATCHER_COUNT; ++i) {
        if (is_matcher_supported(matchers[i])) {
            return matchers[i];
        }
    }

    return NULL;
}
