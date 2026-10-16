/* SPDX-License-Identifier: MIT */
#pragma once

#include <QObject>
#include <QUrl>
#include <QSettings>
#include <QStringList>
#include <QVariantMap>

class ShellSettings final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap panelWidgets READ panelWidgets NOTIFY settingsChanged)
    Q_PROPERTY(QStringList availableFonts READ availableFonts CONSTANT)
    Q_PROPERTY(QStringList availableIconThemes READ availableIconThemes CONSTANT)
    Q_PROPERTY(QString fontFamily READ fontFamily NOTIFY settingsChanged)
    Q_PROPERTY(QString iconTheme READ iconTheme NOTIFY settingsChanged)
    Q_PROPERTY(int iconThemeRevision READ iconThemeRevision NOTIFY settingsChanged)

public:
    explicit ShellSettings(QObject *parent = nullptr);

    Q_INVOKABLE QUrl wallpaperUrl(const QString &screenName) const;
    Q_INVOKABLE void setWallpaper(const QString &screenName, const QUrl &url);
    Q_INVOKABLE void resetWallpaper(const QString &screenName);
    Q_INVOKABLE void setPanelWidgetVisible(const QString &widget, bool visible);
    Q_INVOKABLE void resetPanelWidgets();
    Q_INVOKABLE void setFontFamily(const QString &family);
    Q_INVOKABLE void setIconTheme(const QString &theme);
    Q_INVOKABLE void resetInterfaceAppearance();

    QVariantMap panelWidgets() const;
    QStringList availableFonts() const;
    QStringList availableIconThemes() const;
    QString fontFamily() const;
    QString iconTheme() const;
    int iconThemeRevision() const;

Q_SIGNALS:
    void settingsChanged();

private:
    QSettings m_settings;
    int m_iconThemeRevision = 0;
    QString wallpaperKey(const QString &screenName) const;
    QString panelWidgetKey(const QString &widget) const;
};
