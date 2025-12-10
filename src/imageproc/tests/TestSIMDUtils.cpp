/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "SIMDUtils.h"
#include "Grayscale.h"
#include "BinaryImage.h"
#include "BWColor.h"
#include <QImage>
#include <vector>
#include <cstdlib>
#include <cstring>
#ifndef Q_MOC_RUN
#include <boost/test/unit_test.hpp>
#endif

namespace imageproc
{
namespace tests
{

BOOST_AUTO_TEST_SUITE(SIMDUtilsTestSuite);

BOOST_AUTO_TEST_CASE(test_max3_horizontal)
{
    const int width = 100;
    const int height = 50;
    const int stride = width;

    std::vector<uint8_t> src(width * height);
    std::vector<uint8_t> dst_simd(width * height);
    std::vector<uint8_t> dst_scalar(width * height);

    // Fill with random data
    for (int i = 0; i < width * height; ++i) {
        src[i] = rand() % 256;
    }

    // Compute using SIMD
    simd::max3Horizontal(dst_simd.data(), src.data(), width, height, stride);

    // Compute scalar reference
    for (int y = 0; y < height; ++y) {
        const uint8_t* srcLine = src.data() + y * stride;
        uint8_t* dstLine = dst_scalar.data() + y * stride;

        dstLine[0] = std::max(srcLine[0], srcLine[1]);
        for (int x = 1; x < width - 1; ++x) {
            dstLine[x] = std::max(std::max(srcLine[x-1], srcLine[x]), srcLine[x+1]);
        }
        if (width > 1) {
            dstLine[width-1] = std::max(srcLine[width-2], srcLine[width-1]);
        }
    }

    // Compare results
    BOOST_CHECK_EQUAL_COLLECTIONS(
        dst_simd.begin(), dst_simd.end(),
        dst_scalar.begin(), dst_scalar.end()
    );
}

BOOST_AUTO_TEST_CASE(test_max3_vertical)
{
    const int width = 100;
    const int height = 50;
    const int stride = width;

    std::vector<uint8_t> src(width * height);
    std::vector<uint8_t> dst_simd(width * height);
    std::vector<uint8_t> dst_scalar(width * height);

    // Fill with random data
    for (int i = 0; i < width * height; ++i) {
        src[i] = rand() % 256;
    }

    // Compute using SIMD
    simd::max3Vertical(dst_simd.data(), src.data(), width, height, stride);

    // Compute scalar reference
    // First row
    {
        const uint8_t* currLine = src.data();
        const uint8_t* nextLine = src.data() + stride;
        uint8_t* dstLine = dst_scalar.data();
        for (int x = 0; x < width; ++x) {
            dstLine[x] = std::max(currLine[x], nextLine[x]);
        }
    }

    // Middle rows
    for (int y = 1; y < height - 1; ++y) {
        const uint8_t* prevLine = src.data() + (y - 1) * stride;
        const uint8_t* currLine = src.data() + y * stride;
        const uint8_t* nextLine = src.data() + (y + 1) * stride;
        uint8_t* dstLine = dst_scalar.data() + y * stride;

        for (int x = 0; x < width; ++x) {
            dstLine[x] = std::max(std::max(prevLine[x], currLine[x]), nextLine[x]);
        }
    }

    // Last row
    if (height > 1) {
        const uint8_t* prevLine = src.data() + (height - 2) * stride;
        const uint8_t* currLine = src.data() + (height - 1) * stride;
        uint8_t* dstLine = dst_scalar.data() + (height - 1) * stride;
        for (int x = 0; x < width; ++x) {
            dstLine[x] = std::max(prevLine[x], currLine[x]);
        }
    }

    // Compare results
    BOOST_CHECK_EQUAL_COLLECTIONS(
        dst_simd.begin(), dst_simd.end(),
        dst_scalar.begin(), dst_scalar.end()
    );
}

BOOST_AUTO_TEST_CASE(test_min3_horizontal)
{
    const int width = 100;
    const int height = 50;
    const int stride = width;

    std::vector<uint8_t> src(width * height);
    std::vector<uint8_t> dst_simd(width * height);
    std::vector<uint8_t> dst_scalar(width * height);

    // Fill with random data
    for (int i = 0; i < width * height; ++i) {
        src[i] = rand() % 256;
    }

    // Compute using SIMD
    simd::min3Horizontal(dst_simd.data(), src.data(), width, height, stride);

    // Compute scalar reference
    for (int y = 0; y < height; ++y) {
        const uint8_t* srcLine = src.data() + y * stride;
        uint8_t* dstLine = dst_scalar.data() + y * stride;

        dstLine[0] = std::min(srcLine[0], srcLine[1]);
        for (int x = 1; x < width - 1; ++x) {
            dstLine[x] = std::min(std::min(srcLine[x-1], srcLine[x]), srcLine[x+1]);
        }
        if (width > 1) {
            dstLine[width-1] = std::min(srcLine[width-2], srcLine[width-1]);
        }
    }

    // Compare results
    BOOST_CHECK_EQUAL_COLLECTIONS(
        dst_simd.begin(), dst_simd.end(),
        dst_scalar.begin(), dst_scalar.end()
    );
}

BOOST_AUTO_TEST_CASE(test_invert_words)
{
    const size_t wordCount = 100;
    std::vector<uint32_t> data_simd(wordCount);
    std::vector<uint32_t> data_scalar(wordCount);

    // Fill with random data
    for (size_t i = 0; i < wordCount; ++i) {
        uint32_t val = rand();
        data_simd[i] = val;
        data_scalar[i] = val;
    }

    // Invert using SIMD
    simd::invertWordsSIMD(data_simd.data(), wordCount);

    // Invert scalar
    for (size_t i = 0; i < wordCount; ++i) {
        data_scalar[i] = ~data_scalar[i];
    }

    // Compare results
    BOOST_CHECK_EQUAL_COLLECTIONS(
        data_simd.begin(), data_simd.end(),
        data_scalar.begin(), data_scalar.end()
    );
}

BOOST_AUTO_TEST_CASE(test_count_bits)
{
    const size_t wordCount = 100;
    std::vector<uint32_t> data(wordCount);

    // Fill with random data
    for (size_t i = 0; i < wordCount; ++i) {
        data[i] = rand();
    }

    // Count using SIMD
    int count_simd = simd::countBitsSIMD(data.data(), wordCount);

    // Count scalar
    int count_scalar = 0;
    for (size_t i = 0; i < wordCount; ++i) {
        count_scalar += simd::popcount32(data[i]);
    }

    BOOST_CHECK_EQUAL(count_simd, count_scalar);
}

BOOST_AUTO_TEST_CASE(test_popcount32)
{
    // Test known values
    BOOST_CHECK_EQUAL(simd::popcount32(0x00000000), 0);
    BOOST_CHECK_EQUAL(simd::popcount32(0xFFFFFFFF), 32);
    BOOST_CHECK_EQUAL(simd::popcount32(0x00000001), 1);
    BOOST_CHECK_EQUAL(simd::popcount32(0x80000000), 1);
    BOOST_CHECK_EQUAL(simd::popcount32(0xAAAAAAAA), 16);
    BOOST_CHECK_EQUAL(simd::popcount32(0x55555555), 16);
    BOOST_CHECK_EQUAL(simd::popcount32(0x0F0F0F0F), 16);
}

BOOST_AUTO_TEST_CASE(test_rgb_to_gray)
{
    const int width = 100;
    const int height = 50;
    const int srcStride = width;
    const int dstStride = width;

    std::vector<uint32_t> src(width * height);
    std::vector<uint8_t> dst_simd(width * height);
    std::vector<uint8_t> dst_scalar(width * height);

    // Fill with random RGB data
    for (int i = 0; i < width * height; ++i) {
        int r = rand() % 256;
        int g = rand() % 256;
        int b = rand() % 256;
        src[i] = (0xFF << 24) | (r << 16) | (g << 8) | b;
    }

    // Convert using SIMD
    simd::rgbToGraySIMD(dst_simd.data(), dstStride, src.data(), srcStride, width, height);

    // Convert scalar
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint32_t rgb = src[y * srcStride + x];
            int r = (rgb >> 16) & 0xFF;
            int g = (rgb >> 8) & 0xFF;
            int b = rgb & 0xFF;
            dst_scalar[y * dstStride + x] = static_cast<uint8_t>((r * 11 + g * 16 + b * 5) >> 5);
        }
    }

    // Compare results
    BOOST_CHECK_EQUAL_COLLECTIONS(
        dst_simd.begin(), dst_simd.end(),
        dst_scalar.begin(), dst_scalar.end()
    );
}

BOOST_AUTO_TEST_CASE(test_binary_image_invert)
{
    const int width = 200;
    const int height = 100;

    BinaryImage img1(width, height);
    BinaryImage img2(width, height);

    // Fill with random data
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            BWColor color = (rand() & 1) ? BLACK : WHITE;
            img1.setPixel(x, y, color);
            img2.setPixel(x, y, color);
        }
    }

    // Invert both images
    img1.invert();

    // Create inverted version of img2 manually using inverted()
    BinaryImage img2_inverted = img2.inverted();

    // Compare pixel by pixel
    bool match = true;
    for (int y = 0; y < height && match; ++y) {
        for (int x = 0; x < width && match; ++x) {
            if (img1.pixelAt(x, y) != img2_inverted.pixelAt(x, y)) {
                match = false;
            }
        }
    }

    BOOST_CHECK(match);
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests
} // namespace imageproc
