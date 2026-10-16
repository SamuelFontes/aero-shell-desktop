/* SPDX-License-Identifier: MIT */
#include "shellsettings.h"

#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QIcon>
#include <QStandardPaths>

#include <algorithm>

namespace
{
const QStringList panelWidgetNames{
    QStringLiteral("network"),
    QStringLiteral("audio"),
    QStringLiteral("battery"),
    QStringLiteral("tray"),
    QStringLiteral("keyboard"),
    QStringLiteral("notifications"),
    QStringLiteral("clock"),
};

const QString defaultFontFamily = QStringLiteral("Noto Sans");
const QString defaultIconTheme = QStringLiteral("Windows 7 Aero");
}

ShellSettings::ShellSettings(QObject *parent)
    : QObject(parent)
    , m_settings(QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))
                     .filePath(QStringLiteral("aeroshell/settings.ini")),
                 QSettings::IniFormat)
{
}

QUrl ShellSettings::wallpaperUrl(const QString &screenName) const
{
    const QString path = m_settings.value(wallpaperKey(screenName)).toString();
    if (path.isEmpty() || !QFileInfo(path).isFile()) {
        return {};
    }
    return QUrl::fromLocalFile(path);
}

void ShellSettings::setWallpaper(const QString &screenName, const QUrl &url)
{
    if (!url.isLocalFile()) {
        return;
    }

    const QString path = url.toLocalFile();
    if (!QFileInfo(path).isFile()) {
        return;
    }

    m_settings.setValue(wallpaperKey(screenName), path);
    m_settings.sync();
    if (m_settings.status() != QSettings::NoError) {
        qWarning("AeroShell could not write its wallpaper setting.");
        return;
    }
    Q_EMIT settingsChanged();
}

void ShellSettings::resetWallpaper(const QString &screenName)
{
    m_settings.remove(wallpaperKey(screenName));
    m_settings.sync();
    if (m_settings.status() != QSettings::NoError) {
        qWarning("AeroShell could not reset its wallpaper setting.");
        return;
    }
    Q_EMIT settingsChanged();
}

void ShellSettings::setPanelWidgetVisible(const QString &widget, bool visible)
{
    if (!panelWidgetNames.contains(widget)) {
        return;
    }
    const QString key = panelWidgetKey(widget);
    if (m_settings.value(key, true).toBool() == visible) {
        return;
    }
    m_settings.setValue(key, visible);
    m_settings.sync();
    if (m_settings.status() != QSettings::NoError) {
        qWarning("AeroShell could not save its panel widget setting.");
        return;
    }
    Q_EMIT settingsChanged();
}

void ShellSettings::resetPanelWidgets()
{
    m_settings.beginGroup(QStringLiteral("widgets"));
    m_settings.remove(QStringLiteral("panel"));
    m_settings.endGroup();
    m_settings.sync();
    if (m_settings.status() != QSettings::NoError) {
        qWarning("AeroShell could not reset its panel widget settings.");
        return;
    }
    Q_EMIT settingsChanged();
}

void ShellSettings::setFontFamily(const QString &family)
{
    const QStringList fonts = availableFonts();
    const auto match = std::find_if(fonts.cbegin(), fonts.cend(), [&family](const QString &candidate) {
        return candidate.compare(family, Qt::CaseInsensitive) == 0;
    });
    if (match == fonts.cend()) {
        return;
    }

    m_settings.setValue(QStringLiteral("appearance/fontFamily"), *match);
    m_settings.sync();
    if (m_settings.status() != QSettings::NoError) {
        qWarning("AeroShell could not save its interface font setting.");
        return;
    }
    Q_EMIT settingsChanged();
}

void ShellSettings::setIconTheme(const QString &theme)
{
    const QStringList themes = availableIconThemes();
    if (!themes.contains(theme)) {
        return;
    }

    const QString current = iconTheme();
    if (current == theme) {
        return;
    }
    m_settings.setValue(QStringLiteral("appearance/iconTheme"), theme);
    m_settings.sync();
    if (m_settings.status() != QSettings::NoError) {
        qWarning("AeroShell could not save its icon theme setting.");
        return;
    }
    ++m_iconThemeRevision;
    Q_EMIT settingsChanged();
}

void ShellSettings::resetInterfaceAppearance()
{
    m_settings.remove(QStringLiteral("appearance/fontFamily"));
    m_settings.remove(QStringLiteral("appearance/iconTheme"));
    m_settings.sync();
    if (m_settings.status() != QSettings::NoError) {
        qWarning("AeroShell could not reset its interface appearance.");
        return;
    }
    ++m_iconThemeRevision;
    Q_EMIT settingsChanged();
}

QVariantMap ShellSettings::panelWidgets() const
{
    QVariantMap result;
    for (const QString &widget : panelWidgetNames) {
        result.insert(widget, m_settings.value(panelWidgetKey(widget), true).toBool());
    }
    return result;
}

QStringList ShellSettings::availableFonts() const
{
    QStringList fonts = QFontDatabase::families();
    fonts.removeDuplicates();
    fonts.sort(Qt::CaseInsensitive);
    return fonts;
}

QStringList ShellSettings::availableIconThemes() const
{
    QStringList themes;
    for (const QString &themeRoot : QIcon::themeSearchPaths()) {
        const QDir root(themeRoot);
        for (const QString &theme : root.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
            if (QFileInfo(root.filePath(theme + QStringLiteral("/index.theme"))).isFile()
                && !themes.contains(theme)) {
                themes.push_back(theme);
            }
        }
    }
    themes.sort(Qt::CaseInsensitive);
    return themes;
}

QString ShellSettings::fontFamily() const
{
    const QString requested = m_settings.value(QStringLiteral("appearance/fontFamily"), defaultFontFamily).toString();
    for (const QString &family : availableFonts()) {
        if (family.compare(requested, Qt::CaseInsensitive) == 0) {
            return family;
        }
    }
    return defaultFontFamily;
}

QString ShellSettings::iconTheme() const
{
    const QString requested = m_settings.value(QStringLiteral("appearance/iconTheme"), defaultIconTheme).toString();
    if (availableIconThemes().contains(requested)) {
        return requested;
    }
    if (availableIconThemes().contains(defaultIconTheme)) {
        return defaultIconTheme;
    }
    return QStringLiteral("hicolor");
}

int ShellSettings::iconThemeRevision() const
{
    return m_iconThemeRevision;
}

QString ShellSettings::wallpaperKey(const QString &screenName) const
{
    const QString key = QString::fromLatin1(QUrl::toPercentEncoding(screenName));
    return QStringLiteral("appearance/wallpaper/%1").arg(key.isEmpty() ? QStringLiteral("default") : key);
}

QString ShellSettings::panelWidgetKey(const QString &widget) const
{
    return QStringLiteral("widgets/panel/%1").arg(widget);
}
