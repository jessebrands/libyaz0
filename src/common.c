/*
 * common.c: common library utilities and allocator
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
#include <stdlib.h>

#include "common.h"

static struct yaz0_common_state*
yaz0_get_common_state(struct yaz0_stream const* stream) {
    assert(stream != NULL);

    if (stream == NULL || stream->state == NULL) {
        return NULL;
    }
    return stream->state;
}

void*
yaz0_alloc(struct yaz0_stream const* stream, size_t const size) {
    assert(stream != NULL);

    if (stream == NULL) {
        return NULL;
    }

    if (stream->state == NULL && stream->alloc != NULL) {
        return stream->alloc(stream->opaque, size);
    }

    struct yaz0_common_state const* state = yaz0_get_common_state(stream);
    if (state != NULL && state->alloc != NULL) {
        return state->alloc(state->opaque, size);
    }

    return malloc(size);
}

void
yaz0_free(struct yaz0_stream const* stream, void* ptr) {
    assert(stream != NULL);

    if (stream == NULL) {
        return;
    }

    if (stream->state == NULL && stream->free != NULL) {
        stream->free(stream->opaque, ptr);
        return;
    }

    struct yaz0_common_state const* state = yaz0_get_common_state(stream);
    if (state != NULL && state->free != NULL) {
        state->free(state->opaque, ptr);
        return;
    }

    free(ptr);
}
