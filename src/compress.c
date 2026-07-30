/* compress.c: Yaz0 compressor implementation
   Copyright (C) 2026 Jesse Gerard Brands

   This file is part of libyaz0.

   libyaz0 is free software: you can redistribute it and/or modify it under
   the terms of the GNU Lesser General Public License as published by the Free
   Software Foundation, either version 3 of the License, or (at your option)
   any later version.

   libyaz0 is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
   more details.

   You should have received a copy of the GNU Lesser General Public License
   along with libyaz0. If not, see <https://www.gnu.org/licenses/>.
*/

#include <assert.h>

#include "common.h"
#include "compress.h"

static struct yaz0_compress_state*
yaz0_get_compress_state(struct yaz0_stream const* stream) {
    assert(stream != NULL);

    if (stream == NULL || stream->state == NULL) {
        return NULL;
    }

    struct yaz0_compress_state* state = stream->state;
    if (state->common.kind != YAZ0_KIND_COMPRESSOR) {
        return NULL;
    }

    return state;
}

enum yaz0_result
yaz0_compress_init(struct yaz0_stream* stream, int const level,
                   uint32_t const uncompressed_size) {
    if (stream == NULL) {
        return YAZ0_STREAM_ERROR;
    }
    if ((stream->alloc != NULL) != (stream->free != NULL)) {
        return YAZ0_STREAM_ERROR;
    }

    stream->state = yaz0_alloc(stream, sizeof(struct yaz0_compress_state));
    if (stream->state == NULL) {
        return YAZ0_MEMORY_ERROR;
    }

    struct yaz0_compress_state* state = stream->state;
    state->common.stream = stream;
    state->common.kind = YAZ0_KIND_COMPRESSOR;

    // We store the allocator in the state, as the user might change it.
    state->common.opaque = stream->opaque;
    state->common.alloc = stream->alloc;
    state->common.free = stream->free;

    return YAZ0_OK;
}

enum yaz0_result
yaz0_compress(struct yaz0_stream* stream, enum yaz0_flush const flush) {
    struct yaz0_compress_state const* state = yaz0_get_compress_state(stream);
    if (state == NULL) {
        return YAZ0_STREAM_ERROR;
    }

    return YAZ0_OK;
}

void
yaz0_compress_end(struct yaz0_stream* stream) {
    struct yaz0_compress_state const* state = yaz0_get_compress_state(stream);
    if (state == NULL) {
        return;
    }

    yaz0_free(stream, stream->state);
    stream->state = NULL;
}
