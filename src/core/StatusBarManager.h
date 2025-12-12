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

#ifndef STATUS_BAR_MANAGER_H_
#define STATUS_BAR_MANAGER_H_

#include <QObject>
#include <QString>
#include <QSize>
#include <QPointF>
#include <QRectF>
#include <memory>

class QStatusBar;
class QLabel;
class QProgressBar;

/**
 * @brief Information about the current page for status bar display
 */
struct StatusPageInfo {
    QString filename;
    int pageNumber;
    int totalPages;
    QSize imageSizePixels;
    QSizeF imageSizeMm;
    double dpi;
    int colorDepth;
    QString colorMode;  // "Color", "Grayscale", "B&W"

    StatusPageInfo()
        : pageNumber(0)
        , totalPages(0)
        , dpi(0)
        , colorDepth(0)
    {}
};

/**
 * @brief Information about mouse/cursor position
 */
struct CursorInfo {
    QPointF imagePosition;  // Position in image coordinates
    QPointF physicalPosition;  // Position in mm/inches
    bool valid;

    CursorInfo() : valid(false) {}
};

/**
 * @brief Information about current zoom level
 */
struct ZoomInfo {
    double zoomLevel;  // 1.0 = 100%
    QString zoomText;  // e.g., "100%", "Fit Width"
    bool fitToWindow;
    bool fitToWidth;

    ZoomInfo()
        : zoomLevel(1.0)
        , fitToWindow(false)
        , fitToWidth(false)
    {}
};

/**
 * @brief Information about selection/content area
 */
struct SelectionInfo {
    QRectF rect;  // In image coordinates
    QSizeF physicalSize;  // In mm
    bool valid;

    SelectionInfo() : valid(false) {}
};

/**
 * @brief Processing progress information
 */
struct ProcessingInfo {
    bool processing;
    int currentPage;
    int totalPages;
    double progress;  // 0.0 to 1.0
    QString stage;
    QString message;

    ProcessingInfo()
        : processing(false)
        , currentPage(0)
        , totalPages(0)
        , progress(0)
    {}
};

/**
 * @brief Manages enhanced status bar with detailed page information
 *
 * Displays:
 * - Current page number and filename
 * - Image dimensions and DPI
 * - Mouse/cursor position in multiple units
 * - Zoom level
 * - Selection size
 * - Processing progress
 * - Memory usage
 */
class StatusBarManager : public QObject
{
    Q_OBJECT

public:
    explicit StatusBarManager(QStatusBar* statusBar, QObject* parent = nullptr);
    ~StatusBarManager();

    // Update individual components
    void setPageInfo(const StatusPageInfo& info);
    void setCursorInfo(const CursorInfo& info);
    void setZoomInfo(const ZoomInfo& info);
    void setSelectionInfo(const SelectionInfo& info);
    void setProcessingInfo(const ProcessingInfo& info);

    // Convenience methods
    void setCurrentPage(int current, int total);
    void setImageSize(int width, int height, double dpi);
    void setCursorPosition(const QPointF& imagePos, double dpi);
    void setZoomLevel(double zoom);
    void setZoomFitMode(bool fitToWindow, bool fitToWidth);
    void setSelectionRect(const QRectF& rect, double dpi);
    void clearSelection();

    // Processing state
    void startProcessing(const QString& stage, int totalPages);
    void updateProgress(int currentPage, const QString& message = QString());
    void finishProcessing();

    // Memory display
    void updateMemoryUsage();

    // Message display
    void showMessage(const QString& message, int timeout = 3000);
    void showPermanentMessage(const QString& message);
    void clearMessage();

    // Unit settings
    enum Units { Pixels, Millimeters, Centimeters, Inches };
    void setDisplayUnits(Units units);
    Units displayUnits() const;

public slots:
    void refresh();

signals:
    void unitsChanged(Units newUnits);

private:
    void setupWidgets();
    void updatePageLabel();
    void updateCursorLabel();
    void updateZoomLabel();
    void updateSelectionLabel();
    void updateMemoryLabel();
    QString formatSize(double value) const;
    QString formatPosition(double x, double y) const;

    QStatusBar* m_statusBar;
    QLabel* m_pageLabel;
    QLabel* m_sizeLabel;
    QLabel* m_cursorLabel;
    QLabel* m_zoomLabel;
    QLabel* m_selectionLabel;
    QLabel* m_memoryLabel;
    QProgressBar* m_progressBar;

    StatusPageInfo m_pageInfo;
    CursorInfo m_cursorInfo;
    ZoomInfo m_zoomInfo;
    SelectionInfo m_selectionInfo;
    ProcessingInfo m_processingInfo;
    Units m_units;
};

#endif // STATUS_BAR_MANAGER_H_
