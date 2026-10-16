/* SPDX-License-Identifier: MIT */
#pragma once

#include <QObject>
#include <QUrl>
#include <QSettings>
#include <QVariantMap>

class ShellSettings final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap panelWidgets READ panelWidgets NOTIFY settingsChanged)

public:
    explicit ShellSettings(QObject *parent = nullptr);

    Q_INVOKABLE QUrl wallpaperUrl(const QString &screenName) const;
    Q_INVOKABLE void setWallpaper(const QString &screenName, const QUrl &url);
    Q_INVOKABLE void resetWallpaper(const QString &screenName);
    Q_INVOKABLE void setPanelWidgetVisible(const QString &widget, bool visible);
    Q_INVOKABLE void resetPanelWidgets();

    QVariantMap panelWidgets() const;

Q_SIGNALS:
    void settingsChanged();

private:
    QSettings m_settings;
    QString wallpaperKey(const QString &screenName) const;
    QString panelWidgetKey(const QString &widget) const;
};
