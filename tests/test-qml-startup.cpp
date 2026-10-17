/* SPDX-License-Identifier: MIT */
#include "../src/shell/themeiconprovider.h"
#include "../src/shell/notifications.h"
#include <KWayland/Client/plasmawindowmanagement.h>
#include <KWayland/Client/plasmawindowmodel.h>
#include <KService>

#include <QEventLoop>
#include <QDir>
#include <QImage>
#include <QGuiApplication>
#include <QIcon>
#include <QJSValue>
#include <QMetaProperty>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QQuickItem>
#include <QScreen>
#include <QStandardItemModel>
#include <QTimer>
#include <QTemporaryDir>
#include <QWindow>

#include <memory>
#include <cstdio>

static void settle()
{
    QEventLoop loop;
    QTimer::singleShot(40, &loop, &QEventLoop::quit);
    loop.exec();
}

int main(int argc, char **argv)
{
    QTemporaryDir isolation;
    if (!isolation.isValid()) {
        return 1;
    }
    qputenv("DBUS_SESSION_BUS_ADDRESS", QByteArray("unix:path=") + isolation.path().toUtf8() + "/no-bus");
    for (const char *key : {"XDG_CONFIG_HOME", "XDG_DATA_HOME", "XDG_CACHE_HOME", "XDG_STATE_HOME"}) {
        const QString directory = isolation.path() + QLatin1Char('/') + QString::fromLatin1(key);
        QDir().mkpath(directory);
        qputenv(key, directory.toUtf8());
    }
    qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &message) {
        std::fprintf(stderr, "%s\n", qPrintable(message));
    });
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    const QString source = QStringLiteral(AEROSHELL_SOURCE_DIR);
    const KService shellService(source + QStringLiteral("/session/org.aeroshell.shell.desktop"));
    if (!shellService.isApplication() || !shellService.noDisplay()
        || shellService.exec() != QStringLiteral("/opt/aeroshell/bin/aeroshell-shell")
        || !shellService.property<QStringList>(QStringLiteral("X-KDE-Wayland-Interfaces"))
                .contains(QStringLiteral("org_kde_plasma_window_management"))) {
        qCritical() << "The shell's application metadata does not authorize KWin's taskbar protocol:"
                    << shellService.isApplication() << shellService.noDisplay() << shellService.exec()
                    << shellService.property<QStringList>(QStringLiteral("X-KDE-Wayland-Interfaces"));
        return 1;
    }
    QIcon::setThemeSearchPaths({source + QStringLiteral("/assets/icons")});
    QIcon::setThemeName(QStringLiteral("Windows 7 Aero"));
    QQmlEngine engine;
    engine.addImageProvider(QStringLiteral("theme"), new ThemeIconProvider);
    QStringList warnings;
    QObject::connect(&engine, &QQmlEngine::warnings, &app, [&warnings](const QList<QQmlError> &errors) {
        for (const auto &error : errors) {
            warnings.push_back(error.toString());
        }
    });
    auto *context = engine.rootContext();
    context->setContextProperty(QStringLiteral("aeroThemeAssetsPath"), source + QStringLiteral("/assets"));
    context->setContextProperty(QStringLiteral("aeroDefaultWallpaperUrl"),
        QUrl::fromLocalFile(source + QStringLiteral("/assets/wallpapers/aerothemeplasma-default.jpg")));

    QQmlComponent fixture(&engine, QUrl::fromLocalFile(source + QStringLiteral("/tests/qml/BackendStubs.qml")));
    std::unique_ptr<QObject> backends(fixture.create());
    if (!backends) {
        qCritical() << fixture.errors();
        return 1;
    }
    const auto *meta = backends->metaObject();
    for (int i = meta->propertyOffset(); i < meta->propertyCount(); ++i) {
        const char *name = meta->property(i).name();
        context->setContextProperty(QString::fromLatin1(name), backends->property(name).value<QObject *>());
    }
    // The real notification model uses a disconnected bus, so populated
    // delegates are exercised without claiming the user's notification name.
    NotificationsModel notifications(QDBusConnection(QStringLiteral("aeroshell-qml-test-disconnected")));
    notifications.notify(QStringLiteral("Mail"), 0, QStringLiteral("notifications"), QStringLiteral("Test notification"),
        QStringLiteral("Test body"), {QStringLiteral("open"), QStringLiteral("Open")}, {}, 0);
    context->setContextProperty(QStringLiteral("notificationModel"), &notifications);
    KWayland::Client::PlasmaWindowManagement windowManagement;
    const auto *windowModel = windowManagement.createWindowModel();
    const auto roles = windowModel->roleNames();
    for (const auto &name : {"display", "AppId", "IsActive", "IsMinimized", "IsMaximized", "IsCloseable", "SkipTaskbar"}) {
        if (!roles.values().contains(QByteArray(name))) {
            qCritical() << "KWayland taskbar role does not exist:" << name << roles;
            return 1;
        }
    }
    QStandardItemModel taskWindows;
    taskWindows.setItemRoleNames(roles);
    taskWindows.setRowCount(1);
    taskWindows.setColumnCount(1);
    const auto task = taskWindows.index(0, 0);
    taskWindows.setData(task, QStringLiteral("Test window"), Qt::DisplayRole);
    taskWindows.setData(task, QStringLiteral("application-x-generic"), KWayland::Client::PlasmaWindowModel::AppId);
    for (const auto role : {KWayland::Client::PlasmaWindowModel::IsActive, KWayland::Client::PlasmaWindowModel::IsMinimized,
         KWayland::Client::PlasmaWindowModel::IsMaximized, KWayland::Client::PlasmaWindowModel::IsCloseable,
         KWayland::Client::PlasmaWindowModel::SkipTaskbar}) {
        taskWindows.setData(task, false, role);
    }
    context->setContextProperty(QStringLiteral("aeroWindowModel"), &taskWindows);

    QQmlComponent osdComponent(&engine, QUrl::fromLocalFile(source + QStringLiteral(
        "/assets/plasma/shells/org.aeroshell.desktop/contents/lockscreen/LockOsd.qml")));
    QQuickWindow osdView;
    std::unique_ptr<QObject> osd(osdComponent.create());
    if (!osd) {
        qCritical() << osdComponent.errors();
        return 1;
    }
    qobject_cast<QQuickItem *>(osd.get())->setParentItem(osdView.contentItem());
    osdView.resize(400, 120);
    osdView.show();
    osd->setProperty("icon", QStringLiteral("audio-volume-high"));
    osd->setProperty("osdValue", 50);
    QMetaObject::invokeMethod(osd.get(), "show");
    if (!osd->property("visible").toBool()) {
        qCritical("The lock-screen OSD did not load/show.");
        return 1;
    }
    osdView.hide();

    QQmlComponent component(&engine, QUrl::fromLocalFile(source + QStringLiteral("/src/shell/qml/ShellRoot.qml")));
    std::unique_ptr<QObject> root(component.create());
    if (!root) {
        qCritical() << component.errors();
        return 1;
    }
    settle();
    QObject *screenEvents = backends->property("screenEvents").value<QObject *>();
    QMetaObject::invokeMethod(screenEvents, "screensChanged");
    QMetaObject::invokeMethod(screenEvents, "screensChanged");
    settle();
    const QJSValue bundles = root->property("screenBundles").value<QJSValue>();
    if (bundles.property(QStringLiteral("length")).toInt() != app.screens().size()) {
        qCritical("Shell did not create a desktop bundle for every screen.");
        return 1;
    }
    int popupCount = 0;
    for (int i = 0; i < app.screens().size(); ++i) {
        const QJSValue bundle = bundles.property(i);
        auto *desktop = qobject_cast<QWindow *>(bundle.property(QStringLiteral("desktop")).toQObject());
        auto *panel = qobject_cast<QWindow *>(bundle.property(QStringLiteral("panel")).toQObject());
        const QRect geometry = app.screens().at(i)->geometry();
        if (!desktop || !panel || desktop->size() != geometry.size()
            || panel->width() != geometry.width() || panel->height() != 40
            || panel->y() != geometry.bottom() + 1 - 40) {
            qCritical() << "Desktop/panel screen geometry is invalid:" << geometry
                        << (desktop ? desktop->geometry() : QRect()) << (panel ? panel->geometry() : QRect());
            return 1;
        }
        // Empty space belongs to the window list, never between status widgets.
        // This catches RowLayout's expanding default size policy on the tray.
        for (int width : {800, 1280, 1920}) {
            panel->resize(width, panel->height());
            settle();
            auto *network = panel->findChild<QQuickItem *>(QStringLiteral("aeroshell-networkButton"));
            auto *tray = panel->findChild<QQuickItem *>(QStringLiteral("aeroshell-statusTray"));
            auto *showDesktop = panel->findChild<QQuickItem *>(QStringLiteral("aeroshell-showDesktopButton"));
            if (!network || !tray || !showDesktop
                || network->mapToScene(QPointF()).x() < width - 340
                || tray->width() > 40
                || qAbs(showDesktop->mapToScene(QPointF()).x() + showDesktop->width() - width) > 1) {
                qCritical() << "Status controls are spread across the taskbar at width" << width;
                return 1;
            }
        }
        panel->resize(geometry.width(), panel->height());
        settle();
        // A real Wayland toplevel is told (0,0), independent of the panel's
        // global compositor position. This must not move Start above the output.
        panel->setPosition(0, 0);
        if (panel->property("desiredY").toInt() != geometry.bottom() + 1 - 40
            || bundle.property(QStringLiteral("launcher")).toQObject()->property("desiredY").toInt()
                != geometry.bottom() + 1 - 40 - bundle.property(QStringLiteral("launcher")).toQObject()->property("height").toInt()) {
            qCritical("Compositor-local coordinates corrupted shell placement.");
            return 1;
        }
        panel->setPosition(geometry.x(), geometry.bottom() + 1 - 40);
        if (argc == 2) {
            const QDir output(QString::fromLocal8Bit(argv[1]));
            if (!output.mkpath(QStringLiteral("."))) {
                return 1;
            }
            for (QWindow *window : {desktop, panel}) {
                const auto image = qobject_cast<QQuickWindow *>(window)->grabWindow();
                const QString kind = window == desktop ? QStringLiteral("desktop") : QStringLiteral("panel");
                if (image.isNull() || !image.save(output.filePath(QStringLiteral("%1-%2.png").arg(kind).arg(i)))) {
                    qCritical("Could not save the offscreen UI preview.");
                    return 1;
                }
            }
        }
        // Panel popovers must have their own surface; item popovers are clipped
        // to the panel's 40 pixels by the compositor.
        for (QObject *child : panel->findChildren<QObject *>()) {
            const QVariant type = child->property("popupType");
            if (!type.isValid() || !child->objectName().startsWith(QStringLiteral("aeroshell-"))) {
                continue;
            }
            ++popupCount;
            if (type.toInt() != 1) { // Popup.Window
                qCritical("Panel popover does not use a separate window.");
                return 1;
            }
            QMetaObject::invokeMethod(child, "open");
            settle();
            if (!child->property("visible").toBool()) {
                qCritical("Panel popover did not open.");
                return 1;
            }
            QMetaObject::invokeMethod(child, "close");
        }
        auto *launcher = bundle.property(QStringLiteral("launcher")).toQObject();
        launcher->setProperty("visible", true);
        settle();
        if (argc == 2) {
            const QDir output(QString::fromLocal8Bit(argv[1]));
            auto *window = qobject_cast<QQuickWindow *>(launcher);
            if (!window || !window->grabWindow().save(output.filePath(QStringLiteral("start-%1.png").arg(i)))) {
                qCritical("Could not save the Start menu preview.");
                return 1;
            }
        }
        launcher->setProperty("visible", false);
    }
    if (popupCount < 7 * app.screens().size()) {
        qCritical("Expected panel popovers were not instantiated.");
        return 1;
    }
    if (!warnings.isEmpty()) {
        qCritical().noquote() << warnings.join(QLatin1Char('\n'));
        return 1;
    }
    qInfo("PASS: QML shell creation, actual screen geometry, launcher, and panel popover windows (offscreen, mocked services)");
    return 0;
}
