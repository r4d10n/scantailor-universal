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

#ifndef UNITSMANAGER_H
#define UNITSMANAGER_H

#include <QString>
#include <QStringList>
#include <QSettings>

/**
 * @brief Manages measurement units throughout the application
 *
 * Supports pixels, millimeters, centimeters, and inches.
 * Provides conversion utilities and formatting helpers.
 */
class UnitsManager
{
public:
    enum class Unit {
        Pixels,
        Millimeters,
        Centimeters,
        Inches
    };

    static UnitsManager& instance();

    // Current unit
    Unit currentUnit() const { return m_currentUnit; }
    void setCurrentUnit(Unit unit);
    QString currentUnitName() const;
    QString currentUnitSuffix() const;

    // Available units
    QStringList availableUnitNames() const;
    static Unit unitFromName(const QString& name);
    static QString unitToName(Unit unit);
    static QString unitSuffix(Unit unit);

    // Conversion: from pixels to current unit
    double fromPixels(double pixels, double dpi) const;
    double fromPixelsX(double pixels, double dpiX) const;
    double fromPixelsY(double pixels, double dpiY) const;

    // Conversion: from current unit to pixels
    double toPixels(double value, double dpi) const;
    double toPixelsX(double value, double dpiX) const;
    double toPixelsY(double value, double dpiY) const;

    // Conversion: from millimeters (the internal unit for margins)
    double fromMM(double mm) const;
    double toMM(double value) const;

    // Static conversion utilities
    static double pixelsToMM(double pixels, double dpi);
    static double mmToPixels(double mm, double dpi);
    static double pixelsToCM(double pixels, double dpi);
    static double cmToPixels(double cm, double dpi);
    static double pixelsToInches(double pixels, double dpi);
    static double inchesToPixels(double inches, double dpi);
    static double mmToCM(double mm);
    static double cmToMM(double cm);
    static double mmToInches(double mm);
    static double inchesToMM(double inches);

    // Formatting
    QString formatValue(double value, int decimals = 2) const;
    QString formatWithUnit(double value, int decimals = 2) const;
    QString formatPixels(double pixels, double dpi, int decimals = 2) const;

    // Persistence
    void saveSettings(QSettings& settings) const;
    void loadSettings(const QSettings& settings);

    // Precision (decimal places for each unit type)
    int precision() const;
    static int precisionForUnit(Unit unit);

private:
    UnitsManager();
    ~UnitsManager() = default;
    UnitsManager(const UnitsManager&) = delete;
    UnitsManager& operator=(const UnitsManager&) = delete;

    Unit m_currentUnit;

    // Conversion constants
    static constexpr double MM_PER_INCH = 25.4;
    static constexpr double CM_PER_INCH = 2.54;
};

#endif // UNITSMANAGER_H
