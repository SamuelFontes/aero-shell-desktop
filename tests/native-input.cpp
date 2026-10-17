// Synthetic input is restricted to the disposable virtual compositor fixture.
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/fakeinput.h>
#include <QGuiApplication>
#include <QTimer>
#include <QDebug>

int main(int argc, char **argv)
{
    if (qEnvironmentVariable("AEROSHELL_VIRTUAL_TEST") != QStringLiteral("1")) return 2;
    QGuiApplication::setDesktopFileName(QStringLiteral("org.aeroshell.validation"));
    QGuiApplication app(argc, argv);
    auto *connection = KWayland::Client::ConnectionThread::fromApplication(&app);
    if (!connection) return 3;
    KWayland::Client::Registry registry;
    registry.create(connection);
    QObject::connect(&registry, &KWayland::Client::Registry::fakeInputAnnounced, &app,
        [&](quint32 name, quint32 version) {
            auto *input = registry.createFakeInput(name, version, &app);
            input->authenticate(QStringLiteral("AeroShell validation"), QStringLiteral("Test the disposable virtual desktop"));
            QTimer::singleShot(200, &app, [&, input] {
                const auto args = app.arguments();
                if (args.size() == 3 && args[1] != QStringLiteral("keys")) {
                    input->requestPointerMoveAbsolute(QPointF(args[1].toDouble(), args[2].toDouble()));
                    input->requestPointerButtonClick(Qt::LeftButton);
                } else if (args.size() >= 3 && args[1] == QStringLiteral("keys")) {
                    for (int i = 2; i < args.size(); ++i) {
                        input->requestKeyboardKeyPress(args[i].toUInt());
                        input->requestKeyboardKeyRelease(args[i].toUInt());
                    }
                } else {
                    app.exit(4);
                    return;
                }
                QTimer::singleShot(200, &app, [&] { app.exit(0); });
            });
        });
    registry.setup();
    QTimer::singleShot(3000, &app, [&] { qWarning() << "No virtual input interface"; app.exit(5); });
    return app.exec();
}
