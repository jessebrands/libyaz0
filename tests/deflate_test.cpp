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

/*
 * Parts of this file contain code taken from gcnhax/yaz0-rs which is licensed
 * under the MIT License. The original copyright notice follows:
 *
 * Copyright (c) 2018 Erin Moon
 * https://github.com/gcnhax/yaz0-rs
 */

#include <span>

#include <gtest/gtest.h>

extern "C" {
#include <yaz0.h>
}

TEST(Deflate, Basic) {
        constexpr std::array<uint8_t, 17> inflated{0, 1, 2, 0xa, 0, 1, 2, 3, 0xb, 0, 1, 2, 3, 4, 5, 6, 7};
        constexpr std::array<uint8_t, 16> deflated{0xf6, 0,    1,    2,    0xa, 0x10, 0x03, 3,
                                                   0xb,  0x20, 0x04, 0xf0, 4,   5,    6,    7};

        std::array<uint8_t, 16> out{};
        size_t compressed_size = 0;
        auto const result = yaz0_deflate(out.data(), out.size(), inflated.data(), inflated.size(), YAZ0_DEFAULT_LEVEL,
                                         &compressed_size);
        EXPECT_EQ(result, YAZ0_OK);
        EXPECT_EQ(compressed_size, deflated.size());
        EXPECT_EQ(out, deflated);
}

TEST(Deflate, Uncompressable) {
        constexpr std::array<uint8_t, 3> inflated{0x12, 0x34, 0x56};
        constexpr std::array<uint8_t, 4> deflated{0xe0, 0x12, 0x34, 0x56};

        std::array<uint8_t, 4> out{};
        size_t compressed_size = 0;
        auto const result = yaz0_deflate(out.data(), out.size(), inflated.data(), inflated.size(), YAZ0_DEFAULT_LEVEL,
                                         &compressed_size);
        EXPECT_EQ(result, YAZ0_OK);
        EXPECT_EQ(compressed_size, deflated.size());
        EXPECT_EQ(out, deflated);
}

TEST(Deflate, Lookahead) {
        constexpr std::array<uint8_t, 15> inflated{0, 0, 0, 0xa, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xa};
        constexpr std::array<uint8_t, 9> deflated{0xfa, 0, 0, 0, 10, 0, 0x70, 0x00, 0xa};

        std::array<uint8_t, 9> out{};
        size_t compressed_size = 0;
        auto const result = yaz0_deflate(out.data(), out.size(), inflated.data(), inflated.size(), YAZ0_DEFAULT_LEVEL,
                                         &compressed_size);
        EXPECT_EQ(result, YAZ0_OK);
        EXPECT_EQ(compressed_size, deflated.size());
        EXPECT_EQ(out, deflated);
}

TEST(Deflate, NoCompression) {
        constexpr std::array<uint8_t, 15> inflated{0, 0, 0, 0xa, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xa};
        constexpr std::array<uint8_t, 17> deflated{0xff, 0, 0, 0, 0xa, 0, 0, 0, 0, 0xfe, 0, 0, 0, 0, 0, 0, 0xa};

        std::array<uint8_t, 17> out{};
        size_t compressed_size = 0;
        auto const result = yaz0_deflate(out.data(), out.size(), inflated.data(), inflated.size(), 0, &compressed_size);
        EXPECT_EQ(result, YAZ0_OK);
        EXPECT_EQ(compressed_size, 17);
        EXPECT_EQ(out, deflated);
}