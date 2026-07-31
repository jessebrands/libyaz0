/* test_corpus.c: synthetic test corpus generator
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
#include <string.h>

#include "test_corpus.h"

#define CORPUS_MAX_PERIOD 64u
#define CORPUS_RECORD_SIZE 32u

//
// splitmix64
//
static uint64_t
corpus_next_uint64(uint64_t* state) {
    // @formatter:off
    uint64_t z = *state       += UINT64_C(0x9E3779B97F4A7C15);
             z = (z ^ z >> 30) * UINT64_C(0xBF58476D1CE4E5B9);
             z = (z ^ z >> 27) * UINT64_C(0x94D049BB133111EB);
    // @formatter:on

    return z ^ (z >> 31);
}

static uint64_t
corpus_below_uint64(uint64_t* state, size_t const bound) {
    assert(bound > 0);
    assert(bound <= UINT32_MAX);

    uint64_t const draw = corpus_next_uint64(state) >> 32;
    return draw * (uint64_t) bound >> 32;
}

static uint8_t
corpus_byte(uint64_t* state) {
    return (uint8_t) (corpus_next_uint64(state) >> 56);
}

//
// Full zeroes
//
static void
corpus_zeroes(uint8_t* const dst, size_t const size) {
    memset(dst, 0, size);
}

//
// Alternating runs of bytes
//
static void
corpus_runs(uint8_t* const dst, size_t const size, uint64_t* const state) {
    size_t i = 0;
    while (i < size) {
        size_t run = 1 + corpus_below_uint64(state, 64);
        if (run > size - i) {
            run = size - i;
        }
        memset(&dst[i], corpus_byte(state), run);
        i += run;
    }
}

//
// Two symbols
//
static void
corpus_binary(uint8_t* const dst, size_t const size, uint64_t* const state) {
    uint8_t symbols[2];
    symbols[0] = corpus_byte(state);
    do {
        symbols[1] = corpus_byte(state);
    } while (symbols[1] == symbols[0]);

    for (size_t i = 0; i < size; ++i) {
        dst[i] = symbols[(corpus_next_uint64(state) >> 63)];
    }
}

//
// Short repeated patterns
//
static void
corpus_periodic(uint8_t* const dst, size_t const size, uint64_t* const state) {
    uint64_t const period = 3 + corpus_below_uint64(state, CORPUS_MAX_PERIOD - 3);

    uint8_t pattern[CORPUS_MAX_PERIOD];
    for (size_t i = 0; i < period; ++i) {
        pattern[i] = corpus_byte(state);
    }

    for (size_t i = 0; i < size; ++i) {
        dst[i] = pattern[i % period];
    }

    size_t const mutations = size / 64;
    for (size_t i = 0; i < mutations; ++i) {
        dst[corpus_below_uint64(state, size)] = corpus_byte(state);
    }
}

//
// Stream that resembles structured data
//
static void
corpus_structured(uint8_t* const dst, size_t const size, uint64_t* const state) {
    uint8_t record[CORPUS_RECORD_SIZE];
    for (size_t i = 0; i < CORPUS_RECORD_SIZE; ++i) {
        record[i] = corpus_byte(state);
    }

    size_t i = 0;
    while (i < size) {
        size_t const remaining = size - i;
        size_t const chunk = remaining < CORPUS_RECORD_SIZE
                                 ? remaining
                                 : CORPUS_RECORD_SIZE;
        memcpy(&dst[i], record, chunk);

        for (size_t j = 0; j < CORPUS_RECORD_SIZE / 4; ++j) {
            record[corpus_below_uint64(state, CORPUS_RECORD_SIZE)] = corpus_byte(state);
        }

        i += chunk;
    }
}

//
// Incompressible byte stream generator
//
static void
corpus_random(uint8_t* dst, size_t const size, uint64_t* state) {
    for (size_t i = 0; i < size; ++i) {
        dst[i] = corpus_byte(state);
    }
}

char const* corpus_profile_name(enum corpus_profile const profile) {
    switch (profile) {
        case CORPUS_PROFILE_ZEROES: return "zeroes";
        case CORPUS_PROFILE_ALTERNATING: return "alternating";
        case CORPUS_PROFILE_BINARY: return "binary";
        case CORPUS_PROFILE_PERIODIC: return "periodic";
        case CORPUS_PROFILE_STRUCTURED: return "structured";
        case CORPUS_PROFILE_INCOMPRESSIBLE: return "incompressible";
    }
    return "invalid";
}

void corpus_generate(uint8_t* dst, size_t const size, enum corpus_profile const profile, const uint64_t seed) {
    assert(dst != NULL);

    // Mixing profile into seed keeps two corpora generated from the same seed from being equal.
    uint64_t state = seed ^ UINT64_C(0x2545F4914F6CDD1D) * (uint64_t) profile;

    switch (profile) {
        case CORPUS_PROFILE_ZEROES: {
            corpus_zeroes(dst, size);
            break;
        }

        case CORPUS_PROFILE_ALTERNATING: {
            corpus_runs(dst, size, &state);
            break;
        }

        case CORPUS_PROFILE_BINARY: {
            corpus_binary(dst, size, &state);
            break;
        }

        case CORPUS_PROFILE_PERIODIC: {
            corpus_periodic(dst, size, &state);
            break;
        }

        case CORPUS_PROFILE_STRUCTURED: {
            corpus_structured(dst, size, &state);
            break;
        }

        case CORPUS_PROFILE_INCOMPRESSIBLE:
        default: {
            corpus_random(dst, size, &state);
            break;
        }
    }
}
