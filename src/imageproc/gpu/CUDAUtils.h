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

#ifndef IMAGEPROC_GPU_CUDAUTILS_H_
#define IMAGEPROC_GPU_CUDAUTILS_H_

#include <cstdint>
#include <cstddef>

// Forward declarations for Qt types
class QImage;

namespace imageproc {
namespace gpu {

/**
 * @brief CUDA device information and capabilities
 */
struct CUDADeviceInfo {
    bool available;           // CUDA is available and working
    int deviceCount;          // Number of CUDA devices
    int selectedDevice;       // Currently selected device
    char deviceName[256];     // Device name
    int major;                // Compute capability major
    int minor;                // Compute capability minor
    size_t totalMemory;       // Total global memory in bytes
    size_t freeMemory;        // Free global memory in bytes
    int multiprocessorCount;  // Number of streaming multiprocessors
    int maxThreadsPerBlock;   // Maximum threads per block
    int maxBlockDimX;         // Maximum block dimension X
    int maxBlockDimY;         // Maximum block dimension Y
    int warpSize;             // Warp size
};

/**
 * @brief Initialize CUDA subsystem
 * @return true if CUDA is available and initialized successfully
 */
bool initCUDA();

/**
 * @brief Shutdown CUDA subsystem and free resources
 */
void shutdownCUDA();

/**
 * @brief Check if CUDA is available and initialized
 * @return true if CUDA can be used
 */
bool isCUDAAvailable();

/**
 * @brief Get information about the current CUDA device
 * @return CUDADeviceInfo structure with device details
 */
CUDADeviceInfo getCUDADeviceInfo();

/**
 * @brief Set which CUDA device to use
 * @param deviceId Device index (0-based)
 * @return true if device was set successfully
 */
bool setCUDADevice(int deviceId);

/**
 * @brief Get the last CUDA error message
 * @return Error message string
 */
const char* getLastCUDAError();

// ============================================================================
// GPU Image Processing Functions
// ============================================================================

/**
 * @brief Convert RGB image to grayscale using GPU
 * @param src Source RGB/RGBA image data (row-major, RGBA format)
 * @param dst Destination grayscale data (row-major, single channel)
 * @param width Image width in pixels
 * @param height Image height in pixels
 * @param srcBytesPerPixel Bytes per pixel in source (3 for RGB, 4 for RGBA)
 * @return true if operation succeeded
 */
bool gpuRgbToGrayscale(
    const uint8_t* src,
    uint8_t* dst,
    int width,
    int height,
    int srcBytesPerPixel = 4
);

/**
 * @brief Convert QImage to grayscale using GPU
 * @param src Source QImage
 * @param dst Destination grayscale buffer (must be preallocated: width * height bytes)
 * @return true if operation succeeded
 */
bool gpuRgbToGrayscale(const QImage& src, uint8_t* dst);

/**
 * @brief Perform morphological dilation using GPU
 * @param src Source grayscale image data
 * @param dst Destination image data
 * @param width Image width
 * @param height Image height
 * @param kernelWidth Structuring element width (must be odd)
 * @param kernelHeight Structuring element height (must be odd)
 * @return true if operation succeeded
 */
bool gpuDilate(
    const uint8_t* src,
    uint8_t* dst,
    int width,
    int height,
    int kernelWidth = 3,
    int kernelHeight = 3
);

/**
 * @brief Perform morphological erosion using GPU
 * @param src Source grayscale image data
 * @param dst Destination image data
 * @param width Image width
 * @param height Image height
 * @param kernelWidth Structuring element width (must be odd)
 * @param kernelHeight Structuring element height (must be odd)
 * @return true if operation succeeded
 */
bool gpuErode(
    const uint8_t* src,
    uint8_t* dst,
    int width,
    int height,
    int kernelWidth = 3,
    int kernelHeight = 3
);

/**
 * @brief Compute Squared Euclidean Distance Map using GPU
 * @param src Source binary image (0 = background, non-zero = foreground)
 * @param dst Destination distance map (32-bit integers)
 * @param width Image width
 * @param height Image height
 * @return true if operation succeeded
 */
bool gpuSEDM(
    const uint8_t* src,
    uint32_t* dst,
    int width,
    int height
);

/**
 * @brief Perform image dewarping using GPU
 * @param src Source image data (grayscale or RGBA)
 * @param dst Destination image data
 * @param srcWidth Source image width
 * @param srcHeight Source image height
 * @param dstWidth Destination image width
 * @param dstHeight Destination image height
 * @param mapX X coordinate mapping array (dstWidth * dstHeight floats)
 * @param mapY Y coordinate mapping array (dstWidth * dstHeight floats)
 * @param bytesPerPixel Bytes per pixel (1 for grayscale, 4 for RGBA)
 * @return true if operation succeeded
 */
bool gpuDewarp(
    const uint8_t* src,
    uint8_t* dst,
    int srcWidth,
    int srcHeight,
    int dstWidth,
    int dstHeight,
    const float* mapX,
    const float* mapY,
    int bytesPerPixel = 1
);

/**
 * @brief Perform Otsu binarization using GPU
 * @param src Source grayscale image
 * @param dst Destination binary image (0 or 255)
 * @param width Image width
 * @param height Image height
 * @param threshold Output: computed Otsu threshold
 * @return true if operation succeeded
 */
bool gpuBinarizeOtsu(
    const uint8_t* src,
    uint8_t* dst,
    int width,
    int height,
    int* threshold = nullptr
);

/**
 * @brief Perform Sauvola binarization using GPU
 * @param src Source grayscale image
 * @param dst Destination binary image (0 or 255)
 * @param width Image width
 * @param height Image height
 * @param windowSize Local window size (must be odd)
 * @param k Sauvola parameter (typically 0.2-0.5)
 * @return true if operation succeeded
 */
bool gpuBinarizeSauvola(
    const uint8_t* src,
    uint8_t* dst,
    int width,
    int height,
    int windowSize = 15,
    float k = 0.34f
);

/**
 * @brief Invert binary image using GPU
 * @param data Image data (modified in-place)
 * @param width Image width
 * @param height Image height
 * @return true if operation succeeded
 */
bool gpuInvert(
    uint8_t* data,
    int width,
    int height
);

/**
 * @brief Count black pixels in binary image using GPU
 * @param data Binary image data
 * @param width Image width
 * @param height Image height
 * @return Number of black pixels (value 0), or -1 on error
 */
int64_t gpuCountBlackPixels(
    const uint8_t* data,
    int width,
    int height
);

// ============================================================================
// Memory Management Helpers
// ============================================================================

/**
 * @brief Allocate GPU memory
 * @param size Size in bytes
 * @return Pointer to GPU memory, or nullptr on failure
 */
void* gpuMalloc(size_t size);

/**
 * @brief Free GPU memory
 * @param ptr Pointer to GPU memory
 */
void gpuFree(void* ptr);

/**
 * @brief Copy data from host to GPU
 * @param dst GPU destination pointer
 * @param src Host source pointer
 * @param size Size in bytes
 * @return true if copy succeeded
 */
bool gpuMemcpyHostToDevice(void* dst, const void* src, size_t size);

/**
 * @brief Copy data from GPU to host
 * @param dst Host destination pointer
 * @param src GPU source pointer
 * @param size Size in bytes
 * @return true if copy succeeded
 */
bool gpuMemcpyDeviceToHost(void* dst, const void* src, size_t size);

/**
 * @brief Synchronize GPU operations
 * @return true if synchronization succeeded
 */
bool gpuSynchronize();

} // namespace gpu
} // namespace imageproc

#endif // IMAGEPROC_GPU_CUDAUTILS_H_
