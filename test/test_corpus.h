/* test_corpus.h: synthetic test corpus generator
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

#ifndef LIBYAZ0_TEST_CORPUS_H
#define LIBYAZ0_TEST_CORPUS_H

#include <stddef.h>
#include <stdint.h>

enum corpus_profile {
   CORPUS_PROFILE_ZEROES = 0,
   CORPUS_PROFILE_ALTERNATING,
   CORPUS_PROFILE_BINARY,
   CORPUS_PROFILE_PERIODIC,
   CORPUS_PROFILE_STRUCTURED,
   CORPUS_PROFILE_INCOMPRESSIBLE,
};

char const*
corpus_profile_name(enum corpus_profile profile);

void corpus_generate(uint8_t* dst, size_t size, enum corpus_profile profile, uint64_t seed);

#endif //LIBYAZ0_TEST_CORPUS_H
