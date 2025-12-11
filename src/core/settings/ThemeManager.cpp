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

#include "ThemeManager.h"
#include <QApplication>
#include <QFile>
#include <QStyleFactory>

static const char* KEY_THEME = "appearance/theme";
static const char* KEY_CUSTOM_STYLESHEET = "appearance/custom_stylesheet";

ThemeManager& ThemeManager::instance()
{
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager()
    : m_currentTheme(Theme::System)
    , m_colors(lightThemeColors())
{
}

ThemeColors ThemeManager::lightThemeColors()
{
    ThemeColors colors;
    colors.background = QColor(255, 255, 255);
    colors.foreground = QColor(33, 33, 33);
    colors.accent = QColor(0, 120, 212);
    colors.highlight = QColor(0, 120, 212);
    colors.border = QColor(200, 200, 200);
    colors.buttonBackground = QColor(240, 240, 240);
    colors.buttonForeground = QColor(33, 33, 33);
    colors.inputBackground = QColor(255, 255, 255);
    colors.inputForeground = QColor(33, 33, 33);
    colors.errorColor = QColor(232, 17, 35);
    colors.warningColor = QColor(255, 185, 0);
    colors.successColor = QColor(16, 124, 16);
    return colors;
}

ThemeColors ThemeManager::darkThemeColors()
{
    ThemeColors colors;
    colors.background = QColor(30, 30, 30);
    colors.foreground = QColor(240, 240, 240);
    colors.accent = QColor(0, 120, 212);
    colors.highlight = QColor(0, 99, 177);
    colors.border = QColor(70, 70, 70);
    colors.buttonBackground = QColor(55, 55, 55);
    colors.buttonForeground = QColor(240, 240, 240);
    colors.inputBackground = QColor(45, 45, 45);
    colors.inputForeground = QColor(240, 240, 240);
    colors.errorColor = QColor(255, 99, 71);
    colors.warningColor = QColor(255, 200, 50);
    colors.successColor = QColor(50, 205, 50);
    return colors;
}

void ThemeManager::setTheme(Theme theme)
{
    m_currentTheme = theme;

    switch (theme) {
        case Theme::Light:
            m_colors = lightThemeColors();
            break;
        case Theme::Dark:
            m_colors = darkThemeColors();
            break;
        case Theme::System:
        case Theme::Custom:
        default:
            m_colors = lightThemeColors();
            break;
    }

    applyTheme();
}

QString ThemeManager::currentThemeName() const
{
    switch (m_currentTheme) {
        case Theme::System: return "System";
        case Theme::Light: return "Light";
        case Theme::Dark: return "Dark";
        case Theme::Custom: return "Custom";
        default: return "Unknown";
    }
}

QStringList ThemeManager::availableThemes() const
{
    return QStringList() << "System" << "Light" << "Dark" << "Custom";
}

void ThemeManager::setCustomStylesheet(const QString& path)
{
    m_customStylesheetPath = path;
    if (m_currentTheme == Theme::Custom) {
        applyTheme();
    }
}

QPalette ThemeManager::palette() const
{
    QPalette pal;

    pal.setColor(QPalette::Window, m_colors.background);
    pal.setColor(QPalette::WindowText, m_colors.foreground);
    pal.setColor(QPalette::Base, m_colors.inputBackground);
    pal.setColor(QPalette::AlternateBase, m_colors.background.darker(105));
    pal.setColor(QPalette::Text, m_colors.inputForeground);
    pal.setColor(QPalette::Button, m_colors.buttonBackground);
    pal.setColor(QPalette::ButtonText, m_colors.buttonForeground);
    pal.setColor(QPalette::Highlight, m_colors.highlight);
    pal.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    pal.setColor(QPalette::Link, m_colors.accent);
    pal.setColor(QPalette::LinkVisited, m_colors.accent.darker(120));

    // Disabled colors
    pal.setColor(QPalette::Disabled, QPalette::WindowText, m_colors.foreground.darker(150));
    pal.setColor(QPalette::Disabled, QPalette::Text, m_colors.foreground.darker(150));
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, m_colors.foreground.darker(150));

    return pal;
}

QString ThemeManager::generateStylesheet() const
{
    switch (m_currentTheme) {
        case Theme::Light:
            return generateLightStylesheet();
        case Theme::Dark:
            return generateDarkStylesheet();
        case Theme::Custom:
            if (!m_customStylesheetPath.isEmpty()) {
                QFile file(m_customStylesheetPath);
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    return QString::fromUtf8(file.readAll());
                }
            }
            return QString();
        case Theme::System:
        default:
            return QString();
    }
}

QString ThemeManager::generateLightStylesheet() const
{
    return QString(R"(
        QMainWindow, QDialog, QWidget {
            background-color: %1;
            color: %2;
        }
        QMenuBar {
            background-color: %3;
            color: %2;
        }
        QMenuBar::item:selected {
            background-color: %4;
        }
        QMenu {
            background-color: %1;
            color: %2;
            border: 1px solid %5;
        }
        QMenu::item:selected {
            background-color: %4;
            color: white;
        }
        QPushButton {
            background-color: %3;
            color: %2;
            border: 1px solid %5;
            padding: 5px 15px;
            border-radius: 3px;
        }
        QPushButton:hover {
            background-color: %4;
            color: white;
        }
        QPushButton:pressed {
            background-color: %6;
        }
        QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QComboBox {
            background-color: %7;
            color: %8;
            border: 1px solid %5;
            padding: 3px;
            border-radius: 2px;
        }
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {
            border: 2px solid %4;
        }
        QScrollBar:vertical {
            background-color: %3;
            width: 12px;
        }
        QScrollBar::handle:vertical {
            background-color: %5;
            border-radius: 5px;
            min-height: 20px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: %4;
        }
        QTabWidget::pane {
            border: 1px solid %5;
        }
        QTabBar::tab {
            background-color: %3;
            color: %2;
            padding: 8px 16px;
            border: 1px solid %5;
        }
        QTabBar::tab:selected {
            background-color: %1;
            border-bottom-color: %1;
        }
        QGroupBox {
            border: 1px solid %5;
            border-radius: 5px;
            margin-top: 10px;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
        }
        QStatusBar {
            background-color: %3;
            color: %2;
        }
        QToolBar {
            background-color: %3;
            border: none;
            spacing: 3px;
        }
        QToolButton {
            background-color: transparent;
            border: 1px solid transparent;
            padding: 3px;
            border-radius: 3px;
        }
        QToolButton:hover {
            background-color: %4;
            border-color: %4;
        }
    )")
        .arg(m_colors.background.name())           // %1
        .arg(m_colors.foreground.name())           // %2
        .arg(m_colors.buttonBackground.name())     // %3
        .arg(m_colors.highlight.name())            // %4
        .arg(m_colors.border.name())               // %5
        .arg(m_colors.highlight.darker(120).name()) // %6
        .arg(m_colors.inputBackground.name())      // %7
        .arg(m_colors.inputForeground.name());     // %8
}

QString ThemeManager::generateDarkStylesheet() const
{
    return QString(R"(
        QMainWindow, QDialog, QWidget {
            background-color: %1;
            color: %2;
        }
        QMenuBar {
            background-color: %3;
            color: %2;
        }
        QMenuBar::item:selected {
            background-color: %4;
        }
        QMenu {
            background-color: %3;
            color: %2;
            border: 1px solid %5;
        }
        QMenu::item:selected {
            background-color: %4;
            color: white;
        }
        QPushButton {
            background-color: %3;
            color: %2;
            border: 1px solid %5;
            padding: 5px 15px;
            border-radius: 3px;
        }
        QPushButton:hover {
            background-color: %4;
            color: white;
        }
        QPushButton:pressed {
            background-color: %6;
        }
        QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QComboBox {
            background-color: %7;
            color: %8;
            border: 1px solid %5;
            padding: 3px;
            border-radius: 2px;
        }
        QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {
            border: 2px solid %4;
        }
        QScrollBar:vertical {
            background-color: %3;
            width: 12px;
        }
        QScrollBar::handle:vertical {
            background-color: %5;
            border-radius: 5px;
            min-height: 20px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: %4;
        }
        QTabWidget::pane {
            border: 1px solid %5;
        }
        QTabBar::tab {
            background-color: %3;
            color: %2;
            padding: 8px 16px;
            border: 1px solid %5;
        }
        QTabBar::tab:selected {
            background-color: %1;
            border-bottom-color: %1;
        }
        QGroupBox {
            border: 1px solid %5;
            border-radius: 5px;
            margin-top: 10px;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
        }
        QStatusBar {
            background-color: %3;
            color: %2;
        }
        QToolBar {
            background-color: %3;
            border: none;
            spacing: 3px;
        }
        QToolButton {
            background-color: transparent;
            border: 1px solid transparent;
            padding: 3px;
            border-radius: 3px;
        }
        QToolButton:hover {
            background-color: %4;
            border-color: %4;
        }
        QTreeView, QListView, QTableView {
            background-color: %7;
            alternate-background-color: %3;
            color: %2;
            border: 1px solid %5;
        }
        QTreeView::item:selected, QListView::item:selected, QTableView::item:selected {
            background-color: %4;
        }
        QHeaderView::section {
            background-color: %3;
            color: %2;
            padding: 5px;
            border: 1px solid %5;
        }
        QSlider::groove:horizontal {
            background-color: %5;
            height: 4px;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background-color: %4;
            width: 16px;
            height: 16px;
            margin: -6px 0;
            border-radius: 8px;
        }
        QCheckBox::indicator, QRadioButton::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid %5;
            border-radius: 3px;
            background-color: %7;
        }
        QCheckBox::indicator:checked, QRadioButton::indicator:checked {
            background-color: %4;
        }
    )")
        .arg(m_colors.background.name())           // %1
        .arg(m_colors.foreground.name())           // %2
        .arg(m_colors.buttonBackground.name())     // %3
        .arg(m_colors.highlight.name())            // %4
        .arg(m_colors.border.name())               // %5
        .arg(m_colors.highlight.darker(120).name()) // %6
        .arg(m_colors.inputBackground.name())      // %7
        .arg(m_colors.inputForeground.name());     // %8
}

void ThemeManager::applyTheme()
{
    QApplication* app = qobject_cast<QApplication*>(QApplication::instance());
    if (!app) return;

    // Set style based on theme
    if (m_currentTheme == Theme::Dark) {
        app->setStyle(QStyleFactory::create("Fusion"));
    }

    // Apply palette
    app->setPalette(palette());

    // Apply stylesheet
    QString stylesheet = generateStylesheet();
    app->setStyleSheet(stylesheet);
}

void ThemeManager::saveSettings(QSettings& settings) const
{
    settings.setValue(KEY_THEME, static_cast<int>(m_currentTheme));
    settings.setValue(KEY_CUSTOM_STYLESHEET, m_customStylesheetPath);
}

void ThemeManager::loadSettings(const QSettings& settings)
{
    int themeVal = settings.value(KEY_THEME, static_cast<int>(Theme::System)).toInt();
    m_currentTheme = static_cast<Theme>(themeVal);
    m_customStylesheetPath = settings.value(KEY_CUSTOM_STYLESHEET).toString();

    switch (m_currentTheme) {
        case Theme::Light:
            m_colors = lightThemeColors();
            break;
        case Theme::Dark:
            m_colors = darkThemeColors();
            break;
        default:
            m_colors = lightThemeColors();
            break;
    }
}
