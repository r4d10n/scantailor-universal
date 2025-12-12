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

#ifndef DEVIATIONPROVIDER_H
#define DEVIATIONPROVIDER_H

#include "PageId.h"
#include <QMap>
#include <QSet>
#include <QColor>
#include <QSizeF>
#include <QMutex>
#include <vector>
#include <memory>

class ProjectPages;

/**
 * @brief Types of deviations that can be detected in pages
 */
enum class DeviationType {
    None = 0,
    SkewAngle = 1 << 0,       // Unusual rotation angle
    ContentSize = 1 << 1,     // Content size differs significantly
    MarginSize = 1 << 2,      // Margin settings differ
    PageSize = 1 << 3,        // Physical page size differs
    Orientation = 1 << 4,     // Different orientation
    ColorMode = 1 << 5,       // Different color mode setting
    DPI = 1 << 6,             // Different DPI setting
    Dewarping = 1 << 7,       // Different dewarping mode
    All = 0xFF
};

inline DeviationType operator|(DeviationType a, DeviationType b) {
    return static_cast<DeviationType>(static_cast<int>(a) | static_cast<int>(b));
}

inline DeviationType operator&(DeviationType a, DeviationType b) {
    return static_cast<DeviationType>(static_cast<int>(a) & static_cast<int>(b));
}

/**
 * @brief Information about a detected deviation
 */
struct DeviationInfo {
    DeviationType type;
    QString description;
    double value;           // The actual value
    double expectedValue;   // The expected/average value
    double deviation;       // How much it deviates (as percentage)

    DeviationInfo()
        : type(DeviationType::None)
        , value(0)
        , expectedValue(0)
        , deviation(0)
    {}
};

/**
 * @brief Tracks and reports page deviations/anomalies
 *
 * Identifies pages with unusual characteristics compared to the rest
 * of the project, highlighting them for user attention.
 */
class DeviationProvider
{
public:
    struct Settings {
        bool enabled;
        double skewThreshold;       // Max acceptable skew difference in degrees
        double contentSizeThreshold; // Max acceptable content size difference (percentage)
        double marginThreshold;     // Max acceptable margin difference (mm)
        QColor highlightColor;      // Color for highlighting deviants

        Settings()
            : enabled(true)
            , skewThreshold(1.0)        // 1 degree
            , contentSizeThreshold(20.0) // 20%
            , marginThreshold(5.0)       // 5mm
            , highlightColor(Qt::red)
        {}
    };

    static DeviationProvider& instance();

    // Settings
    Settings settings() const;
    void setSettings(const Settings& settings);

    // Register page values for analysis
    void setPageSkew(const PageId& pageId, double angle);
    void setPageContentSize(const PageId& pageId, double width, double height);
    void setPageMargins(const PageId& pageId, double top, double bottom, double left, double right);
    void setPageSize(const PageId& pageId, double width, double height);

    // Check for deviations
    bool isDeviant(const PageId& pageId) const;
    DeviationType getDeviationTypes(const PageId& pageId) const;
    std::vector<DeviationInfo> getDeviations(const PageId& pageId) const;

    // Get all deviant pages
    QSet<PageId> getDeviantPages() const;
    QSet<PageId> getDeviantPages(DeviationType type) const;

    // Statistics
    double getAverageSkew() const;
    double getAverageContentWidth() const;
    double getAverageContentHeight() const;
    double getStandardDeviationSkew() const;

    // Recompute deviations (call after batch changes)
    void recomputeDeviations();

    // Clear all data
    void clear();

    // Remove data for a page
    void removePage(const PageId& pageId);

private:
    DeviationProvider();
    ~DeviationProvider() = default;
    DeviationProvider(const DeviationProvider&) = delete;
    DeviationProvider& operator=(const DeviationProvider&) = delete;

    void computeStatistics();
    bool checkSkewDeviation(const PageId& pageId, DeviationInfo& info) const;
    bool checkContentSizeDeviation(const PageId& pageId, DeviationInfo& info) const;
    bool checkMarginDeviation(const PageId& pageId, DeviationInfo& info) const;

    mutable QMutex m_mutex;
    Settings m_settings;

    // Page data
    QMap<PageId, double> m_pageSkews;
    QMap<PageId, QSizeF> m_pageContentSizes;
    struct MarginData {
        double top, bottom, left, right;
    };
    QMap<PageId, MarginData> m_pageMargins;
    QMap<PageId, QSizeF> m_pageSizes;

    // Computed statistics
    double m_avgSkew;
    double m_stdDevSkew;
    double m_avgContentWidth;
    double m_avgContentHeight;
    double m_stdDevContentWidth;
    double m_stdDevContentHeight;
    bool m_statisticsValid;

    // Cached deviation results
    mutable QMap<PageId, DeviationType> m_deviationCache;
    mutable bool m_cacheValid;
};

#endif // DEVIATIONPROVIDER_H
