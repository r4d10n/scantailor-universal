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

#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QString>
#include <QStringList>
#include <QColor>
#include <QPalette>
#include <QSettings>

/**
 * @brief Manages application themes (Light/Dark color schemes)
 *
 * Provides easy switching between predefined themes and custom stylesheets.
 */
class ThemeManager
{
public:
    enum class Theme {
        System,     // Follow system theme
        Light,      // Light color scheme
        Dark,       // Dark color scheme
        Custom      // User-defined stylesheet
    };

    struct ThemeColors {
        QColor background;
        QColor foreground;
        QColor accent;
        QColor highlight;
        QColor border;
        QColor buttonBackground;
        QColor buttonForeground;
        QColor inputBackground;
        QColor inputForeground;
        QColor errorColor;
        QColor warningColor;
        QColor successColor;
    };

    static ThemeManager& instance();

    // Theme management
    void setTheme(Theme theme);
    Theme currentTheme() const { return m_currentTheme; }
    QString currentThemeName() const;
    QStringList availableThemes() const;

    // Custom stylesheet support
    void setCustomStylesheet(const QString& path);
    QString customStylesheetPath() const { return m_customStylesheetPath; }

    // Color access
    ThemeColors colors() const { return m_colors; }
    QPalette palette() const;

    // Generate stylesheet for current theme
    QString generateStylesheet() const;

    // Persistence
    void saveSettings(QSettings& settings) const;
    void loadSettings(const QSettings& settings);

    // Predefined palettes
    static ThemeColors lightThemeColors();
    static ThemeColors darkThemeColors();

private:
    ThemeManager();
    ~ThemeManager() = default;
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    void applyTheme();
    QString generateLightStylesheet() const;
    QString generateDarkStylesheet() const;

    Theme m_currentTheme;
    ThemeColors m_colors;
    QString m_customStylesheetPath;
};

#endif // THEMEMANAGER_H
