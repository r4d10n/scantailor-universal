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

#ifndef PROFILEMANAGER_H
#define PROFILEMANAGER_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariant>
#include <QDomDocument>
#include <QDomElement>
#include <memory>

/**
 * @brief A processing profile containing default settings for all filter stages
 *
 * Profiles can be saved, loaded, and applied to projects to quickly set up
 * processing parameters.
 */
class ProcessingProfile
{
public:
    ProcessingProfile(const QString& name = QString());

    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    QString description() const { return m_description; }
    void setDescription(const QString& desc) { m_description = desc; }

    // Generic setting access
    QVariant setting(const QString& key) const;
    void setSetting(const QString& key, const QVariant& value);
    bool hasSetting(const QString& key) const;
    QStringList settingKeys() const;

    // Specific filter settings
    // Output filter
    int outputDpi() const;
    void setOutputDpi(int dpi);
    QString colorMode() const;
    void setColorMode(const QString& mode);
    QString despeckleLevel() const;
    void setDespeckleLevel(const QString& level);
    QString dewarpingMode() const;
    void setDewarpingMode(const QString& mode);
    int binarizationThreshold() const;
    void setBinarizationThreshold(int threshold);

    // Page layout
    double marginTop() const;
    double marginBottom() const;
    double marginLeft() const;
    double marginRight() const;
    void setMargins(double top, double bottom, double left, double right);
    QString alignmentVertical() const;
    QString alignmentHorizontal() const;
    void setAlignment(const QString& vertical, const QString& horizontal);

    // Deskew
    QString deskewMode() const;
    void setDeskewMode(const QString& mode);

    // Serialization
    QDomElement toXml(QDomDocument& doc) const;
    static ProcessingProfile fromXml(const QDomElement& element);

    bool isBuiltin() const { return m_builtin; }
    void setBuiltin(bool builtin) { m_builtin = builtin; }

private:
    QString m_name;
    QString m_description;
    QMap<QString, QVariant> m_settings;
    bool m_builtin;
};


/**
 * @brief Manages processing profiles for filter settings
 *
 * Provides built-in profiles (Default, Source) and support for user-created
 * custom profiles stored in the config/profiles directory.
 */
class ProfileManager
{
public:
    static ProfileManager& instance();

    // Profile management
    QStringList profileNames() const;
    ProcessingProfile* profile(const QString& name);
    const ProcessingProfile* profile(const QString& name) const;

    bool addProfile(const ProcessingProfile& profile);
    bool removeProfile(const QString& name);
    bool renameProfile(const QString& oldName, const QString& newName);
    bool duplicateProfile(const QString& sourceName, const QString& newName);

    // Active profile
    QString activeProfileName() const { return m_activeProfileName; }
    ProcessingProfile* activeProfile();
    void setActiveProfile(const QString& name);

    // Built-in profiles
    static ProcessingProfile createDefaultProfile();
    static ProcessingProfile createSourceProfile();

    // Persistence
    void loadProfiles();
    void saveProfiles();
    QString profilesDirectory() const;

    // Apply profile to settings
    void applyToSettings(const ProcessingProfile& profile);

private:
    ProfileManager();
    ~ProfileManager() = default;
    ProfileManager(const ProfileManager&) = delete;
    ProfileManager& operator=(const ProfileManager&) = delete;

    void initBuiltinProfiles();
    QString profileFilePath(const QString& name) const;

    QMap<QString, ProcessingProfile> m_profiles;
    QString m_activeProfileName;
    QString m_profilesDir;
};

#endif // PROFILEMANAGER_H
