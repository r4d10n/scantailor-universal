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

#ifndef DEWARPING_VERTICAL_HALF_CORRECTION_H_
#define DEWARPING_VERTICAL_HALF_CORRECTION_H_

#include "Curve.h"
#include <QDomElement>
#include <QPointF>
#include <vector>

class QDomDocument;
class QImage;

namespace dewarping {

/**
 * @brief Settings for vertical half correction
 */
struct VerticalHalfSettings {
    enum Mode {
        Disabled,           // No vertical half correction
        Auto,               // Automatically detect and correct
        Manual,             // Use manually specified correction
        TopOnly,            // Only correct top half
        BottomOnly          // Only correct bottom half
    };

    Mode mode;
    double topCorrectionStrength;    // 0.0 - 1.0
    double bottomCorrectionStrength; // 0.0 - 1.0
    double splitPosition;            // 0.0 - 1.0 (0.5 = middle)
    double blendWidth;               // Width of blend zone (0.0 - 0.5)
    bool mirrorTopBottom;            // Use top curve mirrored for bottom

    VerticalHalfSettings()
        : mode(Disabled)
        , topCorrectionStrength(1.0)
        , bottomCorrectionStrength(1.0)
        , splitPosition(0.5)
        , blendWidth(0.1)
        , mirrorTopBottom(false)
    {}
};

/**
 * @brief Analysis result for vertical half correction
 */
struct VerticalHalfAnalysis {
    double topCurvature;       // Average curvature of top half
    double bottomCurvature;    // Average curvature of bottom half
    double asymmetry;          // Difference between top and bottom (0 = symmetric)
    bool needsCorrection;
    double suggestedSplitPos;
    double topCorrectionNeeded;
    double bottomCorrectionNeeded;

    VerticalHalfAnalysis()
        : topCurvature(0)
        , bottomCurvature(0)
        , asymmetry(0)
        , needsCorrection(false)
        , suggestedSplitPos(0.5)
        , topCorrectionNeeded(0)
        , bottomCorrectionNeeded(0)
    {}
};

/**
 * @brief Provides vertical half correction for dewarping
 *
 * Allows independent dewarping correction for the top and bottom
 * halves of a page. This is useful when:
 * - The book binding causes different warping in different areas
 * - The page has asymmetric distortion
 * - Different curvatures need to be applied to top vs bottom
 */
class VerticalHalfCorrection
{
public:
    VerticalHalfCorrection();
    explicit VerticalHalfCorrection(const QDomElement& el);

    QDomElement toXml(QDomDocument& doc, const QString& name) const;

    // Settings
    VerticalHalfSettings settings() const { return m_settings; }
    void setSettings(const VerticalHalfSettings& settings) { m_settings = settings; }

    // Analysis
    static VerticalHalfAnalysis analyze(const Curve& topCurve, const Curve& bottomCurve);
    static VerticalHalfAnalysis analyzeImage(const QImage& image);

    // Curve modification
    Curve correctTopCurve(const Curve& original) const;
    Curve correctBottomCurve(const Curve& original) const;

    /**
     * @brief Apply blending between top and bottom corrections
     * @param topValue Value from top half correction
     * @param bottomValue Value from bottom half correction
     * @param yPosition Normalized Y position (0 = top, 1 = bottom)
     * @return Blended value
     */
    double blend(double topValue, double bottomValue, double yPosition) const;

    /**
     * @brief Calculate correction factor for a given Y position
     * @param yPosition Normalized Y position (0 = top, 1 = bottom)
     * @return Correction factor (0 = no correction, 1 = full correction)
     */
    double correctionFactorAt(double yPosition) const;

    /**
     * @brief Generate middle curve for split dewarping
     *
     * Creates an intermediate curve at the split position by
     * interpolating between top and bottom curves.
     *
     * @param topCurve The top boundary curve
     * @param bottomCurve The bottom boundary curve
     * @return Interpolated middle curve
     */
    Curve generateMiddleCurve(const Curve& topCurve, const Curve& bottomCurve) const;

    /**
     * @brief Apply vertical scaling correction
     *
     * Adjusts the vertical stretch/compression in each half independently.
     *
     * @param topStretch Stretch factor for top half (1.0 = no change)
     * @param bottomStretch Stretch factor for bottom half
     */
    void setVerticalStretch(double topStretch, double bottomStretch);

    double topVerticalStretch() const { return m_topStretch; }
    double bottomVerticalStretch() const { return m_bottomStretch; }

    /**
     * @brief Check if correction is enabled and configured
     */
    bool isEnabled() const;

    /**
     * @brief Auto-configure based on curve analysis
     */
    void autoConfigureFromCurves(const Curve& topCurve, const Curve& bottomCurve);

private:
    double calculateCurvature(const Curve& curve) const;
    std::vector<QPointF> interpolatePoints(const Curve& top, const Curve& bottom, double t) const;

    VerticalHalfSettings m_settings;
    double m_topStretch;
    double m_bottomStretch;
};

} // namespace dewarping

#endif // DEWARPING_VERTICAL_HALF_CORRECTION_H_
