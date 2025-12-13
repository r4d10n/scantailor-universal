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

#ifndef PICTURE_SHAPE_DETECTOR_H_
#define PICTURE_SHAPE_DETECTOR_H_

#include <QPolygonF>
#include <QRectF>
#include <QImage>
#include <vector>

class BinaryImage;

namespace imageproc {

/**
 * @brief Detected shape type for picture zones
 */
enum class PictureShapeType {
    Rectangle,
    Square,
    Polygon,
    Ellipse,
    Circle,
    Irregular
};

/**
 * @brief Result of shape detection
 */
struct PictureShape {
    PictureShapeType type;
    QPolygonF originalPolygon;
    QPolygonF simplifiedPolygon;
    QRectF boundingRect;
    double rectangularity;  // 0-1: how rectangular the shape is
    double aspectRatio;     // width/height
    double coverage;        // ratio of shape area to bounding rect area
    int vertexCount;

    PictureShape()
        : type(PictureShapeType::Irregular)
        , rectangularity(0)
        , aspectRatio(1)
        , coverage(0)
        , vertexCount(0)
    {}
};

/**
 * @brief Settings for picture shape detection
 */
struct PictureShapeSettings {
    double rectangularityThreshold;  // Min value to consider rectangular (0-1)
    double simplificationTolerance;  // Polygon simplification tolerance (pixels)
    double aspectRatioForSquare;     // Max deviation from 1.0 to consider square
    bool convertToRectangle;         // Auto-convert near-rectangular to rectangle
    bool preserveAspectRatio;        // When converting, preserve original aspect ratio

    PictureShapeSettings()
        : rectangularityThreshold(0.85)
        , simplificationTolerance(3.0)
        , aspectRatioForSquare(0.1)
        , convertToRectangle(false)
        , preserveAspectRatio(true)
    {}
};

/**
 * @brief Detects and converts picture zone shapes
 *
 * Analyzes polygon shapes to detect if they are:
 * - Rectangles (4 vertices, right angles)
 * - Squares (rectangle with equal sides)
 * - Regular polygons
 * - Ellipses/Circles
 * - Irregular shapes
 *
 * Can also convert near-rectangular shapes to proper rectangles.
 */
class PictureShapeDetector
{
public:
    /**
     * @brief Analyze a polygon to determine its shape
     * @param polygon The polygon to analyze
     * @param settings Detection settings
     * @return Shape analysis result
     */
    static PictureShape detectShape(const QPolygonF& polygon,
                                    const PictureShapeSettings& settings = PictureShapeSettings());

    /**
     * @brief Simplify a polygon by removing redundant vertices
     * @param polygon Input polygon
     * @param tolerance Maximum distance for a point to be considered redundant
     * @return Simplified polygon
     */
    static QPolygonF simplifyPolygon(const QPolygonF& polygon, double tolerance);

    /**
     * @brief Convert a near-rectangular polygon to a proper rectangle
     * @param polygon Input polygon
     * @param preserveAspectRatio If true, use original aspect ratio; otherwise fit to bounds
     * @return Rectangular polygon (4 vertices)
     */
    static QPolygonF convertToRectangle(const QPolygonF& polygon, bool preserveAspectRatio = true);

    /**
     * @brief Calculate the minimum area bounding rectangle
     * @param polygon Input polygon
     * @return Minimum area oriented bounding rectangle
     */
    static QPolygonF minimumAreaRect(const QPolygonF& polygon);

    /**
     * @brief Calculate rectangularity metric
     *
     * Rectangularity = area / boundingRectArea
     * A value of 1.0 means perfectly rectangular
     *
     * @param polygon Input polygon
     * @return Rectangularity value (0-1)
     */
    static double calculateRectangularity(const QPolygonF& polygon);

    /**
     * @brief Calculate convexity metric
     *
     * Convexity = area / convexHullArea
     * A value of 1.0 means fully convex
     *
     * @param polygon Input polygon
     * @return Convexity value (0-1)
     */
    static double calculateConvexity(const QPolygonF& polygon);

    /**
     * @brief Calculate polygon area
     * @param polygon Input polygon
     * @return Area in square pixels
     */
    static double calculateArea(const QPolygonF& polygon);

    /**
     * @brief Calculate convex hull of polygon
     * @param polygon Input polygon
     * @return Convex hull polygon
     */
    static QPolygonF convexHull(const QPolygonF& polygon);

    /**
     * @brief Check if polygon is approximately rectangular
     * @param polygon Input polygon
     * @param threshold Rectangularity threshold (0-1)
     * @return true if polygon is approximately rectangular
     */
    static bool isApproximatelyRectangular(const QPolygonF& polygon, double threshold = 0.85);

    /**
     * @brief Detect if polygon represents an ellipse
     * @param polygon Input polygon (should have many vertices)
     * @param tolerance How closely points must fit ellipse
     * @return true if polygon approximates an ellipse
     */
    static bool isEllipse(const QPolygonF& polygon, double tolerance = 0.1);

    /**
     * @brief Fit an ellipse to a polygon
     * @param polygon Input polygon
     * @param centerX Output: center X coordinate
     * @param centerY Output: center Y coordinate
     * @param radiusX Output: X radius
     * @param radiusY Output: Y radius
     * @param angle Output: rotation angle in radians
     * @return Fit quality (0-1, higher is better)
     */
    static double fitEllipse(const QPolygonF& polygon,
                            double& centerX, double& centerY,
                            double& radiusX, double& radiusY,
                            double& angle);

    /**
     * @brief Process multiple zones, detecting and optionally converting shapes
     * @param zones Vector of zone polygons
     * @param settings Detection settings
     * @return Vector of processed shapes
     */
    static std::vector<PictureShape> processZones(const std::vector<QPolygonF>& zones,
                                                  const PictureShapeSettings& settings);

private:
    PictureShapeDetector() = delete;

    static double crossProduct(const QPointF& o, const QPointF& a, const QPointF& b);
    static double distance(const QPointF& p1, const QPointF& p2);
    static double pointToLineDistance(const QPointF& point, const QPointF& lineStart, const QPointF& lineEnd);
};

} // namespace imageproc

#endif // PICTURE_SHAPE_DETECTOR_H_
