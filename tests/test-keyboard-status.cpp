/* SPDX-License-Identifier: MIT */
#include "../src/shell/keyboardlayoutstatus.h"
#include <QCoreApplication>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMetaType>
#include <QProcess>
#include <QTimer>
#include <QThread>
#include <QDebug>
struct Layout { QString shortName, displayName, longName; };
Q_DECLARE_METATYPE(Layout)
using Layouts = QList<Layout>;
Q_DECLARE_METATYPE(Layouts)
QDBusArgument &operator<<(QDBusArgument &arg, const Layout &layout)
{
    arg.beginStructure(); arg << layout.shortName << layout.displayName << layout.longName; arg.endStructure(); return arg;
}
const QDBusArgument &operator>>(const QDBusArgument &arg, Layout &layout)
{
    arg.beginStructure(); arg >> layout.shortName >> layout.displayName >> layout.longName; arg.endStructure(); return arg;
}
class Keyboard final : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.KeyboardLayouts")
public Q_SLOTS:
    Layouts getLayoutsList() { return {{QStringLiteral("us"), QStringLiteral("US"), QStringLiteral("English (US)")}, {QStringLiteral("de"), QStringLiteral("DE"), QStringLiteral("German")}}; }
    uint getLayout() { return 0; }
};
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    qDBusRegisterMetaType<Layout>(); qDBusRegisterMetaType<Layouts>();
    if (app.arguments().contains(QStringLiteral("--server"))) {
        Keyboard keyboard;
        auto bus = QDBusConnection::sessionBus();
        if (!bus.registerService(QStringLiteral("org.kde.keyboard")) || !bus.registerObject(QStringLiteral("/Layouts"), &keyboard, QDBusConnection::ExportAllSlots)) return 2;
        std::puts("ready"); std::fflush(stdout);
        return app.exec();
    }
    QProcess server;
    server.start(app.applicationFilePath(), {QStringLiteral("--server")});
    if (!server.waitForStarted(3000) || !server.waitForReadyRead(3000) || !server.readAllStandardOutput().contains("ready")) return 3;
    // A separate service process forces a real a(sss) wire reply, which caught
    // the non-const QDBusArgument overload that previously hung shell startup.
    KeyboardLayoutStatus status;
    const bool valid = status.available() && status.currentLabel() == QStringLiteral("US")
        && status.currentName() == QStringLiteral("English (US)") && status.layouts() == QStringList{QStringLiteral("US"), QStringLiteral("DE")};
    server.terminate(); server.waitForFinished(3000);
    if (!valid) { qCritical() << status.available() << status.layouts(); return 1; }
    return 0;
}
#include "test-keyboard-status.moc"
