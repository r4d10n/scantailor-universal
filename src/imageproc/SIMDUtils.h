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

#ifndef IMAGEPROC_SIMD_UTILS_H_
#define IMAGEPROC_SIMD_UTILS_H_

#include <stdint.h>
#include <cstddef>

// Platform and SIMD detection
#if defined(_MSC_VER)
    // MSVC compiler
    #include <intrin.h>
    #if defined(_M_X64) || defined(_M_IX86)
        #define SIMD_SSE2_AVAILABLE 1
        #if _MSC_VER >= 1500  // VS 2008+
            #define SIMD_SSE4_1_AVAILABLE 1
        #endif
        #if _MSC_VER >= 1700 && defined(__AVX2__)  // VS 2012+
            #define SIMD_AVX2_AVAILABLE 1
        #endif
    #endif
#elif defined(__GNUC__) || defined(__clang__)
    // GCC or Clang compiler
    #if defined(__x86_64__) || defined(__i386__)
        #if defined(__SSE2__)
            #define SIMD_SSE2_AVAILABLE 1
            #include <emmintrin.h>
        #endif
        #if defined(__SSE4_1__)
            #define SIMD_SSE4_1_AVAILABLE 1
            #include <smmintrin.h>
        #endif
        #if defined(__AVX2__)
            #define SIMD_AVX2_AVAILABLE 1
            #include <immintrin.h>
        #endif
    #elif defined(__ARM_NEON) || defined(__ARM_NEON__)
        #define SIMD_NEON_AVAILABLE 1
        #include <arm_neon.h>
    #endif
#endif

// Fallback definitions
#ifndef SIMD_SSE2_AVAILABLE
    #define SIMD_SSE2_AVAILABLE 0
#endif
#ifndef SIMD_SSE4_1_AVAILABLE
    #define SIMD_SSE4_1_AVAILABLE 0
#endif
#ifndef SIMD_AVX2_AVAILABLE
    #define SIMD_AVX2_AVAILABLE 0
#endif
#ifndef SIMD_NEON_AVAILABLE
    #define SIMD_NEON_AVAILABLE 0
#endif

// Memory alignment helpers
#if defined(_MSC_VER)
    #define SIMD_ALIGN(n) __declspec(align(n))
#elif defined(__GNUC__) || defined(__clang__)
    #define SIMD_ALIGN(n) __attribute__((aligned(n)))
#else
    #define SIMD_ALIGN(n)
#endif

// Prefetch hints
#if defined(__GNUC__) || defined(__clang__)
    #define SIMD_PREFETCH(addr) __builtin_prefetch(addr)
#elif defined(_MSC_VER) && SIMD_SSE2_AVAILABLE
    #define SIMD_PREFETCH(addr) _mm_prefetch((const char*)(addr), _MM_HINT_T0)
#else
    #define SIMD_PREFETCH(addr) ((void)0)
#endif

namespace imageproc
{
namespace simd
{

/**
 * Check if a pointer is aligned to the specified boundary
 */
inline bool isAligned(const void* ptr, size_t alignment) {
    return (reinterpret_cast<uintptr_t>(ptr) & (alignment - 1)) == 0;
}

/**
 * Returns true if runtime SIMD is available and beneficial
 */
inline bool simdAvailable() {
#if SIMD_SSE2_AVAILABLE || SIMD_NEON_AVAILABLE || SIMD_AVX2_AVAILABLE
    return true;
#else
    return false;
#endif
}

#if SIMD_SSE2_AVAILABLE

// ============================================================================
// SSE2 Operations (128-bit vectors)
// ============================================================================

/**
 * Load 128 bits of data (aligned)
 */
inline __m128i load128a(const void* ptr) {
    return _mm_load_si128(reinterpret_cast<const __m128i*>(ptr));
}

/**
 * Load 128 bits of data (unaligned)
 */
inline __m128i load128u(const void* ptr) {
    return _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
}

/**
 * Store 128 bits of data (aligned)
 */
inline void store128a(void* ptr, __m128i val) {
    _mm_store_si128(reinterpret_cast<__m128i*>(ptr), val);
}

/**
 * Store 128 bits of data (unaligned)
 */
inline void store128u(void* ptr, __m128i val) {
    _mm_storeu_si128(reinterpret_cast<__m128i*>(ptr), val);
}

/**
 * Maximum of 16 uint8_t values
 */
inline __m128i max_epu8(__m128i a, __m128i b) {
    return _mm_max_epu8(a, b);
}

/**
 * Minimum of 16 uint8_t values
 */
inline __m128i min_epu8(__m128i a, __m128i b) {
    return _mm_min_epu8(a, b);
}

/**
 * Add 16 uint8_t values (saturating)
 */
inline __m128i adds_epu8(__m128i a, __m128i b) {
    return _mm_adds_epu8(a, b);
}

/**
 * Subtract 16 uint8_t values (saturating)
 */
inline __m128i subs_epu8(__m128i a, __m128i b) {
    return _mm_subs_epu8(a, b);
}

/**
 * XOR 128 bits
 */
inline __m128i xor128(__m128i a, __m128i b) {
    return _mm_xor_si128(a, b);
}

/**
 * AND 128 bits
 */
inline __m128i and128(__m128i a, __m128i b) {
    return _mm_and_si128(a, b);
}

/**
 * OR 128 bits
 */
inline __m128i or128(__m128i a, __m128i b) {
    return _mm_or_si128(a, b);
}

/**
 * NOT 128 bits (using XOR with all 1s)
 */
inline __m128i not128(__m128i a) {
    return _mm_xor_si128(a, _mm_set1_epi32(-1));
}

/**
 * Set all bytes to a value
 */
inline __m128i set1_epi8(int8_t val) {
    return _mm_set1_epi8(val);
}

/**
 * Set all 32-bit values
 */
inline __m128i set1_epi32(int32_t val) {
    return _mm_set1_epi32(val);
}

/**
 * Compare equal (bytes)
 */
inline __m128i cmpeq_epi8(__m128i a, __m128i b) {
    return _mm_cmpeq_epi8(a, b);
}

/**
 * Maximum of 4 int32_t values (SSE2 emulation - SSE4.1 has native support)
 * For values that fit in 31 bits (typical for squared distances)
 */
inline __m128i max_epi32_sse2(__m128i a, __m128i b) {
#if SIMD_SSE4_1_AVAILABLE
    return _mm_max_epi32(a, b);
#else
    // SSE2 emulation: max(a,b) = a XOR ((a XOR b) AND (a < b ? -1 : 0))
    __m128i cmp = _mm_cmplt_epi32(a, b);  // a < b ? -1 : 0
    __m128i diff = _mm_xor_si128(a, b);
    return _mm_xor_si128(a, _mm_and_si128(diff, cmp));
#endif
}

/**
 * Population count for a 128-bit vector (sum of all set bits)
 * Returns the count as an integer
 */
inline int popcnt128(__m128i v) {
    // Use lookup table method for portability
    const __m128i lookup = _mm_setr_epi8(
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4
    );
    const __m128i low_mask = _mm_set1_epi8(0x0f);

    __m128i lo = _mm_and_si128(v, low_mask);
    __m128i hi = _mm_and_si128(_mm_srli_epi16(v, 4), low_mask);
    __m128i popcnt_lo = _mm_shuffle_epi8(lookup, lo);
    __m128i popcnt_hi = _mm_shuffle_epi8(lookup, hi);
    __m128i sum8 = _mm_add_epi8(popcnt_lo, popcnt_hi);

    // Horizontal sum: sum all bytes
    __m128i sum16 = _mm_sad_epu8(sum8, _mm_setzero_si128());
    return _mm_extract_epi16(sum16, 0) + _mm_extract_epi16(sum16, 4);
}

/**
 * Horizontal maximum of 16 uint8_t values
 */
inline uint8_t hmax_epu8(__m128i v) {
    __m128i max1 = _mm_max_epu8(v, _mm_srli_si128(v, 8));
    __m128i max2 = _mm_max_epu8(max1, _mm_srli_si128(max1, 4));
    __m128i max3 = _mm_max_epu8(max2, _mm_srli_si128(max2, 2));
    __m128i max4 = _mm_max_epu8(max3, _mm_srli_si128(max3, 1));
    return static_cast<uint8_t>(_mm_extract_epi16(max4, 0) & 0xFF);
}

/**
 * Horizontal minimum of 16 uint8_t values
 */
inline uint8_t hmin_epu8(__m128i v) {
    __m128i min1 = _mm_min_epu8(v, _mm_srli_si128(v, 8));
    __m128i min2 = _mm_min_epu8(min1, _mm_srli_si128(min1, 4));
    __m128i min3 = _mm_min_epu8(min2, _mm_srli_si128(min2, 2));
    __m128i min4 = _mm_min_epu8(min3, _mm_srli_si128(min3, 1));
    return static_cast<uint8_t>(_mm_extract_epi16(min4, 0) & 0xFF);
}

/**
 * Convert 8 RGB32 pixels to 8 grayscale values
 * Uses formula: gray = (R*11 + G*16 + B*5) / 32
 * Returns 8 grayscale bytes packed in the lower 64 bits
 */
inline __m128i rgb32ToGray8_sse2(const uint32_t* src) {
    // Load 4 RGB32 pixels at a time and convert
    __m128i px0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src));
    __m128i px1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + 4));

    // Extract R, G, B channels
    const __m128i mask_r = _mm_set1_epi32(0x00FF0000);
    const __m128i mask_g = _mm_set1_epi32(0x0000FF00);
    const __m128i mask_b = _mm_set1_epi32(0x000000FF);

    // Process first 4 pixels
    __m128i r0 = _mm_srli_epi32(_mm_and_si128(px0, mask_r), 16);
    __m128i g0 = _mm_srli_epi32(_mm_and_si128(px0, mask_g), 8);
    __m128i b0 = _mm_and_si128(px0, mask_b);

    // gray = (R*11 + G*16 + B*5) / 32
    __m128i gray0 = _mm_srli_epi32(
        _mm_add_epi32(
            _mm_add_epi32(
                _mm_mullo_epi16(r0, _mm_set1_epi32(11)),
                _mm_slli_epi32(g0, 4)  // G*16
            ),
            _mm_mullo_epi16(b0, _mm_set1_epi32(5))
        ),
        5  // divide by 32
    );

    // Process second 4 pixels
    __m128i r1 = _mm_srli_epi32(_mm_and_si128(px1, mask_r), 16);
    __m128i g1 = _mm_srli_epi32(_mm_and_si128(px1, mask_g), 8);
    __m128i b1 = _mm_and_si128(px1, mask_b);

    __m128i gray1 = _mm_srli_epi32(
        _mm_add_epi32(
            _mm_add_epi32(
                _mm_mullo_epi16(r1, _mm_set1_epi32(11)),
                _mm_slli_epi32(g1, 4)
            ),
            _mm_mullo_epi16(b1, _mm_set1_epi32(5))
        ),
        5
    );

    // Pack 32-bit to 16-bit to 8-bit
    __m128i gray16 = _mm_packs_epi32(gray0, gray1);
    return _mm_packus_epi16(gray16, _mm_setzero_si128());
}

#endif // SIMD_SSE2_AVAILABLE

#if SIMD_AVX2_AVAILABLE

// ============================================================================
// AVX2 Operations (256-bit vectors)
// ============================================================================

/**
 * Load 256 bits of data (aligned)
 */
inline __m256i load256a(const void* ptr) {
    return _mm256_load_si256(reinterpret_cast<const __m256i*>(ptr));
}

/**
 * Load 256 bits of data (unaligned)
 */
inline __m256i load256u(const void* ptr) {
    return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr));
}

/**
 * Store 256 bits of data (aligned)
 */
inline void store256a(void* ptr, __m256i val) {
    _mm256_store_si256(reinterpret_cast<__m256i*>(ptr), val);
}

/**
 * Store 256 bits of data (unaligned)
 */
inline void store256u(void* ptr, __m256i val) {
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(ptr), val);
}

/**
 * Maximum of 32 uint8_t values
 */
inline __m256i max256_epu8(__m256i a, __m256i b) {
    return _mm256_max_epu8(a, b);
}

/**
 * Minimum of 32 uint8_t values
 */
inline __m256i min256_epu8(__m256i a, __m256i b) {
    return _mm256_min_epu8(a, b);
}

/**
 * XOR 256 bits
 */
inline __m256i xor256(__m256i a, __m256i b) {
    return _mm256_xor_si256(a, b);
}

/**
 * AND 256 bits
 */
inline __m256i and256(__m256i a, __m256i b) {
    return _mm256_and_si256(a, b);
}

/**
 * OR 256 bits
 */
inline __m256i or256(__m256i a, __m256i b) {
    return _mm256_or_si256(a, b);
}

#endif // SIMD_AVX2_AVAILABLE

#if SIMD_NEON_AVAILABLE

// ============================================================================
// NEON Operations (128-bit vectors on ARM)
// ============================================================================

/**
 * Load 128 bits of data
 */
inline uint8x16_t load128_neon(const uint8_t* ptr) {
    return vld1q_u8(ptr);
}

/**
 * Store 128 bits of data
 */
inline void store128_neon(uint8_t* ptr, uint8x16_t val) {
    vst1q_u8(ptr, val);
}

/**
 * Maximum of 16 uint8_t values
 */
inline uint8x16_t max_neon_u8(uint8x16_t a, uint8x16_t b) {
    return vmaxq_u8(a, b);
}

/**
 * Minimum of 16 uint8_t values
 */
inline uint8x16_t min_neon_u8(uint8x16_t a, uint8x16_t b) {
    return vminq_u8(a, b);
}

/**
 * XOR 128 bits
 */
inline uint8x16_t xor_neon(uint8x16_t a, uint8x16_t b) {
    return veorq_u8(a, b);
}

/**
 * AND 128 bits
 */
inline uint8x16_t and_neon(uint8x16_t a, uint8x16_t b) {
    return vandq_u8(a, b);
}

/**
 * OR 128 bits
 */
inline uint8x16_t or_neon(uint8x16_t a, uint8x16_t b) {
    return vorrq_u8(a, b);
}

/**
 * NOT 128 bits
 */
inline uint8x16_t not_neon(uint8x16_t a) {
    return vmvnq_u8(a);
}

/**
 * Population count for NEON
 */
inline int popcnt_neon(uint8x16_t v) {
    uint8x16_t cnt = vcntq_u8(v);
    // Sum all bytes
    return vaddvq_u8(cnt);
}

#endif // SIMD_NEON_AVAILABLE

// ============================================================================
// Scalar fallback operations (for platforms without SIMD)
// ============================================================================

/**
 * Scalar maximum of arrays
 */
inline void maxArrayScalar(
    uint8_t* dst, const uint8_t* src1, const uint8_t* src2, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        dst[i] = src1[i] > src2[i] ? src1[i] : src2[i];
    }
}

/**
 * Scalar minimum of arrays
 */
inline void minArrayScalar(
    uint8_t* dst, const uint8_t* src1, const uint8_t* src2, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        dst[i] = src1[i] < src2[i] ? src1[i] : src2[i];
    }
}

/**
 * Population count for 32-bit integer
 */
inline int popcount32(uint32_t v) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcount(v);
#elif defined(_MSC_VER) && SIMD_SSE4_1_AVAILABLE
    return _mm_popcnt_u32(v);
#else
    // Standard bit manipulation
    v = v - ((v >> 1) & 0x55555555);
    v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
    return (((v + (v >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
#endif
}

/**
 * Population count for 64-bit integer
 */
inline int popcount64(uint64_t v) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcountll(v);
#elif defined(_MSC_VER) && defined(_M_X64)
    return static_cast<int>(_mm_popcnt_u64(v));
#else
    // Fallback: count in two 32-bit halves
    return popcount32(static_cast<uint32_t>(v)) +
           popcount32(static_cast<uint32_t>(v >> 32));
#endif
}

// ============================================================================
// High-level optimized operations
// ============================================================================

/**
 * Compute maximum of array elements with their shifted neighbors
 * Optimized for morphological dilation operations
 * dst[i] = max(src[i-1], src[i], src[i+1])
 */
void max3Horizontal(
    uint8_t* dst, const uint8_t* src, int width, int height, int stride);

/**
 * Compute maximum of array elements vertically
 * dst[i] = max(src[i-stride], src[i], src[i+stride])
 */
void max3Vertical(
    uint8_t* dst, const uint8_t* src, int width, int height, int stride);

/**
 * Compute minimum of array elements with their shifted neighbors
 * Optimized for morphological erosion operations
 * dst[i] = min(src[i-1], src[i], src[i+1])
 */
void min3Horizontal(
    uint8_t* dst, const uint8_t* src, int width, int height, int stride);

/**
 * Compute minimum of array elements vertically
 * dst[i] = min(src[i-stride], src[i], src[i+stride])
 */
void min3Vertical(
    uint8_t* dst, const uint8_t* src, int width, int height, int stride);

/**
 * Convert RGB32 image to grayscale using SIMD
 * gray = (R*11 + G*16 + B*5) / 32
 */
void rgbToGraySIMD(
    uint8_t* dst, int dstStride,
    const uint32_t* src, int srcStride,
    int width, int height);

/**
 * Count non-zero bits in a 32-bit word array
 */
int countBitsSIMD(const uint32_t* data, size_t wordCount);

/**
 * Invert a 32-bit word array in-place
 */
void invertWordsSIMD(uint32_t* data, size_t wordCount);

} // namespace simd
} // namespace imageproc

#endif // IMAGEPROC_SIMD_UTILS_H_
