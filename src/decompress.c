/* decompress.c: Yaz0 decompressor implementation
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

#include "common.h"
#include "decompress.h"
#include "stream.h"

static struct yaz0_decompress_state*
yaz0_get_decompress_state(struct yaz0_stream const* stream) {
    assert(stream != NULL);

    if (stream == NULL || stream->state == NULL) {
        return NULL;
    }

    struct yaz0_decompress_state* state = stream->state;
    if (state->common.kind != YAZ0_KIND_DECOMPRESSOR) {
        return NULL;
    }

    return state;
}

static enum yaz0_step
decompress_error(struct yaz0_decompress_state* state, enum yaz0_result const code,
                 enum yaz0_result* result) {
    state->mode = YAZ0_DECOMPRESS_ERROR;
    state->error = code;
    *result = code;
    return YAZ0_STEP_RETURN;
}

static enum yaz0_step
decompress_suspend(enum yaz0_result* result) {
    *result = YAZ0_OK;
    return YAZ0_STEP_RETURN;
}

static enum yaz0_step
decompress_continue(struct yaz0_decompress_state* state, enum yaz0_decompress_mode const mode,
                    enum yaz0_result* result) {
    state->mode = mode;
    *result = YAZ0_OK;
    return YAZ0_STEP_CONTINUE;
}

enum yaz0_step
decompress_header(struct yaz0_decompress_state* state, enum yaz0_flush const flush, enum yaz0_result* result) {
    size_t const want = 16 - state->history_pos;
    uint8_t* header_buf = &state->history[state->history_pos];
    state->history_pos += yaz0_stream_read(state->common.stream, header_buf, want);

    bool const complete = state->history_pos == 16;
    if (!complete) {
        if (flush == YAZ0_FINISH) {
            return decompress_error(state, YAZ0_TRUNCATED, result);
        }

        return decompress_suspend(result);
    }

    enum yaz0_result const parse_result = yaz0_read_header(state->history, 16, &state->header);
    if (parse_result != YAZ0_OK) {
        return decompress_error(state, parse_result, result);
    }

    state->history_pos = 0;
    state->remaining = state->header.uncompressed_size;

    // An empty file is a valid file.
    if (state->remaining == 0) {
        return decompress_continue(state, YAZ0_DECOMPRESS_DONE, result);
    }

    return decompress_continue(state, YAZ0_DECOMPRESS_START_BLOCK, result);
}

static enum yaz0_step
decompress_start_block(struct yaz0_decompress_state* state, enum yaz0_flush const flush, enum yaz0_result* result) {
    if (!YAZ0_IO_SUCCESS(yaz0_stream_read_byte(state->common.stream, &state->group_bitmask))) {
        if (flush == YAZ0_FINISH) {
            return decompress_error(state, YAZ0_TRUNCATED, result);
        }

        return decompress_suspend(result);
    }

    state->group_remaining = YAZ0_TOKENS_PER_BLOCK;
    return decompress_continue(state, YAZ0_DECOMPRESS_TOKEN, result);
}

static enum yaz0_step
decompress_token(struct yaz0_decompress_state* state, enum yaz0_result* result) {
    if (state->remaining == 0) {
        return decompress_continue(state, YAZ0_DECOMPRESS_DONE, result);
    }

    if (state->group_remaining == 0) {
        return decompress_continue(state, YAZ0_DECOMPRESS_START_BLOCK, result);
    }

    bool const literal = state->group_bitmask & 0x80;
    state->group_bitmask <<= 1;
    state->group_remaining--;

    enum yaz0_decompress_mode const next_mode = literal
                                              ? YAZ0_DECOMPRESS_LITERAL
                                              : YAZ0_DECOMPRESS_DONE;

    return decompress_continue(state, next_mode, result);
}

static enum yaz0_step
decompress_literal(struct yaz0_decompress_state* state, enum yaz0_flush const flush, enum yaz0_result* result) {
    uint8_t b = 0;
    enum yaz0_io_result const io_result = yaz0_stream_copy_byte(state->common.stream, &b);

    if (!YAZ0_IO_SUCCESS(io_result)) {
        if (io_result == YAZ0_IO_READ_ERROR && flush == YAZ0_FINISH) {
            return decompress_error(state, YAZ0_TRUNCATED, result);
        }

        return decompress_suspend(result);
    }

    state->history[state->history_pos & (YAZ0_MAX_DISTANCE - 1)] = b;
    state->history_pos++;
    state->remaining--;

    return decompress_continue(state, YAZ0_DECOMPRESS_TOKEN, result);
}

enum yaz0_result
yaz0_decompress(struct yaz0_stream* stream, enum yaz0_flush const flush) {
    struct yaz0_decompress_state* state = yaz0_get_decompress_state(stream);
    if (state == NULL) {
        return YAZ0_STREAM_ERROR;
    }

    size_t const before_in = stream->avail_in;
    size_t const before_out = stream->avail_out;

    while (true) {
        enum yaz0_result result = YAZ0_OK;
        enum yaz0_step step;

        switch (state->mode) {
            case YAZ0_DECOMPRESS_HEADER:
                step = decompress_header(state, flush, &result);
                break;

            case YAZ0_DECOMPRESS_START_BLOCK:
                step = decompress_start_block(state, flush, &result);
                break;

            case YAZ0_DECOMPRESS_TOKEN:
                step = decompress_token(state, &result);
                break;

            case YAZ0_DECOMPRESS_LITERAL:
                step = decompress_literal(state, flush, &result);
                break;

            case YAZ0_DECOMPRESS_ERROR:
                return state->error;

            case YAZ0_DECOMPRESS_DONE:
                return YAZ0_OK;

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
yaz0_decompress_init(struct yaz0_stream* stream) {
    if (stream == NULL) {
        return YAZ0_STREAM_ERROR;
    }
    if ((stream->alloc != NULL) != (stream->free != NULL)) {
        return YAZ0_STREAM_ERROR;
    }

    stream->state = yaz0_alloc(stream, sizeof(struct yaz0_decompress_state));
    if (stream->state == NULL) {
        return YAZ0_MEMORY_ERROR;
    }

    struct yaz0_decompress_state* state = stream->state;
    state->common.stream = stream;
    state->common.kind = YAZ0_KIND_DECOMPRESSOR;

    // We store the allocator in the state, as the user might change it.
    state->common.opaque = stream->opaque;
    state->common.alloc = stream->alloc;
    state->common.free = stream->free;

    // Set the initial decompressor state.
    state->mode = YAZ0_DECOMPRESS_HEADER;
    state->history_pos = 0;

    return YAZ0_OK;
}

void
yaz0_decompress_end(struct yaz0_stream* stream) {
    struct yaz0_decompress_state const* state = yaz0_get_decompress_state(stream);
    if (state == NULL) {
        return;
    }

    yaz0_free(stream, stream->state);
    stream->state = NULL;
}
