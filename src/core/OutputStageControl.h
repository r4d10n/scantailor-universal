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

#ifndef OUTPUT_STAGE_CONTROL_H_
#define OUTPUT_STAGE_CONTROL_H_

#include <QObject>
#include <QString>
#include <QColor>
#include <QSize>
#include <memory>

class QSettings;

/**
 * @brief Output color mode options
 */
enum class OutputColorMode {
    BlackAndWhite,
    Grayscale,
    Color,
    Mixed,              // Auto-select based on content
    ColorWithEdges      // Color with edge enhancement
};

/**
 * @brief Binarization methods for B&W output
 */
enum class BinarizationMethod {
    Otsu,
    Sauvola,
    Wolf,
    Bradley,
    Niblack,
    Adaptive,
    Manual
};

/**
 * @brief Despeckle strength levels
 */
enum class DespeckleLevel {
    Off = 0,
    Cautious = 1,
    Normal = 2,
    Aggressive = 3
};

/**
 * @brief Output format options
 */
enum class OutputFormat {
    PNG,
    TIFF_LZW,
    TIFF_Deflate,
    TIFF_CCITT_G4,  // For B&W only
    JPEG,
    WebP,
    PDF
};

/**
 * @brief Settings for output processing
 */
struct OutputSettings {
    OutputColorMode colorMode;
    BinarizationMethod binarization;
    int binarizationThreshold;  // 0-255 for manual mode
    double sauvolaWindowSize;   // For Sauvola method
    double sauvolaK;            // Sauvola k parameter

    DespeckleLevel despeckle;
    bool fillMargins;
    QColor marginColor;
    bool equalizeIllumination;
    bool autoWhiteBalance;

    // Picture zones
    bool pictureZonesSeparate;  // Process picture zones separately
    bool colorizeBackground;
    QColor backgroundColor;

    // Mixed output
    bool mixedOutputEnabled;
    double foregroundSensitivity;
    double pictureDetectionSensitivity;

    // Post-processing
    bool sharpen;
    double sharpenAmount;
    bool descreen;
    double descreenDpi;

    // Output format
    OutputFormat format;
    int jpegQuality;  // 0-100
    int webpQuality;  // 0-100
    bool embedProfile; // Embed ICC color profile

    // Resolution
    QSize outputDpi;
    bool keepOriginalDpi;

    OutputSettings()
        : colorMode(OutputColorMode::BlackAndWhite)
        , binarization(BinarizationMethod::Otsu)
        , binarizationThreshold(128)
        , sauvolaWindowSize(200)
        , sauvolaK(0.34)
        , despeckle(DespeckleLevel::Normal)
        , fillMargins(true)
        , marginColor(Qt::white)
        , equalizeIllumination(true)
        , autoWhiteBalance(false)
        , pictureZonesSeparate(true)
        , colorizeBackground(false)
        , backgroundColor(Qt::white)
        , mixedOutputEnabled(false)
        , foregroundSensitivity(0.5)
        , pictureDetectionSensitivity(0.5)
        , sharpen(false)
        , sharpenAmount(0.5)
        , descreen(false)
        , descreenDpi(150)
        , format(OutputFormat::PNG)
        , jpegQuality(90)
        , webpQuality(85)
        , embedProfile(true)
        , outputDpi(600, 600)
        , keepOriginalDpi(true)
    {}
};

/**
 * @brief Enhanced output stage control manager
 *
 * Provides fine-grained control over the output processing stage,
 * including binarization, despeckling, picture handling, and export options.
 */
class OutputStageControl : public QObject
{
    Q_OBJECT

public:
    static OutputStageControl& instance();

    // Settings
    OutputSettings settings() const;
    void setSettings(const OutputSettings& settings);
    void loadFromSettings(const QSettings& settings);
    void saveToSettings(QSettings& settings) const;
    void resetToDefaults();

    // Individual setting accessors
    void setColorMode(OutputColorMode mode);
    OutputColorMode colorMode() const;

    void setBinarizationMethod(BinarizationMethod method);
    BinarizationMethod binarizationMethod() const;

    void setBinarizationThreshold(int threshold);
    int binarizationThreshold() const;

    void setDespeckleLevel(DespeckleLevel level);
    DespeckleLevel despeckleLevel() const;

    void setOutputFormat(OutputFormat format);
    OutputFormat outputFormat() const;

    void setOutputDpi(const QSize& dpi);
    QSize outputDpi() const;

    // Presets
    void applyPreset(const QString& presetName);
    QStringList availablePresets() const;
    void saveCurrentAsPreset(const QString& presetName);
    void deletePreset(const QString& presetName);

    // Utility
    QString colorModeString(OutputColorMode mode) const;
    QString binarizationMethodString(BinarizationMethod method) const;
    QString formatString(OutputFormat format) const;
    QString formatExtension(OutputFormat format) const;

    // Validation
    bool isValidForFormat(OutputFormat format, OutputColorMode colorMode) const;
    QString suggestFormat(OutputColorMode colorMode) const;

signals:
    void settingsChanged();
    void colorModeChanged(OutputColorMode mode);
    void binarizationMethodChanged(BinarizationMethod method);
    void despeckleLevelChanged(DespeckleLevel level);

private:
    OutputStageControl();
    ~OutputStageControl() = default;
    OutputStageControl(const OutputStageControl&) = delete;
    OutputStageControl& operator=(const OutputStageControl&) = delete;

    void loadPresets();
    void savePresets() const;

    OutputSettings m_settings;
    QMap<QString, OutputSettings> m_presets;
};

#endif // OUTPUT_STAGE_CONTROL_H_
