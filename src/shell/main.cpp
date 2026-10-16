/* SPDX-License-Identifier: MIT */
#include "applicationmodel.h"
#include "audiocontrol.h"
#include "batterystatus.h"
#include "desktopitemmodel.h"
#include "dbusmenumodel.h"
#include "keyboardlayoutstatus.h"
#include "networkstatus.h"
#include "notifications.h"
#include "polkitagent.h"
#include "poweractions.h"
#include "screenlockcontroller.h"
#include "shellbridge.h"
#include "shellsettings.h"
#include "screenevents.h"
#include "shellsounds.h"
#include "statusnotifier.h"
#include "themeiconprovider.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QIcon>
#include <QPalette>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include <QQuickStyle>
#include <QStringList>
#include <QUrl>

namespace
{
QColor schemeColor(const QSettings &scheme, const QString &key, const QColor &fallback)
{
    const QVariant value = scheme.value(key);
    QStringList channels = value.toStringList();
    if (channels.size() != 3) {
        channels = value.toString().split(QLatin1Char(','));
    }
    if (channels.size() != 3) {
        return fallback;
    }
    bool redValid = false;
    bool greenValid = false;
    bool blueValid = false;
    const int red = channels.at(0).toInt(&redValid);
    const int green = channels.at(1).toInt(&greenValid);
    const int blue = channels.at(2).toInt(&blueValid);
    if (!redValid || !greenValid || !blueValid || red < 0 || red > 255 || green < 0 || green > 255
        || blue < 0 || blue > 255) {
        return fallback;
    }
    return QColor(red, green, blue);
}
}

int main(int argc, char *argv[])
{
    const QByteArray privateBus = qgetenv("AEROSHELL_SESSION_BUS_ADDRESS");
    if (privateBus.isEmpty() || privateBus != qgetenv("DBUS_SESSION_BUS_ADDRESS")) {
        qCritical("Start AeroShell through its display-manager session entry so it has its own session bus.");
        return 2;
    }
    QGuiApplication::setApplicationName(QStringLiteral("AeroShell"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("AeroShell"));
    QGuiApplication::setOrganizationName(QStringLiteral("AeroShell"));
    QGuiApplication::setDesktopFileName(QStringLiteral("org.aeroshell.shell"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    const QString assetsPath = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("../share/aeroshell"));
    const QString colorSchemePath = QDir(assetsPath).absoluteFilePath(QStringLiteral("color-schemes/AeroColorScheme1.colors"));
    const QSettings colorScheme(colorSchemePath, QSettings::IniFormat);
    QPalette aeroPalette;
    aeroPalette.setColor(QPalette::Window, schemeColor(colorScheme, QStringLiteral("Colors:Window/BackgroundNormal"), QColor(QStringLiteral("#e0dfde"))));
    aeroPalette.setColor(QPalette::WindowText, schemeColor(colorScheme, QStringLiteral("Colors:Window/ForegroundNormal"), QColor(QStringLiteral("#141312"))));
    aeroPalette.setColor(QPalette::Base, schemeColor(colorScheme, QStringLiteral("Colors:View/BackgroundNormal"), QColor(QStringLiteral("#ffffff"))));
    aeroPalette.setColor(QPalette::AlternateBase, schemeColor(colorScheme, QStringLiteral("Colors:View/BackgroundAlternate"), QColor(QStringLiteral("#f8f7f6"))));
    aeroPalette.setColor(QPalette::Text, schemeColor(colorScheme, QStringLiteral("Colors:View/ForegroundNormal"), QColor(QStringLiteral("#141312"))));
    aeroPalette.setColor(QPalette::Button, schemeColor(colorScheme, QStringLiteral("Colors:Button/BackgroundNormal"), QColor(QStringLiteral("#e8e7e6"))));
    aeroPalette.setColor(QPalette::ButtonText, schemeColor(colorScheme, QStringLiteral("Colors:Button/ForegroundNormal"), QColor(QStringLiteral("#141312"))));
    aeroPalette.setColor(QPalette::PlaceholderText, schemeColor(colorScheme, QStringLiteral("Colors:View/ForegroundInactive"), QColor(QStringLiteral("#888786"))));
    aeroPalette.setColor(QPalette::Highlight, schemeColor(colorScheme, QStringLiteral("Colors:Selection/BackgroundNormal"), QColor(QStringLiteral("#418bd4"))));
    aeroPalette.setColor(QPalette::Accent, schemeColor(colorScheme, QStringLiteral("Colors:Window/DecorationFocus"), QColor(QStringLiteral("#2b74c7"))));
    aeroPalette.setColor(QPalette::HighlightedText, schemeColor(colorScheme, QStringLiteral("Colors:Selection/ForegroundNormal"), QColor(QStringLiteral("#000000"))));
    aeroPalette.setColor(QPalette::Link, schemeColor(colorScheme, QStringLiteral("Colors:View/ForegroundLink"), QColor(QStringLiteral("#0057ae"))));
    aeroPalette.setColor(QPalette::ToolTipBase, schemeColor(colorScheme, QStringLiteral("Colors:Tooltip/BackgroundNormal"), QColor(QStringLiteral("#ffffff"))));
    aeroPalette.setColor(QPalette::ToolTipText, schemeColor(colorScheme, QStringLiteral("Colors:Tooltip/ForegroundNormal"), QColor(QStringLiteral("#141312"))));
    aeroPalette.setColor(QPalette::Disabled, QPalette::Text, schemeColor(colorScheme, QStringLiteral("Colors:View/ForegroundInactive"), QColor(QStringLiteral("#888786"))));
    QGuiApplication::setPalette(aeroPalette);
    if (!QGuiApplication::platformName().contains(QStringLiteral("wayland"), Qt::CaseInsensitive)) {
        qCritical("AeroShell's first release requires a Wayland session.");
        return 2;
    }

    const QString defaultWallpaper = QDir(assetsPath).absoluteFilePath(QStringLiteral("wallpapers/aerothemeplasma-default.jpg"));
    const QString soundThemePath = QDir(assetsPath).absoluteFilePath(QStringLiteral("sounds/Windows 7/stereo"));
    const QString iconThemeIndex = QDir(assetsPath).absoluteFilePath(QStringLiteral("icons/Windows 7 Aero/index.theme"));
    const QString gtkTheme = QDir(assetsPath).absoluteFilePath(QStringLiteral("themes/AeroShellAero/gtk-3.0/gtk.css"));
    const QString fontConfig = QDir(assetsPath).absoluteFilePath(QStringLiteral("fontconfig/fonts.conf"));
    const QStringList requiredAppearanceFiles{
        defaultWallpaper,
        QDir(assetsPath).absoluteFilePath(QStringLiteral("sounds/Windows 7/index.theme")),
        QDir(soundThemePath).absoluteFilePath(QStringLiteral("desktop-login.ogg")),
        QDir(soundThemePath).absoluteFilePath(QStringLiteral("desktop-logout.ogg")),
        QDir(soundThemePath).absoluteFilePath(QStringLiteral("dialog-information.ogg")),
        QDir(soundThemePath).absoluteFilePath(QStringLiteral("dialog-warning.ogg")),
        QDir(soundThemePath).absoluteFilePath(QStringLiteral("dialog-error.ogg")),
        iconThemeIndex,
        QDir(assetsPath).absoluteFilePath(QStringLiteral("cursors/aero-drop/index.theme")),
        QDir(assetsPath).absoluteFilePath(QStringLiteral("cursors/aero-drop/cursors/left_ptr")),
        QDir(assetsPath).absoluteFilePath(QStringLiteral("icons/Windows 7 Aero/scalable/places/start-here.svg")),
        QDir(assetsPath).absoluteFilePath(QStringLiteral("icons/Windows 7 Aero/scalable/apps/application-x-generic.svg")),
        QDir(assetsPath).absoluteFilePath(QStringLiteral("icons/Windows 7 Aero/scalable/status/network-wireless.svg")),
        QDir(assetsPath).absoluteFilePath(QStringLiteral("icons/Windows 7 Aero/scalable/status/audio-volume-high.svg")),
        QDir(assetsPath).absoluteFilePath(QStringLiteral("icons/Windows 7 Aero/scalable/status/battery-full.svg")),
        QDir(assetsPath).absoluteFilePath(QStringLiteral("icons/Windows 7 Aero/scalable/status/notifications.svg")),
        gtkTheme,
        QDir(assetsPath).absoluteFilePath(QStringLiteral("themes/AeroShellAero/gtk-3.0/settings.ini")),
        QDir(assetsPath).absoluteFilePath(QStringLiteral("themes/AeroShellAero/gtk-4.0/gtk.css")),
        QDir(assetsPath).absoluteFilePath(QStringLiteral("widgets/aerothemeplasma-start-orb.png")),
        colorSchemePath,
        fontConfig,
    };
    for (const QString &requiredAsset : requiredAppearanceFiles) {
        if (!QFileInfo(requiredAsset).isFile()) {
            qCritical().noquote() << "AeroShell is missing a required default appearance asset:" << requiredAsset;
            return 5;
        }
    }
    if (!QFontDatabase::families().contains(QStringLiteral("Noto Sans"), Qt::CaseInsensitive)) {
        qCritical("AeroShell's required Noto Sans font is unavailable. Install the declared noto-fonts dependency.");
        return 5;
    }
    const QStringList iconPaths = QIcon::themeSearchPaths();
    QStringList aeroIconPaths{QDir(assetsPath).absoluteFilePath(QStringLiteral("icons"))};
    for (const QString &path : iconPaths) {
        if (!aeroIconPaths.contains(path)) {
            aeroIconPaths.push_back(path);
        }
    }
    QIcon::setThemeSearchPaths(aeroIconPaths);
    QIcon::setFallbackThemeName(QStringLiteral("Adwaita"));
    ShellSettings shellSettings(&app);
    const auto applyInterfaceAppearance = [&app, &shellSettings] {
        QIcon::setThemeName(shellSettings.iconTheme());
        QGuiApplication::setFont(QFont(shellSettings.fontFamily()));
    };
    applyInterfaceAppearance();
    QObject::connect(&shellSettings, &ShellSettings::settingsChanged, &app, applyInterfaceAppearance);
    QIcon::setThemeName(QStringLiteral("Windows 7 Aero"));
    for (const QString &requiredIcon : {QStringLiteral("start-here"), QStringLiteral("application-x-generic"),
                                        QStringLiteral("network-wireless"), QStringLiteral("audio-volume-high"),
                                        QStringLiteral("battery-full"), QStringLiteral("notifications")}) {
        if (!QIcon::hasThemeIcon(requiredIcon)) {
            qCritical().noquote() << "AeroShell's required Windows 7 Aero icon is unavailable:" << requiredIcon;
            return 5;
        }
    }
    QIcon::setThemeName(shellSettings.iconTheme());
    ApplicationModel applications(&app);
    AudioControl audioControl(&app);
    BatteryStatus batteryStatus(&app);
    DesktopItemModel desktopItems(&app);
    DBusMenuModel remoteMenu(&app);
    KeyboardLayoutStatus keyboardLayoutStatus(&app);
    NetworkStatus networkStatus(&app);
    NotificationsModel notifications(QDBusConnection::sessionBus(), &app);
    PolkitAgentModel polkitAgent(&app);
    PowerActions powerActions(&app);
    ScreenLockController screenLockController(&app);
    ShellBridge shellBridge(&app);
    ScreenEvents screenEvents(&app, &app);
    ShellSounds shellSounds(soundThemePath, &app);
    StatusNotifierModel statusNotifier(&app);

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("theme"), new ThemeIconProvider);
    engine.rootContext()->setContextProperty(QStringLiteral("applicationModel"), &applications);
    engine.rootContext()->setContextProperty(QStringLiteral("audioControl"), &audioControl);
    engine.rootContext()->setContextProperty(QStringLiteral("batteryStatus"), &batteryStatus);
    engine.rootContext()->setContextProperty(QStringLiteral("desktopItemModel"), &desktopItems);
    engine.rootContext()->setContextProperty(QStringLiteral("remoteMenuModel"), &remoteMenu);
    engine.rootContext()->setContextProperty(QStringLiteral("keyboardLayoutStatus"), &keyboardLayoutStatus);
    engine.rootContext()->setContextProperty(QStringLiteral("networkStatus"), &networkStatus);
    engine.rootContext()->setContextProperty(QStringLiteral("notificationModel"), &notifications);
    engine.rootContext()->setContextProperty(QStringLiteral("polkitAgentModel"), &polkitAgent);
    engine.rootContext()->setContextProperty(QStringLiteral("powerActions"), &powerActions);
    engine.rootContext()->setContextProperty(QStringLiteral("screenLockController"), &screenLockController);
    engine.rootContext()->setContextProperty(QStringLiteral("shellBridge"), &shellBridge);
    engine.rootContext()->setContextProperty(QStringLiteral("shellSettings"), &shellSettings);
    engine.rootContext()->setContextProperty(QStringLiteral("screenEvents"), &screenEvents);
    engine.rootContext()->setContextProperty(QStringLiteral("shellSounds"), &shellSounds);
    engine.rootContext()->setContextProperty(QStringLiteral("statusNotifierModel"), &statusNotifier);
    engine.rootContext()->setContextProperty(QStringLiteral("aeroWindowModel"), shellBridge.windowModel());
    engine.rootContext()->setContextProperty(QStringLiteral("aeroThemeAssetsPath"), assetsPath);
    engine.rootContext()->setContextProperty(
        QStringLiteral("aeroDefaultWallpaperUrl"),
        QUrl::fromLocalFile(defaultWallpaper));
    QObject::connect(&shellBridge, &ShellBridge::windowModelChanged, &engine, [&engine, &shellBridge] {
        engine.rootContext()->setContextProperty(QStringLiteral("aeroWindowModel"), shellBridge.windowModel());
    });
    QObject::connect(&shellBridge, &ShellBridge::integrationError, &app, [](const QString &message) {
        qCritical().noquote() << message;
        QCoreApplication::exit(4);
    });
    QObject::connect(&applications, &ApplicationModel::launchFailed, &app, [&notifications](const QString &name, const QString &message) {
        qWarning().noquote() << name << message;
        notifications.notify(QStringLiteral("AeroShell"), 0, QStringLiteral("dialog-error"),
            QStringLiteral("Could not open %1").arg(name), message, {}, {{QStringLiteral("urgency"), 2U}}, 0);
    });
    QObject::connect(&desktopItems, &DesktopItemModel::openFailed, &app, [&notifications](const QString &name, const QString &message) {
        qWarning().noquote() << name << message;
        notifications.notify(QStringLiteral("AeroShell"), 0, QStringLiteral("dialog-error"),
            QStringLiteral("Could not open %1").arg(name), message, {}, {{QStringLiteral("urgency"), 2U}}, 0);
    });

    engine.loadFromModule(QStringLiteral("AeroShell.Shell"), QStringLiteral("ShellRoot"));
    if (engine.rootObjects().isEmpty()) {
        return 3;
    }
    return app.exec();
}
