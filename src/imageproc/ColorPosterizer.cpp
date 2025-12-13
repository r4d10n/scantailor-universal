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

#include "ColorPosterizer.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <QSet>

namespace imageproc {

// ============================================================================
// ColorPosterizer Implementation
// ============================================================================

QImage ColorPosterizer::posterize(const QImage& input, const Options& options)
{
    if (input.isNull()) {
        return QImage();
    }

    QImage image = input.convertToFormat(QImage::Format_RGB32);
    QVector<QColor> palette;

    // Generate palette based on algorithm
    switch (options.algorithm) {
        case Algorithm::MedianCut:
            palette = medianCutPalette(image, options.maxColors);
            break;
        case Algorithm::KMeans:
            palette = kMeansPalette(image, options.maxColors);
            break;
        case Algorithm::Octree:
            palette = octreePalette(image, options.maxColors);
            break;
        case Algorithm::Uniform:
        default:
            palette = uniformPalette(options.maxColors);
            break;
    }

    // Ensure black and white are in palette if requested
    if (options.preserveBlackWhite) {
        bool hasBlack = false, hasWhite = false;
        for (const QColor& c : palette) {
            if (c == Qt::black) hasBlack = true;
            if (c == Qt::white) hasWhite = true;
        }
        if (!hasBlack && palette.size() > 0) {
            palette[palette.size() - 1] = Qt::black;
        }
        if (!hasWhite && palette.size() > 1) {
            palette[palette.size() - 2] = Qt::white;
        }
    }

    // Apply dithering if requested
    if (options.dither) {
        applyDithering(image, palette, options.ditheringAmount);
    } else {
        // Simple nearest-color mapping
        for (int y = 0; y < image.height(); ++y) {
            QRgb* line = reinterpret_cast<QRgb*>(image.scanLine(y));
            for (int x = 0; x < image.width(); ++x) {
                QColor original(line[x]);
                int idx = findClosestColor(original, palette);
                line[x] = palette[idx].rgb();
            }
        }
    }

    return image;
}

QVector<QColor> ColorPosterizer::extractPalette(const QImage& image)
{
    QSet<QRgb> uniqueColors;
    QImage img = image.convertToFormat(QImage::Format_RGB32);

    for (int y = 0; y < img.height(); ++y) {
        const QRgb* line = reinterpret_cast<const QRgb*>(img.constScanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            uniqueColors.insert(line[x] | 0xFF000000); // Ensure alpha is set
        }
    }

    QVector<QColor> palette;
    palette.reserve(uniqueColors.size());
    for (QRgb rgb : uniqueColors) {
        palette.append(QColor(rgb));
    }

    return palette;
}

QImage ColorPosterizer::toIndexed(const QImage& input, const QVector<QColor>& palette, bool dither)
{
    if (input.isNull() || palette.isEmpty()) {
        return QImage();
    }

    QImage image = input.convertToFormat(QImage::Format_RGB32);

    if (dither) {
        applyDithering(image, palette, 0.5f);
    } else {
        for (int y = 0; y < image.height(); ++y) {
            QRgb* line = reinterpret_cast<QRgb*>(image.scanLine(y));
            for (int x = 0; x < image.width(); ++x) {
                QColor original(line[x]);
                int idx = findClosestColor(original, palette);
                line[x] = palette[idx].rgb();
            }
        }
    }

    // Convert to indexed format if palette is small enough
    if (palette.size() <= 256) {
        QVector<QRgb> colorTable;
        for (const QColor& c : palette) {
            colorTable.append(c.rgb());
        }

        QImage indexed(image.size(), QImage::Format_Indexed8);
        indexed.setColorTable(colorTable);

        for (int y = 0; y < image.height(); ++y) {
            const QRgb* srcLine = reinterpret_cast<const QRgb*>(image.constScanLine(y));
            uchar* dstLine = indexed.scanLine(y);
            for (int x = 0; x < image.width(); ++x) {
                dstLine[x] = static_cast<uchar>(findClosestColor(QColor(srcLine[x]), palette));
            }
        }

        return indexed;
    }

    return image;
}

// Median Cut Algorithm
struct ColorBox {
    QVector<QColor> colors;
    int rMin, rMax, gMin, gMax, bMin, bMax;

    void computeBounds() {
        rMin = gMin = bMin = 255;
        rMax = gMax = bMax = 0;
        for (const QColor& c : colors) {
            rMin = std::min(rMin, c.red());
            rMax = std::max(rMax, c.red());
            gMin = std::min(gMin, c.green());
            gMax = std::max(gMax, c.green());
            bMin = std::min(bMin, c.blue());
            bMax = std::max(bMax, c.blue());
        }
    }

    int longestAxis() const {
        int rRange = rMax - rMin;
        int gRange = gMax - gMin;
        int bRange = bMax - bMin;
        if (rRange >= gRange && rRange >= bRange) return 0;
        if (gRange >= rRange && gRange >= bRange) return 1;
        return 2;
    }

    QColor averageColor() const {
        if (colors.isEmpty()) return Qt::black;
        long long r = 0, g = 0, b = 0;
        for (const QColor& c : colors) {
            r += c.red();
            g += c.green();
            b += c.blue();
        }
        int n = colors.size();
        return QColor(r / n, g / n, b / n);
    }
};

QVector<QColor> ColorPosterizer::medianCutPalette(const QImage& image, int numColors)
{
    // Collect all colors (sampled for large images)
    QVector<QColor> allColors;
    int step = std::max(1, (image.width() * image.height()) / 10000);

    for (int y = 0; y < image.height(); y += step) {
        const QRgb* line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < image.width(); x += step) {
            allColors.append(QColor(line[x]));
        }
    }

    if (allColors.isEmpty()) {
        return QVector<QColor>() << Qt::black;
    }

    // Initialize with one box containing all colors
    QVector<ColorBox> boxes;
    ColorBox initial;
    initial.colors = allColors;
    initial.computeBounds();
    boxes.append(initial);

    // Split boxes until we have enough
    while (boxes.size() < numColors) {
        // Find box with most colors
        int maxIdx = 0;
        int maxSize = 0;
        for (int i = 0; i < boxes.size(); ++i) {
            if (boxes[i].colors.size() > maxSize) {
                maxSize = boxes[i].colors.size();
                maxIdx = i;
            }
        }

        if (maxSize < 2) break;

        ColorBox& box = boxes[maxIdx];
        int axis = box.longestAxis();

        // Sort by the longest axis
        std::sort(box.colors.begin(), box.colors.end(),
                  [axis](const QColor& a, const QColor& b) {
                      if (axis == 0) return a.red() < b.red();
                      if (axis == 1) return a.green() < b.green();
                      return a.blue() < b.blue();
                  });

        // Split at median
        int median = box.colors.size() / 2;
        ColorBox newBox;
        newBox.colors = box.colors.mid(median);
        newBox.computeBounds();

        box.colors = box.colors.mid(0, median);
        box.computeBounds();

        boxes.append(newBox);
    }

    // Extract palette from boxes
    QVector<QColor> palette;
    for (const ColorBox& box : boxes) {
        palette.append(box.averageColor());
    }

    return palette;
}

QVector<QColor> ColorPosterizer::kMeansPalette(const QImage& image, int numColors, int iterations)
{
    // Sample colors from image
    QVector<QColor> samples;
    int step = std::max(1, (image.width() * image.height()) / 5000);

    for (int y = 0; y < image.height(); y += step) {
        const QRgb* line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < image.width(); x += step) {
            samples.append(QColor(line[x]));
        }
    }

    if (samples.isEmpty()) {
        return uniformPalette(numColors);
    }

    // Initialize centroids randomly
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, samples.size() - 1);

    QVector<QColor> centroids;
    for (int i = 0; i < numColors; ++i) {
        centroids.append(samples[dis(gen)]);
    }

    // K-means iterations
    for (int iter = 0; iter < iterations; ++iter) {
        // Assign samples to nearest centroid
        QVector<QVector<QColor>> clusters(numColors);
        for (const QColor& sample : samples) {
            int nearest = findClosestColor(sample, centroids);
            clusters[nearest].append(sample);
        }

        // Update centroids
        for (int i = 0; i < numColors; ++i) {
            if (clusters[i].isEmpty()) continue;

            long long r = 0, g = 0, b = 0;
            for (const QColor& c : clusters[i]) {
                r += c.red();
                g += c.green();
                b += c.blue();
            }
            int n = clusters[i].size();
            centroids[i] = QColor(r / n, g / n, b / n);
        }
    }

    return centroids;
}

QVector<QColor> ColorPosterizer::octreePalette(const QImage& image, int numColors)
{
    // Simplified octree - use median cut for now
    return medianCutPalette(image, numColors);
}

QVector<QColor> ColorPosterizer::uniformPalette(int numColors)
{
    QVector<QColor> palette;

    // Calculate levels per channel
    int levels = static_cast<int>(std::ceil(std::cbrt(numColors)));
    int step = 255 / std::max(1, levels - 1);

    for (int r = 0; r < levels && palette.size() < numColors; ++r) {
        for (int g = 0; g < levels && palette.size() < numColors; ++g) {
            for (int b = 0; b < levels && palette.size() < numColors; ++b) {
                palette.append(QColor(r * step, g * step, b * step));
            }
        }
    }

    return palette;
}

void ColorPosterizer::applyDithering(QImage& image, const QVector<QColor>& palette, float amount)
{
    // Floyd-Steinberg dithering
    int width = image.width();
    int height = image.height();

    // Error accumulation buffers
    QVector<float> errorR(width + 2, 0);
    QVector<float> errorG(width + 2, 0);
    QVector<float> errorB(width + 2, 0);
    QVector<float> nextErrorR(width + 2, 0);
    QVector<float> nextErrorG(width + 2, 0);
    QVector<float> nextErrorB(width + 2, 0);

    for (int y = 0; y < height; ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(image.scanLine(y));

        for (int x = 0; x < width; ++x) {
            QColor old(line[x]);

            // Add accumulated error
            int newR = qBound(0, old.red() + static_cast<int>(errorR[x + 1] * amount), 255);
            int newG = qBound(0, old.green() + static_cast<int>(errorG[x + 1] * amount), 255);
            int newB = qBound(0, old.blue() + static_cast<int>(errorB[x + 1] * amount), 255);
            QColor adjusted(newR, newG, newB);

            // Find closest palette color
            int idx = findClosestColor(adjusted, palette);
            QColor newColor = palette[idx];
            line[x] = newColor.rgb();

            // Calculate error
            float errR = adjusted.red() - newColor.red();
            float errG = adjusted.green() - newColor.green();
            float errB = adjusted.blue() - newColor.blue();

            // Distribute error (Floyd-Steinberg coefficients: 7/16, 3/16, 5/16, 1/16)
            errorR[x + 2] += errR * 7.0f / 16.0f;
            errorG[x + 2] += errG * 7.0f / 16.0f;
            errorB[x + 2] += errB * 7.0f / 16.0f;

            nextErrorR[x] += errR * 3.0f / 16.0f;
            nextErrorG[x] += errG * 3.0f / 16.0f;
            nextErrorB[x] += errB * 3.0f / 16.0f;

            nextErrorR[x + 1] += errR * 5.0f / 16.0f;
            nextErrorG[x + 1] += errG * 5.0f / 16.0f;
            nextErrorB[x + 1] += errB * 5.0f / 16.0f;

            nextErrorR[x + 2] += errR * 1.0f / 16.0f;
            nextErrorG[x + 2] += errG * 1.0f / 16.0f;
            nextErrorB[x + 2] += errB * 1.0f / 16.0f;
        }

        // Swap error buffers
        std::swap(errorR, nextErrorR);
        std::swap(errorG, nextErrorG);
        std::swap(errorB, nextErrorB);
        std::fill(nextErrorR.begin(), nextErrorR.end(), 0);
        std::fill(nextErrorG.begin(), nextErrorG.end(), 0);
        std::fill(nextErrorB.begin(), nextErrorB.end(), 0);
    }
}

int ColorPosterizer::findClosestColor(const QColor& color, const QVector<QColor>& palette)
{
    int bestIdx = 0;
    int bestDist = INT_MAX;

    for (int i = 0; i < palette.size(); ++i) {
        int dist = colorDistance(color, palette[i]);
        if (dist < bestDist) {
            bestDist = dist;
            bestIdx = i;
        }
    }

    return bestIdx;
}

int ColorPosterizer::colorDistance(const QColor& c1, const QColor& c2)
{
    int dr = c1.red() - c2.red();
    int dg = c1.green() - c2.green();
    int db = c1.blue() - c2.blue();
    return dr * dr + dg * dg + db * db;
}

// ============================================================================
// ColorSegmenter Implementation
// ============================================================================

QVector<ColorSegmenter::Segment> ColorSegmenter::segment(const QImage& input, const Options& options)
{
    if (input.isNull()) {
        return QVector<Segment>();
    }

    QImage image = input.convertToFormat(QImage::Format_RGB32);
    int width = image.width();
    int height = image.height();

    // First, posterize to reduce colors
    ColorPosterizer::Options postOpts;
    postOpts.maxColors = options.numSegments;
    postOpts.algorithm = ColorPosterizer::Algorithm::KMeans;
    postOpts.dither = false;
    QImage posterized = ColorPosterizer::posterize(image, postOpts);

    // Extract palette
    QVector<QColor> palette = ColorPosterizer::extractPalette(posterized);

    // Create segments
    QVector<Segment> segments(palette.size());
    for (int i = 0; i < palette.size(); ++i) {
        segments[i].meanColor = palette[i];
        segments[i].pixelCount = 0;
    }

    // Assign pixels to segments
    for (int y = 0; y < height; ++y) {
        const QRgb* line = reinterpret_cast<const QRgb*>(posterized.constScanLine(y));
        for (int x = 0; x < width; ++x) {
            QColor color(line[x]);
            int idx = ColorPosterizer::findClosestColor(color, palette);
            segments[idx].pixels.append(QPoint(x, y));
            segments[idx].pixelCount++;
        }
    }

    // Compute bounding rectangles
    for (Segment& seg : segments) {
        if (seg.pixels.isEmpty()) continue;

        int minX = width, maxX = 0, minY = height, maxY = 0;
        for (const QPoint& p : seg.pixels) {
            minX = std::min(minX, p.x());
            maxX = std::max(maxX, p.x());
            minY = std::min(minY, p.y());
            maxY = std::max(maxY, p.y());
        }
        seg.boundingRect = QRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
    }

    // Remove empty segments and optionally merge small ones
    QVector<Segment> result;
    for (const Segment& seg : segments) {
        if (seg.pixelCount >= options.minSegmentSize) {
            result.append(seg);
        } else if (options.mergeSmall && !result.isEmpty() && seg.pixelCount > 0) {
            // Merge into closest segment by color
            int bestIdx = 0;
            int bestDist = INT_MAX;
            for (int i = 0; i < result.size(); ++i) {
                int dist = ColorPosterizer::colorDistance(seg.meanColor, result[i].meanColor);
                if (dist < bestDist) {
                    bestDist = dist;
                    bestIdx = i;
                }
            }
            result[bestIdx].pixels.append(seg.pixels);
            result[bestIdx].pixelCount += seg.pixelCount;
        }
    }

    return result;
}

QImage ColorSegmenter::visualize(const QImage& input, const QVector<Segment>& segments)
{
    QImage result(input.size(), QImage::Format_RGB32);
    result.fill(Qt::black);

    // Assign distinct colors to each segment
    QVector<QColor> vizColors;
    vizColors << Qt::red << Qt::green << Qt::blue << Qt::yellow
              << Qt::cyan << Qt::magenta << QColor(255, 128, 0)
              << QColor(128, 0, 255) << QColor(0, 255, 128);

    for (int i = 0; i < segments.size(); ++i) {
        QColor color = vizColors[i % vizColors.size()];
        for (const QPoint& p : segments[i].pixels) {
            result.setPixelColor(p, color);
        }
    }

    return result;
}

QImage ColorSegmenter::segmentMask(const QSize& imageSize, const Segment& segment)
{
    QImage mask(imageSize, QImage::Format_Grayscale8);
    mask.fill(0);

    for (const QPoint& p : segment.pixels) {
        mask.setPixelColor(p, Qt::white);
    }

    return mask;
}

} // namespace imageproc
