/*
 * search_swar.c: longest substring match using SWAR techniques
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

#include "search.h"

#if YAZ0_HAVE_SWAR64

size_t yaz0_search_swar64(uint8_t const* data, size_t start_pos, size_t offset, size_t max_lookahead,
    size_t* match_pos) {
    return 0;
}

#endif // YAZ0_HAVE_SWAR

bool
yaz0_search_swar64_supported(void) {
    return YAZ0_HAVE_SWAR64;
}
