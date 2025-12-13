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

#include "VerticalHalfCorrection.h"
#include <QDomDocument>
#include <QImage>
#include <cmath>
#include <algorithm>
#include <functional>

namespace dewarping {

namespace {
// Local clamp helper for C++11/14 compatibility
template<typename T>
constexpr const T& clamp_val(const T& v, const T& lo, const T& hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}
}

VerticalHalfCorrection::VerticalHalfCorrection()
    : m_topStretch(1.0)
    , m_bottomStretch(1.0)
{
}

VerticalHalfCorrection::VerticalHalfCorrection(const QDomElement& el)
    : m_topStretch(1.0)
    , m_bottomStretch(1.0)
{
    m_settings.mode = static_cast<VerticalHalfSettings::Mode>(
        el.attribute("mode", "0").toInt());
    m_settings.topCorrectionStrength = el.attribute("topStrength", "1.0").toDouble();
    m_settings.bottomCorrectionStrength = el.attribute("bottomStrength", "1.0").toDouble();
    m_settings.splitPosition = el.attribute("splitPos", "0.5").toDouble();
    m_settings.blendWidth = el.attribute("blendWidth", "0.1").toDouble();
    m_settings.mirrorTopBottom = el.attribute("mirror", "0").toInt() != 0;
    m_topStretch = el.attribute("topStretch", "1.0").toDouble();
    m_bottomStretch = el.attribute("bottomStretch", "1.0").toDouble();
}

QDomElement VerticalHalfCorrection::toXml(QDomDocument& doc, const QString& name) const
{
    QDomElement el = doc.createElement(name);
    el.setAttribute("mode", static_cast<int>(m_settings.mode));
    el.setAttribute("topStrength", m_settings.topCorrectionStrength);
    el.setAttribute("bottomStrength", m_settings.bottomCorrectionStrength);
    el.setAttribute("splitPos", m_settings.splitPosition);
    el.setAttribute("blendWidth", m_settings.blendWidth);
    el.setAttribute("mirror", m_settings.mirrorTopBottom ? 1 : 0);
    el.setAttribute("topStretch", m_topStretch);
    el.setAttribute("bottomStretch", m_bottomStretch);
    return el;
}

VerticalHalfAnalysis VerticalHalfCorrection::analyze(const Curve& topCurve, const Curve& bottomCurve)
{
    VerticalHalfAnalysis result;

    if (topCurve.polyline().size() < 2 || bottomCurve.polyline().size() < 2) {
        return result;
    }

    // Calculate curvature for each curve
    auto calcCurvature = [](const std::vector<QPointF>& points) -> double {
        if (points.size() < 3) return 0;

        double totalCurvature = 0;
        for (size_t i = 1; i < points.size() - 1; ++i) {
            // Calculate angle change at this point
            QPointF v1 = points[i] - points[i-1];
            QPointF v2 = points[i+1] - points[i];

            double len1 = std::sqrt(v1.x() * v1.x() + v1.y() * v1.y());
            double len2 = std::sqrt(v2.x() * v2.x() + v2.y() * v2.y());

            if (len1 > 0 && len2 > 0) {
                double dot = (v1.x() * v2.x() + v1.y() * v2.y()) / (len1 * len2);
                double angle = std::acos(clamp_val(dot, -1.0, 1.0));
                totalCurvature += angle;
            }
        }
        return totalCurvature / (points.size() - 2);
    };

    // Calculate deviation from straight line
    auto calcDeviation = [](const std::vector<QPointF>& points) -> double {
        if (points.size() < 2) return 0;

        QPointF start = points.front();
        QPointF end = points.back();
        QPointF dir = end - start;
        double lineLen = std::sqrt(dir.x() * dir.x() + dir.y() * dir.y());
        if (lineLen < 1e-6) return 0;

        double maxDev = 0;
        for (const QPointF& p : points) {
            // Distance from point to line
            double t = ((p.x() - start.x()) * dir.x() + (p.y() - start.y()) * dir.y()) /
                      (lineLen * lineLen);
            QPointF proj(start.x() + t * dir.x(), start.y() + t * dir.y());
            double dx = p.x() - proj.x();
            double dy = p.y() - proj.y();
            double dev = std::sqrt(dx * dx + dy * dy);
            maxDev = std::max(maxDev, dev);
        }
        return maxDev / lineLen;  // Normalize by line length
    };

    result.topCurvature = calcDeviation(topCurve.polyline());
    result.bottomCurvature = calcDeviation(bottomCurve.polyline());
    result.asymmetry = std::abs(result.topCurvature - result.bottomCurvature);

    // Determine if correction is needed
    const double curvatureThreshold = 0.02;  // 2% deviation
    const double asymmetryThreshold = 0.01;  // 1% difference

    result.needsCorrection = (result.asymmetry > asymmetryThreshold) ||
                            (result.topCurvature > curvatureThreshold) ||
                            (result.bottomCurvature > curvatureThreshold);

    // Calculate suggested correction strengths
    if (result.topCurvature > result.bottomCurvature) {
        result.topCorrectionNeeded = result.topCurvature;
        result.bottomCorrectionNeeded = result.bottomCurvature * 0.5;
    } else {
        result.topCorrectionNeeded = result.topCurvature * 0.5;
        result.bottomCorrectionNeeded = result.bottomCurvature;
    }

    // Find optimal split position based on curvature distribution
    result.suggestedSplitPos = 0.5;  // Default to middle

    return result;
}

VerticalHalfAnalysis VerticalHalfCorrection::analyzeImage(const QImage& image)
{
    VerticalHalfAnalysis result;

    if (image.isNull()) {
        return result;
    }

    // This is a placeholder for more sophisticated image-based analysis
    // A full implementation would detect text lines or edges in each half
    // and calculate their curvature

    result.suggestedSplitPos = 0.5;
    result.needsCorrection = false;

    return result;
}

Curve VerticalHalfCorrection::correctTopCurve(const Curve& original) const
{
    if (m_settings.mode == VerticalHalfSettings::Disabled ||
        m_settings.mode == VerticalHalfSettings::BottomOnly) {
        return original;
    }

    if (m_settings.topCorrectionStrength <= 0) {
        return original;
    }

    // Apply correction by reducing curvature
    std::vector<QPointF> points = original.polyline();
    if (points.size() < 2) {
        return original;
    }

    // Straighten towards the line connecting endpoints
    QPointF start = points.front();
    QPointF end = points.back();
    double strength = m_settings.topCorrectionStrength;

    std::vector<QPointF> corrected;
    corrected.reserve(points.size());

    for (size_t i = 0; i < points.size(); ++i) {
        double t = static_cast<double>(i) / (points.size() - 1);
        QPointF linePoint(start.x() + t * (end.x() - start.x()),
                         start.y() + t * (end.y() - start.y()));

        // Blend between original and straightened
        QPointF blended(points[i].x() * (1 - strength) + linePoint.x() * strength,
                       points[i].y() * (1 - strength) + linePoint.y() * strength);
        corrected.push_back(blended);
    }

    return Curve(corrected);
}

Curve VerticalHalfCorrection::correctBottomCurve(const Curve& original) const
{
    if (m_settings.mode == VerticalHalfSettings::Disabled ||
        m_settings.mode == VerticalHalfSettings::TopOnly) {
        return original;
    }

    if (m_settings.bottomCorrectionStrength <= 0) {
        return original;
    }

    // Same correction logic as top curve
    std::vector<QPointF> points = original.polyline();
    if (points.size() < 2) {
        return original;
    }

    QPointF start = points.front();
    QPointF end = points.back();
    double strength = m_settings.bottomCorrectionStrength;

    std::vector<QPointF> corrected;
    corrected.reserve(points.size());

    for (size_t i = 0; i < points.size(); ++i) {
        double t = static_cast<double>(i) / (points.size() - 1);
        QPointF linePoint(start.x() + t * (end.x() - start.x()),
                         start.y() + t * (end.y() - start.y()));

        QPointF blended(points[i].x() * (1 - strength) + linePoint.x() * strength,
                       points[i].y() * (1 - strength) + linePoint.y() * strength);
        corrected.push_back(blended);
    }

    return Curve(corrected);
}

double VerticalHalfCorrection::blend(double topValue, double bottomValue, double yPosition) const
{
    double splitPos = m_settings.splitPosition;
    double blendWidth = m_settings.blendWidth;

    if (yPosition < splitPos - blendWidth / 2) {
        return topValue;
    } else if (yPosition > splitPos + blendWidth / 2) {
        return bottomValue;
    } else {
        // In blend zone - smooth interpolation
        double t = (yPosition - (splitPos - blendWidth / 2)) / blendWidth;
        // Smoothstep for nicer blending
        t = t * t * (3 - 2 * t);
        return topValue * (1 - t) + bottomValue * t;
    }
}

double VerticalHalfCorrection::correctionFactorAt(double yPosition) const
{
    if (m_settings.mode == VerticalHalfSettings::Disabled) {
        return 0;
    }

    if (yPosition < m_settings.splitPosition) {
        // Top half
        if (m_settings.mode == VerticalHalfSettings::BottomOnly) {
            return 0;
        }
        return m_settings.topCorrectionStrength;
    } else {
        // Bottom half
        if (m_settings.mode == VerticalHalfSettings::TopOnly) {
            return 0;
        }
        return m_settings.bottomCorrectionStrength;
    }
}

Curve VerticalHalfCorrection::generateMiddleCurve(const Curve& topCurve, const Curve& bottomCurve) const
{
    std::vector<QPointF> middlePoints = interpolatePoints(topCurve, bottomCurve, m_settings.splitPosition);
    return Curve(middlePoints);
}

void VerticalHalfCorrection::setVerticalStretch(double topStretch, double bottomStretch)
{
    m_topStretch = clamp_val(topStretch, 0.5, 2.0);
    m_bottomStretch = clamp_val(bottomStretch, 0.5, 2.0);
}

bool VerticalHalfCorrection::isEnabled() const
{
    return m_settings.mode != VerticalHalfSettings::Disabled;
}

void VerticalHalfCorrection::autoConfigureFromCurves(const Curve& topCurve, const Curve& bottomCurve)
{
    VerticalHalfAnalysis analysis = analyze(topCurve, bottomCurve);

    if (!analysis.needsCorrection) {
        m_settings.mode = VerticalHalfSettings::Disabled;
        return;
    }

    m_settings.mode = VerticalHalfSettings::Auto;
    m_settings.splitPosition = analysis.suggestedSplitPos;
    m_settings.topCorrectionStrength = std::min(1.0, analysis.topCorrectionNeeded * 10);
    m_settings.bottomCorrectionStrength = std::min(1.0, analysis.bottomCorrectionNeeded * 10);
}

double VerticalHalfCorrection::calculateCurvature(const Curve& curve) const
{
    const auto& points = curve.polyline();
    if (points.size() < 3) {
        return 0;
    }

    double totalCurvature = 0;
    for (size_t i = 1; i < points.size() - 1; ++i) {
        QPointF v1 = points[i] - points[i-1];
        QPointF v2 = points[i+1] - points[i];

        double cross = v1.x() * v2.y() - v1.y() * v2.x();
        double len1 = std::sqrt(v1.x() * v1.x() + v1.y() * v1.y());
        double len2 = std::sqrt(v2.x() * v2.x() + v2.y() * v2.y());

        if (len1 > 0 && len2 > 0) {
            totalCurvature += cross / (len1 * len2);
        }
    }

    return totalCurvature / (points.size() - 2);
}

std::vector<QPointF> VerticalHalfCorrection::interpolatePoints(const Curve& top, const Curve& bottom, double t) const
{
    const auto& topPts = top.polyline();
    const auto& botPts = bottom.polyline();

    if (topPts.empty() || botPts.empty()) {
        return {};
    }

    // Use the curve with more points as the reference
    size_t numPoints = std::max(topPts.size(), botPts.size());
    std::vector<QPointF> result;
    result.reserve(numPoints);

    for (size_t i = 0; i < numPoints; ++i) {
        double param = static_cast<double>(i) / (numPoints - 1);

        // Sample both curves at this parameter
        auto sampleCurve = [](const std::vector<QPointF>& pts, double p) -> QPointF {
            if (pts.empty()) return QPointF();
            if (pts.size() == 1) return pts[0];

            double idx = p * (pts.size() - 1);
            size_t i0 = static_cast<size_t>(idx);
            size_t i1 = std::min(i0 + 1, pts.size() - 1);
            double frac = idx - i0;

            return QPointF(
                pts[i0].x() * (1 - frac) + pts[i1].x() * frac,
                pts[i0].y() * (1 - frac) + pts[i1].y() * frac
            );
        };

        QPointF topPt = sampleCurve(topPts, param);
        QPointF botPt = sampleCurve(botPts, param);

        // Interpolate between top and bottom
        QPointF midPt(
            topPt.x() * (1 - t) + botPt.x() * t,
            topPt.y() * (1 - t) + botPt.y() * t
        );
        result.push_back(midPt);
    }

    return result;
}

} // namespace dewarping
