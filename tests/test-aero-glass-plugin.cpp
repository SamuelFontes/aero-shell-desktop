/* SPDX-License-Identifier: MIT */
#include <QCoreApplication>
#include <QDebug>
#include <QJsonObject>
#include <QPluginLoader>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc != 2) return 1;
    QPluginLoader effect(QString::fromLocal8Bit(argv[1]));
    if (!effect.metaData().value(QStringLiteral("IID")).toString().startsWith(QStringLiteral("org.kde.kwin.EffectPluginFactory"))
        || !effect.load()) {
        qCritical() << "Aero Glass is not a loadable KWin effect:" << effect.errorString();
        return 1;
    }
    return 0;
}
