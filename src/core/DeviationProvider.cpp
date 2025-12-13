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

#include "DeviationProvider.h"
#include <cmath>
#include <algorithm>

DeviationProvider& DeviationProvider::instance()
{
    static DeviationProvider instance;
    return instance;
}

DeviationProvider::DeviationProvider()
    : m_avgSkew(0)
    , m_stdDevSkew(0)
    , m_avgContentWidth(0)
    , m_avgContentHeight(0)
    , m_stdDevContentWidth(0)
    , m_stdDevContentHeight(0)
    , m_statisticsValid(false)
    , m_cacheValid(false)
{
}

DeviationProvider::Settings DeviationProvider::settings() const
{
    QMutexLocker lock(&m_mutex);
    return m_settings;
}

void DeviationProvider::setSettings(const Settings& settings)
{
    QMutexLocker lock(&m_mutex);
    m_settings = settings;
    m_cacheValid = false;
}

void DeviationProvider::setPageSkew(const PageId& pageId, double angle)
{
    QMutexLocker lock(&m_mutex);
    m_pageSkews[pageId] = angle;
    m_statisticsValid = false;
    m_cacheValid = false;
}

void DeviationProvider::setPageContentSize(const PageId& pageId, double width, double height)
{
    QMutexLocker lock(&m_mutex);
    m_pageContentSizes[pageId] = QSizeF(width, height);
    m_statisticsValid = false;
    m_cacheValid = false;
}

void DeviationProvider::setPageMargins(const PageId& pageId, double top, double bottom, double left, double right)
{
    QMutexLocker lock(&m_mutex);
    MarginData data;
    data.top = top;
    data.bottom = bottom;
    data.left = left;
    data.right = right;
    m_pageMargins[pageId] = data;
    m_statisticsValid = false;
    m_cacheValid = false;
}

void DeviationProvider::setPageSize(const PageId& pageId, double width, double height)
{
    QMutexLocker lock(&m_mutex);
    m_pageSizes[pageId] = QSizeF(width, height);
    m_statisticsValid = false;
    m_cacheValid = false;
}

bool DeviationProvider::isDeviant(const PageId& pageId) const
{
    return getDeviationTypes(pageId) != DeviationType::None;
}

DeviationType DeviationProvider::getDeviationTypes(const PageId& pageId) const
{
    QMutexLocker lock(&m_mutex);

    if (!m_settings.enabled) {
        return DeviationType::None;
    }

    if (!m_statisticsValid) {
        const_cast<DeviationProvider*>(this)->computeStatistics();
    }

    if (m_cacheValid && m_deviationCache.contains(pageId)) {
        return m_deviationCache[pageId];
    }

    DeviationType result = DeviationType::None;
    DeviationInfo info;

    if (checkSkewDeviation(pageId, info)) {
        result = result | DeviationType::SkewAngle;
    }

    if (checkContentSizeDeviation(pageId, info)) {
        result = result | DeviationType::ContentSize;
    }

    if (checkMarginDeviation(pageId, info)) {
        result = result | DeviationType::MarginSize;
    }

    m_deviationCache[pageId] = result;
    return result;
}

std::vector<DeviationInfo> DeviationProvider::getDeviations(const PageId& pageId) const
{
    QMutexLocker lock(&m_mutex);

    std::vector<DeviationInfo> deviations;

    if (!m_settings.enabled) {
        return deviations;
    }

    if (!m_statisticsValid) {
        const_cast<DeviationProvider*>(this)->computeStatistics();
    }

    DeviationInfo info;

    if (checkSkewDeviation(pageId, info)) {
        deviations.push_back(info);
    }

    if (checkContentSizeDeviation(pageId, info)) {
        deviations.push_back(info);
    }

    if (checkMarginDeviation(pageId, info)) {
        deviations.push_back(info);
    }

    return deviations;
}

QSet<PageId> DeviationProvider::getDeviantPages() const
{
    return getDeviantPages(DeviationType::All);
}

QSet<PageId> DeviationProvider::getDeviantPages(DeviationType type) const
{
    QMutexLocker lock(&m_mutex);
    QSet<PageId> result;

    // Collect all page IDs
    QSet<PageId> allPages;
    for (auto it = m_pageSkews.constBegin(); it != m_pageSkews.constEnd(); ++it) {
        allPages.insert(it.key());
    }
    for (auto it = m_pageContentSizes.constBegin(); it != m_pageContentSizes.constEnd(); ++it) {
        allPages.insert(it.key());
    }
    for (auto it = m_pageMargins.constBegin(); it != m_pageMargins.constEnd(); ++it) {
        allPages.insert(it.key());
    }

    for (const PageId& pageId : allPages) {
        DeviationType pageDeviation = getDeviationTypes(pageId);
        if ((pageDeviation & type) != DeviationType::None) {
            result.insert(pageId);
        }
    }

    return result;
}

double DeviationProvider::getAverageSkew() const
{
    QMutexLocker lock(&m_mutex);
    if (!m_statisticsValid) {
        const_cast<DeviationProvider*>(this)->computeStatistics();
    }
    return m_avgSkew;
}

double DeviationProvider::getAverageContentWidth() const
{
    QMutexLocker lock(&m_mutex);
    if (!m_statisticsValid) {
        const_cast<DeviationProvider*>(this)->computeStatistics();
    }
    return m_avgContentWidth;
}

double DeviationProvider::getAverageContentHeight() const
{
    QMutexLocker lock(&m_mutex);
    if (!m_statisticsValid) {
        const_cast<DeviationProvider*>(this)->computeStatistics();
    }
    return m_avgContentHeight;
}

double DeviationProvider::getStandardDeviationSkew() const
{
    QMutexLocker lock(&m_mutex);
    if (!m_statisticsValid) {
        const_cast<DeviationProvider*>(this)->computeStatistics();
    }
    return m_stdDevSkew;
}

void DeviationProvider::recomputeDeviations()
{
    QMutexLocker lock(&m_mutex);
    m_statisticsValid = false;
    m_cacheValid = false;
    m_deviationCache.clear();
    computeStatistics();
}

void DeviationProvider::clear()
{
    QMutexLocker lock(&m_mutex);
    m_pageSkews.clear();
    m_pageContentSizes.clear();
    m_pageMargins.clear();
    m_pageSizes.clear();
    m_deviationCache.clear();
    m_statisticsValid = false;
    m_cacheValid = false;
}

void DeviationProvider::removePage(const PageId& pageId)
{
    QMutexLocker lock(&m_mutex);
    m_pageSkews.remove(pageId);
    m_pageContentSizes.remove(pageId);
    m_pageMargins.remove(pageId);
    m_pageSizes.remove(pageId);
    m_deviationCache.remove(pageId);
    m_statisticsValid = false;
    m_cacheValid = false;
}

void DeviationProvider::computeStatistics()
{
    // Compute average and standard deviation for skew
    if (!m_pageSkews.isEmpty()) {
        double sum = 0;
        for (auto it = m_pageSkews.constBegin(); it != m_pageSkews.constEnd(); ++it) {
            sum += it.value();
        }
        m_avgSkew = sum / m_pageSkews.size();

        double sqSum = 0;
        for (auto it = m_pageSkews.constBegin(); it != m_pageSkews.constEnd(); ++it) {
            double diff = it.value() - m_avgSkew;
            sqSum += diff * diff;
        }
        m_stdDevSkew = std::sqrt(sqSum / m_pageSkews.size());
    } else {
        m_avgSkew = 0;
        m_stdDevSkew = 0;
    }

    // Compute average and standard deviation for content size
    if (!m_pageContentSizes.isEmpty()) {
        double sumW = 0, sumH = 0;
        for (auto it = m_pageContentSizes.constBegin(); it != m_pageContentSizes.constEnd(); ++it) {
            sumW += it.value().width();
            sumH += it.value().height();
        }
        m_avgContentWidth = sumW / m_pageContentSizes.size();
        m_avgContentHeight = sumH / m_pageContentSizes.size();

        double sqSumW = 0, sqSumH = 0;
        for (auto it = m_pageContentSizes.constBegin(); it != m_pageContentSizes.constEnd(); ++it) {
            double diffW = it.value().width() - m_avgContentWidth;
            double diffH = it.value().height() - m_avgContentHeight;
            sqSumW += diffW * diffW;
            sqSumH += diffH * diffH;
        }
        m_stdDevContentWidth = std::sqrt(sqSumW / m_pageContentSizes.size());
        m_stdDevContentHeight = std::sqrt(sqSumH / m_pageContentSizes.size());
    } else {
        m_avgContentWidth = 0;
        m_avgContentHeight = 0;
        m_stdDevContentWidth = 0;
        m_stdDevContentHeight = 0;
    }

    m_statisticsValid = true;
}

bool DeviationProvider::checkSkewDeviation(const PageId& pageId, DeviationInfo& info) const
{
    if (!m_pageSkews.contains(pageId)) {
        return false;
    }

    double skew = m_pageSkews[pageId];
    double deviation = std::abs(skew - m_avgSkew);

    if (deviation > m_settings.skewThreshold) {
        info.type = DeviationType::SkewAngle;
        info.description = QString("Skew angle deviation: %1° (average: %2°)")
                              .arg(skew, 0, 'f', 2)
                              .arg(m_avgSkew, 0, 'f', 2);
        info.value = skew;
        info.expectedValue = m_avgSkew;
        info.deviation = (m_avgSkew != 0) ? (deviation / std::abs(m_avgSkew) * 100) : deviation;
        return true;
    }

    return false;
}

bool DeviationProvider::checkContentSizeDeviation(const PageId& pageId, DeviationInfo& info) const
{
    if (!m_pageContentSizes.contains(pageId)) {
        return false;
    }

    QSizeF size = m_pageContentSizes[pageId];

    // Check width deviation
    if (m_avgContentWidth > 0) {
        double widthDev = std::abs(size.width() - m_avgContentWidth) / m_avgContentWidth * 100;
        if (widthDev > m_settings.contentSizeThreshold) {
            info.type = DeviationType::ContentSize;
            info.description = QString("Content width deviation: %1% (expected: %2, actual: %3)")
                                  .arg(widthDev, 0, 'f', 1)
                                  .arg(m_avgContentWidth, 0, 'f', 0)
                                  .arg(size.width(), 0, 'f', 0);
            info.value = size.width();
            info.expectedValue = m_avgContentWidth;
            info.deviation = widthDev;
            return true;
        }
    }

    // Check height deviation
    if (m_avgContentHeight > 0) {
        double heightDev = std::abs(size.height() - m_avgContentHeight) / m_avgContentHeight * 100;
        if (heightDev > m_settings.contentSizeThreshold) {
            info.type = DeviationType::ContentSize;
            info.description = QString("Content height deviation: %1% (expected: %2, actual: %3)")
                                  .arg(heightDev, 0, 'f', 1)
                                  .arg(m_avgContentHeight, 0, 'f', 0)
                                  .arg(size.height(), 0, 'f', 0);
            info.value = size.height();
            info.expectedValue = m_avgContentHeight;
            info.deviation = heightDev;
            return true;
        }
    }

    return false;
}

bool DeviationProvider::checkMarginDeviation(const PageId& pageId, DeviationInfo& info) const
{
    if (!m_pageMargins.contains(pageId)) {
        return false;
    }

    // Compute average margins
    if (m_pageMargins.size() < 2) {
        return false;
    }

    double avgTop = 0, avgBottom = 0, avgLeft = 0, avgRight = 0;
    for (auto it = m_pageMargins.constBegin(); it != m_pageMargins.constEnd(); ++it) {
        avgTop += it.value().top;
        avgBottom += it.value().bottom;
        avgLeft += it.value().left;
        avgRight += it.value().right;
    }
    int count = m_pageMargins.size();
    avgTop /= count;
    avgBottom /= count;
    avgLeft /= count;
    avgRight /= count;

    const MarginData& margins = m_pageMargins[pageId];

    // Check if any margin deviates significantly
    double maxDev = 0;
    QString side;
    double actual = 0, expected = 0;

    if (std::abs(margins.top - avgTop) > maxDev) {
        maxDev = std::abs(margins.top - avgTop);
        side = "top";
        actual = margins.top;
        expected = avgTop;
    }
    if (std::abs(margins.bottom - avgBottom) > maxDev) {
        maxDev = std::abs(margins.bottom - avgBottom);
        side = "bottom";
        actual = margins.bottom;
        expected = avgBottom;
    }
    if (std::abs(margins.left - avgLeft) > maxDev) {
        maxDev = std::abs(margins.left - avgLeft);
        side = "left";
        actual = margins.left;
        expected = avgLeft;
    }
    if (std::abs(margins.right - avgRight) > maxDev) {
        maxDev = std::abs(margins.right - avgRight);
        side = "right";
        actual = margins.right;
        expected = avgRight;
    }

    if (maxDev > m_settings.marginThreshold) {
        info.type = DeviationType::MarginSize;
        info.description = QString("%1 margin deviation: %2mm (expected: %3mm)")
                              .arg(side)
                              .arg(actual, 0, 'f', 1)
                              .arg(expected, 0, 'f', 1);
        info.value = actual;
        info.expectedValue = expected;
        info.deviation = maxDev;
        return true;
    }

    return false;
}
