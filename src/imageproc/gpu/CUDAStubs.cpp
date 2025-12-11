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

/**
 * @file CUDAStubs.cpp
 * @brief Stub implementations for CUDA functions when CUDA is not available
 *
 * This file provides fallback implementations that return failure status
 * when CUDA support is not compiled in.
 */

#ifndef CUDA_AVAILABLE

#include "CUDAUtils.h"

namespace imageproc {
namespace gpu {

static char g_lastError[512] = "CUDA support not compiled";

bool initCUDA()
{
    return false;
}

void shutdownCUDA()
{
}

bool isCUDAAvailable()
{
    return false;
}

CUDADeviceInfo getCUDADeviceInfo()
{
    CUDADeviceInfo info = {};
    info.available = false;
    info.deviceCount = 0;
    info.selectedDevice = -1;
    info.deviceName[0] = '\0';
    info.major = 0;
    info.minor = 0;
    info.totalMemory = 0;
    info.freeMemory = 0;
    info.multiprocessorCount = 0;
    info.maxThreadsPerBlock = 0;
    info.maxBlockDimX = 0;
    info.maxBlockDimY = 0;
    info.warpSize = 0;
    return info;
}

bool setCUDADevice(int /*deviceId*/)
{
    return false;
}

const char* getLastCUDAError()
{
    return g_lastError;
}

bool gpuRgbToGrayscale(
    const uint8_t* /*src*/,
    uint8_t* /*dst*/,
    int /*width*/,
    int /*height*/,
    int /*srcBytesPerPixel*/)
{
    return false;
}

bool gpuDilate(
    const uint8_t* /*src*/,
    uint8_t* /*dst*/,
    int /*width*/,
    int /*height*/,
    int /*kernelWidth*/,
    int /*kernelHeight*/)
{
    return false;
}

bool gpuErode(
    const uint8_t* /*src*/,
    uint8_t* /*dst*/,
    int /*width*/,
    int /*height*/,
    int /*kernelWidth*/,
    int /*kernelHeight*/)
{
    return false;
}

bool gpuSEDM(
    const uint8_t* /*src*/,
    uint32_t* /*dst*/,
    int /*width*/,
    int /*height*/)
{
    return false;
}

bool gpuDewarp(
    const uint8_t* /*src*/,
    uint8_t* /*dst*/,
    int /*srcWidth*/,
    int /*srcHeight*/,
    int /*dstWidth*/,
    int /*dstHeight*/,
    const float* /*mapX*/,
    const float* /*mapY*/,
    int /*bytesPerPixel*/)
{
    return false;
}

bool gpuBinarizeOtsu(
    const uint8_t* /*src*/,
    uint8_t* /*dst*/,
    int /*width*/,
    int /*height*/,
    int* /*threshold*/)
{
    return false;
}

bool gpuBinarizeSauvola(
    const uint8_t* /*src*/,
    uint8_t* /*dst*/,
    int /*width*/,
    int /*height*/,
    int /*windowSize*/,
    float /*k*/)
{
    return false;
}

bool gpuInvert(
    uint8_t* /*data*/,
    int /*width*/,
    int /*height*/)
{
    return false;
}

int64_t gpuCountBlackPixels(
    const uint8_t* /*data*/,
    int /*width*/,
    int /*height*/)
{
    return -1;
}

void* gpuMalloc(size_t /*size*/)
{
    return nullptr;
}

void gpuFree(void* /*ptr*/)
{
}

bool gpuMemcpyHostToDevice(void* /*dst*/, const void* /*src*/, size_t /*size*/)
{
    return false;
}

bool gpuMemcpyDeviceToHost(void* /*dst*/, const void* /*src*/, size_t /*size*/)
{
    return false;
}

bool gpuSynchronize()
{
    return false;
}

} // namespace gpu
} // namespace imageproc

#endif // !CUDA_AVAILABLE
