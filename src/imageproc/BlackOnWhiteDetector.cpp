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

#include "BlackOnWhiteDetector.h"
#include "BinaryImage.h"
#include "Grayscale.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace imageproc {

BlackOnWhiteResult BlackOnWhiteDetector::detect(const QImage& image, const QRect& contentRect)
{
    BlackOnWhiteResult result;

    if (image.isNull()) {
        return result;
    }

    // Convert to grayscale if needed
    QImage grayImage = image;
    if (grayImage.format() != QImage::Format_Grayscale8 &&
        grayImage.format() != QImage::Format_Indexed8) {
        grayImage = grayImage.convertToFormat(QImage::Format_Grayscale8);
    }

    QRect rect = contentRect.isValid() ? contentRect : grayImage.rect();

    // Calculate histogram
    int histogram[256];
    calculateHistogram(grayImage, histogram, rect);

    // Calculate total pixels and weighted sums
    long long totalPixels = 0;
    long long darkPixels = 0;    // < 128
    long long lightPixels = 0;   // >= 128
    long long weightedSum = 0;

    for (int i = 0; i < 256; ++i) {
        totalPixels += histogram[i];
        weightedSum += histogram[i] * i;
        if (i < 128) {
            darkPixels += histogram[i];
        } else {
            lightPixels += histogram[i];
        }
    }

    if (totalPixels == 0) {
        return result;
    }

    result.blackRatio = static_cast<double>(darkPixels) / totalPixels;
    result.whiteRatio = static_cast<double>(lightPixels) / totalPixels;

    // Analyze borders
    int borderWidth = std::max(5, std::min(grayImage.width(), grayImage.height()) / 50);
    double borderIntensity = analyzeBorders(grayImage, borderWidth);
    result.borderBlackRatio = (255.0 - borderIntensity) / 255.0;

    // Detect bimodal peaks
    int darkPeak, lightPeak;
    bool bimodal = detectBimodalPeaks(histogram, darkPeak, lightPeak);

    // Decision logic
    double meanIntensity = static_cast<double>(weightedSum) / totalPixels;

    // Check if borders are predominantly dark or light
    bool darkBorders = borderIntensity < 128;
    bool lightBorders = borderIntensity >= 128;

    // Calculate confidence
    double borderConfidence = std::abs(borderIntensity - 128.0) / 128.0;
    double ratioConfidence = std::abs(result.blackRatio - result.whiteRatio);
    result.confidence = (borderConfidence + ratioConfidence) / 2.0;

    // Make decision
    if (lightBorders && result.whiteRatio > result.blackRatio + 0.1) {
        // Light background with dark content (normal)
        result.type = BlackOnWhiteResult::BlackOnWhite;
    } else if (darkBorders && result.blackRatio > result.whiteRatio + 0.1) {
        // Dark background with light content (inverted)
        result.type = BlackOnWhiteResult::WhiteOnBlack;
    } else if (bimodal) {
        // Use peak positions for bimodal distributions
        if (darkPeak < 50 && lightPeak > 200) {
            // Clear separation - check which is background
            result.type = lightBorders ? BlackOnWhiteResult::BlackOnWhite : BlackOnWhiteResult::WhiteOnBlack;
        } else {
            result.type = BlackOnWhiteResult::Mixed;
            result.confidence *= 0.5;
        }
    } else {
        // Unclear - use mean intensity and border analysis
        if (meanIntensity > 160 && lightBorders) {
            result.type = BlackOnWhiteResult::BlackOnWhite;
        } else if (meanIntensity < 96 && darkBorders) {
            result.type = BlackOnWhiteResult::WhiteOnBlack;
        } else {
            result.type = BlackOnWhiteResult::Unknown;
            result.confidence = 0.3;
        }
    }

    return result;
}

BlackOnWhiteResult BlackOnWhiteDetector::detect(const BinaryImage& image, const QRect& contentRect)
{
    BlackOnWhiteResult result;

    if (image.isNull()) {
        return result;
    }

    QRect rect = contentRect.isValid() ? contentRect : image.rect();

    // Count black and white pixels
    int blackCount = 0;
    int whiteCount = 0;

    const uint32_t* line = image.data();
    const int wpl = image.wordsPerLine();
    const int width = image.width();
    const int height = image.height();

    int startY = rect.top();
    int endY = rect.bottom();
    int startX = rect.left();
    int endX = rect.right();

    for (int y = startY; y <= endY; ++y) {
        const uint32_t* curLine = line + y * wpl;
        for (int x = startX; x <= endX; ++x) {
            if ((curLine[x >> 5] >> (31 - (x & 31))) & 1) {
                blackCount++;
            } else {
                whiteCount++;
            }
        }
    }

    int totalPixels = blackCount + whiteCount;
    if (totalPixels == 0) {
        return result;
    }

    result.blackRatio = static_cast<double>(blackCount) / totalPixels;
    result.whiteRatio = static_cast<double>(whiteCount) / totalPixels;

    // Analyze border pixels
    int borderBlack = 0;
    int borderTotal = 0;
    int borderWidth = std::max(5, std::min(width, height) / 50);

    // Top and bottom borders
    for (int y = 0; y < borderWidth && y < height; ++y) {
        const uint32_t* curLine = line + y * wpl;
        for (int x = 0; x < width; ++x) {
            if ((curLine[x >> 5] >> (31 - (x & 31))) & 1) borderBlack++;
            borderTotal++;
        }
    }
    for (int y = height - borderWidth; y < height; ++y) {
        if (y >= borderWidth) {
            const uint32_t* curLine = line + y * wpl;
            for (int x = 0; x < width; ++x) {
                if ((curLine[x >> 5] >> (31 - (x & 31))) & 1) borderBlack++;
                borderTotal++;
            }
        }
    }
    // Left and right borders
    for (int y = borderWidth; y < height - borderWidth; ++y) {
        const uint32_t* curLine = line + y * wpl;
        for (int x = 0; x < borderWidth && x < width; ++x) {
            if ((curLine[x >> 5] >> (31 - (x & 31))) & 1) borderBlack++;
            borderTotal++;
        }
        for (int x = width - borderWidth; x < width; ++x) {
            if (x >= borderWidth) {
                if ((curLine[x >> 5] >> (31 - (x & 31))) & 1) borderBlack++;
                borderTotal++;
            }
        }
    }

    result.borderBlackRatio = (borderTotal > 0) ?
        static_cast<double>(borderBlack) / borderTotal : 0.5;

    // Decision logic for binary image
    bool whiteBorders = result.borderBlackRatio < 0.3;
    bool blackBorders = result.borderBlackRatio > 0.7;

    result.confidence = std::max(
        std::abs(result.borderBlackRatio - 0.5) * 2.0,
        std::abs(result.blackRatio - 0.5) * 2.0
    );

    if (whiteBorders) {
        result.type = BlackOnWhiteResult::BlackOnWhite;
    } else if (blackBorders) {
        result.type = BlackOnWhiteResult::WhiteOnBlack;
    } else if (result.whiteRatio > 0.6) {
        result.type = BlackOnWhiteResult::BlackOnWhite;
    } else if (result.blackRatio > 0.6) {
        result.type = BlackOnWhiteResult::WhiteOnBlack;
    } else {
        result.type = BlackOnWhiteResult::Mixed;
        result.confidence *= 0.5;
    }

    return result;
}

double BlackOnWhiteDetector::analyzeRegion(const QImage& image, const QRect& rect)
{
    if (image.isNull() || !rect.isValid()) {
        return 128.0;
    }

    QRect safeRect = rect.intersected(image.rect());
    if (safeRect.isEmpty()) {
        return 128.0;
    }

    long long sum = 0;
    int count = 0;

    for (int y = safeRect.top(); y <= safeRect.bottom(); ++y) {
        const uchar* line = image.scanLine(y);
        for (int x = safeRect.left(); x <= safeRect.right(); ++x) {
            if (image.format() == QImage::Format_Grayscale8) {
                sum += line[x];
            } else if (image.format() == QImage::Format_Indexed8) {
                sum += qGray(image.color(line[x]));
            } else {
                sum += qGray(image.pixel(x, y));
            }
            count++;
        }
    }

    return (count > 0) ? static_cast<double>(sum) / count : 128.0;
}

double BlackOnWhiteDetector::analyzeBorders(const QImage& image, int borderWidth)
{
    if (image.isNull() || borderWidth <= 0) {
        return 128.0;
    }

    int w = image.width();
    int h = image.height();
    borderWidth = std::min(borderWidth, std::min(w, h) / 4);

    double sum = 0;
    int regions = 0;

    // Top border
    sum += analyzeRegion(image, QRect(0, 0, w, borderWidth));
    regions++;

    // Bottom border
    sum += analyzeRegion(image, QRect(0, h - borderWidth, w, borderWidth));
    regions++;

    // Left border (excluding corners)
    sum += analyzeRegion(image, QRect(0, borderWidth, borderWidth, h - 2 * borderWidth));
    regions++;

    // Right border (excluding corners)
    sum += analyzeRegion(image, QRect(w - borderWidth, borderWidth, borderWidth, h - 2 * borderWidth));
    regions++;

    return sum / regions;
}

bool BlackOnWhiteDetector::needsInversion(const QImage& image, const BlackOnWhiteSettings& settings)
{
    if (settings.mode == BlackOnWhiteSettings::ForceNormal) {
        return false;
    }
    if (settings.mode == BlackOnWhiteSettings::ForceInverted) {
        return true;
    }

    BlackOnWhiteResult result = detect(image);

    if (!settings.invertIfNeeded) {
        return false;
    }

    return result.type == BlackOnWhiteResult::WhiteOnBlack &&
           result.confidence >= settings.confidenceThreshold;
}

BlackOnWhiteResult BlackOnWhiteDetector::autoInvert(QImage& image, const BlackOnWhiteSettings& settings)
{
    BlackOnWhiteResult result;

    if (settings.mode == BlackOnWhiteSettings::ForceNormal) {
        result.type = BlackOnWhiteResult::BlackOnWhite;
        result.confidence = 1.0;
        return result;
    }

    if (settings.mode == BlackOnWhiteSettings::ForceInverted) {
        image.invertPixels();
        result.type = BlackOnWhiteResult::WhiteOnBlack;
        result.confidence = 1.0;
        return result;
    }

    result = detect(image);

    if (settings.invertIfNeeded &&
        result.type == BlackOnWhiteResult::WhiteOnBlack &&
        result.confidence >= settings.confidenceThreshold) {
        image.invertPixels();
    }

    return result;
}

void BlackOnWhiteDetector::calculateHistogram(const QImage& image, int histogram[256], const QRect& rect)
{
    std::memset(histogram, 0, 256 * sizeof(int));

    if (image.isNull()) {
        return;
    }

    QRect safeRect = rect.isValid() ? rect.intersected(image.rect()) : image.rect();

    for (int y = safeRect.top(); y <= safeRect.bottom(); ++y) {
        const uchar* line = image.scanLine(y);
        for (int x = safeRect.left(); x <= safeRect.right(); ++x) {
            int gray;
            if (image.format() == QImage::Format_Grayscale8) {
                gray = line[x];
            } else if (image.format() == QImage::Format_Indexed8) {
                gray = qGray(image.color(line[x]));
            } else {
                gray = qGray(image.pixel(x, y));
            }
            histogram[gray]++;
        }
    }
}

bool BlackOnWhiteDetector::detectBimodalPeaks(const int histogram[256], int& darkPeak, int& lightPeak)
{
    // Find peaks using smoothed histogram
    int smoothed[256];
    const int windowSize = 5;

    for (int i = 0; i < 256; ++i) {
        int sum = 0;
        int count = 0;
        for (int j = std::max(0, i - windowSize); j <= std::min(255, i + windowSize); ++j) {
            sum += histogram[j];
            count++;
        }
        smoothed[i] = sum / count;
    }

    // Find maximum in dark region (0-127)
    darkPeak = 0;
    int maxDark = smoothed[0];
    for (int i = 1; i < 128; ++i) {
        if (smoothed[i] > maxDark) {
            maxDark = smoothed[i];
            darkPeak = i;
        }
    }

    // Find maximum in light region (128-255)
    lightPeak = 128;
    int maxLight = smoothed[128];
    for (int i = 129; i < 256; ++i) {
        if (smoothed[i] > maxLight) {
            maxLight = smoothed[i];
            lightPeak = i;
        }
    }

    // Check if both peaks are significant
    int totalPixels = 0;
    for (int i = 0; i < 256; ++i) {
        totalPixels += histogram[i];
    }

    double darkPeakRatio = (totalPixels > 0) ? static_cast<double>(maxDark) / totalPixels : 0;
    double lightPeakRatio = (totalPixels > 0) ? static_cast<double>(maxLight) / totalPixels : 0;

    // Both peaks should be at least 1% of total
    return darkPeakRatio > 0.01 && lightPeakRatio > 0.01 &&
           std::abs(lightPeak - darkPeak) > 50;
}

} // namespace imageproc
