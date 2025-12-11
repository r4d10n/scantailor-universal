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

#ifndef BLACK_ON_WHITE_DETECTOR_H_
#define BLACK_ON_WHITE_DETECTOR_H_

#include <QImage>
#include <QRect>
#include <QString>

class BinaryImage;

namespace imageproc {

/**
 * @brief Detection result for black-on-white analysis
 */
struct BlackOnWhiteResult {
    enum Type {
        BlackOnWhite,    // Normal: dark content on light background
        WhiteOnBlack,    // Inverted: light content on dark background
        Mixed,           // Both present
        Unknown          // Could not determine
    };

    Type type;
    double confidence;   // 0.0 to 1.0
    double blackRatio;   // Ratio of black pixels
    double whiteRatio;   // Ratio of white pixels
    double borderBlackRatio;  // Black ratio at borders

    BlackOnWhiteResult()
        : type(Unknown)
        , confidence(0)
        , blackRatio(0)
        , whiteRatio(0)
        , borderBlackRatio(0)
    {}

    bool isInverted() const { return type == WhiteOnBlack; }
    bool isNormal() const { return type == BlackOnWhite; }
    QString typeString() const {
        switch (type) {
        case BlackOnWhite: return "Black on White";
        case WhiteOnBlack: return "White on Black";
        case Mixed: return "Mixed";
        default: return "Unknown";
        }
    }
};

/**
 * @brief Settings for black-on-white detection
 */
struct BlackOnWhiteSettings {
    enum Mode {
        Auto,           // Automatically detect
        ForceNormal,    // Assume black on white
        ForceInverted   // Assume white on black (invert)
    };

    Mode mode;
    double borderWidthRatio;   // Border width as ratio of image size (for border analysis)
    double confidenceThreshold; // Minimum confidence to make decision
    bool invertIfNeeded;       // Automatically invert if detected as white-on-black

    BlackOnWhiteSettings()
        : mode(Auto)
        , borderWidthRatio(0.02)  // 2% of image size
        , confidenceThreshold(0.7)
        , invertIfNeeded(true)
    {}
};

/**
 * @brief Detects whether content is black-on-white or white-on-black
 *
 * Analyzes image to determine if it has normal (dark content on light background)
 * or inverted (light content on dark background) content. This is useful for:
 * - Detecting inverted scans
 * - Detecting negative film scans
 * - Handling pages with inverted regions
 */
class BlackOnWhiteDetector
{
public:
    /**
     * @brief Detect black-on-white status of a grayscale image
     * @param image Input grayscale image
     * @param contentRect Optional rectangle to analyze (default: entire image)
     * @return Detection result
     */
    static BlackOnWhiteResult detect(const QImage& image, const QRect& contentRect = QRect());

    /**
     * @brief Detect black-on-white status of a binary image
     * @param image Input binary image
     * @param contentRect Optional rectangle to analyze
     * @return Detection result
     */
    static BlackOnWhiteResult detect(const BinaryImage& image, const QRect& contentRect = QRect());

    /**
     * @brief Analyze a region of the image
     * @param image Input image
     * @param rect Region to analyze
     * @return Average intensity (0-255)
     */
    static double analyzeRegion(const QImage& image, const QRect& rect);

    /**
     * @brief Analyze border regions of the image
     *
     * Borders are typically background, so this helps determine the
     * expected background color.
     *
     * @param image Input image
     * @param borderWidth Width of border region to analyze
     * @return Average intensity of border regions
     */
    static double analyzeBorders(const QImage& image, int borderWidth);

    /**
     * @brief Check if image needs inversion based on detection
     * @param image Input image
     * @param settings Detection settings
     * @return true if image should be inverted
     */
    static bool needsInversion(const QImage& image, const BlackOnWhiteSettings& settings);

    /**
     * @brief Invert image if needed based on detection
     * @param image Input/output image (modified in place)
     * @param settings Detection settings
     * @return Detection result
     */
    static BlackOnWhiteResult autoInvert(QImage& image, const BlackOnWhiteSettings& settings);

    /**
     * @brief Calculate histogram of grayscale image
     * @param image Input grayscale image
     * @param histogram Output histogram (256 bins)
     * @param rect Region to analyze (default: entire image)
     */
    static void calculateHistogram(const QImage& image, int histogram[256], const QRect& rect = QRect());

    /**
     * @brief Detect bimodal peaks in histogram
     * @param histogram Input histogram
     * @param darkPeak Output: position of dark peak
     * @param lightPeak Output: position of light peak
     * @return true if bimodal distribution detected
     */
    static bool detectBimodalPeaks(const int histogram[256], int& darkPeak, int& lightPeak);

private:
    BlackOnWhiteDetector() = delete;
};

} // namespace imageproc

#endif // BLACK_ON_WHITE_DETECTOR_H_
