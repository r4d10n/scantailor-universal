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

#include "PerformanceStats.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>

PerformanceStats& PerformanceStats::instance()
{
    static PerformanceStats instance;
    return instance;
}

PerformanceStats::PerformanceStats()
    : m_enabled(false)
    , m_pageCount(0)
    , m_sessionStart(std::chrono::high_resolution_clock::now())
{
}

void PerformanceStats::setEnabled(bool enabled)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_enabled = enabled;
    if (enabled) {
        m_sessionStart = std::chrono::high_resolution_clock::now();
    }
}

void PerformanceStats::reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats.clear();
    m_timers.clear();
    m_pageCount = 0;
    m_sessionStart = std::chrono::high_resolution_clock::now();
}

void PerformanceStats::startTimer(const std::string& operation, Backend backend)
{
    if (!m_enabled) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    StatsKey key{operation, backend};
    TimerState& timer = m_timers[key];
    timer.startTime = std::chrono::high_resolution_clock::now();
    timer.active = true;
}

void PerformanceStats::stopTimer(const std::string& operation, Backend backend, uint64_t pixelsProcessed)
{
    if (!m_enabled) return;

    auto endTime = std::chrono::high_resolution_clock::now();

    std::lock_guard<std::mutex> lock(m_mutex);

    StatsKey key{operation, backend};
    auto timerIt = m_timers.find(key);

    if (timerIt == m_timers.end() || !timerIt->second.active) {
        return; // Timer was not started
    }

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - timerIt->second.startTime
    );

    int64_t timeUs = duration.count();
    timerIt->second.active = false;

    // Update statistics
    OpStats& stats = m_stats[key];
    stats.totalTimeUs += timeUs;
    stats.minTimeUs = std::min(stats.minTimeUs, timeUs);
    stats.maxTimeUs = std::max(stats.maxTimeUs, timeUs);
    stats.callCount++;
    stats.totalPixels += pixelsProcessed;
}

void PerformanceStats::recordTime(const std::string& operation, Backend backend,
                                   int64_t timeUs, uint64_t pixelsProcessed)
{
    if (!m_enabled) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    StatsKey key{operation, backend};
    OpStats& stats = m_stats[key];
    stats.totalTimeUs += timeUs;
    stats.minTimeUs = std::min(stats.minTimeUs, timeUs);
    stats.maxTimeUs = std::max(stats.maxTimeUs, timeUs);
    stats.callCount++;
    stats.totalPixels += pixelsProcessed;
}

void PerformanceStats::incrementPageCount()
{
    if (!m_enabled) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pageCount++;
}

PerformanceStats::OpStats PerformanceStats::getStats(const std::string& operation, Backend backend) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    StatsKey key{operation, backend};
    auto it = m_stats.find(key);
    if (it != m_stats.end()) {
        return it->second;
    }
    return OpStats{};
}

std::vector<std::string> PerformanceStats::getOperationNames() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<std::string> names;
    std::string lastOp;

    for (const auto& pair : m_stats) {
        if (pair.first.operation != lastOp) {
            names.push_back(pair.first.operation);
            lastOp = pair.first.operation;
        }
    }

    return names;
}

const char* PerformanceStats::backendToString(Backend backend)
{
    switch (backend) {
        case Backend::CPU:  return "CPU";
        case Backend::SIMD: return "SIMD";
        case Backend::GPU:  return "GPU";
        default:            return "Unknown";
    }
}

std::string PerformanceStats::getReportString() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::ostringstream ss;

    auto now = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_sessionStart);

    ss << "\n";
    ss << "================================================================================\n";
    ss << "                    PERFORMANCE STATISTICS REPORT                               \n";
    ss << "================================================================================\n";
    ss << "\n";
    ss << "Session Summary:\n";
    ss << "  Pages processed:    " << m_pageCount << "\n";
    ss << "  Total time:         " << std::fixed << std::setprecision(2)
       << (totalDuration.count() / 1000.0) << " sec\n";
    if (m_pageCount > 0) {
        ss << "  Avg time per page:  " << std::fixed << std::setprecision(1)
           << (totalDuration.count() / static_cast<double>(m_pageCount)) << " ms\n";
        ss << "  Pages per minute:   " << std::fixed << std::setprecision(1)
           << (m_pageCount * 60000.0 / totalDuration.count()) << "\n";
    }
    ss << "\n";

    // Collect operation names
    std::vector<std::string> operations;
    std::string lastOp;
    for (const auto& pair : m_stats) {
        if (pair.first.operation != lastOp) {
            operations.push_back(pair.first.operation);
            lastOp = pair.first.operation;
        }
    }

    if (operations.empty()) {
        ss << "  No operation statistics recorded.\n";
        ss << "\n";
        ss << "================================================================================\n";
        return ss.str();
    }

    // Print detailed statistics
    ss << "Detailed Operation Statistics:\n";
    ss << "--------------------------------------------------------------------------------\n";
    ss << std::left << std::setw(20) << "Operation"
       << std::setw(8) << "Backend"
       << std::right << std::setw(10) << "Calls"
       << std::setw(12) << "Total(ms)"
       << std::setw(12) << "Avg(ms)"
       << std::setw(12) << "Min(ms)"
       << std::setw(12) << "Max(ms)"
       << "\n";
    ss << "--------------------------------------------------------------------------------\n";

    for (const auto& op : operations) {
        bool firstBackend = true;
        for (int b = 0; b <= 2; b++) {
            Backend backend = static_cast<Backend>(b);
            StatsKey key{op, backend};
            auto it = m_stats.find(key);
            if (it != m_stats.end() && it->second.callCount > 0) {
                const OpStats& stats = it->second;

                ss << std::left << std::setw(20) << (firstBackend ? op : "")
                   << std::setw(8) << backendToString(backend)
                   << std::right << std::setw(10) << stats.callCount
                   << std::setw(12) << std::fixed << std::setprecision(2) << (stats.totalTimeUs / 1000.0)
                   << std::setw(12) << std::fixed << std::setprecision(2) << (stats.avgTimeUs() / 1000.0)
                   << std::setw(12) << std::fixed << std::setprecision(2) << (stats.minTimeUs / 1000.0)
                   << std::setw(12) << std::fixed << std::setprecision(2) << (stats.maxTimeUs / 1000.0)
                   << "\n";
                firstBackend = false;
            }
        }
    }

    ss << "\n";

    // Print speedup comparison
    ss << "Speedup Comparison (vs CPU baseline):\n";
    ss << "--------------------------------------------------------------------------------\n";
    ss << std::left << std::setw(20) << "Operation"
       << std::setw(15) << "SIMD Speedup"
       << std::setw(15) << "GPU Speedup"
       << std::setw(20) << "Best Backend"
       << "\n";
    ss << "--------------------------------------------------------------------------------\n";

    for (const auto& op : operations) {
        StatsKey cpuKey{op, Backend::CPU};
        StatsKey simdKey{op, Backend::SIMD};
        StatsKey gpuKey{op, Backend::GPU};

        auto cpuIt = m_stats.find(cpuKey);
        auto simdIt = m_stats.find(simdKey);
        auto gpuIt = m_stats.find(gpuKey);

        double cpuAvg = (cpuIt != m_stats.end() && cpuIt->second.callCount > 0)
                        ? cpuIt->second.avgTimeUs() : 0.0;
        double simdAvg = (simdIt != m_stats.end() && simdIt->second.callCount > 0)
                         ? simdIt->second.avgTimeUs() : 0.0;
        double gpuAvg = (gpuIt != m_stats.end() && gpuIt->second.callCount > 0)
                        ? gpuIt->second.avgTimeUs() : 0.0;

        ss << std::left << std::setw(20) << op;

        // SIMD speedup
        if (cpuAvg > 0 && simdAvg > 0) {
            ss << std::setw(15) << (std::to_string(static_cast<int>((cpuAvg / simdAvg) * 10) / 10.0).substr(0, 5) + "x");
        } else if (simdAvg > 0) {
            ss << std::setw(15) << "(no CPU ref)";
        } else {
            ss << std::setw(15) << "-";
        }

        // GPU speedup
        if (cpuAvg > 0 && gpuAvg > 0) {
            ss << std::setw(15) << (std::to_string(static_cast<int>((cpuAvg / gpuAvg) * 10) / 10.0).substr(0, 5) + "x");
        } else if (gpuAvg > 0) {
            ss << std::setw(15) << "(no CPU ref)";
        } else {
            ss << std::setw(15) << "-";
        }

        // Best backend
        double bestTime = 1e18;
        const char* bestBackend = "N/A";
        if (cpuAvg > 0 && cpuAvg < bestTime) { bestTime = cpuAvg; bestBackend = "CPU"; }
        if (simdAvg > 0 && simdAvg < bestTime) { bestTime = simdAvg; bestBackend = "SIMD"; }
        if (gpuAvg > 0 && gpuAvg < bestTime) { bestTime = gpuAvg; bestBackend = "GPU"; }

        ss << std::setw(20) << bestBackend << "\n";
    }

    ss << "\n";

    // Print throughput statistics if pixel counts available
    bool hasPixelData = false;
    for (const auto& pair : m_stats) {
        if (pair.second.totalPixels > 0) {
            hasPixelData = true;
            break;
        }
    }

    if (hasPixelData) {
        ss << "Throughput (Megapixels per second):\n";
        ss << "--------------------------------------------------------------------------------\n";
        ss << std::left << std::setw(20) << "Operation"
           << std::setw(15) << "CPU MP/s"
           << std::setw(15) << "SIMD MP/s"
           << std::setw(15) << "GPU MP/s"
           << "\n";
        ss << "--------------------------------------------------------------------------------\n";

        for (const auto& op : operations) {
            StatsKey cpuKey{op, Backend::CPU};
            StatsKey simdKey{op, Backend::SIMD};
            StatsKey gpuKey{op, Backend::GPU};

            auto cpuIt = m_stats.find(cpuKey);
            auto simdIt = m_stats.find(simdKey);
            auto gpuIt = m_stats.find(gpuKey);

            ss << std::left << std::setw(20) << op;

            // CPU throughput
            if (cpuIt != m_stats.end() && cpuIt->second.totalPixels > 0) {
                ss << std::setw(15) << std::fixed << std::setprecision(1)
                   << (cpuIt->second.avgPixelsPerSec() / 1e6);
            } else {
                ss << std::setw(15) << "-";
            }

            // SIMD throughput
            if (simdIt != m_stats.end() && simdIt->second.totalPixels > 0) {
                ss << std::setw(15) << std::fixed << std::setprecision(1)
                   << (simdIt->second.avgPixelsPerSec() / 1e6);
            } else {
                ss << std::setw(15) << "-";
            }

            // GPU throughput
            if (gpuIt != m_stats.end() && gpuIt->second.totalPixels > 0) {
                ss << std::setw(15) << std::fixed << std::setprecision(1)
                   << (gpuIt->second.avgPixelsPerSec() / 1e6);
            } else {
                ss << std::setw(15) << "-";
            }

            ss << "\n";
        }

        ss << "\n";
    }

    ss << "================================================================================\n";

    return ss.str();
}

void PerformanceStats::printReport() const
{
    std::cout << getReportString();
}
