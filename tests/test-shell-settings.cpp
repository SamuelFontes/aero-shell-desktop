/* SPDX-License-Identifier: MIT */
#include "../src/shell/shellsettings.h"

#include <QGuiApplication>
#include <QIcon>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QUrl>

#include <cstdlib>
#include <cstdio>

namespace
{
int fail(const char *message)
{
    std::fprintf(stderr, "FAIL: %s\n", message);
    qCritical("FAIL: %s", message);
    return EXIT_FAILURE;
}
}

int main(int argc, char **argv)
{
    QTemporaryDir configHome;
    if (!configHome.isValid()) {
        return fail("could not create an isolated settings fixture");
    }
    QTemporaryDir iconThemeHome;
    if (!iconThemeHome.isValid() || !QDir(iconThemeHome.path()).mkdir(QStringLiteral("AeroShellFixture"))) {
        return fail("could not create an isolated icon theme fixture");
    }
    QFile iconThemeIndex(QDir(iconThemeHome.path()).filePath(QStringLiteral("AeroShellFixture/index.theme")));
    if (!iconThemeIndex.open(QIODevice::WriteOnly)
        || iconThemeIndex.write("[Icon Theme]\nName=AeroShell Fixture\nDirectories=16x16/apps\n") < 0) {
        return fail("could not create the icon theme fixture index");
    }
    iconThemeIndex.close();
    qputenv("XDG_CONFIG_HOME", configHome.path().toLocal8Bit());
    QGuiApplication app(argc, argv);
    QStringList iconPaths{iconThemeHome.path(),
                          QString::fromLocal8Bit(AEROSHELL_SOURCE_DIR) + QStringLiteral("/assets/icons")};
    iconPaths.append(QIcon::themeSearchPaths());
    QIcon::setThemeSearchPaths(iconPaths);

    ShellSettings settings;
    const QStringList widgets{
        QStringLiteral("network"), QStringLiteral("audio"), QStringLiteral("battery"),
        QStringLiteral("tray"), QStringLiteral("keyboard"), QStringLiteral("notifications"),
        QStringLiteral("clock"),
    };
    for (const QString &widget : widgets) {
        if (!settings.panelWidgets().value(widget).toBool()) {
            return fail("a supported panel widget is disabled in a new profile");
        }
    }

    int settingsChanges = 0;
    QObject::connect(&settings, &ShellSettings::settingsChanged, [&settingsChanges] {
        ++settingsChanges;
    });
    settings.setPanelWidgetVisible(QStringLiteral("audio"), false);
    if (settings.panelWidgets().value(QStringLiteral("audio")).toBool() || settingsChanges != 1) {
        return fail("widget visibility changes are not reflected and announced");
    }

    const QString configPath = QDir(configHome.path()).filePath(QStringLiteral("aeroshell/settings.ini"));
    QSettings persisted(configPath, QSettings::IniFormat);
    if (persisted.value(QStringLiteral("widgets/panel/audio"), true).toBool()) {
        return fail("widget preferences were not stored in the AeroShell settings file");
    }

    settings.setPanelWidgetVisible(QStringLiteral("not-a-widget"), false);
    if (settingsChanges != 1) {
        return fail("unknown settings keys were accepted");
    }
    settings.resetPanelWidgets();
    if (!settings.panelWidgets().value(QStringLiteral("audio")).toBool()) {
        return fail("the widget reset did not restore the complete default panel");
    }

    const QStringList fonts = settings.availableFonts();
    if (!fonts.contains(QStringLiteral("Noto Sans"), Qt::CaseInsensitive)) {
        return fail("the mandatory Noto Sans default is not available to AeroShell settings");
    }
    const QStringList iconThemes = settings.availableIconThemes();
    if (!iconThemes.contains(QStringLiteral("Windows 7 Aero"))) {
        return fail("the mandatory Windows 7 Aero icon theme is not available to AeroShell settings");
    }
    const int beforeAppearance = settingsChanges;
    settings.setFontFamily(QStringLiteral("AeroShell Missing Font"));
    if (settings.fontFamily() != QStringLiteral("Noto Sans") || settingsChanges != beforeAppearance) {
        return fail("an unavailable font was accepted by AeroShell settings");
    }
    settings.setFontFamily(QStringLiteral("Noto Sans"));
    settings.setIconTheme(QStringLiteral("AeroShell Missing Theme"));
    if (settings.iconTheme() != QStringLiteral("Windows 7 Aero")
        || settingsChanges != beforeAppearance + 1) {
        return fail("an unavailable icon theme was accepted by AeroShell settings");
    }
    const QString alternateTheme = QStringLiteral("AeroShellFixture");
    if (!iconThemes.contains(alternateTheme)) {
        return fail("the installed icon theme search paths were not exposed to AeroShell settings");
    }
    settings.setIconTheme(alternateTheme);
    if (settings.iconTheme() != alternateTheme || settings.iconThemeRevision() != 1) {
        return fail("a supported shell icon theme did not persist and advance its image revision");
    }
    persisted.sync();
    if (persisted.value(QStringLiteral("appearance/fontFamily")).toString() != QStringLiteral("Noto Sans")
        || persisted.value(QStringLiteral("appearance/iconTheme")).toString() != alternateTheme) {
        return fail("font and icon theme selections were not stored in AeroShell's private settings");
    }
    settings.resetInterfaceAppearance();
    if (settings.fontFamily() != QStringLiteral("Noto Sans")
        || settings.iconTheme() != QStringLiteral("Windows 7 Aero")
        || settings.iconThemeRevision() != 2) {
        return fail("the interface appearance reset did not restore AeroThemePlasma defaults");
    }

    QTemporaryDir wallpaperDirectory;
    const QString wallpaperPath = QDir(wallpaperDirectory.path()).filePath(QStringLiteral("wallpaper.jpg"));
    QFile wallpaper(wallpaperPath);
    if (!wallpaper.open(QIODevice::WriteOnly) || wallpaper.write("fixture") < 0) {
        return fail("could not create a wallpaper fixture");
    }
    wallpaper.close();
    settings.setWallpaper(QStringLiteral("DP-1"), QUrl::fromLocalFile(wallpaperPath));
    if (settings.wallpaperUrl(QStringLiteral("DP-1")).toLocalFile() != wallpaperPath) {
        return fail("wallpaper preferences are not stored per display");
    }
    settings.resetWallpaper(QStringLiteral("DP-1"));
    if (!settings.wallpaperUrl(QStringLiteral("DP-1")).isEmpty()) {
        return fail("wallpaper reset did not clear its private override");
    }

    const QStringList entries = QDir(configHome.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
    if (entries != QStringList{QStringLiteral("aeroshell")}) {
        return fail("settings wrote outside the AeroShell-owned configuration directory");
    }

    qInfo("PASS: new profiles enable all panel widgets; widget and wallpaper changes stay in AeroShell settings");
    return EXIT_SUCCESS;
}
