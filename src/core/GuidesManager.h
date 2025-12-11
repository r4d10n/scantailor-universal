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

#ifndef GUIDES_MANAGER_H_
#define GUIDES_MANAGER_H_

#include <QObject>
#include <QVector>
#include <QPointF>
#include <QLineF>
#include <QRectF>
#include <QColor>
#include <QPainter>
#include <QMutex>
#include <memory>

class QSettings;
class QWidget;

/**
 * @brief Represents a single guide line (horizontal or vertical)
 */
struct Guide {
    enum Type {
        Horizontal,
        Vertical
    };

    Type type;
    double position;  // Y position for horizontal, X position for vertical
    bool locked;
    QColor color;

    Guide(Type t = Horizontal, double pos = 0)
        : type(t), position(pos), locked(false), color(Qt::cyan) {}

    QLineF toLine(const QRectF& bounds) const {
        if (type == Horizontal) {
            return QLineF(bounds.left(), position, bounds.right(), position);
        } else {
            return QLineF(position, bounds.top(), position, bounds.bottom());
        }
    }
};

/**
 * @brief Grid settings for overlay display
 */
struct GridSettings {
    bool visible;
    double horizontalSpacing;
    double verticalSpacing;
    QColor color;
    int lineStyle;  // Qt::PenStyle
    double opacity;

    GridSettings()
        : visible(false)
        , horizontalSpacing(50.0)
        , verticalSpacing(50.0)
        , color(QColor(128, 128, 128, 80))
        , lineStyle(Qt::DotLine)
        , opacity(0.5)
    {}
};

/**
 * @brief Settings for margin guides
 */
struct MarginGuides {
    bool visible;
    double top;
    double bottom;
    double left;
    double right;
    QColor color;

    MarginGuides()
        : visible(false)
        , top(10.0)
        , bottom(10.0)
        , left(10.0)
        , right(10.0)
        , color(QColor(255, 165, 0, 150))  // Orange
    {}

    QRectF innerRect(const QRectF& bounds) const {
        return QRectF(
            bounds.left() + left,
            bounds.top() + top,
            bounds.width() - left - right,
            bounds.height() - top - bottom
        );
    }
};

/**
 * @brief Manages positioning guides for precise alignment
 *
 * Provides horizontal/vertical guide lines, grid overlay,
 * margin guides, and snap-to functionality.
 */
class GuidesManager : public QObject
{
    Q_OBJECT

public:
    struct Settings {
        bool guidesEnabled;
        bool snapEnabled;
        double snapDistance;  // Pixels
        QColor guideColor;
        QColor activeGuideColor;
        bool showRulers;
        GridSettings grid;
        MarginGuides margins;

        Settings()
            : guidesEnabled(true)
            , snapEnabled(true)
            , snapDistance(10.0)
            , guideColor(Qt::cyan)
            , activeGuideColor(Qt::magenta)
            , showRulers(true)
        {}
    };

    static GuidesManager& instance();

    // Settings
    Settings settings() const;
    void setSettings(const Settings& settings);
    void loadFromSettings(const QSettings& settings);
    void saveToSettings(QSettings& settings) const;

    // Guide management
    void addGuide(Guide::Type type, double position);
    void removeGuide(int index);
    void moveGuide(int index, double newPosition);
    void clearGuides();
    void clearGuides(Guide::Type type);
    void lockGuide(int index, bool locked);

    int guideCount() const;
    const Guide& guide(int index) const;
    QVector<Guide> guides() const;
    QVector<Guide> guides(Guide::Type type) const;

    // Grid management
    void setGridVisible(bool visible);
    void setGridSpacing(double horizontal, double vertical);
    bool isGridVisible() const;
    GridSettings gridSettings() const;
    void setGridSettings(const GridSettings& settings);

    // Margin guides
    void setMarginGuidesVisible(bool visible);
    void setMarginGuides(double top, double bottom, double left, double right);
    bool areMarginGuidesVisible() const;
    MarginGuides marginGuides() const;
    void setMarginGuidesSettings(const MarginGuides& settings);

    // Snap functionality
    QPointF snapPoint(const QPointF& point, const QRectF& bounds) const;
    double snapToGuide(double value, Guide::Type type) const;
    double snapToGrid(double value, bool horizontal) const;
    bool isNearGuide(const QPointF& point, const QRectF& bounds, int* outGuideIndex = nullptr) const;

    // Drawing
    void drawGuides(QPainter& painter, const QRectF& bounds, const QTransform& transform) const;
    void drawGrid(QPainter& painter, const QRectF& bounds, const QTransform& transform) const;
    void drawMarginGuides(QPainter& painter, const QRectF& bounds, const QTransform& transform) const;
    void drawRulers(QPainter& painter, const QRectF& viewRect, const QRectF& imageRect,
                    const QTransform& transform, double dpi) const;

    // Utility
    void centerGuidesOn(const QRectF& rect);
    void addCenterGuides(const QRectF& bounds);
    void addThirdsGuides(const QRectF& bounds);
    void addGoldenRatioGuides(const QRectF& bounds);

signals:
    void guidesChanged();
    void settingsChanged();

private:
    GuidesManager();
    ~GuidesManager() = default;
    GuidesManager(const GuidesManager&) = delete;
    GuidesManager& operator=(const GuidesManager&) = delete;

    mutable QMutex m_mutex;
    Settings m_settings;
    QVector<Guide> m_guides;
};

#endif // GUIDES_MANAGER_H_
