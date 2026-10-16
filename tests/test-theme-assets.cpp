/* SPDX-License-Identifier: MIT */
#include <QFontDatabase>
#include <QGuiApplication>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QDir>
#include <QSettings>

#include "../src/shell/themeiconprovider.h"

#include <cstdio>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    const QString assets = QStringLiteral(AEROSHELL_SOURCE_DIR) + QStringLiteral("/assets");
    QIcon::setThemeSearchPaths({QDir(assets).absoluteFilePath(QStringLiteral("icons"))});
    QIcon::setFallbackThemeName(QStringLiteral("Adwaita"));
    QIcon::setThemeName(QStringLiteral("Windows 7 Aero"));

    if (!QFontDatabase::families().contains(QStringLiteral("Noto Sans"), Qt::CaseInsensitive)) {
        std::fputs("Noto Sans is unavailable to the AeroShell UI.\n", stderr);
        return 1;
    }

    for (const QString &name : {QStringLiteral("start-here"), QStringLiteral("network-wireless"),
                                QStringLiteral("audio-volume-high"), QStringLiteral("battery-full"),
                                QStringLiteral("notifications"),
                                QStringLiteral("folder-documents"), QStringLiteral("folder-pictures"),
                                QStringLiteral("folder-music"), QStringLiteral("folder-downloads"),
                                QStringLiteral("folder-videos")}) {
        const QIcon icon = QIcon::fromTheme(name);
        if (icon.isNull() || icon.pixmap(QSize(48, 48)).isNull()) {
            std::fprintf(stderr, "Theme icon did not render: %s\n", qPrintable(name));
            return 2;
        }
    }

    ThemeIconProvider iconProvider;
    QSize iconSize;
    for (const QString &name : {QStringLiteral("start-here"), QStringLiteral("network-wireless"), QStringLiteral("battery-full")}) {
        const QImage expected = QIcon::fromTheme(name).pixmap(QSize(32, 32)).toImage();
        const QImage actual = iconProvider.requestPixmap(name + QStringLiteral("?v=42"), &iconSize, QSize(32, 32)).toImage();
        if (actual != expected) {
            std::fprintf(stderr, "Cache-busting query changed the theme icon: %s\n", qPrintable(name));
            return 3;
        }
    }
    if (iconProvider.requestPixmap(QStringLiteral("aero-test-icon-without-a-theme-entry"), &iconSize, QSize(32, 32)).isNull()) {
        std::fputs("Unknown application icons do not fall back to the Aero generic file icon.\n", stderr);
        return 3;
    }

    QImage wallpaper(assets + QStringLiteral("/wallpapers/aerothemeplasma-default.jpg"));
    if (wallpaper.width() < 1280 || wallpaper.height() < 720) {
        std::fputs("The first-login wallpaper is missing or too small.\n", stderr);
        return 4;
    }

    const QSettings aeroColors(assets + QStringLiteral("/color-schemes/AeroColorScheme1.colors"), QSettings::IniFormat);
    const QStringList selection = aeroColors.value(QStringLiteral("Colors:Selection/BackgroundNormal")).toStringList();
    if (selection != QStringList{QStringLiteral("65"), QStringLiteral("139"), QStringLiteral("212")}) {
        std::fputs("The AeroThemePlasma selection color was not loaded.\n", stderr);
        return 5;
    }

    return 0;
}
