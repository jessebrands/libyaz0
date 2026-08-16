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

static struct yaz0_matcher_impl const* const matchers[] = {
    &yaz0_matcher_standard,
};

#define YAZ0_MATCHER_COUNT (sizeof matchers / sizeof matchers[0])

static bool
is_matcher_supported(struct yaz0_matcher_impl const* const matcher) {
    return matcher->supported == NULL || matcher->supported();
}

static enum yaz0_matcher
matcher_for_level(int const level) {
    (void) level;
    return YAZ0_MATCHER_STANDARD;
}

static struct yaz0_matcher_impl const*
find_matcher(enum yaz0_matcher const id) {
    for (size_t i = 0; i < YAZ0_MATCHER_COUNT; ++i) {
        if (matchers[i]->id == id && is_matcher_supported(matchers[i])) {
            return matchers[i];
        }
    }

    return NULL;
}

struct yaz0_matcher_impl const*
yaz0_matcher_select(struct yaz0_matcher_config const* cfg) {
    // An explicit request that cannot be honoured is an error rather than a
    // fallback: a benchmark asking for one strategy must never quietly
    // measure a different one.
    if (cfg->matcher != YAZ0_MATCHER_AUTO) {
        return find_matcher(cfg->matcher);
    }

    struct yaz0_matcher_impl const* const chosen = find_matcher(matcher_for_level(cfg->level));
    if (chosen != NULL) {
        return chosen;
    }

    // Policy picked something this build cannot provide; take what is left.
    for (size_t i = 0; i < YAZ0_MATCHER_COUNT; ++i) {
        if (is_matcher_supported(matchers[i])) {
            return matchers[i];
        }
    }

    return NULL;
}

char const*
yaz0_matcher_name(enum yaz0_matcher const matcher) {
    if (matcher == YAZ0_MATCHER_AUTO) {
        return "auto";
    }

    for (size_t i = 0; i < YAZ0_MATCHER_COUNT; ++i) {
        if (matchers[i]->id == matcher) {
            return matchers[i]->name;
        }
    }

    return "unknown";
}
