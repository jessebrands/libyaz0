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
#include <stdint.h>
#include <string.h>

#include "compress.h"
#include "search.h"
#include "stream.h"

static int
compress_effective_level(int const level) {
    return (level == YAZ0_DEFAULT_COMPRESSION) ? YAZ0_BEST_COMPRESSION : level;
}

static size_t
compress_search_distance(int const level) {
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

static enum yaz0_result
compress_configure_matcher(struct yaz0_compress_state* state,
                           struct yaz0_matcher_config const* cfg) {
    struct yaz0_matcher_impl const* const matcher = yaz0_matcher_select(cfg);
    if (matcher == NULL) {
        return YAZ0_UNSUPPORTED;
    }

    if (state->matcher != matcher) {
        yaz0_free(state->common.stream, state->matcher_alloc);
        state->matcher_alloc = NULL;
        state->matcher_state = NULL;
    }

    size_t const want = matcher->state_size();
    if (want > 0 && state->matcher_alloc == NULL) {
        size_t const align = matcher->state_align();

        state->matcher_alloc = yaz0_alloc(state->common.stream, want + align - 1);
        if (state->matcher_alloc == NULL) {
            return YAZ0_MEMORY_ERROR;
        }

        uintptr_t const raw = (uintptr_t) state->matcher_alloc;
        state->matcher_state = (void*) ((raw + (align - 1)) & ~(uintptr_t) (align - 1));
    }

    state->matcher = matcher;
    matcher->init(state->matcher_state, cfg);

    return YAZ0_OK;
}

static enum yaz0_result
compress_configure(struct yaz0_compress_state* state,
                   uint32_t const uncompressed_size,
                   struct yaz0_compress_options const* options) {
    int const level = compress_effective_level(options->level);

    state->mode = YAZ0_COMPRESS_HEADER;
    state->error = YAZ0_OK;
    state->search_distance = compress_search_distance(level);

    struct yaz0_search_impl const* const impl = yaz0_search_select(options->search);
    state->search_id = (impl != NULL) ? impl->id : YAZ0_SEARCH_AUTO;

    struct yaz0_matcher_config const cfg = {
        .level = level,
        .max_distance = state->search_distance,
        .uncompressed_size = uncompressed_size,
        .search = options->search,
        .matcher = state->requested_matcher,
    };

    enum yaz0_result const configured = compress_configure_matcher(state, &cfg);
    if (configured != YAZ0_OK) {
        return configured;
    }

    state->uncompressed_size = uncompressed_size;
    state->alignment = options->alignment;
    memcpy(state->header_reserved, options->reserved, sizeof state->header_reserved);

    state->received = 0;
    state->window_pos = 0;
    state->window_size = 0;
    state->match_distance = 0;
    state->match_length = 0;

    memset(state->block, 0, sizeof state->block);
    state->block_pos = 1;
    state->block_out = 0;
    state->block_tokens = 0;

    return YAZ0_OK;
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
    state->matcher->slide(state->matcher_state, drop);
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

static enum yaz0_step
compress_find_match(struct yaz0_compress_state* state, enum yaz0_result* result) {
    struct yaz0_token const token = state->matcher->find(
        state->matcher_state,
        state->window,
        state->window_size,
        state->window_pos
    );

    assert(token.length == 1 || (token.length >= YAZ0_MIN_MATCH && token.length <= YAZ0_MAX_MATCH));
    assert(state->window_pos + token.length <= state->window_size);
    if (token.length >= YAZ0_MIN_MATCH) {
        assert(token.distance >= 1 && token.distance <= YAZ0_MAX_DISTANCE);
        assert(token.distance <= state->window_pos);
        for (size_t i = 0; i < token.length; ++i) {
            assert(state->window[state->window_pos + i]
                == state->window[state->window_pos - token.distance + i]);
        }
    }

    state->match_length = token.length;
    state->match_distance = token.distance;
    return compress_continue(state, YAZ0_COMPRESS_EMIT, result);
}

static enum yaz0_step
compress_emit(struct yaz0_compress_state* state, enum yaz0_result* result) {
    size_t const l = state->match_length;
    size_t const d = (l < YAZ0_MIN_MATCH) ? 0 : state->match_distance - YAZ0_DISTANCE_BIAS;
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
yaz0_compress_init_with_matcher(struct yaz0_stream* stream, uint32_t const uncompressed_size,
                                struct yaz0_compress_options const options,
                                enum yaz0_matcher const matcher) {
    enum yaz0_result const result =
        yaz0_compress_init_with_options(stream, uncompressed_size, options);
    if (result != YAZ0_OK) {
        return result;
    }

    if (matcher == YAZ0_MATCHER_AUTO) {
        return YAZ0_OK;
    }

    struct yaz0_compress_state* state = stream->state;
    state->requested_matcher = matcher;

    enum yaz0_result const reconfigured = compress_configure(state, uncompressed_size, &options);
    if (reconfigured != YAZ0_OK) {
        yaz0_compress_end(stream);
        return reconfigured;
    }

    return YAZ0_OK;
}

enum yaz0_matcher
yaz0_compress_matcher(struct yaz0_stream const* stream) {
    struct yaz0_compress_state const* state = yaz0_get_compress_state(stream);
    if (state == NULL || state->matcher == NULL) {
        return YAZ0_MATCHER_AUTO;
    }

    return state->matcher->id;
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

    // The allocation is not zeroed, and configuring the matcher inspects
    // these to decide whether it can reuse an existing block.
    state->matcher = NULL;
    state->matcher_state = NULL;
    state->matcher_alloc = NULL;
    state->requested_matcher = YAZ0_MATCHER_AUTO;

    enum yaz0_result const configured = compress_configure(state, uncompressed_size, &options);
    if (configured != YAZ0_OK) {
        yaz0_free(stream, stream->state);
        stream->state = NULL;
        return configured;
    }

    return YAZ0_OK;
}

void
yaz0_compress_end(struct yaz0_stream* stream) {
    struct yaz0_compress_state const* state = yaz0_get_compress_state(stream);
    if (state == NULL) {
        return;
    }

    yaz0_free(stream, state->matcher_alloc);
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

    return compress_configure(state, uncompressed_size, &options);
}

enum yaz0_search
yaz0_compress_search(struct yaz0_stream const* stream) {
    struct yaz0_compress_state const* state = yaz0_get_compress_state(stream);
    if (state == NULL) {
        return YAZ0_SEARCH_AUTO;
    }

    return state->search_id;
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
