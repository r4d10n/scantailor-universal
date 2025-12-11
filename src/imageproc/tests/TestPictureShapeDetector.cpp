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

#include "../PictureShapeDetector.h"
#include <boost/test/unit_test.hpp>
#include <cmath>

using namespace imageproc;

BOOST_AUTO_TEST_SUITE(PictureShapeDetectorTestSuite)

BOOST_AUTO_TEST_CASE(test_detect_rectangle)
{
    // Create a perfect rectangle
    QPolygonF rect;
    rect << QPointF(0, 0) << QPointF(100, 0) << QPointF(100, 50) << QPointF(0, 50);

    PictureShapeSettings settings;
    PictureShape result = PictureShapeDetector::detectShape(rect, settings);

    BOOST_CHECK_EQUAL(result.type, PictureShapeType::Rectangle);
    BOOST_CHECK_CLOSE(result.rectangularity, 1.0, 0.01);
    BOOST_CHECK_EQUAL(result.vertexCount, 4);
}

BOOST_AUTO_TEST_CASE(test_detect_square)
{
    // Create a square
    QPolygonF square;
    square << QPointF(0, 0) << QPointF(100, 0) << QPointF(100, 100) << QPointF(0, 100);

    PictureShapeSettings settings;
    PictureShape result = PictureShapeDetector::detectShape(square, settings);

    BOOST_CHECK_EQUAL(result.type, PictureShapeType::Square);
    BOOST_CHECK_CLOSE(result.aspectRatio, 1.0, 0.01);
}

BOOST_AUTO_TEST_CASE(test_calculate_area)
{
    // Rectangle area = 100 * 50 = 5000
    QPolygonF rect;
    rect << QPointF(0, 0) << QPointF(100, 0) << QPointF(100, 50) << QPointF(0, 50);

    double area = PictureShapeDetector::calculateArea(rect);
    BOOST_CHECK_CLOSE(area, 5000.0, 0.01);
}

BOOST_AUTO_TEST_CASE(test_calculate_area_triangle)
{
    // Triangle with base 100 and height 50, area = 100 * 50 / 2 = 2500
    QPolygonF triangle;
    triangle << QPointF(0, 0) << QPointF(100, 0) << QPointF(50, 50);

    double area = PictureShapeDetector::calculateArea(triangle);
    BOOST_CHECK_CLOSE(area, 2500.0, 0.01);
}

BOOST_AUTO_TEST_CASE(test_simplify_polygon)
{
    // Create polygon with redundant points on a line
    QPolygonF polygon;
    polygon << QPointF(0, 0) << QPointF(25, 0) << QPointF(50, 0)
            << QPointF(75, 0) << QPointF(100, 0) << QPointF(100, 50)
            << QPointF(0, 50);

    QPolygonF simplified = PictureShapeDetector::simplifyPolygon(polygon, 1.0);

    // Should simplify to 4 points (rectangle corners)
    BOOST_CHECK_LE(simplified.size(), 4);
}

BOOST_AUTO_TEST_CASE(test_convex_hull)
{
    // Create polygon with interior point
    QPolygonF polygon;
    polygon << QPointF(0, 0) << QPointF(100, 0) << QPointF(50, 25)  // Interior point
            << QPointF(100, 50) << QPointF(0, 50);

    QPolygonF hull = PictureShapeDetector::convexHull(polygon);

    // Hull should be a rectangle (4 points)
    BOOST_CHECK_EQUAL(hull.size(), 4);
}

BOOST_AUTO_TEST_CASE(test_rectangularity)
{
    // Perfect rectangle has rectangularity = 1.0
    QPolygonF rect;
    rect << QPointF(0, 0) << QPointF(100, 0) << QPointF(100, 50) << QPointF(0, 50);

    double rectang = PictureShapeDetector::calculateRectangularity(rect);
    BOOST_CHECK_CLOSE(rectang, 1.0, 0.01);

    // Triangle has rectangularity < 1.0
    QPolygonF triangle;
    triangle << QPointF(0, 0) << QPointF(100, 0) << QPointF(50, 100);

    rectang = PictureShapeDetector::calculateRectangularity(triangle);
    BOOST_CHECK_LT(rectang, 1.0);
    BOOST_CHECK_CLOSE(rectang, 0.5, 1.0);  // Triangle is about 0.5
}

BOOST_AUTO_TEST_CASE(test_convert_to_rectangle)
{
    // Create slightly irregular quadrilateral
    QPolygonF quad;
    quad << QPointF(2, 3) << QPointF(98, 1) << QPointF(101, 52) << QPointF(-1, 48);

    QPolygonF rect = PictureShapeDetector::convertToRectangle(quad, false);

    BOOST_CHECK_EQUAL(rect.size(), 4);

    // Check it's approximately rectangular
    double rectang = PictureShapeDetector::calculateRectangularity(rect);
    BOOST_CHECK_CLOSE(rectang, 1.0, 0.01);
}

BOOST_AUTO_TEST_CASE(test_is_approximately_rectangular)
{
    // Perfect rectangle
    QPolygonF rect;
    rect << QPointF(0, 0) << QPointF(100, 0) << QPointF(100, 50) << QPointF(0, 50);

    BOOST_CHECK(PictureShapeDetector::isApproximatelyRectangular(rect, 0.95));

    // Triangle is not rectangular
    QPolygonF triangle;
    triangle << QPointF(0, 0) << QPointF(100, 0) << QPointF(50, 100);

    BOOST_CHECK(!PictureShapeDetector::isApproximatelyRectangular(triangle, 0.95));
}

BOOST_AUTO_TEST_CASE(test_convexity)
{
    // Convex polygon has convexity = 1.0
    QPolygonF convex;
    convex << QPointF(0, 0) << QPointF(100, 0) << QPointF(100, 50) << QPointF(0, 50);

    double conv = PictureShapeDetector::calculateConvexity(convex);
    BOOST_CHECK_CLOSE(conv, 1.0, 0.01);

    // Concave polygon has convexity < 1.0
    QPolygonF concave;
    concave << QPointF(0, 0) << QPointF(50, 0) << QPointF(50, 25) << QPointF(100, 0)
            << QPointF(100, 50) << QPointF(0, 50);

    conv = PictureShapeDetector::calculateConvexity(concave);
    BOOST_CHECK_LT(conv, 1.0);
}

BOOST_AUTO_TEST_CASE(test_process_zones)
{
    std::vector<QPolygonF> zones;

    // Add rectangle
    QPolygonF rect;
    rect << QPointF(0, 0) << QPointF(100, 0) << QPointF(100, 50) << QPointF(0, 50);
    zones.push_back(rect);

    // Add square
    QPolygonF square;
    square << QPointF(0, 0) << QPointF(50, 0) << QPointF(50, 50) << QPointF(0, 50);
    zones.push_back(square);

    PictureShapeSettings settings;
    auto results = PictureShapeDetector::processZones(zones, settings);

    BOOST_CHECK_EQUAL(results.size(), 2);
    BOOST_CHECK_EQUAL(results[0].type, PictureShapeType::Rectangle);
    BOOST_CHECK_EQUAL(results[1].type, PictureShapeType::Square);
}

BOOST_AUTO_TEST_CASE(test_empty_polygon)
{
    QPolygonF empty;
    PictureShape result = PictureShapeDetector::detectShape(empty, PictureShapeSettings());

    BOOST_CHECK_EQUAL(result.type, PictureShapeType::Irregular);
    BOOST_CHECK_EQUAL(result.vertexCount, 0);
}

BOOST_AUTO_TEST_SUITE_END()
