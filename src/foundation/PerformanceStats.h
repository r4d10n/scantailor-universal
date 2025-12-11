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

#ifndef FOUNDATION_PERFORMANCE_STATS_H_
#define FOUNDATION_PERFORMANCE_STATS_H_

#include <chrono>
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <cstdint>

/**
 * @brief Performance statistics tracking for CPU, SIMD, and GPU operations
 *
 * This class provides a thread-safe singleton for collecting and reporting
 * performance metrics across different processing backends.
 *
 * Usage:
 *   // Start timing an operation
 *   PerformanceStats::instance().startTimer("grayscale", Backend::SIMD);
 *
 *   // ... perform operation ...
 *
 *   // Stop timing
 *   PerformanceStats::instance().stopTimer("grayscale", Backend::SIMD);
 *
 *   // At end of batch, print report
 *   PerformanceStats::instance().printReport();
 */
class PerformanceStats
{
public:
    /**
     * @brief Processing backend type
     */
    enum class Backend {
        CPU,    ///< Baseline CPU implementation
        SIMD,   ///< SIMD-optimized (SSE/AVX/NEON)
        GPU     ///< CUDA GPU implementation
    };

    /**
     * @brief Statistics for a single operation
     */
    struct OpStats {
        int64_t totalTimeUs = 0;    ///< Total time in microseconds
        int64_t minTimeUs = INT64_MAX;  ///< Minimum time
        int64_t maxTimeUs = 0;      ///< Maximum time
        uint32_t callCount = 0;     ///< Number of calls
        uint64_t totalPixels = 0;   ///< Total pixels processed

        double avgTimeUs() const {
            return callCount > 0 ? static_cast<double>(totalTimeUs) / callCount : 0.0;
        }

        double avgPixelsPerSec() const {
            return totalTimeUs > 0 ? (totalPixels * 1e6) / totalTimeUs : 0.0;
        }
    };

    /**
     * @brief Get singleton instance
     */
    static PerformanceStats& instance();

    /**
     * @brief Enable or disable statistics collection
     */
    void setEnabled(bool enabled);

    /**
     * @brief Check if statistics collection is enabled
     */
    bool isEnabled() const { return m_enabled; }

    /**
     * @brief Reset all statistics
     */
    void reset();

    /**
     * @brief Start timing an operation
     * @param operation Operation name (e.g., "grayscale", "morphology")
     * @param backend Which backend is being used
     */
    void startTimer(const std::string& operation, Backend backend);

    /**
     * @brief Stop timing an operation and record statistics
     * @param operation Operation name (must match startTimer)
     * @param backend Which backend was used
     * @param pixelsProcessed Optional: number of pixels processed
     */
    void stopTimer(const std::string& operation, Backend backend, uint64_t pixelsProcessed = 0);

    /**
     * @brief Record a timing directly (for external measurements)
     * @param operation Operation name
     * @param backend Backend used
     * @param timeUs Time in microseconds
     * @param pixelsProcessed Number of pixels processed
     */
    void recordTime(const std::string& operation, Backend backend,
                    int64_t timeUs, uint64_t pixelsProcessed = 0);

    /**
     * @brief Increment page counter
     */
    void incrementPageCount();

    /**
     * @brief Get total pages processed
     */
    uint32_t getPageCount() const { return m_pageCount; }

    /**
     * @brief Get statistics for a specific operation and backend
     */
    OpStats getStats(const std::string& operation, Backend backend) const;

    /**
     * @brief Get all operation names that have been recorded
     */
    std::vector<std::string> getOperationNames() const;

    /**
     * @brief Print formatted statistics report to stdout
     */
    void printReport() const;

    /**
     * @brief Get report as string
     */
    std::string getReportString() const;

    /**
     * @brief Convert backend enum to string
     */
    static const char* backendToString(Backend backend);

private:
    PerformanceStats();
    ~PerformanceStats() = default;

    // Non-copyable
    PerformanceStats(const PerformanceStats&) = delete;
    PerformanceStats& operator=(const PerformanceStats&) = delete;

    // Key for stats map: operation name + backend
    struct StatsKey {
        std::string operation;
        Backend backend;

        bool operator<(const StatsKey& other) const {
            if (operation != other.operation) return operation < other.operation;
            return static_cast<int>(backend) < static_cast<int>(other.backend);
        }
    };

    // Timer state for in-progress operations
    struct TimerState {
        std::chrono::high_resolution_clock::time_point startTime;
        bool active = false;
    };

    mutable std::mutex m_mutex;
    bool m_enabled;
    uint32_t m_pageCount;
    std::chrono::high_resolution_clock::time_point m_sessionStart;

    std::map<StatsKey, OpStats> m_stats;
    std::map<StatsKey, TimerState> m_timers;
};

/**
 * @brief RAII helper for timing operations
 *
 * Usage:
 *   {
 *       ScopedTimer timer("grayscale", PerformanceStats::Backend::SIMD, width * height);
 *       // ... operation ...
 *   } // Timer automatically stopped
 */
class ScopedTimer
{
public:
    ScopedTimer(const std::string& operation, PerformanceStats::Backend backend,
                uint64_t pixelsProcessed = 0)
        : m_operation(operation)
        , m_backend(backend)
        , m_pixels(pixelsProcessed)
    {
        PerformanceStats::instance().startTimer(operation, backend);
    }

    ~ScopedTimer() {
        PerformanceStats::instance().stopTimer(m_operation, m_backend, m_pixels);
    }

private:
    std::string m_operation;
    PerformanceStats::Backend m_backend;
    uint64_t m_pixels;
};

#endif // FOUNDATION_PERFORMANCE_STATS_H_
