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

#include "CUDAUtils.h"

#ifdef CUDA_AVAILABLE

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace imageproc {
namespace gpu {

// ============================================================================
// Global State
// ============================================================================

static bool g_cudaInitialized = false;
static CUDADeviceInfo g_deviceInfo = {};
static char g_lastError[512] = "";

// Thread block dimensions for 2D kernels
static const int BLOCK_DIM_X = 16;
static const int BLOCK_DIM_Y = 16;
static const int BLOCK_SIZE = BLOCK_DIM_X * BLOCK_DIM_Y;

// ============================================================================
// Error Handling
// ============================================================================

#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            snprintf(g_lastError, sizeof(g_lastError), \
                     "CUDA error at %s:%d: %s", \
                     __FILE__, __LINE__, cudaGetErrorString(err)); \
            return false; \
        } \
    } while(0)

#define CUDA_CHECK_RETURN(call, retval) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            snprintf(g_lastError, sizeof(g_lastError), \
                     "CUDA error at %s:%d: %s", \
                     __FILE__, __LINE__, cudaGetErrorString(err)); \
            return retval; \
        } \
    } while(0)

// ============================================================================
// CUDA Kernels
// ============================================================================

/**
 * @brief RGB to Grayscale conversion kernel
 * Uses standard luminance formula: Y = 0.299*R + 0.587*G + 0.114*B
 */
__global__ void grayscaleKernel(
    const uint8_t* __restrict__ src,
    uint8_t* __restrict__ dst,
    int width,
    int height,
    int srcBytesPerPixel)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x < width && y < height) {
        int srcIdx = (y * width + x) * srcBytesPerPixel;
        int dstIdx = y * width + x;

        // RGB order (Qt uses ARGB32 which is BGRA in memory on little-endian)
        uint8_t b = src[srcIdx + 0];
        uint8_t g = src[srcIdx + 1];
        uint8_t r = src[srcIdx + 2];

        // Fixed-point arithmetic for speed: (R*77 + G*150 + B*29) >> 8
        uint32_t gray = (r * 77 + g * 150 + b * 29) >> 8;
        dst[dstIdx] = static_cast<uint8_t>(gray);
    }
}

/**
 * @brief Morphological dilation kernel with shared memory tiling
 */
__global__ void dilationKernel(
    const uint8_t* __restrict__ src,
    uint8_t* __restrict__ dst,
    int width,
    int height,
    int kernelRadiusX,
    int kernelRadiusY)
{
    // Shared memory tile with halo
    extern __shared__ uint8_t tile[];

    int tx = threadIdx.x;
    int ty = threadIdx.y;
    int x = blockIdx.x * blockDim.x + tx;
    int y = blockIdx.y * blockDim.y + ty;

    int tileWidth = blockDim.x + 2 * kernelRadiusX;
    int tileHeight = blockDim.y + 2 * kernelRadiusY;

    // Load tile including halo region
    for (int dy = ty; dy < tileHeight; dy += blockDim.y) {
        for (int dx = tx; dx < tileWidth; dx += blockDim.x) {
            int srcX = blockIdx.x * blockDim.x + dx - kernelRadiusX;
            int srcY = blockIdx.y * blockDim.y + dy - kernelRadiusY;

            // Clamp to image bounds
            srcX = max(0, min(width - 1, srcX));
            srcY = max(0, min(height - 1, srcY));

            tile[dy * tileWidth + dx] = src[srcY * width + srcX];
        }
    }

    __syncthreads();

    if (x < width && y < height) {
        uint8_t maxVal = 0;

        // Find maximum in kernel neighborhood
        for (int ky = -kernelRadiusY; ky <= kernelRadiusY; ky++) {
            for (int kx = -kernelRadiusX; kx <= kernelRadiusX; kx++) {
                int tileX = tx + kernelRadiusX + kx;
                int tileY = ty + kernelRadiusY + ky;
                maxVal = max(maxVal, tile[tileY * tileWidth + tileX]);
            }
        }

        dst[y * width + x] = maxVal;
    }
}

/**
 * @brief Morphological erosion kernel with shared memory tiling
 */
__global__ void erosionKernel(
    const uint8_t* __restrict__ src,
    uint8_t* __restrict__ dst,
    int width,
    int height,
    int kernelRadiusX,
    int kernelRadiusY)
{
    extern __shared__ uint8_t tile[];

    int tx = threadIdx.x;
    int ty = threadIdx.y;
    int x = blockIdx.x * blockDim.x + tx;
    int y = blockIdx.y * blockDim.y + ty;

    int tileWidth = blockDim.x + 2 * kernelRadiusX;
    int tileHeight = blockDim.y + 2 * kernelRadiusY;

    // Load tile including halo region
    for (int dy = ty; dy < tileHeight; dy += blockDim.y) {
        for (int dx = tx; dx < tileWidth; dx += blockDim.x) {
            int srcX = blockIdx.x * blockDim.x + dx - kernelRadiusX;
            int srcY = blockIdx.y * blockDim.y + dy - kernelRadiusY;

            srcX = max(0, min(width - 1, srcX));
            srcY = max(0, min(height - 1, srcY));

            tile[dy * tileWidth + dx] = src[srcY * width + srcX];
        }
    }

    __syncthreads();

    if (x < width && y < height) {
        uint8_t minVal = 255;

        for (int ky = -kernelRadiusY; ky <= kernelRadiusY; ky++) {
            for (int kx = -kernelRadiusX; kx <= kernelRadiusX; kx++) {
                int tileX = tx + kernelRadiusX + kx;
                int tileY = ty + kernelRadiusY + ky;
                minVal = min(minVal, tile[tileY * tileWidth + tileX]);
            }
        }

        dst[y * width + x] = minVal;
    }
}

/**
 * @brief SEDM Phase 1: Horizontal distance pass
 */
__global__ void sedmHorizontalKernel(
    const uint8_t* __restrict__ src,
    uint32_t* __restrict__ hDist,
    int width,
    int height)
{
    int y = blockIdx.x * blockDim.x + threadIdx.x;

    if (y < height) {
        const uint8_t* row = src + y * width;
        uint32_t* distRow = hDist + y * width;

        // Forward pass
        distRow[0] = (row[0] == 0) ? 0 : width;
        for (int x = 1; x < width; x++) {
            if (row[x] == 0) {
                distRow[x] = 0;
            } else {
                distRow[x] = distRow[x - 1] + 1;
            }
        }

        // Backward pass
        for (int x = width - 2; x >= 0; x--) {
            if (distRow[x + 1] + 1 < distRow[x]) {
                distRow[x] = distRow[x + 1] + 1;
            }
        }
    }
}

/**
 * @brief SEDM Phase 2: Vertical pass with parabola envelope
 */
__global__ void sedmVerticalKernel(
    const uint32_t* __restrict__ hDist,
    uint32_t* __restrict__ dst,
    int width,
    int height)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;

    if (x < width) {
        // Allocate stack in local memory
        int s[1024];  // Parabola indices
        int t[1024];  // Boundary positions

        int q = 0;
        s[0] = 0;
        t[0] = 0;

        // Forward scan - build lower envelope
        for (int y = 1; y < height; y++) {
            uint32_t f_y = hDist[y * width + x];
            f_y = f_y * f_y;  // Square the horizontal distance

            while (q >= 0) {
                uint32_t f_s = hDist[s[q] * width + x];
                f_s = f_s * f_s;

                // Check if y's parabola dominates s[q]'s parabola
                int sep = ((f_y + y * y) - (f_s + s[q] * s[q])) / (2 * (y - s[q]));
                if (sep > t[q]) {
                    break;
                }
                q--;
            }

            q++;
            s[q] = y;
            if (q > 0) {
                uint32_t f_s = hDist[s[q - 1] * width + x];
                f_s = f_s * f_s;
                t[q] = ((f_y + y * y) - (f_s + s[q - 1] * s[q - 1])) / (2 * (y - s[q - 1])) + 1;
            } else {
                t[q] = 0;
            }
        }

        // Backward scan - evaluate distance transform
        for (int y = height - 1; y >= 0; y--) {
            while (q > 0 && t[q] > y) {
                q--;
            }

            int dy = y - s[q];
            uint32_t f_s = hDist[s[q] * width + x];
            dst[y * width + x] = f_s * f_s + dy * dy;
        }
    }
}

/**
 * @brief Image dewarping kernel with bilinear interpolation
 */
__global__ void dewarpKernel(
    const uint8_t* __restrict__ src,
    uint8_t* __restrict__ dst,
    int srcWidth,
    int srcHeight,
    int dstWidth,
    int dstHeight,
    const float* __restrict__ mapX,
    const float* __restrict__ mapY,
    int bytesPerPixel)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x < dstWidth && y < dstHeight) {
        int mapIdx = y * dstWidth + x;
        float srcX = mapX[mapIdx];
        float srcY = mapY[mapIdx];

        // Check bounds
        if (srcX < 0 || srcX >= srcWidth - 1 || srcY < 0 || srcY >= srcHeight - 1) {
            // Fill with white (background)
            for (int c = 0; c < bytesPerPixel; c++) {
                dst[(y * dstWidth + x) * bytesPerPixel + c] = 255;
            }
            return;
        }

        // Bilinear interpolation
        int x0 = static_cast<int>(srcX);
        int y0 = static_cast<int>(srcY);
        int x1 = x0 + 1;
        int y1 = y0 + 1;

        float fx = srcX - x0;
        float fy = srcY - y0;

        float w00 = (1.0f - fx) * (1.0f - fy);
        float w01 = (1.0f - fx) * fy;
        float w10 = fx * (1.0f - fy);
        float w11 = fx * fy;

        for (int c = 0; c < bytesPerPixel; c++) {
            float val = w00 * src[(y0 * srcWidth + x0) * bytesPerPixel + c]
                      + w01 * src[(y1 * srcWidth + x0) * bytesPerPixel + c]
                      + w10 * src[(y0 * srcWidth + x1) * bytesPerPixel + c]
                      + w11 * src[(y1 * srcWidth + x1) * bytesPerPixel + c];
            dst[(y * dstWidth + x) * bytesPerPixel + c] = static_cast<uint8_t>(val + 0.5f);
        }
    }
}

/**
 * @brief Compute histogram for Otsu thresholding
 */
__global__ void histogramKernel(
    const uint8_t* __restrict__ src,
    unsigned int* __restrict__ hist,
    int width,
    int height)
{
    __shared__ unsigned int localHist[256];

    // Initialize shared histogram
    if (threadIdx.x < 256) {
        localHist[threadIdx.x] = 0;
    }
    __syncthreads();

    // Compute local histogram
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x;
    int totalPixels = width * height;

    for (int i = idx; i < totalPixels; i += stride) {
        atomicAdd(&localHist[src[i]], 1);
    }
    __syncthreads();

    // Merge to global histogram
    if (threadIdx.x < 256) {
        atomicAdd(&hist[threadIdx.x], localHist[threadIdx.x]);
    }
}

/**
 * @brief Apply binary threshold
 */
__global__ void thresholdKernel(
    const uint8_t* __restrict__ src,
    uint8_t* __restrict__ dst,
    int width,
    int height,
    uint8_t threshold)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x < width && y < height) {
        int idx = y * width + x;
        dst[idx] = (src[idx] > threshold) ? 255 : 0;
    }
}

/**
 * @brief Compute integral image for Sauvola binarization (horizontal pass)
 */
__global__ void integralImageHorizontalKernel(
    const uint8_t* __restrict__ src,
    uint64_t* __restrict__ sumRow,
    uint64_t* __restrict__ sqSumRow,
    int width,
    int height)
{
    int y = blockIdx.x * blockDim.x + threadIdx.x;

    if (y < height) {
        uint64_t sum = 0;
        uint64_t sqSum = 0;

        for (int x = 0; x < width; x++) {
            uint8_t val = src[y * width + x];
            sum += val;
            sqSum += val * val;
            sumRow[y * width + x] = sum;
            sqSumRow[y * width + x] = sqSum;
        }
    }
}

/**
 * @brief Complete integral image (vertical accumulation)
 */
__global__ void integralImageVerticalKernel(
    uint64_t* __restrict__ sum,
    uint64_t* __restrict__ sqSum,
    int width,
    int height)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;

    if (x < width) {
        for (int y = 1; y < height; y++) {
            sum[y * width + x] += sum[(y - 1) * width + x];
            sqSum[y * width + x] += sqSum[(y - 1) * width + x];
        }
    }
}

/**
 * @brief Sauvola binarization using integral images
 */
__global__ void sauvolaKernel(
    const uint8_t* __restrict__ src,
    uint8_t* __restrict__ dst,
    const uint64_t* __restrict__ sum,
    const uint64_t* __restrict__ sqSum,
    int width,
    int height,
    int windowRadius,
    float k)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x < width && y < height) {
        // Window bounds (clamped to image)
        int x0 = max(0, x - windowRadius) - 1;
        int y0 = max(0, y - windowRadius) - 1;
        int x1 = min(width - 1, x + windowRadius);
        int y1 = min(height - 1, y + windowRadius);

        // Compute window size
        int windowWidth = (x1 - max(0, x - windowRadius) + 1);
        int windowHeight = (y1 - max(0, y - windowRadius) + 1);
        float area = windowWidth * windowHeight;

        // Get sums from integral image
        uint64_t s = sum[y1 * width + x1];
        uint64_t sq = sqSum[y1 * width + x1];

        if (x0 >= 0) {
            s -= sum[y1 * width + x0];
            sq -= sqSum[y1 * width + x0];
        }
        if (y0 >= 0) {
            s -= sum[y0 * width + x1];
            sq -= sqSum[y0 * width + x1];
        }
        if (x0 >= 0 && y0 >= 0) {
            s += sum[y0 * width + x0];
            sq += sqSum[y0 * width + x0];
        }

        // Compute mean and standard deviation
        float mean = s / area;
        float variance = (sq / area) - (mean * mean);
        float stdDev = sqrtf(max(0.0f, variance));

        // Sauvola threshold
        float threshold = mean * (1.0f + k * (stdDev / 128.0f - 1.0f));

        // Apply threshold
        dst[y * width + x] = (src[y * width + x] > threshold) ? 255 : 0;
    }
}

/**
 * @brief Image inversion kernel
 */
__global__ void invertKernel(
    uint8_t* __restrict__ data,
    int size)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx < size) {
        data[idx] = 255 - data[idx];
    }
}

/**
 * @brief Count black pixels reduction kernel
 */
__global__ void countBlackPixelsKernel(
    const uint8_t* __restrict__ data,
    unsigned long long* __restrict__ count,
    int size)
{
    __shared__ unsigned long long localCount;

    if (threadIdx.x == 0) {
        localCount = 0;
    }
    __syncthreads();

    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x;

    unsigned long long myCount = 0;
    for (int i = idx; i < size; i += stride) {
        if (data[i] == 0) {
            myCount++;
        }
    }

    atomicAdd(&localCount, myCount);
    __syncthreads();

    if (threadIdx.x == 0) {
        atomicAdd(count, localCount);
    }
}

// ============================================================================
// API Implementation
// ============================================================================

bool initCUDA()
{
    if (g_cudaInitialized) {
        return true;
    }

    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);

    if (err != cudaSuccess || deviceCount == 0) {
        snprintf(g_lastError, sizeof(g_lastError),
                 "No CUDA devices found: %s", cudaGetErrorString(err));
        g_deviceInfo.available = false;
        return false;
    }

    // Select device 0 by default
    CUDA_CHECK(cudaSetDevice(0));

    // Get device properties
    cudaDeviceProp prop;
    CUDA_CHECK(cudaGetDeviceProperties(&prop, 0));

    g_deviceInfo.available = true;
    g_deviceInfo.deviceCount = deviceCount;
    g_deviceInfo.selectedDevice = 0;
    strncpy(g_deviceInfo.deviceName, prop.name, sizeof(g_deviceInfo.deviceName) - 1);
    g_deviceInfo.major = prop.major;
    g_deviceInfo.minor = prop.minor;
    g_deviceInfo.totalMemory = prop.totalGlobalMem;
    g_deviceInfo.multiprocessorCount = prop.multiProcessorCount;
    g_deviceInfo.maxThreadsPerBlock = prop.maxThreadsPerBlock;
    g_deviceInfo.maxBlockDimX = prop.maxThreadsDim[0];
    g_deviceInfo.maxBlockDimY = prop.maxThreadsDim[1];
    g_deviceInfo.warpSize = prop.warpSize;

    // Get free memory
    size_t freeMem, totalMem;
    CUDA_CHECK(cudaMemGetInfo(&freeMem, &totalMem));
    g_deviceInfo.freeMemory = freeMem;

    g_cudaInitialized = true;
    g_lastError[0] = '\0';

    return true;
}

void shutdownCUDA()
{
    if (g_cudaInitialized) {
        cudaDeviceReset();
        g_cudaInitialized = false;
        g_deviceInfo.available = false;
    }
}

bool isCUDAAvailable()
{
    return g_cudaInitialized && g_deviceInfo.available;
}

CUDADeviceInfo getCUDADeviceInfo()
{
    return g_deviceInfo;
}

bool setCUDADevice(int deviceId)
{
    if (!g_cudaInitialized) {
        return false;
    }

    if (deviceId >= g_deviceInfo.deviceCount) {
        snprintf(g_lastError, sizeof(g_lastError),
                 "Invalid device ID: %d (have %d devices)",
                 deviceId, g_deviceInfo.deviceCount);
        return false;
    }

    CUDA_CHECK(cudaSetDevice(deviceId));
    g_deviceInfo.selectedDevice = deviceId;

    return true;
}

const char* getLastCUDAError()
{
    return g_lastError;
}

// ============================================================================
// Image Processing Functions
// ============================================================================

bool gpuRgbToGrayscale(
    const uint8_t* src,
    uint8_t* dst,
    int width,
    int height,
    int srcBytesPerPixel)
{
    if (!isCUDAAvailable()) {
        snprintf(g_lastError, sizeof(g_lastError), "CUDA not available");
        return false;
    }

    size_t srcSize = width * height * srcBytesPerPixel;
    size_t dstSize = width * height;

    uint8_t* d_src = nullptr;
    uint8_t* d_dst = nullptr;

    CUDA_CHECK(cudaMalloc(&d_src, srcSize));
    CUDA_CHECK(cudaMalloc(&d_dst, dstSize));
    CUDA_CHECK(cudaMemcpy(d_src, src, srcSize, cudaMemcpyHostToDevice));

    dim3 blockDim(BLOCK_DIM_X, BLOCK_DIM_Y);
    dim3 gridDim((width + BLOCK_DIM_X - 1) / BLOCK_DIM_X,
                 (height + BLOCK_DIM_Y - 1) / BLOCK_DIM_Y);

    grayscaleKernel<<<gridDim, blockDim>>>(d_src, d_dst, width, height, srcBytesPerPixel);

    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaMemcpy(dst, d_dst, dstSize, cudaMemcpyDeviceToHost));

    cudaFree(d_src);
    cudaFree(d_dst);

    return true;
}

bool gpuDilate(
    const uint8_t* src,
    uint8_t* dst,
    int width,
    int height,
    int kernelWidth,
    int kernelHeight)
{
    if (!isCUDAAvailable()) {
        snprintf(g_lastError, sizeof(g_lastError), "CUDA not available");
        return false;
    }

    int kernelRadiusX = kernelWidth / 2;
    int kernelRadiusY = kernelHeight / 2;
    size_t imageSize = width * height;

    uint8_t* d_src = nullptr;
    uint8_t* d_dst = nullptr;

    CUDA_CHECK(cudaMalloc(&d_src, imageSize));
    CUDA_CHECK(cudaMalloc(&d_dst, imageSize));
    CUDA_CHECK(cudaMemcpy(d_src, src, imageSize, cudaMemcpyHostToDevice));

    dim3 blockDim(BLOCK_DIM_X, BLOCK_DIM_Y);
    dim3 gridDim((width + BLOCK_DIM_X - 1) / BLOCK_DIM_X,
                 (height + BLOCK_DIM_Y - 1) / BLOCK_DIM_Y);

    // Shared memory size for tile with halo
    int tileWidth = BLOCK_DIM_X + 2 * kernelRadiusX;
    int tileHeight = BLOCK_DIM_Y + 2 * kernelRadiusY;
    size_t sharedMemSize = tileWidth * tileHeight * sizeof(uint8_t);

    dilationKernel<<<gridDim, blockDim, sharedMemSize>>>(
        d_src, d_dst, width, height, kernelRadiusX, kernelRadiusY);

    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaMemcpy(dst, d_dst, imageSize, cudaMemcpyDeviceToHost));

    cudaFree(d_src);
    cudaFree(d_dst);

    return true;
}

bool gpuErode(
    const uint8_t* src,
    uint8_t* dst,
    int width,
    int height,
    int kernelWidth,
    int kernelHeight)
{
    if (!isCUDAAvailable()) {
        snprintf(g_lastError, sizeof(g_lastError), "CUDA not available");
        return false;
    }

    int kernelRadiusX = kernelWidth / 2;
    int kernelRadiusY = kernelHeight / 2;
    size_t imageSize = width * height;

    uint8_t* d_src = nullptr;
    uint8_t* d_dst = nullptr;

    CUDA_CHECK(cudaMalloc(&d_src, imageSize));
    CUDA_CHECK(cudaMalloc(&d_dst, imageSize));
    CUDA_CHECK(cudaMemcpy(d_src, src, imageSize, cudaMemcpyHostToDevice));

    dim3 blockDim(BLOCK_DIM_X, BLOCK_DIM_Y);
    dim3 gridDim((width + BLOCK_DIM_X - 1) / BLOCK_DIM_X,
                 (height + BLOCK_DIM_Y - 1) / BLOCK_DIM_Y);

    int tileWidth = BLOCK_DIM_X + 2 * kernelRadiusX;
    int tileHeight = BLOCK_DIM_Y + 2 * kernelRadiusY;
    size_t sharedMemSize = tileWidth * tileHeight * sizeof(uint8_t);

    erosionKernel<<<gridDim, blockDim, sharedMemSize>>>(
        d_src, d_dst, width, height, kernelRadiusX, kernelRadiusY);

    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaMemcpy(dst, d_dst, imageSize, cudaMemcpyDeviceToHost));

    cudaFree(d_src);
    cudaFree(d_dst);

    return true;
}

bool gpuSEDM(
    const uint8_t* src,
    uint32_t* dst,
    int width,
    int height)
{
    if (!isCUDAAvailable()) {
        snprintf(g_lastError, sizeof(g_lastError), "CUDA not available");
        return false;
    }

    size_t srcSize = width * height;
    size_t dstSize = width * height * sizeof(uint32_t);

    uint8_t* d_src = nullptr;
    uint32_t* d_hDist = nullptr;
    uint32_t* d_dst = nullptr;

    CUDA_CHECK(cudaMalloc(&d_src, srcSize));
    CUDA_CHECK(cudaMalloc(&d_hDist, dstSize));
    CUDA_CHECK(cudaMalloc(&d_dst, dstSize));
    CUDA_CHECK(cudaMemcpy(d_src, src, srcSize, cudaMemcpyHostToDevice));

    // Phase 1: Horizontal pass (one thread per row)
    int blockSize = 256;
    int gridSize = (height + blockSize - 1) / blockSize;
    sedmHorizontalKernel<<<gridSize, blockSize>>>(d_src, d_hDist, width, height);
    CUDA_CHECK(cudaGetLastError());

    // Phase 2: Vertical pass (one thread per column)
    gridSize = (width + blockSize - 1) / blockSize;
    sedmVerticalKernel<<<gridSize, blockSize>>>(d_hDist, d_dst, width, height);
    CUDA_CHECK(cudaGetLastError());

    CUDA_CHECK(cudaMemcpy(dst, d_dst, dstSize, cudaMemcpyDeviceToHost));

    cudaFree(d_src);
    cudaFree(d_hDist);
    cudaFree(d_dst);

    return true;
}

bool gpuDewarp(
    const uint8_t* src,
    uint8_t* dst,
    int srcWidth,
    int srcHeight,
    int dstWidth,
    int dstHeight,
    const float* mapX,
    const float* mapY,
    int bytesPerPixel)
{
    if (!isCUDAAvailable()) {
        snprintf(g_lastError, sizeof(g_lastError), "CUDA not available");
        return false;
    }

    size_t srcSize = srcWidth * srcHeight * bytesPerPixel;
    size_t dstSize = dstWidth * dstHeight * bytesPerPixel;
    size_t mapSize = dstWidth * dstHeight * sizeof(float);

    uint8_t* d_src = nullptr;
    uint8_t* d_dst = nullptr;
    float* d_mapX = nullptr;
    float* d_mapY = nullptr;

    CUDA_CHECK(cudaMalloc(&d_src, srcSize));
    CUDA_CHECK(cudaMalloc(&d_dst, dstSize));
    CUDA_CHECK(cudaMalloc(&d_mapX, mapSize));
    CUDA_CHECK(cudaMalloc(&d_mapY, mapSize));

    CUDA_CHECK(cudaMemcpy(d_src, src, srcSize, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_mapX, mapX, mapSize, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_mapY, mapY, mapSize, cudaMemcpyHostToDevice));

    dim3 blockDim(BLOCK_DIM_X, BLOCK_DIM_Y);
    dim3 gridDim((dstWidth + BLOCK_DIM_X - 1) / BLOCK_DIM_X,
                 (dstHeight + BLOCK_DIM_Y - 1) / BLOCK_DIM_Y);

    dewarpKernel<<<gridDim, blockDim>>>(
        d_src, d_dst, srcWidth, srcHeight, dstWidth, dstHeight,
        d_mapX, d_mapY, bytesPerPixel);

    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaMemcpy(dst, d_dst, dstSize, cudaMemcpyDeviceToHost));

    cudaFree(d_src);
    cudaFree(d_dst);
    cudaFree(d_mapX);
    cudaFree(d_mapY);

    return true;
}

// Helper function to compute Otsu threshold on CPU (histogram computed on GPU)
static int computeOtsuThreshold(const unsigned int* hist, int totalPixels)
{
    float sum = 0;
    for (int i = 0; i < 256; i++) {
        sum += i * hist[i];
    }

    float sumB = 0;
    int wB = 0;
    int wF = 0;

    float maxVariance = 0;
    int threshold = 0;

    for (int t = 0; t < 256; t++) {
        wB += hist[t];
        if (wB == 0) continue;

        wF = totalPixels - wB;
        if (wF == 0) break;

        sumB += t * hist[t];

        float mB = sumB / wB;
        float mF = (sum - sumB) / wF;

        float variance = wB * wF * (mB - mF) * (mB - mF);

        if (variance > maxVariance) {
            maxVariance = variance;
            threshold = t;
        }
    }

    return threshold;
}

bool gpuBinarizeOtsu(
    const uint8_t* src,
    uint8_t* dst,
    int width,
    int height,
    int* thresholdOut)
{
    if (!isCUDAAvailable()) {
        snprintf(g_lastError, sizeof(g_lastError), "CUDA not available");
        return false;
    }

    size_t imageSize = width * height;
    int totalPixels = width * height;

    uint8_t* d_src = nullptr;
    uint8_t* d_dst = nullptr;
    unsigned int* d_hist = nullptr;
    unsigned int hist[256] = {0};

    CUDA_CHECK(cudaMalloc(&d_src, imageSize));
    CUDA_CHECK(cudaMalloc(&d_dst, imageSize));
    CUDA_CHECK(cudaMalloc(&d_hist, 256 * sizeof(unsigned int)));
    CUDA_CHECK(cudaMemset(d_hist, 0, 256 * sizeof(unsigned int)));
    CUDA_CHECK(cudaMemcpy(d_src, src, imageSize, cudaMemcpyHostToDevice));

    // Compute histogram on GPU
    int blockSize = 256;
    int gridSize = (totalPixels + blockSize - 1) / blockSize;
    gridSize = std::min(gridSize, 256);  // Limit grid size

    histogramKernel<<<gridSize, blockSize>>>(d_src, d_hist, width, height);
    CUDA_CHECK(cudaGetLastError());

    CUDA_CHECK(cudaMemcpy(hist, d_hist, 256 * sizeof(unsigned int), cudaMemcpyDeviceToHost));

    // Compute Otsu threshold on CPU
    int threshold = computeOtsuThreshold(hist, totalPixels);
    if (thresholdOut) {
        *thresholdOut = threshold;
    }

    // Apply threshold on GPU
    dim3 blockDim2D(BLOCK_DIM_X, BLOCK_DIM_Y);
    dim3 gridDim2D((width + BLOCK_DIM_X - 1) / BLOCK_DIM_X,
                   (height + BLOCK_DIM_Y - 1) / BLOCK_DIM_Y);

    thresholdKernel<<<gridDim2D, blockDim2D>>>(
        d_src, d_dst, width, height, static_cast<uint8_t>(threshold));

    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaMemcpy(dst, d_dst, imageSize, cudaMemcpyDeviceToHost));

    cudaFree(d_src);
    cudaFree(d_dst);
    cudaFree(d_hist);

    return true;
}

bool gpuBinarizeSauvola(
    const uint8_t* src,
    uint8_t* dst,
    int width,
    int height,
    int windowSize,
    float k)
{
    if (!isCUDAAvailable()) {
        snprintf(g_lastError, sizeof(g_lastError), "CUDA not available");
        return false;
    }

    size_t imageSize = width * height;
    size_t integralSize = width * height * sizeof(uint64_t);
    int windowRadius = windowSize / 2;

    uint8_t* d_src = nullptr;
    uint8_t* d_dst = nullptr;
    uint64_t* d_sum = nullptr;
    uint64_t* d_sqSum = nullptr;

    CUDA_CHECK(cudaMalloc(&d_src, imageSize));
    CUDA_CHECK(cudaMalloc(&d_dst, imageSize));
    CUDA_CHECK(cudaMalloc(&d_sum, integralSize));
    CUDA_CHECK(cudaMalloc(&d_sqSum, integralSize));
    CUDA_CHECK(cudaMemcpy(d_src, src, imageSize, cudaMemcpyHostToDevice));

    int blockSize = 256;

    // Compute integral image (horizontal pass)
    int gridSizeH = (height + blockSize - 1) / blockSize;
    integralImageHorizontalKernel<<<gridSizeH, blockSize>>>(
        d_src, d_sum, d_sqSum, width, height);
    CUDA_CHECK(cudaGetLastError());

    // Compute integral image (vertical pass)
    int gridSizeV = (width + blockSize - 1) / blockSize;
    integralImageVerticalKernel<<<gridSizeV, blockSize>>>(d_sum, d_sqSum, width, height);
    CUDA_CHECK(cudaGetLastError());

    // Apply Sauvola binarization
    dim3 blockDim(BLOCK_DIM_X, BLOCK_DIM_Y);
    dim3 gridDim((width + BLOCK_DIM_X - 1) / BLOCK_DIM_X,
                 (height + BLOCK_DIM_Y - 1) / BLOCK_DIM_Y);

    sauvolaKernel<<<gridDim, blockDim>>>(
        d_src, d_dst, d_sum, d_sqSum, width, height, windowRadius, k);

    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaMemcpy(dst, d_dst, imageSize, cudaMemcpyDeviceToHost));

    cudaFree(d_src);
    cudaFree(d_dst);
    cudaFree(d_sum);
    cudaFree(d_sqSum);

    return true;
}

bool gpuInvert(uint8_t* data, int width, int height)
{
    if (!isCUDAAvailable()) {
        snprintf(g_lastError, sizeof(g_lastError), "CUDA not available");
        return false;
    }

    size_t size = width * height;

    uint8_t* d_data = nullptr;
    CUDA_CHECK(cudaMalloc(&d_data, size));
    CUDA_CHECK(cudaMemcpy(d_data, data, size, cudaMemcpyHostToDevice));

    int blockSize = 256;
    int gridSize = (size + blockSize - 1) / blockSize;

    invertKernel<<<gridSize, blockSize>>>(d_data, size);

    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaMemcpy(data, d_data, size, cudaMemcpyDeviceToHost));

    cudaFree(d_data);

    return true;
}

int64_t gpuCountBlackPixels(const uint8_t* data, int width, int height)
{
    if (!isCUDAAvailable()) {
        snprintf(g_lastError, sizeof(g_lastError), "CUDA not available");
        return -1;
    }

    size_t size = width * height;

    uint8_t* d_data = nullptr;
    unsigned long long* d_count = nullptr;
    unsigned long long count = 0;

    cudaError_t err;

    err = cudaMalloc(&d_data, size);
    if (err != cudaSuccess) {
        snprintf(g_lastError, sizeof(g_lastError), "cudaMalloc failed: %s", cudaGetErrorString(err));
        return -1;
    }

    err = cudaMalloc(&d_count, sizeof(unsigned long long));
    if (err != cudaSuccess) {
        cudaFree(d_data);
        snprintf(g_lastError, sizeof(g_lastError), "cudaMalloc failed: %s", cudaGetErrorString(err));
        return -1;
    }

    err = cudaMemcpy(d_data, data, size, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        cudaFree(d_data);
        cudaFree(d_count);
        snprintf(g_lastError, sizeof(g_lastError), "cudaMemcpy failed: %s", cudaGetErrorString(err));
        return -1;
    }

    err = cudaMemset(d_count, 0, sizeof(unsigned long long));
    if (err != cudaSuccess) {
        cudaFree(d_data);
        cudaFree(d_count);
        snprintf(g_lastError, sizeof(g_lastError), "cudaMemset failed: %s", cudaGetErrorString(err));
        return -1;
    }

    int blockSize = 256;
    int gridSize = std::min((int)((size + blockSize - 1) / blockSize), 256);

    countBlackPixelsKernel<<<gridSize, blockSize>>>(d_data, d_count, size);

    err = cudaGetLastError();
    if (err != cudaSuccess) {
        cudaFree(d_data);
        cudaFree(d_count);
        snprintf(g_lastError, sizeof(g_lastError), "Kernel launch failed: %s", cudaGetErrorString(err));
        return -1;
    }

    err = cudaMemcpy(&count, d_count, sizeof(unsigned long long), cudaMemcpyDeviceToHost);
    if (err != cudaSuccess) {
        cudaFree(d_data);
        cudaFree(d_count);
        snprintf(g_lastError, sizeof(g_lastError), "cudaMemcpy failed: %s", cudaGetErrorString(err));
        return -1;
    }

    cudaFree(d_data);
    cudaFree(d_count);

    return static_cast<int64_t>(count);
}

// ============================================================================
// Memory Management
// ============================================================================

void* gpuMalloc(size_t size)
{
    if (!isCUDAAvailable()) {
        return nullptr;
    }

    void* ptr = nullptr;
    cudaError_t err = cudaMalloc(&ptr, size);
    if (err != cudaSuccess) {
        snprintf(g_lastError, sizeof(g_lastError),
                 "cudaMalloc failed: %s", cudaGetErrorString(err));
        return nullptr;
    }
    return ptr;
}

void gpuFree(void* ptr)
{
    if (ptr) {
        cudaFree(ptr);
    }
}

bool gpuMemcpyHostToDevice(void* dst, const void* src, size_t size)
{
    if (!isCUDAAvailable()) {
        return false;
    }
    CUDA_CHECK(cudaMemcpy(dst, src, size, cudaMemcpyHostToDevice));
    return true;
}

bool gpuMemcpyDeviceToHost(void* dst, const void* src, size_t size)
{
    if (!isCUDAAvailable()) {
        return false;
    }
    CUDA_CHECK(cudaMemcpy(dst, src, size, cudaMemcpyDeviceToHost));
    return true;
}

bool gpuSynchronize()
{
    if (!isCUDAAvailable()) {
        return false;
    }
    CUDA_CHECK(cudaDeviceSynchronize());
    return true;
}

} // namespace gpu
} // namespace imageproc

#else // CUDA_AVAILABLE not defined

// Stub implementations when CUDA is not available
namespace imageproc {
namespace gpu {

static char g_lastError[512] = "CUDA support not compiled";

bool initCUDA() { return false; }
void shutdownCUDA() {}
bool isCUDAAvailable() { return false; }

CUDADeviceInfo getCUDADeviceInfo()
{
    CUDADeviceInfo info = {};
    info.available = false;
    return info;
}

bool setCUDADevice(int) { return false; }
const char* getLastCUDAError() { return g_lastError; }

bool gpuRgbToGrayscale(const uint8_t*, uint8_t*, int, int, int) { return false; }
bool gpuDilate(const uint8_t*, uint8_t*, int, int, int, int) { return false; }
bool gpuErode(const uint8_t*, uint8_t*, int, int, int, int) { return false; }
bool gpuSEDM(const uint8_t*, uint32_t*, int, int) { return false; }
bool gpuDewarp(const uint8_t*, uint8_t*, int, int, int, int, const float*, const float*, int) { return false; }
bool gpuBinarizeOtsu(const uint8_t*, uint8_t*, int, int, int*) { return false; }
bool gpuBinarizeSauvola(const uint8_t*, uint8_t*, int, int, int, float) { return false; }
bool gpuInvert(uint8_t*, int, int) { return false; }
int64_t gpuCountBlackPixels(const uint8_t*, int, int) { return -1; }

void* gpuMalloc(size_t) { return nullptr; }
void gpuFree(void*) {}
bool gpuMemcpyHostToDevice(void*, const void*, size_t) { return false; }
bool gpuMemcpyDeviceToHost(void*, const void*, size_t) { return false; }
bool gpuSynchronize() { return false; }

} // namespace gpu
} // namespace imageproc

#endif // CUDA_AVAILABLE
