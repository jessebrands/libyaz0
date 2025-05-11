/*
 * This file is part of Zelda64.
 *
 * Zelda64 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Zelda64 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Zelda64. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef YAZ0_BYTESWAP_H
#define YAZ0_BYTESWAP_H

#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
//
// Byte swaps a 32-bit integer.
//
#define yaz0_swap32(x) __builtin_bswap32(x)

#else
//
// Byte swaps a 32-bit integer.
//
inline uint32_t yaz0_swap32(uint32_t const x) {
        return (x & 0xff000000) >> 24 | (x & 0x00ff0000) >> 8 | (x & 0x0000ff00) << 8 | (x & 0x000000ff) << 24;
}

#endif


#ifdef YAZ0_BIG_ENDIAN
//
// Swaps from native endianness to big endian.
//
#define yaz0_to_be_u32(x) x

//
// Swaps from big endian to native endianness.
//
#define yaz0_to_native_u32(x) x

#else
//
// Swaps from native endianness to big endian.
//
#define yaz0_to_be_u32(x) yaz0_swap32(x)

//
// Swaps from big endian to native endianness.
//
#define yaz0_to_native_u32(x) yaz0_swap32(x)

#endif

#endif // YAZ0_BYTESWAP_H
