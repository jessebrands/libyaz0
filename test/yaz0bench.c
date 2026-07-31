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

static double bench_now(void) {
    LARGE_INTEGER freq, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    return (double) now.QuadPart / (double) freq.QuadPart;
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

#include <stdio.h>
#include <stdlib.h>

#include "yaz0/yaz0.h"
#include "test_corpus.h"

#define BENCHMARK_BUDGET       2.5
#define BENCHMARK_PAYLOAD_SIZE (256 * 1024)

struct benchmark {
    int level;
    size_t iterations;
    enum corpus_profile profile;

    size_t compressed; // how many bytes were compressed
    double ratio; // compression ratio; should never change between runs!
    double elapsed;
};

static size_t
bench_run_compress(uint8_t const* src, size_t const src_size,
                   uint8_t* dst, size_t const dst_size, int const level) {
    struct yaz0_stream stream = {0};
    if (yaz0_compress_init(&stream, level, (uint32_t) src_size) != YAZ0_OK) {
        return 0;
    }

    stream.next_in = src;
    stream.avail_in = src_size;
    stream.next_out = dst;
    stream.avail_out = dst_size;

    enum yaz0_result result;
    do {
        result = yaz0_compress(&stream, YAZ0_FINISH);
    } while (result == YAZ0_OK);

    size_t const produced = stream.total_out;
    yaz0_compress_end(&stream);

    return result == YAZ0_STREAM_END ? produced : 0;
}

static double
bench_measure(struct benchmark* bench,
              uint8_t const* src, size_t const src_size,
              uint8_t* dst, size_t const dst_size) {
    size_t const expected = bench_run_compress(src, src_size, dst, dst_size, bench->level);
    if (expected == 0) {
        return -1.0;
    }

    double best = -1.0;
    double const start = bench_now();

    do {
        double const t0 = bench_now();
        size_t const produced = bench_run_compress(src, src_size, dst, dst_size, bench->level);
        double const elapsed = bench_now() - t0;

        if (produced != expected) {
            return -1.0;
        }
        if (best < 0.0 || elapsed < best) {
            best = elapsed;
        }

        bench->iterations++;
    } while (bench_now() - start < BENCHMARK_BUDGET);
    double const end = bench_now();

    bench->compressed = expected;
    bench->ratio = (double) expected / (double) src_size;
    bench->elapsed = end - start;
    return best;
}

static void
bench_compress(struct benchmark* bench) {
    uint8_t* data = malloc(BENCHMARK_PAYLOAD_SIZE);
    if (data == NULL) {
        return;
    }

    corpus_generate(data, BENCHMARK_PAYLOAD_SIZE, bench->profile, 0x4D697865);

    size_t const out_size = 16 + BENCHMARK_PAYLOAD_SIZE + (BENCHMARK_PAYLOAD_SIZE + 7) / 8;
    uint8_t* out = malloc(out_size);
    if (out == NULL) {
        free(data);
        return;
    }

    double const best = bench_measure(bench, data, BENCHMARK_PAYLOAD_SIZE, out, out_size);

    printf("\t%15s  %-5d  %10.2f MB/s  %10.2f MB/s  %8.2f %%  %9.2f msec\n",
           corpus_profile_name(bench->profile),
           bench->level,
           (double) BENCHMARK_PAYLOAD_SIZE / best / 1024.0 / 1024.0,
           (double) bench->compressed / best / 1024.0 / 1024.0,
           bench->ratio * 100.0,
           best * 1000.0
    );

    free(out);
    free(data);
}

int main(int argc, char** argv) {
    printf("\t%15s  %-5s  %15s  %15s  %10s  %14s\n",
           "name", "level", "in", "out", "ratio", "best");

    for (int p = 0; p < 6; ++p) {
        enum corpus_profile const profiles[] = {
            CORPUS_PROFILE_ZEROES,
            CORPUS_PROFILE_ALTERNATING,
            CORPUS_PROFILE_BINARY,
            CORPUS_PROFILE_PERIODIC,
            CORPUS_PROFILE_STRUCTURED,
            CORPUS_PROFILE_INCOMPRESSIBLE,
        };

        for (int l = 0; l < 4; ++l) {
            int const levels[] = {0, 1, 6, 9};
            struct benchmark bench = {
                .level = levels[l],
                .profile = profiles[p],
                .iterations = 0,
                .compressed = 0,
                .ratio = 0.0
            };
            bench_compress(&bench);
        }
    }

    return EXIT_SUCCESS;
}
