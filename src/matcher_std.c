/*
 * matcher_std.c: Nintendo-like match finder implementation
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
#include <stddef.h>
#include <stdint.h>

#include "common.h"
#include "matcher.h"
#include "search.h"

struct matcher_state {
    bool deferred;
    size_t deferred_length;
    size_t deferred_distance;

    size_t max_distance;
    struct yaz0_search_impl const* impl;
};

static bool
search(struct matcher_state const* state,
       uint8_t const* window, size_t const window_size,
       const size_t position,
       struct yaz0_token* token) {
    token->distance = 0;
    token->length = 1;

    assert(position < window_size);
    size_t lookahead = window_size - position;
    if (lookahead > YAZ0_MAX_MATCH) {
        lookahead = YAZ0_MAX_MATCH;
    }

    if (state->max_distance == 0 || lookahead < YAZ0_MIN_MATCH) {
        return false;
    }

    size_t start_pos = 0;
    if (position > state->max_distance) {
        start_pos = position - (state->max_distance + 1);
    }

    size_t match_pos = 0;
    size_t const length = state->impl->search(window, start_pos, position, lookahead, &match_pos);

    if (length > 0) {
        token->distance = position - match_pos;
        token->length = length;
    }

    return true;
}

static struct yaz0_token
find_match(void* self,
           uint8_t const* window, size_t const window_size,
           const size_t position) {
    struct matcher_state* state = self;

    if (state->deferred) {
        state->deferred = false;
        return (struct yaz0_token){
            .length = state->deferred_length,
            .distance = state->deferred_distance,
        };
    }

    struct yaz0_token match = {0};
    bool has_searched = search(state, window, window_size, position, &match);

    if (!has_searched) {
        return match;
    }

    if (match.length >= YAZ0_MIN_MATCH) {
        struct yaz0_token deferred = {0};
        has_searched = search(state, window, window_size, position + 1, &deferred);
        if (!has_searched) {
            return match;
        }

        if (deferred.length >= match.length + 2) {
            state->deferred = true;
            state->deferred_length = deferred.length;
            state->deferred_distance = deferred.distance;
            match.length = 1;
            match.distance = deferred.distance;
        }
    }

    return match;
}

static size_t
state_size(void) {
    return sizeof(struct matcher_state);
}

static size_t
state_align(void) {
    return sizeof(void*);
}

static void
init(void* self, struct yaz0_matcher_config const* cfg) {
    struct matcher_state* state = self;

    state->deferred = false;
    state->deferred_length = 0;
    state->deferred_distance = 0;
    state->max_distance = cfg->max_distance;
    state->impl = yaz0_search_select(cfg->search);
}

static void
slide(void* self, size_t const drop) {
    (void) self;
    (void) drop;
}

struct yaz0_matcher_impl const yaz0_matcher_standard = {
    .id = YAZ0_MATCHER_STANDARD,
    .name = "std",
    .supported = NULL,
    .state_size = state_size,
    .state_align = state_align,
    .init = init,
    .slide = slide,
    .find = find_match,
};
