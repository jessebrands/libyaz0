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

#ifndef ZELDA64_YAZ0_H
#define ZELDA64_YAZ0_H

#include <stddef.h>
#include <stdint.h>

//
// Magic constant marking start of Yaz0 file.
//
#define YAZ0_MAGIC "Yaz0"

//
// Function status codes.
//
enum yaz0_result {
        YAZ0_OK, // The operation completed successfully.
};

//
// 16-byte header that marks the start of every yaz0 byte stream.
//
struct yaz0_header {
        uint8_t magic[4];           // Always "Yaz0"
        uint32_t uncompressed_size; // Size of the uncompressed data in bytes.
        uint32_t alignment;         // Data alignment in bytes.
        uint32_t reserved;          // Currently unused.
};

//
// Reads the header from the data in the buffer.
//
enum yaz0_result yaz0_read_header(uint8_t const* data, size_t size, struct yaz0_header* header);

//
// Decompresses the data at src into the destination buffer.
//
enum yaz0_result yaz0_inflate(uint8_t* dst, size_t dst_size, uint8_t const* src, size_t src_size);

//
// Compresses the data at src into the destination buffer.
//
enum yaz0_result yaz0_deflate(uint8_t* dst, uint8_t const* src, size_t size, int level);

#endif // ZELDA64_YAZ0_H
