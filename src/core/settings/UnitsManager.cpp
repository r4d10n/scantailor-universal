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

#include "UnitsManager.h"

static const char* KEY_CURRENT_UNIT = "units/current";

UnitsManager& UnitsManager::instance()
{
    static UnitsManager instance;
    return instance;
}

UnitsManager::UnitsManager()
    : m_currentUnit(Unit::Millimeters)
{
}

void UnitsManager::setCurrentUnit(Unit unit)
{
    m_currentUnit = unit;
}

QString UnitsManager::currentUnitName() const
{
    return unitToName(m_currentUnit);
}

QString UnitsManager::currentUnitSuffix() const
{
    return unitSuffix(m_currentUnit);
}

QStringList UnitsManager::availableUnitNames() const
{
    return QStringList() << "Pixels" << "Millimeters" << "Centimeters" << "Inches";
}

UnitsManager::Unit UnitsManager::unitFromName(const QString& name)
{
    if (name == "Pixels" || name == "px") return Unit::Pixels;
    if (name == "Millimeters" || name == "mm") return Unit::Millimeters;
    if (name == "Centimeters" || name == "cm") return Unit::Centimeters;
    if (name == "Inches" || name == "in") return Unit::Inches;
    return Unit::Millimeters;
}

QString UnitsManager::unitToName(Unit unit)
{
    switch (unit) {
        case Unit::Pixels: return "Pixels";
        case Unit::Millimeters: return "Millimeters";
        case Unit::Centimeters: return "Centimeters";
        case Unit::Inches: return "Inches";
        default: return "Millimeters";
    }
}

QString UnitsManager::unitSuffix(Unit unit)
{
    switch (unit) {
        case Unit::Pixels: return "px";
        case Unit::Millimeters: return "mm";
        case Unit::Centimeters: return "cm";
        case Unit::Inches: return "in";
        default: return "mm";
    }
}

double UnitsManager::fromPixels(double pixels, double dpi) const
{
    switch (m_currentUnit) {
        case Unit::Pixels:
            return pixels;
        case Unit::Millimeters:
            return pixelsToMM(pixels, dpi);
        case Unit::Centimeters:
            return pixelsToCM(pixels, dpi);
        case Unit::Inches:
            return pixelsToInches(pixels, dpi);
        default:
            return pixels;
    }
}

double UnitsManager::fromPixelsX(double pixels, double dpiX) const
{
    return fromPixels(pixels, dpiX);
}

double UnitsManager::fromPixelsY(double pixels, double dpiY) const
{
    return fromPixels(pixels, dpiY);
}

double UnitsManager::toPixels(double value, double dpi) const
{
    switch (m_currentUnit) {
        case Unit::Pixels:
            return value;
        case Unit::Millimeters:
            return mmToPixels(value, dpi);
        case Unit::Centimeters:
            return cmToPixels(value, dpi);
        case Unit::Inches:
            return inchesToPixels(value, dpi);
        default:
            return value;
    }
}

double UnitsManager::toPixelsX(double value, double dpiX) const
{
    return toPixels(value, dpiX);
}

double UnitsManager::toPixelsY(double value, double dpiY) const
{
    return toPixels(value, dpiY);
}

double UnitsManager::fromMM(double mm) const
{
    switch (m_currentUnit) {
        case Unit::Pixels:
            // Need DPI for this conversion - use 300 as default
            return mmToPixels(mm, 300.0);
        case Unit::Millimeters:
            return mm;
        case Unit::Centimeters:
            return mmToCM(mm);
        case Unit::Inches:
            return mmToInches(mm);
        default:
            return mm;
    }
}

double UnitsManager::toMM(double value) const
{
    switch (m_currentUnit) {
        case Unit::Pixels:
            // Need DPI for this conversion - use 300 as default
            return pixelsToMM(value, 300.0);
        case Unit::Millimeters:
            return value;
        case Unit::Centimeters:
            return cmToMM(value);
        case Unit::Inches:
            return inchesToMM(value);
        default:
            return value;
    }
}

// Static conversion utilities
double UnitsManager::pixelsToMM(double pixels, double dpi)
{
    return (pixels / dpi) * MM_PER_INCH;
}

double UnitsManager::mmToPixels(double mm, double dpi)
{
    return (mm / MM_PER_INCH) * dpi;
}

double UnitsManager::pixelsToCM(double pixels, double dpi)
{
    return (pixels / dpi) * CM_PER_INCH;
}

double UnitsManager::cmToPixels(double cm, double dpi)
{
    return (cm / CM_PER_INCH) * dpi;
}

double UnitsManager::pixelsToInches(double pixels, double dpi)
{
    return pixels / dpi;
}

double UnitsManager::inchesToPixels(double inches, double dpi)
{
    return inches * dpi;
}

double UnitsManager::mmToCM(double mm)
{
    return mm / 10.0;
}

double UnitsManager::cmToMM(double cm)
{
    return cm * 10.0;
}

double UnitsManager::mmToInches(double mm)
{
    return mm / MM_PER_INCH;
}

double UnitsManager::inchesToMM(double inches)
{
    return inches * MM_PER_INCH;
}

QString UnitsManager::formatValue(double value, int decimals) const
{
    return QString::number(value, 'f', decimals);
}

QString UnitsManager::formatWithUnit(double value, int decimals) const
{
    return QString("%1 %2").arg(value, 0, 'f', decimals).arg(currentUnitSuffix());
}

QString UnitsManager::formatPixels(double pixels, double dpi, int decimals) const
{
    double converted = fromPixels(pixels, dpi);
    return formatWithUnit(converted, decimals);
}

int UnitsManager::precision() const
{
    return precisionForUnit(m_currentUnit);
}

int UnitsManager::precisionForUnit(Unit unit)
{
    switch (unit) {
        case Unit::Pixels: return 0;
        case Unit::Millimeters: return 1;
        case Unit::Centimeters: return 2;
        case Unit::Inches: return 3;
        default: return 2;
    }
}

void UnitsManager::saveSettings(QSettings& settings) const
{
    settings.setValue(KEY_CURRENT_UNIT, static_cast<int>(m_currentUnit));
}

void UnitsManager::loadSettings(const QSettings& settings)
{
    int unitVal = settings.value(KEY_CURRENT_UNIT, static_cast<int>(Unit::Millimeters)).toInt();
    m_currentUnit = static_cast<Unit>(unitVal);
}
