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

#include "ProfileManager.h"
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QXmlStreamWriter>

// Settings keys
static const char* KEY_OUTPUT_DPI = "output/dpi";
static const char* KEY_COLOR_MODE = "output/color_mode";
static const char* KEY_DESPECKLE = "output/despeckle";
static const char* KEY_DEWARPING = "output/dewarping";
static const char* KEY_BIN_THRESHOLD = "output/binarization_threshold";
static const char* KEY_MARGIN_TOP = "layout/margin_top";
static const char* KEY_MARGIN_BOTTOM = "layout/margin_bottom";
static const char* KEY_MARGIN_LEFT = "layout/margin_left";
static const char* KEY_MARGIN_RIGHT = "layout/margin_right";
static const char* KEY_ALIGN_VERT = "layout/alignment_vertical";
static const char* KEY_ALIGN_HOR = "layout/alignment_horizontal";
static const char* KEY_DESKEW_MODE = "deskew/mode";

// ============================================================================
// ProcessingProfile Implementation
// ============================================================================

ProcessingProfile::ProcessingProfile(const QString& name)
    : m_name(name)
    , m_builtin(false)
{
}

QVariant ProcessingProfile::setting(const QString& key) const
{
    return m_settings.value(key);
}

void ProcessingProfile::setSetting(const QString& key, const QVariant& value)
{
    m_settings[key] = value;
}

bool ProcessingProfile::hasSetting(const QString& key) const
{
    return m_settings.contains(key);
}

QStringList ProcessingProfile::settingKeys() const
{
    return m_settings.keys();
}

int ProcessingProfile::outputDpi() const
{
    return m_settings.value(KEY_OUTPUT_DPI, 600).toInt();
}

void ProcessingProfile::setOutputDpi(int dpi)
{
    m_settings[KEY_OUTPUT_DPI] = dpi;
}

QString ProcessingProfile::colorMode() const
{
    return m_settings.value(KEY_COLOR_MODE, "black_and_white").toString();
}

void ProcessingProfile::setColorMode(const QString& mode)
{
    m_settings[KEY_COLOR_MODE] = mode;
}

QString ProcessingProfile::despeckleLevel() const
{
    return m_settings.value(KEY_DESPECKLE, "cautious").toString();
}

void ProcessingProfile::setDespeckleLevel(const QString& level)
{
    m_settings[KEY_DESPECKLE] = level;
}

QString ProcessingProfile::dewarpingMode() const
{
    return m_settings.value(KEY_DEWARPING, "off").toString();
}

void ProcessingProfile::setDewarpingMode(const QString& mode)
{
    m_settings[KEY_DEWARPING] = mode;
}

int ProcessingProfile::binarizationThreshold() const
{
    return m_settings.value(KEY_BIN_THRESHOLD, 0).toInt();
}

void ProcessingProfile::setBinarizationThreshold(int threshold)
{
    m_settings[KEY_BIN_THRESHOLD] = threshold;
}

double ProcessingProfile::marginTop() const
{
    return m_settings.value(KEY_MARGIN_TOP, 0.0).toDouble();
}

double ProcessingProfile::marginBottom() const
{
    return m_settings.value(KEY_MARGIN_BOTTOM, 0.0).toDouble();
}

double ProcessingProfile::marginLeft() const
{
    return m_settings.value(KEY_MARGIN_LEFT, 0.0).toDouble();
}

double ProcessingProfile::marginRight() const
{
    return m_settings.value(KEY_MARGIN_RIGHT, 0.0).toDouble();
}

void ProcessingProfile::setMargins(double top, double bottom, double left, double right)
{
    m_settings[KEY_MARGIN_TOP] = top;
    m_settings[KEY_MARGIN_BOTTOM] = bottom;
    m_settings[KEY_MARGIN_LEFT] = left;
    m_settings[KEY_MARGIN_RIGHT] = right;
}

QString ProcessingProfile::alignmentVertical() const
{
    return m_settings.value(KEY_ALIGN_VERT, "center").toString();
}

QString ProcessingProfile::alignmentHorizontal() const
{
    return m_settings.value(KEY_ALIGN_HOR, "center").toString();
}

void ProcessingProfile::setAlignment(const QString& vertical, const QString& horizontal)
{
    m_settings[KEY_ALIGN_VERT] = vertical;
    m_settings[KEY_ALIGN_HOR] = horizontal;
}

QString ProcessingProfile::deskewMode() const
{
    return m_settings.value(KEY_DESKEW_MODE, "auto").toString();
}

void ProcessingProfile::setDeskewMode(const QString& mode)
{
    m_settings[KEY_DESKEW_MODE] = mode;
}

QDomElement ProcessingProfile::toXml(QDomDocument& doc) const
{
    QDomElement root = doc.createElement("profile");
    root.setAttribute("name", m_name);
    root.setAttribute("builtin", m_builtin ? "true" : "false");

    if (!m_description.isEmpty()) {
        QDomElement descEl = doc.createElement("description");
        descEl.appendChild(doc.createTextNode(m_description));
        root.appendChild(descEl);
    }

    QDomElement settingsEl = doc.createElement("settings");
    for (auto it = m_settings.constBegin(); it != m_settings.constEnd(); ++it) {
        QDomElement settingEl = doc.createElement("setting");
        settingEl.setAttribute("key", it.key());
        settingEl.setAttribute("value", it.value().toString());
        settingsEl.appendChild(settingEl);
    }
    root.appendChild(settingsEl);

    return root;
}

ProcessingProfile ProcessingProfile::fromXml(const QDomElement& element)
{
    ProcessingProfile profile(element.attribute("name"));
    profile.setBuiltin(element.attribute("builtin") == "true");

    QDomElement descEl = element.firstChildElement("description");
    if (!descEl.isNull()) {
        profile.setDescription(descEl.text());
    }

    QDomElement settingsEl = element.firstChildElement("settings");
    QDomElement settingEl = settingsEl.firstChildElement("setting");
    while (!settingEl.isNull()) {
        QString key = settingEl.attribute("key");
        QString value = settingEl.attribute("value");
        profile.setSetting(key, value);
        settingEl = settingEl.nextSiblingElement("setting");
    }

    return profile;
}

// ============================================================================
// ProfileManager Implementation
// ============================================================================

ProfileManager& ProfileManager::instance()
{
    static ProfileManager instance;
    return instance;
}

ProfileManager::ProfileManager()
    : m_activeProfileName("Default")
{
    // Set profiles directory
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    m_profilesDir = configPath + "/profiles";

    // Create directory if it doesn't exist
    QDir dir(m_profilesDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    initBuiltinProfiles();
    loadProfiles();
}

void ProfileManager::initBuiltinProfiles()
{
    m_profiles["Default"] = createDefaultProfile();
    m_profiles["Source"] = createSourceProfile();
}

ProcessingProfile ProfileManager::createDefaultProfile()
{
    ProcessingProfile profile("Default");
    profile.setBuiltin(true);
    profile.setDescription("Standard processing settings for most documents");

    profile.setOutputDpi(600);
    profile.setColorMode("black_and_white");
    profile.setDespeckleLevel("cautious");
    profile.setDewarpingMode("off");
    profile.setBinarizationThreshold(0);
    profile.setMargins(5.0, 5.0, 5.0, 5.0);
    profile.setAlignment("center", "center");
    profile.setDeskewMode("auto");

    return profile;
}

ProcessingProfile ProfileManager::createSourceProfile()
{
    ProcessingProfile profile("Source");
    profile.setBuiltin(true);
    profile.setDescription("Minimal processing - preserve original as much as possible");

    profile.setOutputDpi(300);
    profile.setColorMode("color_grayscale");
    profile.setDespeckleLevel("off");
    profile.setDewarpingMode("off");
    profile.setBinarizationThreshold(0);
    profile.setMargins(0.0, 0.0, 0.0, 0.0);
    profile.setAlignment("original", "original");
    profile.setDeskewMode("off");

    return profile;
}

QStringList ProfileManager::profileNames() const
{
    return m_profiles.keys();
}

ProcessingProfile* ProfileManager::profile(const QString& name)
{
    if (m_profiles.contains(name)) {
        return &m_profiles[name];
    }
    return nullptr;
}

const ProcessingProfile* ProfileManager::profile(const QString& name) const
{
    auto it = m_profiles.find(name);
    if (it != m_profiles.end()) {
        return &it.value();
    }
    return nullptr;
}

bool ProfileManager::addProfile(const ProcessingProfile& profile)
{
    if (profile.name().isEmpty() || m_profiles.contains(profile.name())) {
        return false;
    }

    m_profiles[profile.name()] = profile;
    saveProfiles();
    return true;
}

bool ProfileManager::removeProfile(const QString& name)
{
    if (!m_profiles.contains(name)) {
        return false;
    }

    // Can't remove builtin profiles
    if (m_profiles[name].isBuiltin()) {
        return false;
    }

    // Remove the profile file
    QString filePath = profileFilePath(name);
    QFile::remove(filePath);

    m_profiles.remove(name);

    // Reset active profile if needed
    if (m_activeProfileName == name) {
        m_activeProfileName = "Default";
    }

    return true;
}

bool ProfileManager::renameProfile(const QString& oldName, const QString& newName)
{
    if (!m_profiles.contains(oldName) || m_profiles.contains(newName)) {
        return false;
    }

    if (m_profiles[oldName].isBuiltin()) {
        return false;
    }

    ProcessingProfile profile = m_profiles.take(oldName);
    profile.setName(newName);
    m_profiles[newName] = profile;

    // Remove old file
    QFile::remove(profileFilePath(oldName));

    saveProfiles();

    if (m_activeProfileName == oldName) {
        m_activeProfileName = newName;
    }

    return true;
}

bool ProfileManager::duplicateProfile(const QString& sourceName, const QString& newName)
{
    if (!m_profiles.contains(sourceName) || m_profiles.contains(newName)) {
        return false;
    }

    ProcessingProfile newProfile = m_profiles[sourceName];
    newProfile.setName(newName);
    newProfile.setBuiltin(false);

    m_profiles[newName] = newProfile;
    saveProfiles();

    return true;
}

ProcessingProfile* ProfileManager::activeProfile()
{
    return profile(m_activeProfileName);
}

void ProfileManager::setActiveProfile(const QString& name)
{
    if (m_profiles.contains(name)) {
        m_activeProfileName = name;
    }
}

QString ProfileManager::profilesDirectory() const
{
    return m_profilesDir;
}

QString ProfileManager::profileFilePath(const QString& name) const
{
    QString safeName = name;
    safeName.replace(QRegExp("[^a-zA-Z0-9_-]"), "_");
    return m_profilesDir + "/" + safeName + ".xml";
}

void ProfileManager::loadProfiles()
{
    QDir dir(m_profilesDir);
    QStringList filters;
    filters << "*.xml";

    QStringList files = dir.entryList(filters, QDir::Files);
    for (const QString& file : files) {
        QString filePath = m_profilesDir + "/" + file;
        QFile f(filePath);
        if (f.open(QIODevice::ReadOnly)) {
            QDomDocument doc;
            if (doc.setContent(&f)) {
                QDomElement root = doc.documentElement();
                if (root.tagName() == "profile") {
                    ProcessingProfile profile = ProcessingProfile::fromXml(root);
                    if (!profile.name().isEmpty() && !profile.isBuiltin()) {
                        m_profiles[profile.name()] = profile;
                    }
                }
            }
            f.close();
        }
    }
}

void ProfileManager::saveProfiles()
{
    for (auto it = m_profiles.constBegin(); it != m_profiles.constEnd(); ++it) {
        // Skip builtin profiles
        if (it.value().isBuiltin()) {
            continue;
        }

        QString filePath = profileFilePath(it.key());
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QDomDocument doc;
            QDomElement root = it.value().toXml(doc);
            doc.appendChild(root);

            QTextStream stream(&file);
            stream << doc.toString(4);
            file.close();
        }
    }
}

void ProfileManager::applyToSettings(const ProcessingProfile& profile)
{
    // This method would integrate with GlobalStaticSettings and filter settings
    // to apply the profile's values
    // Implementation depends on how settings are structured in the application
}
