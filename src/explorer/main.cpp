/* SPDX-License-Identifier: MIT */
#include "explorer.h"
#include "taskmanager.h"
#include "recyclebin.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QIcon>
#include <QTimer>
#include <QPixmap>
#include <QLineEdit>
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("AeroShell Explorer"));
    app.setDesktopFileName(QStringLiteral("org.aeroshell.explorer"));
    app.setStyle(QStringLiteral("Fusion"));
    QIcon::setThemeSearchPaths(QStringList{QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("../share/aeroshell/icons"))} + QIcon::themeSearchPaths());
    QIcon::setThemeName(QStringLiteral("Windows 7 Aero"));
    app.setFont(QFont(QStringLiteral("Noto Sans"), 9));
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("folder"), QStringLiteral("Folder to open"));
    parser.addOption({QStringLiteral("smoke-test"), QStringLiteral("Save a preview and exit"), QStringLiteral("image")});
    parser.addOption({QStringLiteral("task-manager"), QStringLiteral("Show running processes")});
    parser.addOption({QStringLiteral("recycle-bin"), QStringLiteral("Open the Recycle Bin")});
    parser.addOption({QStringLiteral("search"), QStringLiteral("Focus file search")});
    parser.process(app);
    QMainWindow *window = parser.isSet(QStringLiteral("task-manager")) ? static_cast<QMainWindow *>(new TaskManager) : parser.isSet(QStringLiteral("recycle-bin")) || parser.positionalArguments().value(0).startsWith(QStringLiteral("trash:")) ? static_cast<QMainWindow *>(new RecycleBin) : new Explorer(parser.positionalArguments().value(0));
    window->show();
    if (parser.isSet(QStringLiteral("search"))) {
        if (auto *search = window->findChild<QLineEdit *>(QStringLiteral("search"))) search->setFocus();
    }
    if (parser.isSet(QStringLiteral("smoke-test"))) QTimer::singleShot(1500, &app, [&] {
        app.exit(window->grab().save(parser.value(QStringLiteral("smoke-test"))) ? 0 : 1);
    });
    return app.exec();
}
