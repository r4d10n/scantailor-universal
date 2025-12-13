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

#ifndef COLORPOSTERIZER_H
#define COLORPOSTERIZER_H

#include <QImage>
#include <QColor>
#include <QVector>
#include <cstdint>

namespace imageproc {

/**
 * @brief Color quantization and posterization for indexed image output
 *
 * Reduces the number of colors in an image using various algorithms.
 * Particularly useful for creating DjVu foreground layers.
 */
class ColorPosterizer
{
public:
    enum class Algorithm {
        MedianCut,      // Median cut quantization
        KMeans,         // K-means clustering
        Octree,         // Octree-based quantization
        Uniform         // Uniform color reduction
    };

    struct Options {
        int maxColors;          // Maximum colors in output (2-256)
        Algorithm algorithm;    // Quantization algorithm
        bool dither;            // Apply dithering
        float ditheringAmount;  // Dithering strength (0.0-1.0)
        bool preserveBlackWhite; // Keep pure black and white

        Options()
            : maxColors(16)
            , algorithm(Algorithm::MedianCut)
            , dither(true)
            , ditheringAmount(0.5f)
            , preserveBlackWhite(true)
        {}
    };

    /**
     * @brief Posterize an image to reduce colors
     * @param input Input image (RGB or RGBA)
     * @param options Posterization options
     * @return Posterized image with reduced color palette
     */
    static QImage posterize(const QImage& input, const Options& options);

    /**
     * @brief Get the palette from a posterized image
     * @param image The posterized image
     * @return Vector of colors in the palette
     */
    static QVector<QColor> extractPalette(const QImage& image);

    /**
     * @brief Convert image to indexed format with given palette
     * @param input Input image
     * @param palette Color palette to use
     * @param dither Apply dithering
     * @return Indexed color image
     */
    static QImage toIndexed(const QImage& input, const QVector<QColor>& palette, bool dither = true);

    // Find closest color in palette (public for use by ColorSegmenter)
    static int findClosestColor(const QColor& color, const QVector<QColor>& palette);

    // Color distance (squared Euclidean in RGB space) (public for use by ColorSegmenter)
    static int colorDistance(const QColor& c1, const QColor& c2);

private:
    // Median cut algorithm implementation
    static QVector<QColor> medianCutPalette(const QImage& image, int numColors);

    // K-means clustering implementation
    static QVector<QColor> kMeansPalette(const QImage& image, int numColors, int iterations = 10);

    // Octree quantization implementation
    static QVector<QColor> octreePalette(const QImage& image, int numColors);

    // Uniform quantization implementation
    static QVector<QColor> uniformPalette(int numColors);

    // Apply Floyd-Steinberg dithering
    static void applyDithering(QImage& image, const QVector<QColor>& palette, float amount);
};

/**
 * @brief Color segmentation for separating image into distinct color regions
 *
 * Segments an image into regions of similar colors, useful for
 * layer separation and content analysis.
 */
class ColorSegmenter
{
public:
    struct Segment {
        QColor meanColor;       // Average color of segment
        QVector<QPoint> pixels; // Pixels in this segment
        QRect boundingRect;     // Bounding rectangle
        int pixelCount;         // Number of pixels
    };

    struct Options {
        int numSegments;        // Target number of segments
        float colorThreshold;   // Color similarity threshold (0.0-1.0)
        int minSegmentSize;     // Minimum pixels per segment
        bool mergeSmall;        // Merge segments below minimum size

        Options()
            : numSegments(8)
            , colorThreshold(0.1f)
            , minSegmentSize(100)
            , mergeSmall(true)
        {}
    };

    /**
     * @brief Segment image into color regions
     * @param input Input image
     * @param options Segmentation options
     * @return Vector of segments
     */
    static QVector<Segment> segment(const QImage& input, const Options& options);

    /**
     * @brief Create a visualization of segments
     * @param input Original image
     * @param segments Segments from segment()
     * @return Image with each segment colored differently
     */
    static QImage visualize(const QImage& input, const QVector<Segment>& segments);

    /**
     * @brief Extract a mask for a specific segment
     * @param imageSize Size of the original image
     * @param segment The segment to create mask for
     * @return Binary mask (white = segment, black = background)
     */
    static QImage segmentMask(const QSize& imageSize, const Segment& segment);
};

} // namespace imageproc

#endif // COLORPOSTERIZER_H
