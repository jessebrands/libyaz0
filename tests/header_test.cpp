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

#include "../src/byteswap.h"

#include <gtest/gtest.h>

extern "C" {
#include "yaz0.h"
}

TEST(Header, Read) {
        constexpr std::array<uint8_t, 16> buffer = {'Y', 'a', 'z', '0', 0x11, 0x22, 0x33, 0x44, 0, 0, 0, 0, 0, 0, 0, 0};

        yaz0_header header = {};
        auto const result = yaz0_read_header(buffer.data(), buffer.size(), &header);
        EXPECT_EQ(result, YAZ0_OK);
        EXPECT_TRUE(strncmp(header.magic, YAZ0_MAGIC, sizeof(header.magic)) == 0);
        EXPECT_EQ(header.uncompressed_size, yaz0_to_native_u32(0x11223344));
}

TEST(Header, ReadFailsOnSmallBuffer) {
        constexpr std::array<uint8_t, 8> buffer = {'Y', 'a', 'z', '0', 0x11, 0x22, 0x33, 0x44};
        yaz0_header header = {};
        EXPECT_EQ(yaz0_read_header(buffer.data(), buffer.size(), &header), YAZ0_OUT_OF_RANGE);
}

TEST(Header, Write) {
        std::array<uint8_t, 16> buffer = {};
        auto const result = yaz0_write_header(buffer.data(), buffer.size(), 0x11223344, 0);
        constexpr std::array<uint8_t, 16> expected = {'Y', 'a', 'z', '0', 0x11, 0x22, 0x33, 0x44,
                                                      0,   0,   0,   0,   0,    0,    0,    0};

        EXPECT_EQ(result, YAZ0_OK);
        EXPECT_EQ(buffer, expected);
}

TEST(Header, WriteFailsOnSmallBuffer) {
        std::array<uint8_t, 8> buffer = {};
        EXPECT_EQ(yaz0_write_header(buffer.data(), buffer.size(), 0x11223344, 0), YAZ0_DESTINATION_TOO_SMALL);
}
