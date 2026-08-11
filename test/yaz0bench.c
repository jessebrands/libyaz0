/* yaz0bench.c: libyaz0 benchmark
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

#if defined _WIN32
#  include <windows.h>

static double
bench_now(void) {
    static double period = 0.0;
    if (period == 0.0) {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        period = 1.0 / (double) freq.QuadPart;
    }

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (double) now.QuadPart * period;
}
#else
#  define _POSIX_C_SOURCE 199309L
#  include <time.h>
static double bench_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double) ts.tv_sec + (double) ts.tv_nsec * 1e-9;
}
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "yaz0/yaz0.h"
#include "test_corpus.h"

#define BENCHMARK_BUDGET       2.5
#define BENCHMARK_PAYLOAD_SIZE (256 * 1024)
#define BENCHMARK_MIN_ROUNDS   5
#define BENCHMARK_MAX_IMPLS    8

static enum yaz0_search const benchmark_searches[] = {
    YAZ0_SEARCH_SCALAR,
    YAZ0_SEARCH_SSE2,
    YAZ0_SEARCH_AVX2,
    YAZ0_SEARCH_AVX512,
    YAZ0_SEARCH_NEON,
    YAZ0_SEARCH_SIMD128,
    YAZ0_SEARCH_SWAR64,
};

struct benchmark {
    int level;
    size_t iterations;
    enum corpus_profile profile;

    size_t compressed; // how many bytes were compressed
    double ratio; // compression ratio; should never change between runs!
    double elapsed;
};

struct benchmark_impl {
    enum yaz0_search search;
    struct yaz0_stream stream;
    size_t produced;
    double best;
};

struct bench_totals {
    enum yaz0_search search[BENCHMARK_MAX_IMPLS];
    double seconds[BENCHMARK_MAX_IMPLS];
    size_t count;
};

static bool
bench_impl_init(struct benchmark_impl* impl, enum yaz0_search const search,
                uint32_t const src_size, int const level) {
    struct yaz0_compress_options options = yaz0_default_compress_options();
    options.level = level;
    options.search = search;

    *impl = (struct benchmark_impl){.search = search, .best = -1.0};

    return yaz0_compress_init_with_options(&impl->stream, src_size, options) == YAZ0_OK;
}

static size_t
bench_run_compress(struct benchmark_impl* impl, uint8_t const* src, size_t const src_size,
                   uint8_t* dst, size_t const dst_size, int const level) {
    struct yaz0_compress_options options = yaz0_default_compress_options();
    options.level = level;
    options.search = impl->search;

    if (yaz0_compress_reset(&impl->stream, (uint32_t) src_size, options) != YAZ0_OK) {
        return 0;
    }

    impl->stream.next_in = src;
    impl->stream.avail_in = src_size;
    impl->stream.next_out = dst;
    impl->stream.avail_out = dst_size;

    enum yaz0_result result;
    do {
        result = yaz0_compress(&impl->stream, YAZ0_FINISH);
    } while (result == YAZ0_OK);

    return result == YAZ0_STREAM_END ? impl->stream.total_out : 0;
}

static bool
bench_measure(struct benchmark_impl* impls, size_t const count,
              uint8_t const* src, size_t const src_size,
              uint8_t* dst, size_t const dst_size, uint8_t* golden,
              int const level, double const budget, size_t* rounds) {
    // Do an initial run to warm up caches and verify the algorithm.
    for (size_t i = 0; i < count; ++i) {
        size_t const produced = bench_run_compress(&impls[i], src, src_size, dst, dst_size, level);
        if (produced == 0) {
            return false;
        }
        impls[i].produced = produced;

        if (i == 0) {
            memcpy(golden, dst, produced);
        } else if (produced != impls[0].produced || memcmp(golden, dst, produced) != 0) {
            fprintf(stderr, "error: %s output differs from %s\n",
                    yaz0_search_name(impls[i].search), yaz0_search_name(impls[0].search));
            return false;
        }
    }

    double const start = bench_now();
    *rounds = 0;

    // We run over our search implementations in interleaving order to ensure
    // instructions aren't cached beyond what we expect.
    do {
        for (size_t i = 0; i < count; ++i) {
            double const t0 = bench_now();
            size_t const produced = bench_run_compress(&impls[i], src, src_size, dst, dst_size, level);
            double const elapsed = bench_now() - t0;

            if (produced != impls[i].produced) {
                return false;
            }
            if (impls[i].best < 0.0 || elapsed < impls[i].best) {
                impls[i].best = elapsed;
            }
        }
        ++(*rounds);
    } while (*rounds < BENCHMARK_MIN_ROUNDS || bench_now() - start < budget);

    return true;
}

static void
bench_compress(enum corpus_profile const profile, int const level,
               double const budget, bool const use_reference,
               struct bench_totals* const totals) {
    size_t const out_size = yaz0_compress_bound(BENCHMARK_PAYLOAD_SIZE);

    uint8_t* data = malloc(BENCHMARK_PAYLOAD_SIZE);
    uint8_t* out = malloc(out_size);
    uint8_t* golden = malloc(out_size);

    if (data == NULL || out == NULL || golden == NULL) {
        fprintf(stderr, "error: out of memory\n");
        goto done;
    }

    corpus_generate(data, BENCHMARK_PAYLOAD_SIZE, profile, 0x4D697865);

    struct benchmark_impl impls[BENCHMARK_MAX_IMPLS];
    size_t count = 0;

    if (use_reference && bench_impl_init(&impls[count], YAZ0_SEARCH_REFERENCE, BENCHMARK_PAYLOAD_SIZE, level)) {
        ++count;
    }

    for (size_t i = 0; i < sizeof benchmark_searches / sizeof benchmark_searches[0]; ++i) {
        if (count == BENCHMARK_MAX_IMPLS) {
            break;
        }
        if (bench_impl_init(&impls[count], benchmark_searches[i], BENCHMARK_PAYLOAD_SIZE, level)) {
            ++count;
        }
    }

    if (count == 0) {
        fprintf(stderr, "error: no search implementations available\n");
        goto done;
    }

    size_t rounds = 0;
    if (bench_measure(impls, count, data, BENCHMARK_PAYLOAD_SIZE,
                      out, out_size, golden, level, budget, &rounds)) {
        double const base = impls[0].best;

        for (size_t i = 0; i < count; ++i) {
            double const best = impls[i].best;

            printf("%15s/%1d  %-9s  %10.2f MB/s  %10.2f MB/s  %8.2f %%  %9.2f msec  %7.2fx  %6zu\n",
                   corpus_profile_name(profile),
                   level,
                   yaz0_search_name(impls[i].search),
                   (double) BENCHMARK_PAYLOAD_SIZE / best / 1024.0 / 1024.0,
                   (double) impls[i].produced / best / 1024.0 / 1024.0,
                   (double) impls[i].produced / (double) BENCHMARK_PAYLOAD_SIZE * 100.0,
                   best * 1000.0,
                   base / best,
                   rounds);

            totals->count = count;
            for (size_t k = 0; k < count; ++k) {
                totals->search[k] = impls[k].search;
                totals->seconds[k] += impls[k].best;
            }
        }
    }

    for (size_t i = 0; i < count; ++i) {
        yaz0_compress_end(&impls[i].stream);
    }

done:
    free(golden);
    free(out);
    free(data);
}

#if defined _WIN32
static void
bench_pin_cpu(void) {
    if (SetThreadAffinityMask(GetCurrentThread(), 1) == 0) {
        fprintf(stderr, "warning: could not pin to a single CPU\n");
    }
}
#else
static void bench_pin_cpu(void) {
    fprintf(stderr, "note: CPU pinning unavailable on this platform\n");
    fprintf(stderr, "note: benchmark results may not be completely reliable!\n");
    fprintf(stderr, "note: please implement bench_pin_cpu() for your platform!\n\n");
}
#endif

int main(int argc, char** argv) {
    double budget = BENCHMARK_BUDGET;
    bool reference = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--reference") == 0) {
            reference = true;
        } else if (strcmp(argv[i], "--budget") == 0 && i + 1 < argc) {
            budget = atof(argv[++i]);
        } else {
            fprintf(stderr, "Usage: %s [--reference] [--budget SECONDS]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    // Pin the benchmark to a single core, if the platform supports it.
    // Schedulers like to fuck with us otherwise =/
    bench_pin_cpu();

    static enum corpus_profile const profiles[] = {
        CORPUS_PROFILE_ZEROES,
        CORPUS_PROFILE_ALTERNATING,
        CORPUS_PROFILE_BINARY,
        CORPUS_PROFILE_PERIODIC,
        CORPUS_PROFILE_STRUCTURED,
        CORPUS_PROFILE_INCOMPRESSIBLE,
    };

    static int const levels[] = {0, 1, 6, 9};
    static struct bench_totals totals[sizeof levels / sizeof levels[0]];

    // Look ma, structured output!
    printf("%17s  %-9s  %15s  %15s  %10s  %14s  %8s  %6s\n",
           "name", "impl", "in", "out", "ratio", "best", "rel", "rounds");

    for (size_t p = 0; p < sizeof profiles / sizeof profiles[0]; ++p) {
        for (size_t l = 0; l < sizeof levels / sizeof levels[0]; ++l) {
            bench_compress(profiles[p], levels[l], budget, reference, &totals[l]);
        }
    }

    printf("\n%17s    %14s  %8s\n", "impl", "total", "rel");
    for (size_t l = 0; l < sizeof levels / sizeof levels[0]; ++l) {
        if (totals[l].count == 0) {
            continue;
        }

        for (size_t k = 0; k < totals[l].count; ++k) {
            printf("%15s/%1d    %11.2f ms  %7.2fx\n",
                   yaz0_search_name(totals[l].search[k]),
                   levels[l],
                   totals[l].seconds[k] * 1000.0,
                   totals[l].seconds[0] / totals[l].seconds[k]);
        }
    }

    return EXIT_SUCCESS;
}
