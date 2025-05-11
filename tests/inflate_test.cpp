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

#include <gtest/gtest.h>

extern "C" {
#include <yaz0.h>
}

TEST(Inflate, Decompress) {
        constexpr std::array<uint8_t, 25> deflated{

            'Y',  'a',  'z',  '0',                            // Header
            0x00, 0x00, 0x00, 0x0f,                           // 15 bytes
            0x00, 0x00, 0x00, 0x00,                           // Alignment
            0x00, 0x00, 0x00, 0x00,                           // Reserved
            0xfa, 0x00, 0x00, 0x00, 0x0a, 0, 0x70, 0x00, 0xa, // Data
        };

        // Should turn into:
        constexpr std::array<uint8_t, 15> inflated{0, 0, 0, 0xa, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xa};

        // Attempt decompression.
        std::array<uint8_t, 15> out{};
        auto const result = yaz0_inflate(out.data(), out.size(), deflated.data(), deflated.size());
        EXPECT_EQ(result, YAZ0_OK);
        EXPECT_EQ(out, inflated);
}

TEST(Inflate, SmallDataFails) {
        constexpr std::array<uint8_t, 8> deflated{
            'Y',  'a',  'z',  '0',  // Invalid header
            0x00, 0x00, 0x00, 0x0f, // 15 bytes
                                    // Rest of data missing!
        };

        // Attempt decompression.
        std::array<uint8_t, 15> out{};
        auto const result = yaz0_inflate(out.data(), out.size(), deflated.data(), deflated.size());
        EXPECT_EQ(result, YAZ0_OUT_OF_RANGE);
}

TEST(Inflate, InvalidHeaderFails) {
        constexpr std::array<uint8_t, 25> deflated{
            'Y',  'A',  'Z',  '1',                            // Invalid header
            0x00, 0x00, 0x00, 0x0f,                           // 15 bytes
            0x00, 0x00, 0x00, 0x00,                           // Alignment
            0x00, 0x00, 0x00, 0x00,                           // Reserved
            0xfa, 0x00, 0x00, 0x00, 0x0a, 0, 0x70, 0x00, 0xa, // Data
        };

        // Attempt decompression.
        std::array<uint8_t, 15> out{};
        auto const result = yaz0_inflate(out.data(), out.size(), deflated.data(), deflated.size());
        EXPECT_EQ(result, YAZ0_INVALID_DATA);
}

TEST(Inflate, OutBufferTooSmallFAils) {
        constexpr std::array<uint8_t, 16> deflated{
            'Y',  'a',  'z',  '0',  // Invalid header
            0x00, 0x00, 0x00, 0xff, // 255 bytes
            0x00, 0x00, 0x00, 0x00, // Alignment
            0x00, 0x00, 0x00, 0x00, // Reserved
        };

        // Output far too small (needs 255 byts.)
        std::array<uint8_t, 15> out{};
        auto const result = yaz0_inflate(out.data(), out.size(), deflated.data(), deflated.size());
        EXPECT_EQ(result, YAZ0_DESTINATION_TOO_SMALL);
}
