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

#include "../BlackOnWhiteDetector.h"
#include <QImage>
#include <boost/test/unit_test.hpp>

using namespace imageproc;

BOOST_AUTO_TEST_SUITE(BlackOnWhiteDetectorTestSuite)

BOOST_AUTO_TEST_CASE(test_detect_black_on_white)
{
    // Create white image with black content in center
    QImage image(100, 100, QImage::Format_Grayscale8);
    image.fill(Qt::white);

    // Add black rectangle in center
    for (int y = 30; y < 70; ++y) {
        uchar* line = image.scanLine(y);
        for (int x = 30; x < 70; ++x) {
            line[x] = 0;  // Black
        }
    }

    BlackOnWhiteResult result = BlackOnWhiteDetector::detect(image);

    BOOST_CHECK_EQUAL(result.type, BlackOnWhiteResult::BlackOnWhite);
    BOOST_CHECK_GT(result.confidence, 0.5);
}

BOOST_AUTO_TEST_CASE(test_detect_white_on_black)
{
    // Create black image with white content in center
    QImage image(100, 100, QImage::Format_Grayscale8);
    image.fill(Qt::black);

    // Add white rectangle in center
    for (int y = 30; y < 70; ++y) {
        uchar* line = image.scanLine(y);
        for (int x = 30; x < 70; ++x) {
            line[x] = 255;  // White
        }
    }

    BlackOnWhiteResult result = BlackOnWhiteDetector::detect(image);

    BOOST_CHECK_EQUAL(result.type, BlackOnWhiteResult::WhiteOnBlack);
    BOOST_CHECK_GT(result.confidence, 0.5);
}

BOOST_AUTO_TEST_CASE(test_analyze_borders)
{
    // White image should have light borders
    QImage whiteImage(100, 100, QImage::Format_Grayscale8);
    whiteImage.fill(Qt::white);

    double borderIntensity = BlackOnWhiteDetector::analyzeBorders(whiteImage, 10);
    BOOST_CHECK_GT(borderIntensity, 200);  // Should be close to 255

    // Black image should have dark borders
    QImage blackImage(100, 100, QImage::Format_Grayscale8);
    blackImage.fill(Qt::black);

    borderIntensity = BlackOnWhiteDetector::analyzeBorders(blackImage, 10);
    BOOST_CHECK_LT(borderIntensity, 50);  // Should be close to 0
}

BOOST_AUTO_TEST_CASE(test_histogram_calculation)
{
    // Create image with known distribution
    QImage image(100, 100, QImage::Format_Grayscale8);
    image.fill(128);  // All mid-gray

    int histogram[256];
    BlackOnWhiteDetector::calculateHistogram(image, histogram);

    // All pixels should be at value 128
    BOOST_CHECK_EQUAL(histogram[128], 10000);

    // All other bins should be 0
    int nonZeroCount = 0;
    for (int i = 0; i < 256; ++i) {
        if (i != 128 && histogram[i] != 0) {
            nonZeroCount++;
        }
    }
    BOOST_CHECK_EQUAL(nonZeroCount, 0);
}

BOOST_AUTO_TEST_CASE(test_bimodal_detection)
{
    // Create bimodal histogram
    int histogram[256] = {0};
    histogram[50] = 5000;   // Dark peak
    histogram[200] = 5000;  // Light peak

    int darkPeak, lightPeak;
    bool bimodal = BlackOnWhiteDetector::detectBimodalPeaks(histogram, darkPeak, lightPeak);

    BOOST_CHECK(bimodal);
    BOOST_CHECK_EQUAL(darkPeak, 50);
    BOOST_CHECK_EQUAL(lightPeak, 200);
}

BOOST_AUTO_TEST_CASE(test_needs_inversion)
{
    // Black image should need inversion
    QImage blackImage(100, 100, QImage::Format_Grayscale8);
    blackImage.fill(Qt::black);

    // Add small white area
    for (int y = 40; y < 60; ++y) {
        uchar* line = blackImage.scanLine(y);
        for (int x = 40; x < 60; ++x) {
            line[x] = 255;
        }
    }

    BlackOnWhiteSettings settings;
    settings.mode = BlackOnWhiteSettings::Auto;
    settings.invertIfNeeded = true;

    bool needsInvert = BlackOnWhiteDetector::needsInversion(blackImage, settings);
    BOOST_CHECK(needsInvert);

    // Force normal mode should not need inversion
    settings.mode = BlackOnWhiteSettings::ForceNormal;
    needsInvert = BlackOnWhiteDetector::needsInversion(blackImage, settings);
    BOOST_CHECK(!needsInvert);
}

BOOST_AUTO_TEST_CASE(test_empty_image)
{
    QImage emptyImage;
    BlackOnWhiteResult result = BlackOnWhiteDetector::detect(emptyImage);

    BOOST_CHECK_EQUAL(result.type, BlackOnWhiteResult::Unknown);
}

BOOST_AUTO_TEST_SUITE_END()
