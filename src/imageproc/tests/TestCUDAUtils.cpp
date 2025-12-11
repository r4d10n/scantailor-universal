/*
    Scan Tailor Universal - Interactive post-processing tool for scanned pages.
    Copyright (C) 2024

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

#include <boost/test/unit_test.hpp>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstring>
#include "../gpu/CUDAUtils.h"

using namespace imageproc::gpu;

BOOST_AUTO_TEST_SUITE(CUDAUtilsTestSuite);

/**
 * @brief Test CUDA initialization and device query
 */
BOOST_AUTO_TEST_CASE(TestCUDAInitialization)
{
    // Try to initialize CUDA
    bool initialized = initCUDA();

    // Get device info regardless of initialization status
    CUDADeviceInfo info = getCUDADeviceInfo();

    if (initialized) {
        BOOST_CHECK(info.available);
        BOOST_CHECK_GE(info.deviceCount, 1);
        BOOST_CHECK_GE(info.totalMemory, 0u);
        BOOST_CHECK_GE(info.multiprocessorCount, 1);
        BOOST_TEST_MESSAGE("CUDA initialized successfully");
        BOOST_TEST_MESSAGE("  Device: " << info.deviceName);
        BOOST_TEST_MESSAGE("  Compute capability: " << info.major << "." << info.minor);
        BOOST_TEST_MESSAGE("  Total memory: " << (info.totalMemory / (1024 * 1024)) << " MB");
        BOOST_TEST_MESSAGE("  Multiprocessors: " << info.multiprocessorCount);
    } else {
        BOOST_CHECK(!info.available);
        BOOST_TEST_MESSAGE("CUDA not available: " << getLastCUDAError());
    }
}

/**
 * @brief Test RGB to Grayscale conversion on GPU
 */
BOOST_AUTO_TEST_CASE(TestGpuGrayscale)
{
    if (!isCUDAAvailable()) {
        BOOST_TEST_MESSAGE("Skipping GPU grayscale test - CUDA not available");
        return;
    }

    const int width = 64;
    const int height = 64;
    const int srcBpp = 4; // RGBA

    // Create test image with known pattern
    std::vector<uint8_t> src(width * height * srcBpp);
    std::vector<uint8_t> dst(width * height);
    std::vector<uint8_t> expected(width * height);

    // Fill with test pattern and compute expected grayscale
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * srcBpp;
            // BGRA format (Qt's format on little-endian)
            uint8_t b = static_cast<uint8_t>((x + y) % 256);
            uint8_t g = static_cast<uint8_t>((x * 2) % 256);
            uint8_t r = static_cast<uint8_t>((y * 2) % 256);
            src[idx + 0] = b;
            src[idx + 1] = g;
            src[idx + 2] = r;
            src[idx + 3] = 255; // Alpha

            // Expected grayscale using the same formula as the kernel
            // Fixed-point: (R*77 + G*150 + B*29) >> 8
            uint32_t gray = (r * 77 + g * 150 + b * 29) >> 8;
            expected[y * width + x] = static_cast<uint8_t>(gray);
        }
    }

    // Perform GPU conversion
    bool result = gpuRgbToGrayscale(src.data(), dst.data(), width, height, srcBpp);
    BOOST_CHECK(result);

    if (result) {
        // Compare results
        int maxDiff = 0;
        for (int i = 0; i < width * height; ++i) {
            int diff = std::abs(static_cast<int>(dst[i]) - static_cast<int>(expected[i]));
            maxDiff = std::max(maxDiff, diff);
        }
        // Allow for small rounding differences
        BOOST_CHECK_LE(maxDiff, 1);
        BOOST_TEST_MESSAGE("GPU grayscale max difference: " << maxDiff);
    }
}

/**
 * @brief Test morphological dilation on GPU
 */
BOOST_AUTO_TEST_CASE(TestGpuDilation)
{
    if (!isCUDAAvailable()) {
        BOOST_TEST_MESSAGE("Skipping GPU dilation test - CUDA not available");
        return;
    }

    const int width = 32;
    const int height = 32;

    // Create test image with a single bright pixel in center
    std::vector<uint8_t> src(width * height, 0);
    std::vector<uint8_t> dst(width * height, 0);

    // Place a bright pixel at center
    int cx = width / 2;
    int cy = height / 2;
    src[cy * width + cx] = 255;

    // Perform 3x3 dilation
    bool result = gpuDilate(src.data(), dst.data(), width, height, 3, 3);
    BOOST_CHECK(result);

    if (result) {
        // After 3x3 dilation, a 3x3 region around center should be 255
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                int px = cx + dx;
                int py = cy + dy;
                BOOST_CHECK_EQUAL(dst[py * width + px], 255);
            }
        }

        // Pixels outside should still be 0
        BOOST_CHECK_EQUAL(dst[0], 0);
        BOOST_CHECK_EQUAL(dst[width - 1], 0);
    }
}

/**
 * @brief Test morphological erosion on GPU
 */
BOOST_AUTO_TEST_CASE(TestGpuErosion)
{
    if (!isCUDAAvailable()) {
        BOOST_TEST_MESSAGE("Skipping GPU erosion test - CUDA not available");
        return;
    }

    const int width = 32;
    const int height = 32;

    // Create test image - all white
    std::vector<uint8_t> src(width * height, 255);
    std::vector<uint8_t> dst(width * height, 0);

    // Create a black border (should propagate inward after erosion)
    for (int x = 0; x < width; ++x) {
        src[x] = 0;                         // Top row
        src[(height - 1) * width + x] = 0;  // Bottom row
    }
    for (int y = 0; y < height; ++y) {
        src[y * width] = 0;                 // Left column
        src[y * width + width - 1] = 0;     // Right column
    }

    // Perform 3x3 erosion
    bool result = gpuErode(src.data(), dst.data(), width, height, 3, 3);
    BOOST_CHECK(result);

    if (result) {
        // Border should remain 0
        BOOST_CHECK_EQUAL(dst[0], 0);
        BOOST_CHECK_EQUAL(dst[width - 1], 0);

        // One pixel inside the border should also be 0 (eroded)
        BOOST_CHECK_EQUAL(dst[width + 1], 0);

        // Center should still be 255 (not affected by border erosion)
        int cx = width / 2;
        int cy = height / 2;
        BOOST_CHECK_EQUAL(dst[cy * width + cx], 255);
    }
}

/**
 * @brief Test SEDM (Squared Euclidean Distance Map) on GPU
 */
BOOST_AUTO_TEST_CASE(TestGpuSEDM)
{
    if (!isCUDAAvailable()) {
        BOOST_TEST_MESSAGE("Skipping GPU SEDM test - CUDA not available");
        return;
    }

    const int width = 16;
    const int height = 16;

    // Create test image - all foreground except center pixel
    std::vector<uint8_t> src(width * height, 255);
    std::vector<uint32_t> dst(width * height, 0);

    // Background pixel at center
    int cx = width / 2;
    int cy = height / 2;
    src[cy * width + cx] = 0;

    // Perform SEDM
    bool result = gpuSEDM(src.data(), dst.data(), width, height);
    BOOST_CHECK(result);

    if (result) {
        // Distance at center should be 0 (it's background)
        BOOST_CHECK_EQUAL(dst[cy * width + cx], 0u);

        // Distance at adjacent pixels should be 1 (distance² = 1)
        BOOST_CHECK_EQUAL(dst[cy * width + cx + 1], 1u);
        BOOST_CHECK_EQUAL(dst[cy * width + cx - 1], 1u);
        BOOST_CHECK_EQUAL(dst[(cy + 1) * width + cx], 1u);
        BOOST_CHECK_EQUAL(dst[(cy - 1) * width + cx], 1u);

        // Distance at diagonal pixels should be 2 (1² + 1² = 2)
        BOOST_CHECK_EQUAL(dst[(cy + 1) * width + cx + 1], 2u);
    }
}

/**
 * @brief Test Otsu binarization on GPU
 */
BOOST_AUTO_TEST_CASE(TestGpuBinarizeOtsu)
{
    if (!isCUDAAvailable()) {
        BOOST_TEST_MESSAGE("Skipping GPU Otsu test - CUDA not available");
        return;
    }

    const int width = 64;
    const int height = 64;

    // Create bimodal test image
    std::vector<uint8_t> src(width * height);
    std::vector<uint8_t> dst(width * height);

    // Left half dark (50), right half bright (200)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            src[y * width + x] = (x < width / 2) ? 50 : 200;
        }
    }

    int threshold = 0;
    bool result = gpuBinarizeOtsu(src.data(), dst.data(), width, height, &threshold);
    BOOST_CHECK(result);

    if (result) {
        // Threshold should be somewhere between 50 and 200
        BOOST_CHECK_GT(threshold, 50);
        BOOST_CHECK_LT(threshold, 200);
        BOOST_TEST_MESSAGE("Otsu threshold: " << threshold);

        // Left half should be black (0), right half should be white (255)
        BOOST_CHECK_EQUAL(dst[height / 2 * width + 0], 0);              // Left
        BOOST_CHECK_EQUAL(dst[height / 2 * width + width - 1], 255);    // Right
    }
}

/**
 * @brief Test Sauvola binarization on GPU
 */
BOOST_AUTO_TEST_CASE(TestGpuBinarizeSauvola)
{
    if (!isCUDAAvailable()) {
        BOOST_TEST_MESSAGE("Skipping GPU Sauvola test - CUDA not available");
        return;
    }

    const int width = 64;
    const int height = 64;

    // Create test image with varying intensity
    std::vector<uint8_t> src(width * height);
    std::vector<uint8_t> dst(width * height);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Create a gradient pattern
            src[y * width + x] = static_cast<uint8_t>((x + y) * 2);
        }
    }

    bool result = gpuBinarizeSauvola(src.data(), dst.data(), width, height, 15, 0.34f);
    BOOST_CHECK(result);

    if (result) {
        // Check that output is binary (0 or 255 only)
        for (int i = 0; i < width * height; ++i) {
            BOOST_CHECK(dst[i] == 0 || dst[i] == 255);
        }
    }
}

/**
 * @brief Test image inversion on GPU
 */
BOOST_AUTO_TEST_CASE(TestGpuInvert)
{
    if (!isCUDAAvailable()) {
        BOOST_TEST_MESSAGE("Skipping GPU invert test - CUDA not available");
        return;
    }

    const int width = 32;
    const int height = 32;

    std::vector<uint8_t> data(width * height);

    // Fill with known pattern
    for (int i = 0; i < width * height; ++i) {
        data[i] = static_cast<uint8_t>(i % 256);
    }

    // Keep copy for comparison
    std::vector<uint8_t> original = data;

    bool result = gpuInvert(data.data(), width, height);
    BOOST_CHECK(result);

    if (result) {
        // Check inversion
        for (int i = 0; i < width * height; ++i) {
            BOOST_CHECK_EQUAL(data[i], static_cast<uint8_t>(255 - original[i]));
        }
    }
}

/**
 * @brief Test black pixel counting on GPU
 */
BOOST_AUTO_TEST_CASE(TestGpuCountBlackPixels)
{
    if (!isCUDAAvailable()) {
        BOOST_TEST_MESSAGE("Skipping GPU count black pixels test - CUDA not available");
        return;
    }

    const int width = 100;
    const int height = 100;

    std::vector<uint8_t> data(width * height);

    // Create pattern: first 1000 pixels black, rest white
    const int numBlack = 1000;
    for (int i = 0; i < width * height; ++i) {
        data[i] = (i < numBlack) ? 0 : 255;
    }

    int64_t count = gpuCountBlackPixels(data.data(), width, height);
    BOOST_CHECK_EQUAL(count, numBlack);
}

/**
 * @brief Test dewarping on GPU (basic test)
 */
BOOST_AUTO_TEST_CASE(TestGpuDewarp)
{
    if (!isCUDAAvailable()) {
        BOOST_TEST_MESSAGE("Skipping GPU dewarp test - CUDA not available");
        return;
    }

    const int srcWidth = 64;
    const int srcHeight = 64;
    const int dstWidth = 64;
    const int dstHeight = 64;

    // Create source image with checkerboard pattern
    std::vector<uint8_t> src(srcWidth * srcHeight);
    std::vector<uint8_t> dst(dstWidth * dstHeight);

    for (int y = 0; y < srcHeight; ++y) {
        for (int x = 0; x < srcWidth; ++x) {
            src[y * srcWidth + x] = ((x / 8 + y / 8) % 2 == 0) ? 255 : 0;
        }
    }

    // Create identity mapping (no distortion)
    std::vector<float> mapX(dstWidth * dstHeight);
    std::vector<float> mapY(dstWidth * dstHeight);

    for (int y = 0; y < dstHeight; ++y) {
        for (int x = 0; x < dstWidth; ++x) {
            mapX[y * dstWidth + x] = static_cast<float>(x);
            mapY[y * dstWidth + x] = static_cast<float>(y);
        }
    }

    bool result = gpuDewarp(
        src.data(), dst.data(),
        srcWidth, srcHeight,
        dstWidth, dstHeight,
        mapX.data(), mapY.data(),
        1  // grayscale
    );
    BOOST_CHECK(result);

    if (result) {
        // With identity mapping, output should match input (except at borders)
        int matches = 0;
        for (int y = 1; y < dstHeight - 1; ++y) {
            for (int x = 1; x < dstWidth - 1; ++x) {
                if (src[y * srcWidth + x] == dst[y * dstWidth + x]) {
                    matches++;
                }
            }
        }
        // Most interior pixels should match
        int interior = (dstWidth - 2) * (dstHeight - 2);
        BOOST_CHECK_GT(matches, interior * 0.95);
    }
}

/**
 * @brief Performance test for GPU operations (informational)
 */
BOOST_AUTO_TEST_CASE(TestGpuPerformance)
{
    if (!isCUDAAvailable()) {
        BOOST_TEST_MESSAGE("Skipping GPU performance test - CUDA not available");
        return;
    }

    // Test with larger image for meaningful timing
    const int width = 1024;
    const int height = 1024;
    const int iterations = 10;

    std::vector<uint8_t> src(width * height * 4);  // RGBA
    std::vector<uint8_t> gray(width * height);
    std::vector<uint8_t> dilated(width * height);
    std::vector<uint32_t> sedm(width * height);

    // Initialize with random-ish data
    for (size_t i = 0; i < src.size(); ++i) {
        src[i] = static_cast<uint8_t>(i * 17 + 31);
    }

    BOOST_TEST_MESSAGE("GPU Performance Test (1024x1024, " << iterations << " iterations):");

    // Measure grayscale conversion
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        gpuRgbToGrayscale(src.data(), gray.data(), width, height, 4);
    }
    gpuSynchronize();
    auto end = std::chrono::high_resolution_clock::now();
    auto grayscaleTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    BOOST_TEST_MESSAGE("  Grayscale: " << (grayscaleTime / iterations) << " us/iteration");

    // Measure dilation
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        gpuDilate(gray.data(), dilated.data(), width, height, 3, 3);
    }
    gpuSynchronize();
    end = std::chrono::high_resolution_clock::now();
    auto dilationTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    BOOST_TEST_MESSAGE("  Dilation (3x3): " << (dilationTime / iterations) << " us/iteration");

    // Measure SEDM
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        gpuSEDM(gray.data(), sedm.data(), width, height);
    }
    gpuSynchronize();
    end = std::chrono::high_resolution_clock::now();
    auto sedmTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    BOOST_TEST_MESSAGE("  SEDM: " << (sedmTime / iterations) << " us/iteration");

    // These are just informational, so always pass
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(TestCUDAShutdown)
{
    // Clean up CUDA resources
    shutdownCUDA();

    // After shutdown, CUDA should report as unavailable
    CUDADeviceInfo info = getCUDADeviceInfo();
    BOOST_CHECK(!info.available);
}

BOOST_AUTO_TEST_SUITE_END();
