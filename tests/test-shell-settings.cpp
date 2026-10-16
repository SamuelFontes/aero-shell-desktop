/* SPDX-License-Identifier: MIT */
#include "../src/shell/shellsettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QUrl>

#include <cstdlib>

namespace
{
int fail(const char *message)
{
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
    qputenv("XDG_CONFIG_HOME", configHome.path().toLocal8Bit());
    QCoreApplication app(argc, argv);

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
