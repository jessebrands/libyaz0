/*
 * search.c: longest substring match implementation
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
#include <string.h>

#include "search.h"

size_t
yaz0_length_reference(uint8_t const* a, uint8_t const* b, size_t const max_lookahead) {
    size_t i = 0;
    while (i < max_lookahead && a[i] == b[i]) {
        ++i;
    }
    return i;
}

size_t
yaz0_search_reference(uint8_t const* data, size_t const start_pos, size_t const offset,
                      size_t const max_lookahead, size_t* match_pos) {
    size_t longest_run = 0;

    for (size_t i = start_pos; i < offset; ++i) {
        size_t const run_length = yaz0_length_reference(&data[i], &data[offset], max_lookahead);
        if (run_length > longest_run) {
            *match_pos = i;
            longest_run = run_length;

            if (run_length == max_lookahead) {
                break;
            }
        }
    }

    return (longest_run >= YAZ0_MIN_MATCH) ? longest_run : 0;
}

static bool
yaz0_search_reference_supported(void) {
    return true;
}

size_t
yaz0_length_scalar(uint8_t const* a, uint8_t const* b, size_t const max_lookahead) {
    size_t i = 0;
    // If we have the headroom for it, we can scan 8 bytes at a time.
    while (max_lookahead - i >= sizeof(uint64_t)) {
        uint64_t x, y;
        memcpy(&x, a + i, sizeof x);
        memcpy(&y, b + i, sizeof y);
        if (x != y) {
            break;
        }
        i += sizeof(uint64_t);
    }
    // The wide scan can only tell us a block differs. Find the exact position where the match ends.
    while (i < max_lookahead && a[i] == b[i]) {
        ++i;
    }
    return i;
}

size_t
yaz0_search_scalar(uint8_t const* data, size_t const start_pos,
                   size_t const offset, size_t const max_lookahead, size_t* match_pos) {
    assert(data != NULL);
    assert(start_pos <= offset);
    assert(match_pos != NULL);

    size_t longest_run = 0;
    for (size_t i = start_pos; i < offset; ++i) {
        // Scan for a match, if we cannot find any we can bail immediately.
        uint8_t const* hit = memchr(&data[i], data[offset], offset - i);
        if (hit == NULL) {
            break;
        }

        // Update our scanning index, anything before this point is not a match so don't scan it again.
        i = (size_t) (hit - data);

        // A match can only be beaten if the first byte after the longest run matches.
        // This rejects the vast majority of potential matches and avoids the expensive count operation.
        if (longest_run > 0 && data[i + longest_run] != data[offset + longest_run]) {
            continue;
        }

        // We may have a potential match, count the length.
        size_t const run_length = yaz0_length_scalar(&data[i], &data[offset], max_lookahead);
        if (run_length > longest_run) {
            *match_pos = i;
            longest_run = run_length;

            if (run_length == max_lookahead) {
                break;
            }
        }
    }

    return (longest_run >= YAZ0_MIN_MATCH) ? longest_run : 0;
}

static bool
yaz0_search_scalar_supported(void) {
    return true;
}

static struct yaz0_search_impl const implementations[] = {
#if YAZ0_HAVE_AVX2
    {YAZ0_SEARCH_AVX2, "avx2", yaz0_search_avx2, yaz0_search_avx2_supported},
#endif
#if YAZ0_HAVE_SSE2
    {YAZ0_SEARCH_SSE2, "sse2", yaz0_search_sse2, yaz0_search_sse2_supported},
#endif
#if YAZ0_HAVE_SWAR64
    {YAZ0_SEARCH_SWAR64, "swar64", yaz0_search_swar64, yaz0_search_swar64_supported},
#endif
    {YAZ0_SEARCH_SCALAR, "scalar", yaz0_search_scalar, yaz0_search_scalar_supported},
    {YAZ0_SEARCH_REFERENCE, "reference", yaz0_search_reference, yaz0_search_reference_supported},
};

#define YAZ0_SEARCH_IMPL_COUNT (sizeof implementations / sizeof implementations[0])

struct yaz0_search_impl const*
yaz0_search_select(enum yaz0_search const search) {
    if (search == YAZ0_SEARCH_AUTO) {
        // Find the first supported option, which is the fastest by definition.
        for (size_t i = 0; i < YAZ0_SEARCH_IMPL_COUNT; ++i) {
            if (implementations[i].supported()) {
                return &implementations[i];
            }
        }
    } else {
        // Find the requested implementation and check if it's supported.
        for (size_t i = 0; i < YAZ0_SEARCH_IMPL_COUNT; ++i) {
            if (implementations[i].id == search && implementations[i].supported()) {
                return &implementations[i];
            }
        }
    }

    return NULL;
}

enum yaz0_search
yaz0_default_search(void) {
    struct yaz0_search_impl const* const impl = yaz0_search_select(YAZ0_SEARCH_AUTO);
    return impl != NULL ? impl->id : YAZ0_SEARCH_SCALAR;
}
