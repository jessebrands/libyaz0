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
#include <stdbool.h>

#include "compress.h"

#include <string.h>

#include "stream.h"

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

static enum yaz0_step
compress_error(struct yaz0_compress_state* state, enum yaz0_result const code,
               enum yaz0_result* result) {
    state->mode = YAZ0_COMPRESS_ERROR;
    state->error = code;
    *result = code;
    return YAZ0_STEP_RETURN;
}

static enum yaz0_step
compress_suspend(enum yaz0_result* result) {
    *result = YAZ0_OK;
    return YAZ0_STEP_RETURN;
}

static enum yaz0_step
compress_continue(struct yaz0_compress_state* state, enum yaz0_compress_mode const mode,
                  enum yaz0_result* result) {
    state->mode = mode;
    *result = YAZ0_OK;
    return YAZ0_STEP_CONTINUE;
}

static enum yaz0_step
compress_header(struct yaz0_compress_state* state, enum yaz0_result* result) {
    struct yaz0_header header = {
        .uncompressed_size = state->uncompressed_size,
        .alignment = 0,
        .reserved = {0}
    };

    memcpy(&header.magic, YAZ0_MAGIC, sizeof header.magic);

    uint8_t header_buf[YAZ0_HEADER_SIZE];
    enum yaz0_result const header_result = yaz0_write_header(&header, header_buf, YAZ0_HEADER_SIZE);
    if (header_result != YAZ0_OK) {
        return compress_error(state, header_result, result);
    }

    size_t const want = YAZ0_HEADER_SIZE - state->window_pos;
    uint8_t const* in_ptr = &header_buf[state->window_pos];
    state->window_pos += yaz0_stream_write(state->common.stream, in_ptr, want);

    bool const complete = state->window_pos == YAZ0_HEADER_SIZE;
    if (!complete) {
        return compress_suspend(result);
    }

    state->window_pos = 0;

    return compress_continue(state, YAZ0_COMPRESS_DONE, result);
}

enum yaz0_result
yaz0_compress(struct yaz0_stream* stream, enum yaz0_flush const flush) {
    struct yaz0_compress_state* state = yaz0_get_compress_state(stream);
    if (state == NULL) {
        return YAZ0_STREAM_ERROR;
    }

    size_t const before_in = stream->avail_in;
    size_t const before_out = stream->avail_out;

    while (true) {
        enum yaz0_result result = YAZ0_OK;
        enum yaz0_step step;

        switch (state->mode) {
            case YAZ0_COMPRESS_HEADER:
                step = compress_header(state, &result);
                break;

            case YAZ0_COMPRESS_ERROR:
                return state->error;

            case YAZ0_COMPRESS_DONE:
                return YAZ0_STREAM_END;

            default:
                return YAZ0_STREAM_ERROR;
        }

        if (step == YAZ0_STEP_RETURN) {
            // If we're not moving bytes, we're stalling.
            if (result == YAZ0_OK && stream->avail_in == before_in && stream->avail_out == before_out) {
                return YAZ0_BUFFER_ERROR;
            }
            return result;
        }
    }
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

    state->mode = YAZ0_COMPRESS_HEADER;
    state->level = level;
    state->uncompressed_size = uncompressed_size;
    state->window_pos = 0;

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
