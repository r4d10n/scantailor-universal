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
#include <algorithm>
#include <cstring>

namespace imageproc
{
namespace simd
{

void max3Horizontal(
    uint8_t* dst, const uint8_t* src, int width, int height, int stride)
{
    if (width <= 0 || height <= 0) return;

#if SIMD_SSE2_AVAILABLE
    const int simdWidth = 16;

    for (int y = 0; y < height; ++y) {
        const uint8_t* srcLine = src + y * stride;
        uint8_t* dstLine = dst + y * stride;

        // First pixel: max(src[0], src[1])
        dstLine[0] = std::max(srcLine[0], srcLine[1]);

        // Process middle section with SIMD
        int x = 1;
        for (; x + simdWidth <= width - 1; x += simdWidth) {
            __m128i prev = load128u(srcLine + x - 1);
            __m128i curr = load128u(srcLine + x);
            __m128i next = load128u(srcLine + x + 1);

            __m128i result = max_epu8(max_epu8(prev, curr), next);
            store128u(dstLine + x, result);
        }

        // Process remaining pixels
        for (; x < width - 1; ++x) {
            dstLine[x] = std::max(std::max(srcLine[x-1], srcLine[x]), srcLine[x+1]);
        }

        // Last pixel: max(src[width-2], src[width-1])
        if (width > 1) {
            dstLine[width-1] = std::max(srcLine[width-2], srcLine[width-1]);
        }
    }

#elif SIMD_NEON_AVAILABLE
    const int simdWidth = 16;

    for (int y = 0; y < height; ++y) {
        const uint8_t* srcLine = src + y * stride;
        uint8_t* dstLine = dst + y * stride;

        dstLine[0] = std::max(srcLine[0], srcLine[1]);

        int x = 1;
        for (; x + simdWidth <= width - 1; x += simdWidth) {
            uint8x16_t prev = vld1q_u8(srcLine + x - 1);
            uint8x16_t curr = vld1q_u8(srcLine + x);
            uint8x16_t next = vld1q_u8(srcLine + x + 1);

            uint8x16_t result = vmaxq_u8(vmaxq_u8(prev, curr), next);
            vst1q_u8(dstLine + x, result);
        }

        for (; x < width - 1; ++x) {
            dstLine[x] = std::max(std::max(srcLine[x-1], srcLine[x]), srcLine[x+1]);
        }

        if (width > 1) {
            dstLine[width-1] = std::max(srcLine[width-2], srcLine[width-1]);
        }
    }

#else
    // Scalar fallback
    for (int y = 0; y < height; ++y) {
        const uint8_t* srcLine = src + y * stride;
        uint8_t* dstLine = dst + y * stride;

        dstLine[0] = std::max(srcLine[0], srcLine[1]);

        for (int x = 1; x < width - 1; ++x) {
            dstLine[x] = std::max(std::max(srcLine[x-1], srcLine[x]), srcLine[x+1]);
        }

        if (width > 1) {
            dstLine[width-1] = std::max(srcLine[width-2], srcLine[width-1]);
        }
    }
#endif
}

void max3Vertical(
    uint8_t* dst, const uint8_t* src, int width, int height, int stride)
{
    if (width <= 0 || height <= 0) return;

#if SIMD_SSE2_AVAILABLE
    const int simdWidth = 16;

    // First row: max(src[0], src[stride])
    {
        const uint8_t* currLine = src;
        const uint8_t* nextLine = src + stride;
        uint8_t* dstLine = dst;

        int x = 0;
        for (; x + simdWidth <= width; x += simdWidth) {
            __m128i curr = load128u(currLine + x);
            __m128i next = load128u(nextLine + x);
            store128u(dstLine + x, max_epu8(curr, next));
        }
        for (; x < width; ++x) {
            dstLine[x] = std::max(currLine[x], nextLine[x]);
        }
    }

    // Middle rows
    for (int y = 1; y < height - 1; ++y) {
        const uint8_t* prevLine = src + (y - 1) * stride;
        const uint8_t* currLine = src + y * stride;
        const uint8_t* nextLine = src + (y + 1) * stride;
        uint8_t* dstLine = dst + y * stride;

        int x = 0;
        for (; x + simdWidth <= width; x += simdWidth) {
            __m128i prev = load128u(prevLine + x);
            __m128i curr = load128u(currLine + x);
            __m128i next = load128u(nextLine + x);
            store128u(dstLine + x, max_epu8(max_epu8(prev, curr), next));
        }
        for (; x < width; ++x) {
            dstLine[x] = std::max(std::max(prevLine[x], currLine[x]), nextLine[x]);
        }
    }

    // Last row: max(src[(height-2)*stride], src[(height-1)*stride])
    if (height > 1) {
        const uint8_t* prevLine = src + (height - 2) * stride;
        const uint8_t* currLine = src + (height - 1) * stride;
        uint8_t* dstLine = dst + (height - 1) * stride;

        int x = 0;
        for (; x + simdWidth <= width; x += simdWidth) {
            __m128i prev = load128u(prevLine + x);
            __m128i curr = load128u(currLine + x);
            store128u(dstLine + x, max_epu8(prev, curr));
        }
        for (; x < width; ++x) {
            dstLine[x] = std::max(prevLine[x], currLine[x]);
        }
    }

#elif SIMD_NEON_AVAILABLE
    const int simdWidth = 16;

    // First row
    {
        const uint8_t* currLine = src;
        const uint8_t* nextLine = src + stride;
        uint8_t* dstLine = dst;

        int x = 0;
        for (; x + simdWidth <= width; x += simdWidth) {
            uint8x16_t curr = vld1q_u8(currLine + x);
            uint8x16_t next = vld1q_u8(nextLine + x);
            vst1q_u8(dstLine + x, vmaxq_u8(curr, next));
        }
        for (; x < width; ++x) {
            dstLine[x] = std::max(currLine[x], nextLine[x]);
        }
    }

    // Middle rows
    for (int y = 1; y < height - 1; ++y) {
        const uint8_t* prevLine = src + (y - 1) * stride;
        const uint8_t* currLine = src + y * stride;
        const uint8_t* nextLine = src + (y + 1) * stride;
        uint8_t* dstLine = dst + y * stride;

        int x = 0;
        for (; x + simdWidth <= width; x += simdWidth) {
            uint8x16_t prev = vld1q_u8(prevLine + x);
            uint8x16_t curr = vld1q_u8(currLine + x);
            uint8x16_t next = vld1q_u8(nextLine + x);
            vst1q_u8(dstLine + x, vmaxq_u8(vmaxq_u8(prev, curr), next));
        }
        for (; x < width; ++x) {
            dstLine[x] = std::max(std::max(prevLine[x], currLine[x]), nextLine[x]);
        }
    }

    // Last row
    if (height > 1) {
        const uint8_t* prevLine = src + (height - 2) * stride;
        const uint8_t* currLine = src + (height - 1) * stride;
        uint8_t* dstLine = dst + (height - 1) * stride;

        int x = 0;
        for (; x + simdWidth <= width; x += simdWidth) {
            uint8x16_t prev = vld1q_u8(prevLine + x);
            uint8x16_t curr = vld1q_u8(currLine + x);
            vst1q_u8(dstLine + x, vmaxq_u8(prev, curr));
        }
        for (; x < width; ++x) {
            dstLine[x] = std::max(prevLine[x], currLine[x]);
        }
    }

#else
    // Scalar fallback
    // First row
    {
        const uint8_t* currLine = src;
        const uint8_t* nextLine = src + stride;
        uint8_t* dstLine = dst;
        for (int x = 0; x < width; ++x) {
            dstLine[x] = std::max(currLine[x], nextLine[x]);
        }
    }

    // Middle rows
    for (int y = 1; y < height - 1; ++y) {
        const uint8_t* prevLine = src + (y - 1) * stride;
        const uint8_t* currLine = src + y * stride;
        const uint8_t* nextLine = src + (y + 1) * stride;
        uint8_t* dstLine = dst + y * stride;

        for (int x = 0; x < width; ++x) {
            dstLine[x] = std::max(std::max(prevLine[x], currLine[x]), nextLine[x]);
        }
    }

    // Last row
    if (height > 1) {
        const uint8_t* prevLine = src + (height - 2) * stride;
        const uint8_t* currLine = src + (height - 1) * stride;
        uint8_t* dstLine = dst + (height - 1) * stride;
        for (int x = 0; x < width; ++x) {
            dstLine[x] = std::max(prevLine[x], currLine[x]);
        }
    }
#endif
}

void min3Horizontal(
    uint8_t* dst, const uint8_t* src, int width, int height, int stride)
{
    if (width <= 0 || height <= 0) return;

#if SIMD_SSE2_AVAILABLE
    const int simdWidth = 16;

    for (int y = 0; y < height; ++y) {
        const uint8_t* srcLine = src + y * stride;
        uint8_t* dstLine = dst + y * stride;

        dstLine[0] = std::min(srcLine[0], srcLine[1]);

        int x = 1;
        for (; x + simdWidth <= width - 1; x += simdWidth) {
            __m128i prev = load128u(srcLine + x - 1);
            __m128i curr = load128u(srcLine + x);
            __m128i next = load128u(srcLine + x + 1);

            __m128i result = min_epu8(min_epu8(prev, curr), next);
            store128u(dstLine + x, result);
        }

        for (; x < width - 1; ++x) {
            dstLine[x] = std::min(std::min(srcLine[x-1], srcLine[x]), srcLine[x+1]);
        }

        if (width > 1) {
            dstLine[width-1] = std::min(srcLine[width-2], srcLine[width-1]);
        }
    }

#elif SIMD_NEON_AVAILABLE
    const int simdWidth = 16;

    for (int y = 0; y < height; ++y) {
        const uint8_t* srcLine = src + y * stride;
        uint8_t* dstLine = dst + y * stride;

        dstLine[0] = std::min(srcLine[0], srcLine[1]);

        int x = 1;
        for (; x + simdWidth <= width - 1; x += simdWidth) {
            uint8x16_t prev = vld1q_u8(srcLine + x - 1);
            uint8x16_t curr = vld1q_u8(srcLine + x);
            uint8x16_t next = vld1q_u8(srcLine + x + 1);

            uint8x16_t result = vminq_u8(vminq_u8(prev, curr), next);
            vst1q_u8(dstLine + x, result);
        }

        for (; x < width - 1; ++x) {
            dstLine[x] = std::min(std::min(srcLine[x-1], srcLine[x]), srcLine[x+1]);
        }

        if (width > 1) {
            dstLine[width-1] = std::min(srcLine[width-2], srcLine[width-1]);
        }
    }

#else
    // Scalar fallback
    for (int y = 0; y < height; ++y) {
        const uint8_t* srcLine = src + y * stride;
        uint8_t* dstLine = dst + y * stride;

        dstLine[0] = std::min(srcLine[0], srcLine[1]);

        for (int x = 1; x < width - 1; ++x) {
            dstLine[x] = std::min(std::min(srcLine[x-1], srcLine[x]), srcLine[x+1]);
        }

        if (width > 1) {
            dstLine[width-1] = std::min(srcLine[width-2], srcLine[width-1]);
        }
    }
#endif
}

void min3Vertical(
    uint8_t* dst, const uint8_t* src, int width, int height, int stride)
{
    if (width <= 0 || height <= 0) return;

#if SIMD_SSE2_AVAILABLE
    const int simdWidth = 16;

    // First row
    {
        const uint8_t* currLine = src;
        const uint8_t* nextLine = src + stride;
        uint8_t* dstLine = dst;

        int x = 0;
        for (; x + simdWidth <= width; x += simdWidth) {
            __m128i curr = load128u(currLine + x);
            __m128i next = load128u(nextLine + x);
            store128u(dstLine + x, min_epu8(curr, next));
        }
        for (; x < width; ++x) {
            dstLine[x] = std::min(currLine[x], nextLine[x]);
        }
    }

    // Middle rows
    for (int y = 1; y < height - 1; ++y) {
        const uint8_t* prevLine = src + (y - 1) * stride;
        const uint8_t* currLine = src + y * stride;
        const uint8_t* nextLine = src + (y + 1) * stride;
        uint8_t* dstLine = dst + y * stride;

        int x = 0;
        for (; x + simdWidth <= width; x += simdWidth) {
            __m128i prev = load128u(prevLine + x);
            __m128i curr = load128u(currLine + x);
            __m128i next = load128u(nextLine + x);
            store128u(dstLine + x, min_epu8(min_epu8(prev, curr), next));
        }
        for (; x < width; ++x) {
            dstLine[x] = std::min(std::min(prevLine[x], currLine[x]), nextLine[x]);
        }
    }

    // Last row
    if (height > 1) {
        const uint8_t* prevLine = src + (height - 2) * stride;
        const uint8_t* currLine = src + (height - 1) * stride;
        uint8_t* dstLine = dst + (height - 1) * stride;

        int x = 0;
        for (; x + simdWidth <= width; x += simdWidth) {
            __m128i prev = load128u(prevLine + x);
            __m128i curr = load128u(currLine + x);
            store128u(dstLine + x, min_epu8(prev, curr));
        }
        for (; x < width; ++x) {
            dstLine[x] = std::min(prevLine[x], currLine[x]);
        }
    }

#elif SIMD_NEON_AVAILABLE
    const int simdWidth = 16;

    // First row
    {
        const uint8_t* currLine = src;
        const uint8_t* nextLine = src + stride;
        uint8_t* dstLine = dst;

        int x = 0;
        for (; x + simdWidth <= width; x += simdWidth) {
            uint8x16_t curr = vld1q_u8(currLine + x);
            uint8x16_t next = vld1q_u8(nextLine + x);
            vst1q_u8(dstLine + x, vminq_u8(curr, next));
        }
        for (; x < width; ++x) {
            dstLine[x] = std::min(currLine[x], nextLine[x]);
        }
    }

    // Middle rows
    for (int y = 1; y < height - 1; ++y) {
        const uint8_t* prevLine = src + (y - 1) * stride;
        const uint8_t* currLine = src + y * stride;
        const uint8_t* nextLine = src + (y + 1) * stride;
        uint8_t* dstLine = dst + y * stride;

        int x = 0;
        for (; x + simdWidth <= width; x += simdWidth) {
            uint8x16_t prev = vld1q_u8(prevLine + x);
            uint8x16_t curr = vld1q_u8(currLine + x);
            uint8x16_t next = vld1q_u8(nextLine + x);
            vst1q_u8(dstLine + x, vminq_u8(vminq_u8(prev, curr), next));
        }
        for (; x < width; ++x) {
            dstLine[x] = std::min(std::min(prevLine[x], currLine[x]), nextLine[x]);
        }
    }

    // Last row
    if (height > 1) {
        const uint8_t* prevLine = src + (height - 2) * stride;
        const uint8_t* currLine = src + (height - 1) * stride;
        uint8_t* dstLine = dst + (height - 1) * stride;

        int x = 0;
        for (; x + simdWidth <= width; x += simdWidth) {
            uint8x16_t prev = vld1q_u8(prevLine + x);
            uint8x16_t curr = vld1q_u8(currLine + x);
            vst1q_u8(dstLine + x, vminq_u8(prev, curr));
        }
        for (; x < width; ++x) {
            dstLine[x] = std::min(prevLine[x], currLine[x]);
        }
    }

#else
    // Scalar fallback
    // First row
    {
        const uint8_t* currLine = src;
        const uint8_t* nextLine = src + stride;
        uint8_t* dstLine = dst;
        for (int x = 0; x < width; ++x) {
            dstLine[x] = std::min(currLine[x], nextLine[x]);
        }
    }

    // Middle rows
    for (int y = 1; y < height - 1; ++y) {
        const uint8_t* prevLine = src + (y - 1) * stride;
        const uint8_t* currLine = src + y * stride;
        const uint8_t* nextLine = src + (y + 1) * stride;
        uint8_t* dstLine = dst + y * stride;

        for (int x = 0; x < width; ++x) {
            dstLine[x] = std::min(std::min(prevLine[x], currLine[x]), nextLine[x]);
        }
    }

    // Last row
    if (height > 1) {
        const uint8_t* prevLine = src + (height - 2) * stride;
        const uint8_t* currLine = src + (height - 1) * stride;
        uint8_t* dstLine = dst + (height - 1) * stride;
        for (int x = 0; x < width; ++x) {
            dstLine[x] = std::min(prevLine[x], currLine[x]);
        }
    }
#endif
}

void rgbToGraySIMD(
    uint8_t* dst, int dstStride,
    const uint32_t* src, int srcStride,
    int width, int height)
{
    if (width <= 0 || height <= 0) return;

#if SIMD_SSE2_AVAILABLE
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < height; ++y) {
        const uint32_t* srcLine = src + y * srcStride;
        uint8_t* dstLine = dst + y * dstStride;

        int x = 0;
        // Process 8 pixels at a time
        for (; x + 8 <= width; x += 8) {
            __m128i gray8 = rgb32ToGray8_sse2(srcLine + x);

            // Store lower 8 bytes
            // Extract and store byte by byte or use _mm_storel_epi64
            _mm_storel_epi64(reinterpret_cast<__m128i*>(dstLine + x), gray8);
        }

        // Process remaining pixels
        for (; x < width; ++x) {
            uint32_t rgb = srcLine[x];
            int r = (rgb >> 16) & 0xFF;
            int g = (rgb >> 8) & 0xFF;
            int b = rgb & 0xFF;
            dstLine[x] = static_cast<uint8_t>((r * 11 + g * 16 + b * 5) >> 5);
        }
    }

#elif SIMD_NEON_AVAILABLE
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < height; ++y) {
        const uint32_t* srcLine = src + y * srcStride;
        uint8_t* dstLine = dst + y * dstStride;

        int x = 0;
        // Process 8 pixels at a time
        for (; x + 8 <= width; x += 8) {
            // Load 8 pixels (4 at a time)
            uint32x4_t px0 = vld1q_u32(srcLine + x);
            uint32x4_t px1 = vld1q_u32(srcLine + x + 4);

            // Extract channels - NEON doesn't have a direct extract
            // so we need to do it manually
            uint8x8x4_t rgb0 = vld4_u8(reinterpret_cast<const uint8_t*>(srcLine + x));
            uint8x8x4_t rgb1 = vld4_u8(reinterpret_cast<const uint8_t*>(srcLine + x + 4));

            // Combine B, G, R channels (assuming ARGB layout)
            // B = rgb.val[0], G = rgb.val[1], R = rgb.val[2]
            // gray = (R*11 + G*16 + B*5) / 32

            uint16x8_t sum = vmull_u8(rgb0.val[2], vdup_n_u8(11));  // R*11
            sum = vmlal_u8(sum, rgb0.val[1], vdup_n_u8(16));        // + G*16
            sum = vmlal_u8(sum, rgb0.val[0], vdup_n_u8(5));         // + B*5

            uint8x8_t gray = vshrn_n_u16(sum, 5);  // / 32
            vst1_u8(dstLine + x, gray);
        }

        // Process remaining pixels
        for (; x < width; ++x) {
            uint32_t rgb = srcLine[x];
            int r = (rgb >> 16) & 0xFF;
            int g = (rgb >> 8) & 0xFF;
            int b = rgb & 0xFF;
            dstLine[x] = static_cast<uint8_t>((r * 11 + g * 16 + b * 5) >> 5);
        }
    }

#else
    // Scalar fallback with OpenMP
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < height; ++y) {
        const uint32_t* srcLine = src + y * srcStride;
        uint8_t* dstLine = dst + y * dstStride;

        for (int x = 0; x < width; ++x) {
            uint32_t rgb = srcLine[x];
            int r = (rgb >> 16) & 0xFF;
            int g = (rgb >> 8) & 0xFF;
            int b = rgb & 0xFF;
            dstLine[x] = static_cast<uint8_t>((r * 11 + g * 16 + b * 5) >> 5);
        }
    }
#endif
}

int countBitsSIMD(const uint32_t* data, size_t wordCount)
{
    if (wordCount == 0) return 0;

    int count = 0;

#if SIMD_SSE2_AVAILABLE && defined(__SSSE3__)
    const size_t simdWords = 4;  // Process 4 words (128 bits) at a time
    size_t i = 0;

    for (; i + simdWords <= wordCount; i += simdWords) {
        __m128i v = load128u(data + i);
        count += popcnt128(v);
    }

    // Process remaining words
    for (; i < wordCount; ++i) {
        count += popcount32(data[i]);
    }

#elif SIMD_NEON_AVAILABLE
    const size_t simdWords = 4;
    size_t i = 0;

    for (; i + simdWords <= wordCount; i += simdWords) {
        uint8x16_t v = vld1q_u8(reinterpret_cast<const uint8_t*>(data + i));
        count += popcnt_neon(v);
    }

    for (; i < wordCount; ++i) {
        count += popcount32(data[i]);
    }

#else
    // Scalar fallback
    for (size_t i = 0; i < wordCount; ++i) {
        count += popcount32(data[i]);
    }
#endif

    return count;
}

void invertWordsSIMD(uint32_t* data, size_t wordCount)
{
    if (wordCount == 0) return;

#if SIMD_SSE2_AVAILABLE
    const size_t simdWords = 4;  // 4 words per 128-bit register
    size_t i = 0;

    __m128i allOnes = _mm_set1_epi32(-1);

    for (; i + simdWords <= wordCount; i += simdWords) {
        __m128i v = load128u(data + i);
        v = xor128(v, allOnes);
        store128u(data + i, v);
    }

    // Process remaining words
    for (; i < wordCount; ++i) {
        data[i] = ~data[i];
    }

#elif SIMD_AVX2_AVAILABLE
    const size_t simdWords = 8;  // 8 words per 256-bit register
    size_t i = 0;

    __m256i allOnes = _mm256_set1_epi32(-1);

    for (; i + simdWords <= wordCount; i += simdWords) {
        __m256i v = load256u(data + i);
        v = xor256(v, allOnes);
        store256u(data + i, v);
    }

    // Process remaining with SSE2
    for (; i + 4 <= wordCount; i += 4) {
        __m128i v = load128u(data + i);
        v = xor128(v, _mm_set1_epi32(-1));
        store128u(data + i, v);
    }

    for (; i < wordCount; ++i) {
        data[i] = ~data[i];
    }

#elif SIMD_NEON_AVAILABLE
    const size_t simdWords = 4;
    size_t i = 0;

    for (; i + simdWords <= wordCount; i += simdWords) {
        uint32x4_t v = vld1q_u32(data + i);
        v = vmvnq_u32(v);
        vst1q_u32(data + i, v);
    }

    for (; i < wordCount; ++i) {
        data[i] = ~data[i];
    }

#else
    // Scalar fallback
    for (size_t i = 0; i < wordCount; ++i) {
        data[i] = ~data[i];
    }
#endif
}

} // namespace simd
} // namespace imageproc
