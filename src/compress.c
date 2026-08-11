/*
 * compress.c: Yaz0 compressor
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
#include <string.h>

#include "compress.h"
#include "search.h"
#include "stream.h"

static size_t
compress_search_distance(int level) {
    if (level == YAZ0_DEFAULT_COMPRESSION) {
        level = YAZ0_BEST_COMPRESSION;
    }
    if (level == YAZ0_NO_COMPRESSION) {
        return 0;
    }
    int const max_search = YAZ0_MAX_DISTANCE - 1;
    return (size_t) (max_search * (level + 1)) / 10;
}

static enum yaz0_result
compress_validate_options(struct yaz0_compress_options const* options) {
    if ((options->level < YAZ0_NO_COMPRESSION || options->level > YAZ0_BEST_COMPRESSION)
        && options->level != YAZ0_DEFAULT_COMPRESSION) {
        return YAZ0_STREAM_ERROR;
    }

    if (yaz0_search_select(options->search) == NULL) {
        return YAZ0_UNSUPPORTED;
    }

    return YAZ0_OK;
}

static void
compress_configure(struct yaz0_compress_state* state, uint32_t const uncompressed_size,
                   struct yaz0_compress_options const* options) {
    state->mode = YAZ0_COMPRESS_HEADER;
    state->error = YAZ0_OK;
    state->search_distance = compress_search_distance(options->level);
    state->search = yaz0_search_select(options->search);

    state->uncompressed_size = uncompressed_size;
    state->alignment = options->alignment;
    memcpy(state->header_reserved, options->reserved, sizeof state->header_reserved);

    state->received = 0;
    state->window_pos = 0;
    state->window_size = 0;
    state->match_distance = 0;
    state->match_length = 0;
    state->deferred = false;
    state->deferred_distance = 0;
    state->deferred_length = 0;

    memset(state->block, 0, sizeof state->block);
    state->block_pos = 1;
    state->block_out = 0;
    state->block_tokens = 0;
}

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
        .alignment = state->alignment,
    };

    memcpy(&header.magic, YAZ0_MAGIC, sizeof header.magic);
    memcpy(header.reserved, state->header_reserved, sizeof header.reserved);

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
    state->window_size = 0;

    return compress_continue(state, YAZ0_COMPRESS_FILL, result);
}

static void
compress_slide_window(struct yaz0_compress_state* state) {
    if (state->window_pos < 2 * YAZ0_MAX_DISTANCE) {
        return;
    }

    size_t const drop = YAZ0_MAX_DISTANCE;
    memmove(state->window, &state->window[drop], state->window_size - drop);
    state->window_pos -= drop;
    state->window_size -= drop;
}

static enum yaz0_step
compress_fill(struct yaz0_compress_state* state, enum yaz0_flush const flush, enum yaz0_result* result) {
    assert(state->window_pos <= state->window_size);
    compress_slide_window(state);

    size_t const min_lookahead = (flush == YAZ0_FINISH) ? 1 : (YAZ0_MAX_MATCH + 1);
    size_t const want = YAZ0_WINDOW_SIZE - state->window_size;
    uint8_t* out = &state->window[state->window_size];

    size_t const bytes_in = yaz0_stream_read(state->common.stream, out, want);
    state->received += bytes_in;
    state->window_size += bytes_in;

    if (state->received > state->uncompressed_size) {
        return compress_error(state, YAZ0_SIZE_MISMATCH, result);
    }

    size_t const lookahead = state->window_size - state->window_pos;
    if (lookahead >= min_lookahead) {
        return compress_continue(state, YAZ0_COMPRESS_FIND_MATCH, result);
    }
    if (flush != YAZ0_FINISH) {
        return compress_suspend(result);
    }

    return compress_continue(state, YAZ0_COMPRESS_FLUSH, result);
}

static bool
compress_search(struct yaz0_compress_state* state, size_t const position,
                size_t* match_distance, size_t* match_length) {
    *match_distance = 0;
    *match_length = 1;

    assert(position < state->window_size);
    size_t lookahead = state->window_size - position;
    if (lookahead > YAZ0_MAX_MATCH) {
        lookahead = YAZ0_MAX_MATCH;
    }

    if (state->search_distance == 0 || lookahead < YAZ0_MIN_MATCH) {
        return false;
    }

    size_t start_pos = 0;
    if (position > state->search_distance) {
        start_pos = position - (state->search_distance + 1);
    }

    size_t match_pos = 0;
    size_t const length = state->search->search(state->window, start_pos, position, lookahead, &match_pos);
    if (length > 0) {
        *match_distance = position - match_pos - 1;
        *match_length = length;
    }

    return true;
}

static enum yaz0_step
compress_find_match(struct yaz0_compress_state* state, enum yaz0_result* result) {
    if (state->deferred) {
        state->deferred = false;
        state->match_distance = state->deferred_distance;
        state->match_length = state->deferred_length;
        return compress_continue(state, YAZ0_COMPRESS_EMIT, result);
    }

    bool has_searched = compress_search(
        state, state->window_pos,
        &state->match_distance,
        &state->match_length
    );

    if (!has_searched) {
        return compress_continue(state, YAZ0_COMPRESS_EMIT, result);
    }

    if (state->match_length >= YAZ0_MIN_MATCH) {
        has_searched = compress_search(
            state, state->window_pos + 1,
            &state->deferred_distance,
            &state->deferred_length
        );
        if (!has_searched) {
            return compress_continue(state, YAZ0_COMPRESS_EMIT, result);
        }

        if (state->deferred_length >= state->match_length + 2) {
            state->deferred = true;
            state->match_length = 1;
            state->match_distance = state->deferred_distance;
        }
    }

    return compress_continue(state, YAZ0_COMPRESS_EMIT, result);
}

static enum yaz0_step
compress_emit(struct yaz0_compress_state* state, enum yaz0_result* result) {
    size_t const l = state->match_length;
    size_t const d = state->match_distance;
    size_t const consumed = (l < YAZ0_MIN_MATCH) ? 1 : l;

    if (consumed > state->window_size - state->window_pos) {
        return compress_error(state, YAZ0_STREAM_ERROR, result);
    }

    // TODO: This section could do with improved clarity.
    if (l < YAZ0_MIN_MATCH) {
        state->block[state->block_pos++] = state->window[state->window_pos];
        state->block[0] |= (uint8_t) (0x80 >> state->block_tokens);
    } else if (l < 18) {
        state->block[state->block_pos++] = (uint8_t) (((l - 2) << 4) | (d >> 8));
        state->block[state->block_pos++] = (uint8_t) (d & 0xFF);
    } else {
        state->block[state->block_pos++] = (uint8_t) (d >> 8);
        state->block[state->block_pos++] = (uint8_t) (d & 0xFF);
        state->block[state->block_pos++] = (uint8_t) (l - 18);
    }

    state->block_tokens++;
    state->window_pos += consumed;

    if (state->block_tokens == YAZ0_TOKENS_PER_BLOCK) {
        return compress_continue(state, YAZ0_COMPRESS_WRITE_BLOCK, result);
    }

    return compress_continue(state, YAZ0_COMPRESS_FILL, result);
}

static enum yaz0_step
compress_write_block(struct yaz0_compress_state* state, enum yaz0_result* result) {
    size_t const have = state->block_pos - state->block_out;
    uint8_t const* src = &state->block[state->block_out];
    state->block_out += yaz0_stream_write(state->common.stream, src, have);

    if (state->block_out != state->block_pos) {
        return compress_suspend(result);
    }

    state->block[0] = 0;
    state->block_pos = 1;
    state->block_out = 0;
    state->block_tokens = 0;

    return compress_continue(state, YAZ0_COMPRESS_FILL, result);
}

static enum yaz0_step
compress_flush(struct yaz0_compress_state* state, enum yaz0_result* result) {
    if (state->received != state->uncompressed_size) {
        return compress_error(state, YAZ0_SIZE_MISMATCH, result);
    }

    if (state->block_tokens != 0) {
        size_t const have = state->block_pos - state->block_out;
        uint8_t const* src = &state->block[state->block_out];
        state->block_out += yaz0_stream_write(state->common.stream, src, have);

        if (state->block_out != state->block_pos) {
            return compress_suspend(result);
        }
    }

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

            case YAZ0_COMPRESS_FILL:
                step = compress_fill(state, flush, &result);
                break;

            case YAZ0_COMPRESS_FIND_MATCH:
                step = compress_find_match(state, &result);
                break;

            case YAZ0_COMPRESS_EMIT:
                step = compress_emit(state, &result);
                break;

            case YAZ0_COMPRESS_WRITE_BLOCK:
                step = compress_write_block(state, &result);
                break;

            case YAZ0_COMPRESS_FLUSH:
                step = compress_flush(state, &result);
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

struct yaz0_compress_options
yaz0_default_compress_options(void) {
    return (struct yaz0_compress_options){
        .level = YAZ0_DEFAULT_COMPRESSION,
        .alignment = 0,
        .reserved = {0},
        .search = YAZ0_SEARCH_AUTO,
    };
}

enum yaz0_result
yaz0_compress_init(struct yaz0_stream* stream, int const level,
                   uint32_t const uncompressed_size) {
    struct yaz0_compress_options options = yaz0_default_compress_options();
    options.level = level;

    return yaz0_compress_init_with_options(stream, uncompressed_size, options);
}

enum yaz0_result
yaz0_compress_init_with_options(struct yaz0_stream* stream, uint32_t const uncompressed_size,
                                struct yaz0_compress_options const options) {
    if (stream == NULL) {
        return YAZ0_STREAM_ERROR;
    }
    if ((stream->alloc != NULL) != (stream->free != NULL)) {
        return YAZ0_STREAM_ERROR;
    }

    enum yaz0_result const valid = compress_validate_options(&options);
    if (valid != YAZ0_OK) {
        return valid;
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

    compress_configure(state, uncompressed_size, &options);

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

enum yaz0_result
yaz0_compress_reset(struct yaz0_stream* stream, uint32_t const uncompressed_size,
                    struct yaz0_compress_options const options) {
    struct yaz0_compress_state* state = yaz0_get_compress_state(stream);
    if (state == NULL) {
        return YAZ0_STREAM_ERROR;
    }

    enum yaz0_result const valid = compress_validate_options(&options);
    if (valid != YAZ0_OK) {
        return valid;
    }

    stream->total_in = 0;
    stream->total_out = 0;

    compress_configure(state, uncompressed_size, &options);

    return YAZ0_OK;
}

enum yaz0_search
yaz0_compress_search(struct yaz0_stream const* stream) {
    struct yaz0_compress_state const* state = yaz0_get_compress_state(stream);
    if (state == NULL) {
        return YAZ0_SEARCH_AUTO;
    }

    return state->search->id;
}

size_t
yaz0_compress_bound(uint32_t const uncompressed_size) {
    uint64_t const bound = (uint64_t) YAZ0_HEADER_SIZE
                           + (uint64_t) uncompressed_size
                           + ((uint64_t) uncompressed_size + 7) / 8;

#if SIZE_MAX < UINT64_MAX
    if (bound > (uint64_t) SIZE_MAX) {
        return 0;
    }
#endif

    return (size_t) bound;
}
