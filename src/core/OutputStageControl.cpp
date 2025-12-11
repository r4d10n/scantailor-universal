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

#include "OutputStageControl.h"
#include <QSettings>
#include <QMap>

static const char* KEY_COLOR_MODE = "output/colorMode";
static const char* KEY_BINARIZATION = "output/binarization";
static const char* KEY_BINARIZATION_THRESHOLD = "output/binarizationThreshold";
static const char* KEY_SAUVOLA_WINDOW = "output/sauvolaWindow";
static const char* KEY_SAUVOLA_K = "output/sauvolaK";
static const char* KEY_DESPECKLE = "output/despeckle";
static const char* KEY_FILL_MARGINS = "output/fillMargins";
static const char* KEY_MARGIN_COLOR = "output/marginColor";
static const char* KEY_EQUALIZE_ILLUMINATION = "output/equalizeIllumination";
static const char* KEY_AUTO_WHITE_BALANCE = "output/autoWhiteBalance";
static const char* KEY_PICTURE_ZONES_SEPARATE = "output/pictureZonesSeparate";
static const char* KEY_COLORIZE_BG = "output/colorizeBackground";
static const char* KEY_BG_COLOR = "output/backgroundColor";
static const char* KEY_MIXED_OUTPUT = "output/mixedOutput";
static const char* KEY_FG_SENSITIVITY = "output/foregroundSensitivity";
static const char* KEY_PICTURE_SENSITIVITY = "output/pictureSensitivity";
static const char* KEY_SHARPEN = "output/sharpen";
static const char* KEY_SHARPEN_AMOUNT = "output/sharpenAmount";
static const char* KEY_DESCREEN = "output/descreen";
static const char* KEY_DESCREEN_DPI = "output/descreenDpi";
static const char* KEY_FORMAT = "output/format";
static const char* KEY_JPEG_QUALITY = "output/jpegQuality";
static const char* KEY_WEBP_QUALITY = "output/webpQuality";
static const char* KEY_EMBED_PROFILE = "output/embedProfile";
static const char* KEY_OUTPUT_DPI = "output/outputDpi";
static const char* KEY_KEEP_ORIGINAL_DPI = "output/keepOriginalDpi";

OutputStageControl& OutputStageControl::instance()
{
    static OutputStageControl instance;
    return instance;
}

OutputStageControl::OutputStageControl()
{
    QSettings settings;
    loadFromSettings(settings);
    loadPresets();
}

OutputSettings OutputStageControl::settings() const
{
    return m_settings;
}

void OutputStageControl::setSettings(const OutputSettings& settings)
{
    m_settings = settings;
    emit settingsChanged();
}

void OutputStageControl::loadFromSettings(const QSettings& settings)
{
    m_settings.colorMode = static_cast<OutputColorMode>(
        settings.value(KEY_COLOR_MODE, 0).toInt());
    m_settings.binarization = static_cast<BinarizationMethod>(
        settings.value(KEY_BINARIZATION, 0).toInt());
    m_settings.binarizationThreshold = settings.value(KEY_BINARIZATION_THRESHOLD, 128).toInt();
    m_settings.sauvolaWindowSize = settings.value(KEY_SAUVOLA_WINDOW, 200.0).toDouble();
    m_settings.sauvolaK = settings.value(KEY_SAUVOLA_K, 0.34).toDouble();
    m_settings.despeckle = static_cast<DespeckleLevel>(
        settings.value(KEY_DESPECKLE, 2).toInt());
    m_settings.fillMargins = settings.value(KEY_FILL_MARGINS, true).toBool();
    m_settings.marginColor = settings.value(KEY_MARGIN_COLOR, QColor(Qt::white)).value<QColor>();
    m_settings.equalizeIllumination = settings.value(KEY_EQUALIZE_ILLUMINATION, true).toBool();
    m_settings.autoWhiteBalance = settings.value(KEY_AUTO_WHITE_BALANCE, false).toBool();
    m_settings.pictureZonesSeparate = settings.value(KEY_PICTURE_ZONES_SEPARATE, true).toBool();
    m_settings.colorizeBackground = settings.value(KEY_COLORIZE_BG, false).toBool();
    m_settings.backgroundColor = settings.value(KEY_BG_COLOR, QColor(Qt::white)).value<QColor>();
    m_settings.mixedOutputEnabled = settings.value(KEY_MIXED_OUTPUT, false).toBool();
    m_settings.foregroundSensitivity = settings.value(KEY_FG_SENSITIVITY, 0.5).toDouble();
    m_settings.pictureDetectionSensitivity = settings.value(KEY_PICTURE_SENSITIVITY, 0.5).toDouble();
    m_settings.sharpen = settings.value(KEY_SHARPEN, false).toBool();
    m_settings.sharpenAmount = settings.value(KEY_SHARPEN_AMOUNT, 0.5).toDouble();
    m_settings.descreen = settings.value(KEY_DESCREEN, false).toBool();
    m_settings.descreenDpi = settings.value(KEY_DESCREEN_DPI, 150.0).toDouble();
    m_settings.format = static_cast<OutputFormat>(settings.value(KEY_FORMAT, 0).toInt());
    m_settings.jpegQuality = settings.value(KEY_JPEG_QUALITY, 90).toInt();
    m_settings.webpQuality = settings.value(KEY_WEBP_QUALITY, 85).toInt();
    m_settings.embedProfile = settings.value(KEY_EMBED_PROFILE, true).toBool();
    m_settings.outputDpi = settings.value(KEY_OUTPUT_DPI, QSize(600, 600)).toSize();
    m_settings.keepOriginalDpi = settings.value(KEY_KEEP_ORIGINAL_DPI, true).toBool();
}

void OutputStageControl::saveToSettings(QSettings& settings) const
{
    settings.setValue(KEY_COLOR_MODE, static_cast<int>(m_settings.colorMode));
    settings.setValue(KEY_BINARIZATION, static_cast<int>(m_settings.binarization));
    settings.setValue(KEY_BINARIZATION_THRESHOLD, m_settings.binarizationThreshold);
    settings.setValue(KEY_SAUVOLA_WINDOW, m_settings.sauvolaWindowSize);
    settings.setValue(KEY_SAUVOLA_K, m_settings.sauvolaK);
    settings.setValue(KEY_DESPECKLE, static_cast<int>(m_settings.despeckle));
    settings.setValue(KEY_FILL_MARGINS, m_settings.fillMargins);
    settings.setValue(KEY_MARGIN_COLOR, m_settings.marginColor);
    settings.setValue(KEY_EQUALIZE_ILLUMINATION, m_settings.equalizeIllumination);
    settings.setValue(KEY_AUTO_WHITE_BALANCE, m_settings.autoWhiteBalance);
    settings.setValue(KEY_PICTURE_ZONES_SEPARATE, m_settings.pictureZonesSeparate);
    settings.setValue(KEY_COLORIZE_BG, m_settings.colorizeBackground);
    settings.setValue(KEY_BG_COLOR, m_settings.backgroundColor);
    settings.setValue(KEY_MIXED_OUTPUT, m_settings.mixedOutputEnabled);
    settings.setValue(KEY_FG_SENSITIVITY, m_settings.foregroundSensitivity);
    settings.setValue(KEY_PICTURE_SENSITIVITY, m_settings.pictureDetectionSensitivity);
    settings.setValue(KEY_SHARPEN, m_settings.sharpen);
    settings.setValue(KEY_SHARPEN_AMOUNT, m_settings.sharpenAmount);
    settings.setValue(KEY_DESCREEN, m_settings.descreen);
    settings.setValue(KEY_DESCREEN_DPI, m_settings.descreenDpi);
    settings.setValue(KEY_FORMAT, static_cast<int>(m_settings.format));
    settings.setValue(KEY_JPEG_QUALITY, m_settings.jpegQuality);
    settings.setValue(KEY_WEBP_QUALITY, m_settings.webpQuality);
    settings.setValue(KEY_EMBED_PROFILE, m_settings.embedProfile);
    settings.setValue(KEY_OUTPUT_DPI, m_settings.outputDpi);
    settings.setValue(KEY_KEEP_ORIGINAL_DPI, m_settings.keepOriginalDpi);
}

void OutputStageControl::resetToDefaults()
{
    m_settings = OutputSettings();
    emit settingsChanged();
}

void OutputStageControl::setColorMode(OutputColorMode mode)
{
    if (m_settings.colorMode != mode) {
        m_settings.colorMode = mode;
        emit colorModeChanged(mode);
        emit settingsChanged();
    }
}

OutputColorMode OutputStageControl::colorMode() const
{
    return m_settings.colorMode;
}

void OutputStageControl::setBinarizationMethod(BinarizationMethod method)
{
    if (m_settings.binarization != method) {
        m_settings.binarization = method;
        emit binarizationMethodChanged(method);
        emit settingsChanged();
    }
}

BinarizationMethod OutputStageControl::binarizationMethod() const
{
    return m_settings.binarization;
}

void OutputStageControl::setBinarizationThreshold(int threshold)
{
    m_settings.binarizationThreshold = qBound(0, threshold, 255);
    emit settingsChanged();
}

int OutputStageControl::binarizationThreshold() const
{
    return m_settings.binarizationThreshold;
}

void OutputStageControl::setDespeckleLevel(DespeckleLevel level)
{
    if (m_settings.despeckle != level) {
        m_settings.despeckle = level;
        emit despeckleLevelChanged(level);
        emit settingsChanged();
    }
}

DespeckleLevel OutputStageControl::despeckleLevel() const
{
    return m_settings.despeckle;
}

void OutputStageControl::setOutputFormat(OutputFormat format)
{
    m_settings.format = format;
    emit settingsChanged();
}

OutputFormat OutputStageControl::outputFormat() const
{
    return m_settings.format;
}

void OutputStageControl::setOutputDpi(const QSize& dpi)
{
    m_settings.outputDpi = dpi;
    emit settingsChanged();
}

QSize OutputStageControl::outputDpi() const
{
    return m_settings.outputDpi;
}

void OutputStageControl::applyPreset(const QString& presetName)
{
    if (m_presets.contains(presetName)) {
        setSettings(m_presets[presetName]);
    } else if (presetName == "Document (B&W)") {
        m_settings.colorMode = OutputColorMode::BlackAndWhite;
        m_settings.binarization = BinarizationMethod::Otsu;
        m_settings.despeckle = DespeckleLevel::Normal;
        m_settings.equalizeIllumination = true;
        m_settings.format = OutputFormat::TIFF_CCITT_G4;
        emit settingsChanged();
    } else if (presetName == "Photo") {
        m_settings.colorMode = OutputColorMode::Color;
        m_settings.sharpen = true;
        m_settings.sharpenAmount = 0.3;
        m_settings.autoWhiteBalance = true;
        m_settings.format = OutputFormat::PNG;
        emit settingsChanged();
    } else if (presetName == "Mixed Content") {
        m_settings.colorMode = OutputColorMode::Mixed;
        m_settings.mixedOutputEnabled = true;
        m_settings.pictureZonesSeparate = true;
        m_settings.format = OutputFormat::PNG;
        emit settingsChanged();
    } else if (presetName == "Archive") {
        m_settings.colorMode = OutputColorMode::Color;
        m_settings.format = OutputFormat::TIFF_Deflate;
        m_settings.outputDpi = QSize(600, 600);
        m_settings.embedProfile = true;
        emit settingsChanged();
    }
}

QStringList OutputStageControl::availablePresets() const
{
    QStringList presets;
    presets << "Document (B&W)" << "Photo" << "Mixed Content" << "Archive";
    presets << m_presets.keys();
    return presets;
}

void OutputStageControl::saveCurrentAsPreset(const QString& presetName)
{
    m_presets[presetName] = m_settings;
    savePresets();
}

void OutputStageControl::deletePreset(const QString& presetName)
{
    m_presets.remove(presetName);
    savePresets();
}

QString OutputStageControl::colorModeString(OutputColorMode mode) const
{
    switch (mode) {
    case OutputColorMode::BlackAndWhite: return tr("Black and White");
    case OutputColorMode::Grayscale: return tr("Grayscale");
    case OutputColorMode::Color: return tr("Color");
    case OutputColorMode::Mixed: return tr("Mixed");
    case OutputColorMode::ColorWithEdges: return tr("Color with Edges");
    default: return QString();
    }
}

QString OutputStageControl::binarizationMethodString(BinarizationMethod method) const
{
    switch (method) {
    case BinarizationMethod::Otsu: return "Otsu";
    case BinarizationMethod::Sauvola: return "Sauvola";
    case BinarizationMethod::Wolf: return "Wolf";
    case BinarizationMethod::Bradley: return "Bradley";
    case BinarizationMethod::Niblack: return "Niblack";
    case BinarizationMethod::Adaptive: return tr("Adaptive");
    case BinarizationMethod::Manual: return tr("Manual");
    default: return QString();
    }
}

QString OutputStageControl::formatString(OutputFormat format) const
{
    switch (format) {
    case OutputFormat::PNG: return "PNG";
    case OutputFormat::TIFF_LZW: return "TIFF (LZW)";
    case OutputFormat::TIFF_Deflate: return "TIFF (Deflate)";
    case OutputFormat::TIFF_CCITT_G4: return "TIFF (CCITT G4)";
    case OutputFormat::JPEG: return "JPEG";
    case OutputFormat::WebP: return "WebP";
    case OutputFormat::PDF: return "PDF";
    default: return QString();
    }
}

QString OutputStageControl::formatExtension(OutputFormat format) const
{
    switch (format) {
    case OutputFormat::PNG: return ".png";
    case OutputFormat::TIFF_LZW:
    case OutputFormat::TIFF_Deflate:
    case OutputFormat::TIFF_CCITT_G4: return ".tif";
    case OutputFormat::JPEG: return ".jpg";
    case OutputFormat::WebP: return ".webp";
    case OutputFormat::PDF: return ".pdf";
    default: return ".png";
    }
}

bool OutputStageControl::isValidForFormat(OutputFormat format, OutputColorMode colorMode) const
{
    // CCITT G4 is only valid for B&W
    if (format == OutputFormat::TIFF_CCITT_G4 &&
        colorMode != OutputColorMode::BlackAndWhite) {
        return false;
    }
    return true;
}

QString OutputStageControl::suggestFormat(OutputColorMode colorMode) const
{
    switch (colorMode) {
    case OutputColorMode::BlackAndWhite:
        return formatString(OutputFormat::TIFF_CCITT_G4);
    case OutputColorMode::Grayscale:
        return formatString(OutputFormat::PNG);
    default:
        return formatString(OutputFormat::PNG);
    }
}

void OutputStageControl::loadPresets()
{
    QSettings settings;
    int count = settings.value("output/presets/count", 0).toInt();

    for (int i = 0; i < count; ++i) {
        QString prefix = QString("output/presets/%1/").arg(i);
        QString name = settings.value(prefix + "name").toString();
        if (!name.isEmpty()) {
            OutputSettings preset;
            preset.colorMode = static_cast<OutputColorMode>(
                settings.value(prefix + "colorMode", 0).toInt());
            preset.binarization = static_cast<BinarizationMethod>(
                settings.value(prefix + "binarization", 0).toInt());
            preset.despeckle = static_cast<DespeckleLevel>(
                settings.value(prefix + "despeckle", 2).toInt());
            preset.format = static_cast<OutputFormat>(
                settings.value(prefix + "format", 0).toInt());
            m_presets[name] = preset;
        }
    }
}

void OutputStageControl::savePresets() const
{
    QSettings settings;
    settings.setValue("output/presets/count", m_presets.size());

    int i = 0;
    for (auto it = m_presets.begin(); it != m_presets.end(); ++it, ++i) {
        QString prefix = QString("output/presets/%1/").arg(i);
        settings.setValue(prefix + "name", it.key());
        settings.setValue(prefix + "colorMode", static_cast<int>(it.value().colorMode));
        settings.setValue(prefix + "binarization", static_cast<int>(it.value().binarization));
        settings.setValue(prefix + "despeckle", static_cast<int>(it.value().despeckle));
        settings.setValue(prefix + "format", static_cast<int>(it.value().format));
    }
}
