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

#include "GuidesManager.h"
#include <QSettings>
#include <QMutexLocker>
#include <cmath>

static const char* KEY_GUIDES_ENABLED = "guides/enabled";
static const char* KEY_SNAP_ENABLED = "guides/snapEnabled";
static const char* KEY_SNAP_DISTANCE = "guides/snapDistance";
static const char* KEY_GUIDE_COLOR = "guides/color";
static const char* KEY_SHOW_RULERS = "guides/showRulers";
static const char* KEY_GRID_VISIBLE = "guides/grid/visible";
static const char* KEY_GRID_H_SPACING = "guides/grid/hSpacing";
static const char* KEY_GRID_V_SPACING = "guides/grid/vSpacing";
static const char* KEY_GRID_COLOR = "guides/grid/color";
static const char* KEY_GRID_OPACITY = "guides/grid/opacity";
static const char* KEY_MARGINS_VISIBLE = "guides/margins/visible";
static const char* KEY_MARGINS_TOP = "guides/margins/top";
static const char* KEY_MARGINS_BOTTOM = "guides/margins/bottom";
static const char* KEY_MARGINS_LEFT = "guides/margins/left";
static const char* KEY_MARGINS_RIGHT = "guides/margins/right";

GuidesManager& GuidesManager::instance()
{
    static GuidesManager instance;
    return instance;
}

GuidesManager::GuidesManager()
{
    QSettings settings;
    loadFromSettings(settings);
}

GuidesManager::Settings GuidesManager::settings() const
{
    QMutexLocker lock(&m_mutex);
    return m_settings;
}

void GuidesManager::setSettings(const Settings& settings)
{
    QMutexLocker lock(&m_mutex);
    m_settings = settings;
    emit settingsChanged();
}

void GuidesManager::loadFromSettings(const QSettings& settings)
{
    QMutexLocker lock(&m_mutex);

    m_settings.guidesEnabled = settings.value(KEY_GUIDES_ENABLED, true).toBool();
    m_settings.snapEnabled = settings.value(KEY_SNAP_ENABLED, true).toBool();
    m_settings.snapDistance = settings.value(KEY_SNAP_DISTANCE, 10.0).toDouble();
    m_settings.guideColor = settings.value(KEY_GUIDE_COLOR, QColor(Qt::cyan)).value<QColor>();
    m_settings.showRulers = settings.value(KEY_SHOW_RULERS, true).toBool();

    m_settings.grid.visible = settings.value(KEY_GRID_VISIBLE, false).toBool();
    m_settings.grid.horizontalSpacing = settings.value(KEY_GRID_H_SPACING, 50.0).toDouble();
    m_settings.grid.verticalSpacing = settings.value(KEY_GRID_V_SPACING, 50.0).toDouble();
    m_settings.grid.color = settings.value(KEY_GRID_COLOR, QColor(128, 128, 128, 80)).value<QColor>();
    m_settings.grid.opacity = settings.value(KEY_GRID_OPACITY, 0.5).toDouble();

    m_settings.margins.visible = settings.value(KEY_MARGINS_VISIBLE, false).toBool();
    m_settings.margins.top = settings.value(KEY_MARGINS_TOP, 10.0).toDouble();
    m_settings.margins.bottom = settings.value(KEY_MARGINS_BOTTOM, 10.0).toDouble();
    m_settings.margins.left = settings.value(KEY_MARGINS_LEFT, 10.0).toDouble();
    m_settings.margins.right = settings.value(KEY_MARGINS_RIGHT, 10.0).toDouble();
}

void GuidesManager::saveToSettings(QSettings& settings) const
{
    QMutexLocker lock(&m_mutex);

    settings.setValue(KEY_GUIDES_ENABLED, m_settings.guidesEnabled);
    settings.setValue(KEY_SNAP_ENABLED, m_settings.snapEnabled);
    settings.setValue(KEY_SNAP_DISTANCE, m_settings.snapDistance);
    settings.setValue(KEY_GUIDE_COLOR, m_settings.guideColor);
    settings.setValue(KEY_SHOW_RULERS, m_settings.showRulers);

    settings.setValue(KEY_GRID_VISIBLE, m_settings.grid.visible);
    settings.setValue(KEY_GRID_H_SPACING, m_settings.grid.horizontalSpacing);
    settings.setValue(KEY_GRID_V_SPACING, m_settings.grid.verticalSpacing);
    settings.setValue(KEY_GRID_COLOR, m_settings.grid.color);
    settings.setValue(KEY_GRID_OPACITY, m_settings.grid.opacity);

    settings.setValue(KEY_MARGINS_VISIBLE, m_settings.margins.visible);
    settings.setValue(KEY_MARGINS_TOP, m_settings.margins.top);
    settings.setValue(KEY_MARGINS_BOTTOM, m_settings.margins.bottom);
    settings.setValue(KEY_MARGINS_LEFT, m_settings.margins.left);
    settings.setValue(KEY_MARGINS_RIGHT, m_settings.margins.right);
}

void GuidesManager::addGuide(Guide::Type type, double position)
{
    QMutexLocker lock(&m_mutex);
    Guide g(type, position);
    g.color = m_settings.guideColor;
    m_guides.append(g);
    emit guidesChanged();
}

void GuidesManager::removeGuide(int index)
{
    QMutexLocker lock(&m_mutex);
    if (index >= 0 && index < m_guides.size()) {
        m_guides.remove(index);
        emit guidesChanged();
    }
}

void GuidesManager::moveGuide(int index, double newPosition)
{
    QMutexLocker lock(&m_mutex);
    if (index >= 0 && index < m_guides.size() && !m_guides[index].locked) {
        m_guides[index].position = newPosition;
        emit guidesChanged();
    }
}

void GuidesManager::clearGuides()
{
    QMutexLocker lock(&m_mutex);
    m_guides.clear();
    emit guidesChanged();
}

void GuidesManager::clearGuides(Guide::Type type)
{
    QMutexLocker lock(&m_mutex);
    for (int i = m_guides.size() - 1; i >= 0; --i) {
        if (m_guides[i].type == type) {
            m_guides.remove(i);
        }
    }
    emit guidesChanged();
}

void GuidesManager::lockGuide(int index, bool locked)
{
    QMutexLocker lock(&m_mutex);
    if (index >= 0 && index < m_guides.size()) {
        m_guides[index].locked = locked;
        emit guidesChanged();
    }
}

int GuidesManager::guideCount() const
{
    QMutexLocker lock(&m_mutex);
    return m_guides.size();
}

const Guide& GuidesManager::guide(int index) const
{
    QMutexLocker lock(&m_mutex);
    return m_guides[index];
}

QVector<Guide> GuidesManager::guides() const
{
    QMutexLocker lock(&m_mutex);
    return m_guides;
}

QVector<Guide> GuidesManager::guides(Guide::Type type) const
{
    QMutexLocker lock(&m_mutex);
    QVector<Guide> result;
    for (const Guide& g : m_guides) {
        if (g.type == type) {
            result.append(g);
        }
    }
    return result;
}

void GuidesManager::setGridVisible(bool visible)
{
    QMutexLocker lock(&m_mutex);
    m_settings.grid.visible = visible;
    emit settingsChanged();
}

void GuidesManager::setGridSpacing(double horizontal, double vertical)
{
    QMutexLocker lock(&m_mutex);
    m_settings.grid.horizontalSpacing = horizontal;
    m_settings.grid.verticalSpacing = vertical;
    emit settingsChanged();
}

bool GuidesManager::isGridVisible() const
{
    QMutexLocker lock(&m_mutex);
    return m_settings.grid.visible;
}

GridSettings GuidesManager::gridSettings() const
{
    QMutexLocker lock(&m_mutex);
    return m_settings.grid;
}

void GuidesManager::setGridSettings(const GridSettings& settings)
{
    QMutexLocker lock(&m_mutex);
    m_settings.grid = settings;
    emit settingsChanged();
}

void GuidesManager::setMarginGuidesVisible(bool visible)
{
    QMutexLocker lock(&m_mutex);
    m_settings.margins.visible = visible;
    emit settingsChanged();
}

void GuidesManager::setMarginGuides(double top, double bottom, double left, double right)
{
    QMutexLocker lock(&m_mutex);
    m_settings.margins.top = top;
    m_settings.margins.bottom = bottom;
    m_settings.margins.left = left;
    m_settings.margins.right = right;
    emit settingsChanged();
}

bool GuidesManager::areMarginGuidesVisible() const
{
    QMutexLocker lock(&m_mutex);
    return m_settings.margins.visible;
}

MarginGuides GuidesManager::marginGuides() const
{
    QMutexLocker lock(&m_mutex);
    return m_settings.margins;
}

void GuidesManager::setMarginGuidesSettings(const MarginGuides& settings)
{
    QMutexLocker lock(&m_mutex);
    m_settings.margins = settings;
    emit settingsChanged();
}

QPointF GuidesManager::snapPoint(const QPointF& point, const QRectF& bounds) const
{
    QMutexLocker lock(&m_mutex);

    if (!m_settings.snapEnabled) {
        return point;
    }

    double x = point.x();
    double y = point.y();

    // Snap to guides
    double minDistX = m_settings.snapDistance;
    double minDistY = m_settings.snapDistance;

    for (const Guide& g : m_guides) {
        if (g.type == Guide::Vertical) {
            double dist = std::abs(point.x() - g.position);
            if (dist < minDistX) {
                minDistX = dist;
                x = g.position;
            }
        } else {
            double dist = std::abs(point.y() - g.position);
            if (dist < minDistY) {
                minDistY = dist;
                y = g.position;
            }
        }
    }

    // Snap to grid if enabled
    if (m_settings.grid.visible) {
        double gridX = std::round(point.x() / m_settings.grid.verticalSpacing) * m_settings.grid.verticalSpacing;
        double gridY = std::round(point.y() / m_settings.grid.horizontalSpacing) * m_settings.grid.horizontalSpacing;

        if (std::abs(point.x() - gridX) < m_settings.snapDistance && std::abs(point.x() - gridX) < minDistX) {
            x = gridX;
        }
        if (std::abs(point.y() - gridY) < m_settings.snapDistance && std::abs(point.y() - gridY) < minDistY) {
            y = gridY;
        }
    }

    // Snap to bounds edges
    if (std::abs(point.x() - bounds.left()) < m_settings.snapDistance) x = bounds.left();
    if (std::abs(point.x() - bounds.right()) < m_settings.snapDistance) x = bounds.right();
    if (std::abs(point.y() - bounds.top()) < m_settings.snapDistance) y = bounds.top();
    if (std::abs(point.y() - bounds.bottom()) < m_settings.snapDistance) y = bounds.bottom();

    // Snap to center
    double centerX = bounds.center().x();
    double centerY = bounds.center().y();
    if (std::abs(point.x() - centerX) < m_settings.snapDistance) x = centerX;
    if (std::abs(point.y() - centerY) < m_settings.snapDistance) y = centerY;

    return QPointF(x, y);
}

double GuidesManager::snapToGuide(double value, Guide::Type type) const
{
    QMutexLocker lock(&m_mutex);

    if (!m_settings.snapEnabled) {
        return value;
    }

    for (const Guide& g : m_guides) {
        if (g.type == type) {
            if (std::abs(value - g.position) < m_settings.snapDistance) {
                return g.position;
            }
        }
    }
    return value;
}

double GuidesManager::snapToGrid(double value, bool horizontal) const
{
    QMutexLocker lock(&m_mutex);

    if (!m_settings.snapEnabled || !m_settings.grid.visible) {
        return value;
    }

    double spacing = horizontal ? m_settings.grid.horizontalSpacing : m_settings.grid.verticalSpacing;
    double snapped = std::round(value / spacing) * spacing;

    if (std::abs(value - snapped) < m_settings.snapDistance) {
        return snapped;
    }
    return value;
}

bool GuidesManager::isNearGuide(const QPointF& point, const QRectF& bounds, int* outGuideIndex) const
{
    QMutexLocker lock(&m_mutex);

    for (int i = 0; i < m_guides.size(); ++i) {
        const Guide& g = m_guides[i];
        double dist = (g.type == Guide::Horizontal)
            ? std::abs(point.y() - g.position)
            : std::abs(point.x() - g.position);

        if (dist < m_settings.snapDistance) {
            QLineF line = g.toLine(bounds);
            // Check if point is within bounds of guide
            if (g.type == Guide::Horizontal) {
                if (point.x() >= line.x1() && point.x() <= line.x2()) {
                    if (outGuideIndex) *outGuideIndex = i;
                    return true;
                }
            } else {
                if (point.y() >= line.y1() && point.y() <= line.y2()) {
                    if (outGuideIndex) *outGuideIndex = i;
                    return true;
                }
            }
        }
    }

    if (outGuideIndex) *outGuideIndex = -1;
    return false;
}

void GuidesManager::drawGuides(QPainter& painter, const QRectF& bounds, const QTransform& transform) const
{
    QMutexLocker lock(&m_mutex);

    if (!m_settings.guidesEnabled) {
        return;
    }

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen;
    pen.setWidth(1);
    pen.setStyle(Qt::DashLine);
    pen.setCosmetic(true);

    for (const Guide& g : m_guides) {
        pen.setColor(g.color);
        painter.setPen(pen);

        QLineF line = g.toLine(bounds);
        QLineF screenLine(transform.map(line.p1()), transform.map(line.p2()));
        painter.drawLine(screenLine);
    }

    painter.restore();
}

void GuidesManager::drawGrid(QPainter& painter, const QRectF& bounds, const QTransform& transform) const
{
    QMutexLocker lock(&m_mutex);

    if (!m_settings.grid.visible) {
        return;
    }

    painter.save();
    painter.setOpacity(m_settings.grid.opacity);

    QPen pen(m_settings.grid.color);
    pen.setWidth(1);
    pen.setStyle(static_cast<Qt::PenStyle>(m_settings.grid.lineStyle));
    pen.setCosmetic(true);
    painter.setPen(pen);

    // Draw vertical lines
    for (double x = bounds.left(); x <= bounds.right(); x += m_settings.grid.verticalSpacing) {
        QLineF line(x, bounds.top(), x, bounds.bottom());
        QLineF screenLine(transform.map(line.p1()), transform.map(line.p2()));
        painter.drawLine(screenLine);
    }

    // Draw horizontal lines
    for (double y = bounds.top(); y <= bounds.bottom(); y += m_settings.grid.horizontalSpacing) {
        QLineF line(bounds.left(), y, bounds.right(), y);
        QLineF screenLine(transform.map(line.p1()), transform.map(line.p2()));
        painter.drawLine(screenLine);
    }

    painter.restore();
}

void GuidesManager::drawMarginGuides(QPainter& painter, const QRectF& bounds, const QTransform& transform) const
{
    QMutexLocker lock(&m_mutex);

    if (!m_settings.margins.visible) {
        return;
    }

    painter.save();

    QPen pen(m_settings.margins.color);
    pen.setWidth(1);
    pen.setStyle(Qt::DashDotLine);
    pen.setCosmetic(true);
    painter.setPen(pen);

    QRectF innerRect = m_settings.margins.innerRect(bounds);
    QRectF screenRect(transform.map(innerRect.topLeft()), transform.map(innerRect.bottomRight()));
    painter.drawRect(screenRect);

    painter.restore();
}

void GuidesManager::drawRulers(QPainter& painter, const QRectF& viewRect, const QRectF& imageRect,
                               const QTransform& transform, double dpi) const
{
    QMutexLocker lock(&m_mutex);

    if (!m_settings.showRulers) {
        return;
    }

    painter.save();

    const int rulerHeight = 20;
    const QColor rulerBg(240, 240, 240);
    const QColor rulerFg(100, 100, 100);

    // Horizontal ruler
    painter.fillRect(0, 0, viewRect.width(), rulerHeight, rulerBg);
    painter.setPen(rulerFg);

    // Calculate tick spacing based on zoom level
    double scale = transform.m11();  // Assuming uniform scaling
    double pixelsPerMm = dpi / 25.4;
    double tickSpacing = 10.0 * pixelsPerMm * scale;  // 10mm ticks

    // Adjust tick spacing if too dense or sparse
    while (tickSpacing < 20) tickSpacing *= 2;
    while (tickSpacing > 100) tickSpacing /= 2;

    QPointF imageTopLeft = transform.map(imageRect.topLeft());

    // Draw horizontal ticks
    for (double x = imageTopLeft.x(); x < viewRect.right(); x += tickSpacing) {
        if (x >= rulerHeight) {
            painter.drawLine(QPointF(x, rulerHeight - 5), QPointF(x, rulerHeight));
        }
    }
    for (double x = imageTopLeft.x(); x > rulerHeight; x -= tickSpacing) {
        painter.drawLine(QPointF(x, rulerHeight - 5), QPointF(x, rulerHeight));
    }

    // Vertical ruler
    painter.fillRect(0, rulerHeight, rulerHeight, viewRect.height() - rulerHeight, rulerBg);

    // Draw vertical ticks
    for (double y = imageTopLeft.y(); y < viewRect.bottom(); y += tickSpacing) {
        if (y >= rulerHeight) {
            painter.drawLine(QPointF(rulerHeight - 5, y), QPointF(rulerHeight, y));
        }
    }
    for (double y = imageTopLeft.y(); y > rulerHeight; y -= tickSpacing) {
        painter.drawLine(QPointF(rulerHeight - 5, y), QPointF(rulerHeight, y));
    }

    // Draw corner square
    painter.fillRect(0, 0, rulerHeight, rulerHeight, rulerBg);
    painter.drawRect(0, 0, rulerHeight, rulerHeight);

    painter.restore();
}

void GuidesManager::centerGuidesOn(const QRectF& rect)
{
    QMutexLocker lock(&m_mutex);
    addGuide(Guide::Horizontal, rect.center().y());
    addGuide(Guide::Vertical, rect.center().x());
    emit guidesChanged();
}

void GuidesManager::addCenterGuides(const QRectF& bounds)
{
    addGuide(Guide::Horizontal, bounds.center().y());
    addGuide(Guide::Vertical, bounds.center().x());
}

void GuidesManager::addThirdsGuides(const QRectF& bounds)
{
    // Rule of thirds
    addGuide(Guide::Horizontal, bounds.top() + bounds.height() / 3.0);
    addGuide(Guide::Horizontal, bounds.top() + 2.0 * bounds.height() / 3.0);
    addGuide(Guide::Vertical, bounds.left() + bounds.width() / 3.0);
    addGuide(Guide::Vertical, bounds.left() + 2.0 * bounds.width() / 3.0);
}

void GuidesManager::addGoldenRatioGuides(const QRectF& bounds)
{
    const double phi = 1.6180339887;  // Golden ratio

    addGuide(Guide::Horizontal, bounds.top() + bounds.height() / phi);
    addGuide(Guide::Horizontal, bounds.bottom() - bounds.height() / phi);
    addGuide(Guide::Vertical, bounds.left() + bounds.width() / phi);
    addGuide(Guide::Vertical, bounds.right() - bounds.width() / phi);
}
