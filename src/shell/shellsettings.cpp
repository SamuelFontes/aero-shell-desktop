/* SPDX-License-Identifier: MIT */
#include "shellsettings.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

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

QVariantMap ShellSettings::panelWidgets() const
{
    QVariantMap result;
    for (const QString &widget : panelWidgetNames) {
        result.insert(widget, m_settings.value(panelWidgetKey(widget), true).toBool());
    }
    return result;
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
