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

#include "StatusBarManager.h"
#include <QStatusBar>
#include <QLabel>
#include <QProgressBar>
#include <QFrame>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#elif defined(Q_OS_LINUX)
#include <sys/resource.h>
#include <unistd.h>
#include <fstream>
#endif

static const double MM_PER_INCH = 25.4;

StatusBarManager::StatusBarManager(QStatusBar* statusBar, QObject* parent)
    : QObject(parent)
    , m_statusBar(statusBar)
    , m_pageLabel(nullptr)
    , m_sizeLabel(nullptr)
    , m_cursorLabel(nullptr)
    , m_zoomLabel(nullptr)
    , m_selectionLabel(nullptr)
    , m_memoryLabel(nullptr)
    , m_progressBar(nullptr)
    , m_units(Pixels)
{
    setupWidgets();

    // Periodic memory update
    QTimer* memTimer = new QTimer(this);
    connect(memTimer, &QTimer::timeout, this, &StatusBarManager::updateMemoryUsage);
    memTimer->start(5000);  // Update every 5 seconds
}

StatusBarManager::~StatusBarManager()
{
}

void StatusBarManager::setupWidgets()
{
    // Page info label
    m_pageLabel = new QLabel();
    m_pageLabel->setMinimumWidth(150);
    m_pageLabel->setFrameStyle(QFrame::NoFrame);
    m_statusBar->addWidget(m_pageLabel);

    // Image size label
    m_sizeLabel = new QLabel();
    m_sizeLabel->setMinimumWidth(180);
    m_sizeLabel->setFrameStyle(QFrame::NoFrame);
    m_statusBar->addWidget(m_sizeLabel);

    // Cursor position label
    m_cursorLabel = new QLabel();
    m_cursorLabel->setMinimumWidth(140);
    m_cursorLabel->setFrameStyle(QFrame::NoFrame);
    m_statusBar->addWidget(m_cursorLabel);

    // Zoom label
    m_zoomLabel = new QLabel();
    m_zoomLabel->setMinimumWidth(80);
    m_zoomLabel->setFrameStyle(QFrame::NoFrame);
    m_statusBar->addWidget(m_zoomLabel);

    // Selection label
    m_selectionLabel = new QLabel();
    m_selectionLabel->setMinimumWidth(120);
    m_selectionLabel->setFrameStyle(QFrame::NoFrame);
    m_statusBar->addWidget(m_selectionLabel);

    // Progress bar (hidden by default)
    m_progressBar = new QProgressBar();
    m_progressBar->setMinimumWidth(150);
    m_progressBar->setMaximumWidth(200);
    m_progressBar->setVisible(false);
    m_statusBar->addWidget(m_progressBar);

    // Memory label (permanent widget on right)
    m_memoryLabel = new QLabel();
    m_memoryLabel->setMinimumWidth(100);
    m_memoryLabel->setFrameStyle(QFrame::NoFrame);
    m_statusBar->addPermanentWidget(m_memoryLabel);

    updateMemoryUsage();
}

void StatusBarManager::setPageInfo(const StatusPageInfo& info)
{
    m_pageInfo = info;
    updatePageLabel();
}

void StatusBarManager::setCursorInfo(const CursorInfo& info)
{
    m_cursorInfo = info;
    updateCursorLabel();
}

void StatusBarManager::setZoomInfo(const ZoomInfo& info)
{
    m_zoomInfo = info;
    updateZoomLabel();
}

void StatusBarManager::setSelectionInfo(const SelectionInfo& info)
{
    m_selectionInfo = info;
    updateSelectionLabel();
}

void StatusBarManager::setProcessingInfo(const ProcessingInfo& info)
{
    m_processingInfo = info;

    if (info.processing) {
        m_progressBar->setVisible(true);
        m_progressBar->setValue(static_cast<int>(info.progress * 100));
        m_progressBar->setFormat(QString("%1/%2").arg(info.currentPage).arg(info.totalPages));
    } else {
        m_progressBar->setVisible(false);
    }
}

void StatusBarManager::setCurrentPage(int current, int total)
{
    m_pageInfo.pageNumber = current;
    m_pageInfo.totalPages = total;
    updatePageLabel();
}

void StatusBarManager::setImageSize(int width, int height, double dpi)
{
    m_pageInfo.imageSizePixels = QSize(width, height);
    m_pageInfo.dpi = dpi;
    if (dpi > 0) {
        m_pageInfo.imageSizeMm = QSizeF(
            width / dpi * MM_PER_INCH,
            height / dpi * MM_PER_INCH
        );
    }
    updatePageLabel();
}

void StatusBarManager::setCursorPosition(const QPointF& imagePos, double dpi)
{
    m_cursorInfo.imagePosition = imagePos;
    m_cursorInfo.valid = true;
    if (dpi > 0) {
        m_cursorInfo.physicalPosition = QPointF(
            imagePos.x() / dpi * MM_PER_INCH,
            imagePos.y() / dpi * MM_PER_INCH
        );
    }
    updateCursorLabel();
}

void StatusBarManager::setZoomLevel(double zoom)
{
    m_zoomInfo.zoomLevel = zoom;
    m_zoomInfo.fitToWindow = false;
    m_zoomInfo.fitToWidth = false;
    m_zoomInfo.zoomText = QString("%1%").arg(static_cast<int>(zoom * 100));
    updateZoomLabel();
}

void StatusBarManager::setZoomFitMode(bool fitToWindow, bool fitToWidth)
{
    m_zoomInfo.fitToWindow = fitToWindow;
    m_zoomInfo.fitToWidth = fitToWidth;
    if (fitToWindow) {
        m_zoomInfo.zoomText = tr("Fit");
    } else if (fitToWidth) {
        m_zoomInfo.zoomText = tr("Fit Width");
    }
    updateZoomLabel();
}

void StatusBarManager::setSelectionRect(const QRectF& rect, double dpi)
{
    m_selectionInfo.rect = rect;
    m_selectionInfo.valid = true;
    if (dpi > 0) {
        m_selectionInfo.physicalSize = QSizeF(
            rect.width() / dpi * MM_PER_INCH,
            rect.height() / dpi * MM_PER_INCH
        );
    }
    updateSelectionLabel();
}

void StatusBarManager::clearSelection()
{
    m_selectionInfo.valid = false;
    updateSelectionLabel();
}

void StatusBarManager::startProcessing(const QString& stage, int totalPages)
{
    m_processingInfo.processing = true;
    m_processingInfo.stage = stage;
    m_processingInfo.totalPages = totalPages;
    m_processingInfo.currentPage = 0;
    m_processingInfo.progress = 0;
    setProcessingInfo(m_processingInfo);
}

void StatusBarManager::updateProgress(int currentPage, const QString& message)
{
    m_processingInfo.currentPage = currentPage;
    m_processingInfo.progress = static_cast<double>(currentPage) / m_processingInfo.totalPages;
    m_processingInfo.message = message;
    setProcessingInfo(m_processingInfo);
}

void StatusBarManager::finishProcessing()
{
    m_processingInfo.processing = false;
    setProcessingInfo(m_processingInfo);
}

void StatusBarManager::updateMemoryUsage()
{
    qint64 memoryUsage = 0;

#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        memoryUsage = pmc.WorkingSetSize;
    }
#elif defined(Q_OS_LINUX)
    std::ifstream statm("/proc/self/statm");
    if (statm.is_open()) {
        long pages = 0;
        statm >> pages;  // First value is total program size
        statm >> pages;  // Second value is resident set size
        memoryUsage = pages * sysconf(_SC_PAGESIZE);
    }
#endif

    QString memText;
    if (memoryUsage > 0) {
        if (memoryUsage >= 1024 * 1024 * 1024) {
            memText = QString("Mem: %1 GB").arg(memoryUsage / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
        } else {
            memText = QString("Mem: %1 MB").arg(memoryUsage / (1024.0 * 1024.0), 0, 'f', 0);
        }
    }
    m_memoryLabel->setText(memText);
}

void StatusBarManager::showMessage(const QString& message, int timeout)
{
    m_statusBar->showMessage(message, timeout);
}

void StatusBarManager::showPermanentMessage(const QString& message)
{
    m_statusBar->showMessage(message);
}

void StatusBarManager::clearMessage()
{
    m_statusBar->clearMessage();
}

void StatusBarManager::setDisplayUnits(Units units)
{
    if (m_units != units) {
        m_units = units;
        refresh();
        emit unitsChanged(units);
    }
}

StatusBarManager::Units StatusBarManager::displayUnits() const
{
    return m_units;
}

void StatusBarManager::refresh()
{
    updatePageLabel();
    updateCursorLabel();
    updateZoomLabel();
    updateSelectionLabel();
    updateMemoryLabel();
}

void StatusBarManager::updatePageLabel()
{
    QString text;

    if (m_pageInfo.totalPages > 0) {
        text = QString("Page %1/%2").arg(m_pageInfo.pageNumber).arg(m_pageInfo.totalPages);

        if (!m_pageInfo.imageSizePixels.isEmpty()) {
            QString sizeStr;
            if (m_units == Pixels) {
                sizeStr = QString(" | %1x%2 px")
                    .arg(m_pageInfo.imageSizePixels.width())
                    .arg(m_pageInfo.imageSizePixels.height());
            } else {
                sizeStr = QString(" | %1").arg(formatSize(m_pageInfo.imageSizeMm.width()));
                sizeStr += QString("x%1").arg(formatSize(m_pageInfo.imageSizeMm.height()));
            }

            if (m_pageInfo.dpi > 0) {
                sizeStr += QString(" @ %1 DPI").arg(static_cast<int>(m_pageInfo.dpi));
            }

            text += sizeStr;
        }
    }

    m_pageLabel->setText(text);
    m_sizeLabel->setText("");  // Size is combined with page label
}

void StatusBarManager::updateCursorLabel()
{
    QString text;

    if (m_cursorInfo.valid) {
        text = formatPosition(m_cursorInfo.imagePosition.x(), m_cursorInfo.imagePosition.y());
    }

    m_cursorLabel->setText(text);
}

void StatusBarManager::updateZoomLabel()
{
    QString text;

    if (m_zoomInfo.fitToWindow) {
        text = QString("Zoom: %1 (%2%)")
            .arg(tr("Fit"))
            .arg(static_cast<int>(m_zoomInfo.zoomLevel * 100));
    } else if (m_zoomInfo.fitToWidth) {
        text = QString("Zoom: %1 (%2%)")
            .arg(tr("Width"))
            .arg(static_cast<int>(m_zoomInfo.zoomLevel * 100));
    } else {
        text = QString("Zoom: %1%").arg(static_cast<int>(m_zoomInfo.zoomLevel * 100));
    }

    m_zoomLabel->setText(text);
}

void StatusBarManager::updateSelectionLabel()
{
    QString text;

    if (m_selectionInfo.valid) {
        text = QString("Sel: %1x%2")
            .arg(formatSize(m_selectionInfo.rect.width()))
            .arg(formatSize(m_selectionInfo.rect.height()));
    }

    m_selectionLabel->setText(text);
}

void StatusBarManager::updateMemoryLabel()
{
    updateMemoryUsage();
}

QString StatusBarManager::formatSize(double value) const
{
    switch (m_units) {
    case Pixels:
        return QString("%1 px").arg(static_cast<int>(value));
    case Millimeters:
        return QString("%1 mm").arg(value, 0, 'f', 1);
    case Centimeters:
        return QString("%1 cm").arg(value / 10.0, 0, 'f', 2);
    case Inches:
        return QString("%1\"").arg(value / MM_PER_INCH, 0, 'f', 2);
    default:
        return QString("%1").arg(value, 0, 'f', 1);
    }
}

QString StatusBarManager::formatPosition(double x, double y) const
{
    QString unit;
    double convX = x, convY = y;

    switch (m_units) {
    case Pixels:
        unit = "px";
        break;
    case Millimeters:
        unit = "mm";
        if (m_pageInfo.dpi > 0) {
            convX = x / m_pageInfo.dpi * MM_PER_INCH;
            convY = y / m_pageInfo.dpi * MM_PER_INCH;
        }
        break;
    case Centimeters:
        unit = "cm";
        if (m_pageInfo.dpi > 0) {
            convX = x / m_pageInfo.dpi * MM_PER_INCH / 10.0;
            convY = y / m_pageInfo.dpi * MM_PER_INCH / 10.0;
        }
        break;
    case Inches:
        unit = "in";
        if (m_pageInfo.dpi > 0) {
            convX = x / m_pageInfo.dpi;
            convY = y / m_pageInfo.dpi;
        }
        break;
    }

    if (m_units == Pixels) {
        return QString("X: %1, Y: %2 %3").arg(static_cast<int>(convX)).arg(static_cast<int>(convY)).arg(unit);
    } else {
        return QString("X: %1, Y: %2 %3").arg(convX, 0, 'f', 1).arg(convY, 0, 'f', 1).arg(unit);
    }
}
