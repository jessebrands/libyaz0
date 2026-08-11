/*
 * test_search.c: search algorithm test harness
 * Copyright (C) 2026 Jesse Gerard Brands
 *
 * This file is part of libyaz0.
 *
 * libyaz0 is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * libyaz0 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libyaz0. If not, see <https://www.gnu.org/licenses/>.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "search.h"
#include "test_corpus.h"

#define EXIT_SKIP 2
#define MAX_REPORTED_FAILURES 10
#define TEST_SEARCH_DISTANCE (YAZ0_MAX_DISTANCE - 1)

struct search_opts {
    enum yaz0_search search;
    enum corpus_profile corpus;
};

struct run_info {
    char const* phase;
    enum corpus_profile profile;
    uint64_t seed;
    size_t size;
    uint8_t poison;
};

static bool
parse_opts(int argc, char** argv, struct search_opts* opts) {
    *opts = (struct search_opts){0};

    if (argc != 3) {
        fprintf(stderr, "Usage: test_search <algorithm> <corpus>\n");
        return false;
    }

    if (strcmp(argv[1], "reference") == 0) {
        // included as a smoke test
        opts->search = YAZ0_SEARCH_REFERENCE;
    } else if (strcmp(argv[1], "scalar") == 0) {
        opts->search = YAZ0_SEARCH_SCALAR;
    } else if (strcmp(argv[1], "sse2") == 0) {
        opts->search = YAZ0_SEARCH_SSE2;
    } else if (strcmp(argv[1], "avx2") == 0) {
        opts->search = YAZ0_SEARCH_AVX2;
    } else if (strcmp(argv[1], "avx512") == 0) {
        opts->search = YAZ0_SEARCH_AVX512;
    } else if (strcmp(argv[1], "neon") == 0) {
        opts->search = YAZ0_SEARCH_NEON;
    } else if (strcmp(argv[1], "simd128") == 0) {
        opts->search = YAZ0_SEARCH_SIMD128;
    } else if (strcmp(argv[1], "swar64") == 0) {
        opts->search = YAZ0_SEARCH_SWAR64;
    } else {
        fprintf(stderr, "error: unknown algorithm: %s\n", argv[1]);
        return false;
    }

    if (strcmp(argv[2], "zeroes") == 0) {
        opts->corpus = CORPUS_PROFILE_ZEROES;
    } else if (strcmp(argv[2], "alternating") == 0) {
        opts->corpus = CORPUS_PROFILE_ALTERNATING;
    } else if (strcmp(argv[2], "binary") == 0) {
        opts->corpus = CORPUS_PROFILE_BINARY;
    } else if (strcmp(argv[2], "periodic") == 0) {
        opts->corpus = CORPUS_PROFILE_PERIODIC;
    } else if (strcmp(argv[2], "structured") == 0) {
        opts->corpus = CORPUS_PROFILE_STRUCTURED;
    } else if (strcmp(argv[2], "incompressible") == 0) {
        opts->corpus = CORPUS_PROFILE_INCOMPRESSIBLE;
    } else {
        fprintf(stderr, "error: invalid corpus: %s\n", argv[2]);
        return false;
    }

    return true;
}

static unsigned long long comparisons;
static unsigned long long failures;

static void
case_compare(struct yaz0_search_impl const* reference,
             struct yaz0_search_impl const* target,
             uint8_t const* data, size_t const start_pos, size_t const offset,
             size_t const max_lookahead, struct run_info const* info) {
    size_t ref_pos = SIZE_MAX;
    size_t tgt_pos = SIZE_MAX;

    size_t const ref_len = reference->search(data, start_pos, offset, max_lookahead, &ref_pos);
    size_t const tgt_len = target->search(data, start_pos, offset, max_lookahead, &tgt_pos);

    ++comparisons;

    // The match position is only meaningful when something was found.
    bool diverged = ref_len != tgt_len;
    if (!diverged && ref_len > 0) {
        diverged = ref_pos != tgt_pos;
    }

    if (!diverged) {
        return;
    }

    ++failures;
    if (failures > MAX_REPORTED_FAILURES) {
        return;
    }

    fprintf(stderr,
            "FAILED: [%s] profile=%s seed=%" PRIu64 " size=%zu poison=0x%02X\n"
            "        start_pos=%zu offset=%zu max_lookahead=%zu\n"
            "        %-9s length=%zu match_pos=%zu\n"
            "        %-9s length=%zu match_pos=%zu\n",
            info->phase, corpus_profile_name(info->profile), info->seed, info->size, info->poison,
            start_pos, offset, max_lookahead,
            yaz0_search_name(reference->id), ref_len, ref_pos,
            yaz0_search_name(target->id), tgt_len, tgt_pos
    );
}

static bool
case_lookahead(size_t const size, size_t const offset, size_t* max_lookahead) {
    size_t lookahead = size - offset;
    if (lookahead > YAZ0_MAX_MATCH) {
        lookahead = YAZ0_MAX_MATCH;
    }
    if (lookahead < YAZ0_MIN_MATCH) {
        return false;
    }

    *max_lookahead = lookahead;
    return true;
}

static void
fill(uint8_t* buffer, struct run_info const* info) {
    corpus_generate(buffer, info->size, info->profile, info->seed);
    memset(&buffer[info->size], info->poison, YAZ0_WINDOW_PADDING);
}

static void
sweep_exhaustive(struct yaz0_search_impl const* reference, struct yaz0_search_impl const* target,
                 uint8_t* buffer, struct run_info* info) {
    info->phase = "exhaustive";

    for (size_t size = 4; size <= 96; ++size) {
        for (unsigned trial = 0; trial < 8; ++trial) {
            info->size = size;
            info->seed = (uint64_t) size * 1000003u + trial;
            fill(buffer, info);

            for (size_t offset = 1; offset < size; ++offset) {
                size_t max_lookahead;
                if (!case_lookahead(size, offset, &max_lookahead)) {
                    continue;
                }

                for (size_t start_pos = 0; start_pos <= offset; ++start_pos) {
                    case_compare(reference, target, buffer, start_pos, offset,
                                 max_lookahead, info);
                }
            }
        }
    }
}

static void
sweep_window(struct yaz0_search_impl const* reference, struct yaz0_search_impl const* target,
             uint8_t* buffer, struct run_info* info) {
    info->phase = "window";

    for (unsigned trial = 0; trial < 2; ++trial) {
        info->size = YAZ0_WINDOW_SIZE;
        info->seed = 0xCAFEBABEu + trial; // we love java, right?
        fill(buffer, info);

        for (size_t offset = 1; offset < info->size; ++offset) {
            size_t max_lookahead;
            if (!case_lookahead(info->size, offset, &max_lookahead)) {
                continue;
            }

            case_compare(reference, target, buffer, 0, offset, max_lookahead, info);

            if (offset > TEST_SEARCH_DISTANCE) {
                size_t const start_pos = offset - (TEST_SEARCH_DISTANCE + 1);
                case_compare(reference, target, buffer, start_pos, offset,
                             max_lookahead, info);
            }
        }
    }
}

int main(int argc, char** argv) {
    struct search_opts opts;
    if (!parse_opts(argc, argv, &opts)) {
        return EXIT_FAILURE;
    }

    struct yaz0_search_impl const* reference = yaz0_search_select(YAZ0_SEARCH_REFERENCE);
    struct yaz0_search_impl const* target = yaz0_search_select(opts.search);

    if (reference == NULL) {
        fprintf(stderr, "error: could not get reference implementation!!\n");
        return EXIT_FAILURE;
    }

    if (target == NULL) {
        fprintf(stdout, "search algorithm '%s' is not implemented on this platform, skipping test\n",
                yaz0_search_name(opts.search));
        return EXIT_SKIP;
    }

    uint8_t* const buffer = malloc(YAZ0_WINDOW_SIZE + YAZ0_WINDOW_PADDING);
    if (buffer == NULL) {
        fprintf(stderr, "error: out of memory\n");
        return EXIT_FAILURE;
    }

    // The padding exists so vector searches may over-read past the data.
    // Make sure searches never read this data...
    static uint8_t const poisons[] = {0x00, 0xFF};

    for (size_t p = 0; p < sizeof poisons / sizeof poisons[0]; ++p) {
        struct run_info info = {
            .profile = opts.corpus,
            .poison = poisons[p],
        };

        sweep_exhaustive(reference, target, buffer, &info);
        sweep_window(reference, target, buffer, &info);
    }

    free(buffer);

    if (comparisons == 0) {
        fprintf(stderr, "error: no cases were compared\n");
        return EXIT_FAILURE;
    }

    printf("%s vs %s on %s: %llu comparisons, %llu failures\n",
           yaz0_search_name(target->id), yaz0_search_name(reference->id),
           corpus_profile_name(opts.corpus), comparisons, failures);

    return failures == 0
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
