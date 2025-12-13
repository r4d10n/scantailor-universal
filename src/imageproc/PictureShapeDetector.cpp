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

#include "PictureShapeDetector.h"
#include <algorithm>
#include <cmath>
#include <stack>

namespace imageproc {

PictureShape PictureShapeDetector::detectShape(const QPolygonF& polygon,
                                               const PictureShapeSettings& settings)
{
    PictureShape result;

    if (polygon.size() < 3) {
        return result;
    }

    result.originalPolygon = polygon;
    result.simplifiedPolygon = simplifyPolygon(polygon, settings.simplificationTolerance);
    result.boundingRect = polygon.boundingRect();
    result.vertexCount = result.simplifiedPolygon.size();
    result.rectangularity = calculateRectangularity(polygon);
    result.aspectRatio = result.boundingRect.width() / result.boundingRect.height();
    result.coverage = calculateArea(polygon) /
                     (result.boundingRect.width() * result.boundingRect.height());

    // Determine shape type
    int vertices = result.simplifiedPolygon.size();

    if (vertices == 4 && result.rectangularity >= settings.rectangularityThreshold) {
        // Check if it's a square
        double ar = result.aspectRatio;
        if (std::abs(ar - 1.0) <= settings.aspectRatioForSquare) {
            result.type = PictureShapeType::Square;
        } else {
            result.type = PictureShapeType::Rectangle;
        }
    } else if (vertices >= 8 && isEllipse(result.simplifiedPolygon)) {
        // Check for ellipse/circle
        if (std::abs(result.aspectRatio - 1.0) <= settings.aspectRatioForSquare) {
            result.type = PictureShapeType::Circle;
        } else {
            result.type = PictureShapeType::Ellipse;
        }
    } else if (vertices >= 3 && vertices <= 8) {
        result.type = PictureShapeType::Polygon;
    } else {
        result.type = PictureShapeType::Irregular;
    }

    // Optional conversion to rectangle
    if (settings.convertToRectangle &&
        result.rectangularity >= settings.rectangularityThreshold &&
        result.type != PictureShapeType::Ellipse &&
        result.type != PictureShapeType::Circle) {
        result.simplifiedPolygon = convertToRectangle(polygon, settings.preserveAspectRatio);
        result.type = PictureShapeType::Rectangle;
        result.vertexCount = 4;
    }

    return result;
}

QPolygonF PictureShapeDetector::simplifyPolygon(const QPolygonF& polygon, double tolerance)
{
    if (polygon.size() <= 3 || tolerance <= 0) {
        return polygon;
    }

    // Douglas-Peucker algorithm
    std::vector<bool> keep(polygon.size(), false);
    keep[0] = true;
    keep[polygon.size() - 1] = true;

    std::stack<std::pair<int, int>> ranges;
    ranges.push({0, static_cast<int>(polygon.size() - 1)});

    while (!ranges.empty()) {
        auto [start, end] = ranges.top();
        ranges.pop();

        if (end - start < 2) continue;

        double maxDist = 0;
        int maxIdx = start;

        for (int i = start + 1; i < end; ++i) {
            double dist = pointToLineDistance(polygon[i], polygon[start], polygon[end]);
            if (dist > maxDist) {
                maxDist = dist;
                maxIdx = i;
            }
        }

        if (maxDist > tolerance) {
            keep[maxIdx] = true;
            ranges.push({start, maxIdx});
            ranges.push({maxIdx, end});
        }
    }

    QPolygonF result;
    for (int i = 0; i < polygon.size(); ++i) {
        if (keep[i]) {
            result.append(polygon[i]);
        }
    }

    return result;
}

QPolygonF PictureShapeDetector::convertToRectangle(const QPolygonF& polygon, bool preserveAspectRatio)
{
    if (polygon.isEmpty()) {
        return polygon;
    }

    QRectF bounds = polygon.boundingRect();

    if (preserveAspectRatio) {
        // Use minimum area rectangle for better fit
        QPolygonF minRect = minimumAreaRect(polygon);
        if (minRect.size() == 4) {
            return minRect;
        }
    }

    // Fall back to axis-aligned bounding rectangle
    QPolygonF rect;
    rect << bounds.topLeft()
         << bounds.topRight()
         << bounds.bottomRight()
         << bounds.bottomLeft();

    return rect;
}

QPolygonF PictureShapeDetector::minimumAreaRect(const QPolygonF& polygon)
{
    if (polygon.size() < 3) {
        return polygon;
    }

    // Get convex hull first
    QPolygonF hull = convexHull(polygon);
    if (hull.size() < 3) {
        return polygon;
    }

    // Rotating calipers algorithm for minimum area rectangle
    double minArea = std::numeric_limits<double>::max();
    QPolygonF minRect;

    for (int i = 0; i < hull.size(); ++i) {
        QPointF edge = hull[(i + 1) % hull.size()] - hull[i];
        double angle = std::atan2(edge.y(), edge.x());

        // Rotate all points
        double cosA = std::cos(-angle);
        double sinA = std::sin(-angle);

        double minX = std::numeric_limits<double>::max();
        double maxX = std::numeric_limits<double>::lowest();
        double minY = std::numeric_limits<double>::max();
        double maxY = std::numeric_limits<double>::lowest();

        for (const QPointF& p : hull) {
            double rx = p.x() * cosA - p.y() * sinA;
            double ry = p.x() * sinA + p.y() * cosA;
            minX = std::min(minX, rx);
            maxX = std::max(maxX, rx);
            minY = std::min(minY, ry);
            maxY = std::max(maxY, ry);
        }

        double area = (maxX - minX) * (maxY - minY);
        if (area < minArea) {
            minArea = area;

            // Rotate rectangle back
            double cosA2 = std::cos(angle);
            double sinA2 = std::sin(angle);

            QPointF corners[4] = {
                {minX, minY},
                {maxX, minY},
                {maxX, maxY},
                {minX, maxY}
            };

            minRect.clear();
            for (const auto& c : corners) {
                minRect << QPointF(c.x() * cosA2 - c.y() * sinA2,
                                   c.x() * sinA2 + c.y() * cosA2);
            }
        }
    }

    return minRect;
}

double PictureShapeDetector::calculateRectangularity(const QPolygonF& polygon)
{
    if (polygon.size() < 3) {
        return 0;
    }

    double area = calculateArea(polygon);
    QRectF bounds = polygon.boundingRect();
    double boundsArea = bounds.width() * bounds.height();

    if (boundsArea <= 0) {
        return 0;
    }

    return area / boundsArea;
}

double PictureShapeDetector::calculateConvexity(const QPolygonF& polygon)
{
    if (polygon.size() < 3) {
        return 0;
    }

    double area = calculateArea(polygon);
    QPolygonF hull = convexHull(polygon);
    double hullArea = calculateArea(hull);

    if (hullArea <= 0) {
        return 0;
    }

    return area / hullArea;
}

double PictureShapeDetector::calculateArea(const QPolygonF& polygon)
{
    if (polygon.size() < 3) {
        return 0;
    }

    // Shoelace formula
    double area = 0;
    int n = polygon.size();

    for (int i = 0; i < n; ++i) {
        int j = (i + 1) % n;
        area += polygon[i].x() * polygon[j].y();
        area -= polygon[j].x() * polygon[i].y();
    }

    return std::abs(area) / 2.0;
}

QPolygonF PictureShapeDetector::convexHull(const QPolygonF& polygon)
{
    if (polygon.size() < 3) {
        return polygon;
    }

    // Graham scan algorithm
    std::vector<QPointF> points(polygon.begin(), polygon.end());

    // Find bottom-most point (or left-most in case of tie)
    int minIdx = 0;
    for (int i = 1; i < points.size(); ++i) {
        if (points[i].y() < points[minIdx].y() ||
            (points[i].y() == points[minIdx].y() && points[i].x() < points[minIdx].x())) {
            minIdx = i;
        }
    }
    std::swap(points[0], points[minIdx]);
    QPointF pivot = points[0];

    // Sort by polar angle
    std::sort(points.begin() + 1, points.end(), [&pivot](const QPointF& a, const QPointF& b) {
        double v = crossProduct(pivot, a, b);
        if (std::abs(v) < 1e-10) {
            // Collinear - sort by distance
            return distance(pivot, a) < distance(pivot, b);
        }
        return v > 0;
    });

    // Build hull
    std::vector<QPointF> hull;
    for (const QPointF& p : points) {
        while (hull.size() > 1 && crossProduct(hull[hull.size()-2], hull[hull.size()-1], p) <= 0) {
            hull.pop_back();
        }
        hull.push_back(p);
    }

    QPolygonF result;
    for (const QPointF& p : hull) {
        result << p;
    }
    return result;
}

bool PictureShapeDetector::isApproximatelyRectangular(const QPolygonF& polygon, double threshold)
{
    return calculateRectangularity(polygon) >= threshold;
}

bool PictureShapeDetector::isEllipse(const QPolygonF& polygon, double tolerance)
{
    if (polygon.size() < 8) {
        return false;
    }

    double cx, cy, rx, ry, angle;
    double quality = fitEllipse(polygon, cx, cy, rx, ry, angle);

    return quality >= (1.0 - tolerance);
}

double PictureShapeDetector::fitEllipse(const QPolygonF& polygon,
                                        double& centerX, double& centerY,
                                        double& radiusX, double& radiusY,
                                        double& angle)
{
    if (polygon.size() < 5) {
        return 0;
    }

    // Simple ellipse fitting using bounding box
    QRectF bounds = polygon.boundingRect();
    centerX = bounds.center().x();
    centerY = bounds.center().y();
    radiusX = bounds.width() / 2.0;
    radiusY = bounds.height() / 2.0;
    angle = 0;

    // Calculate fit quality
    double totalError = 0;
    for (const QPointF& p : polygon) {
        // Distance from point to ellipse (normalized)
        double dx = (p.x() - centerX) / radiusX;
        double dy = (p.y() - centerY) / radiusY;
        double dist = std::sqrt(dx * dx + dy * dy);
        totalError += std::abs(dist - 1.0);
    }

    double avgError = totalError / polygon.size();
    return std::max(0.0, 1.0 - avgError);
}

std::vector<PictureShape> PictureShapeDetector::processZones(const std::vector<QPolygonF>& zones,
                                                             const PictureShapeSettings& settings)
{
    std::vector<PictureShape> results;
    results.reserve(zones.size());

    for (const QPolygonF& zone : zones) {
        results.push_back(detectShape(zone, settings));
    }

    return results;
}

double PictureShapeDetector::crossProduct(const QPointF& o, const QPointF& a, const QPointF& b)
{
    return (a.x() - o.x()) * (b.y() - o.y()) - (a.y() - o.y()) * (b.x() - o.x());
}

double PictureShapeDetector::distance(const QPointF& p1, const QPointF& p2)
{
    double dx = p2.x() - p1.x();
    double dy = p2.y() - p1.y();
    return std::sqrt(dx * dx + dy * dy);
}

double PictureShapeDetector::pointToLineDistance(const QPointF& point,
                                                 const QPointF& lineStart,
                                                 const QPointF& lineEnd)
{
    double dx = lineEnd.x() - lineStart.x();
    double dy = lineEnd.y() - lineStart.y();
    double lineLenSq = dx * dx + dy * dy;

    if (lineLenSq < 1e-10) {
        return distance(point, lineStart);
    }

    // Perpendicular distance
    double t = std::max(0.0, std::min(1.0,
        ((point.x() - lineStart.x()) * dx + (point.y() - lineStart.y()) * dy) / lineLenSq));

    QPointF projection(lineStart.x() + t * dx, lineStart.y() + t * dy);
    return distance(point, projection);
}

} // namespace imageproc
