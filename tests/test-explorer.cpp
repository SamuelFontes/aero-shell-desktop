/* SPDX-License-Identifier: MIT */
#include "../src/explorer/explorer.h"
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileSystemModel>
#include <QLineEdit>
#include <QMimeData>
#include <QShortcut>
#include <QTemporaryDir>
#include <QTreeView>
#include <QTimer>
#include <QEventLoop>
#include <cstdio>
static void settle() { QEventLoop loop; QTimer::singleShot(150, &loop, &QEventLoop::quit); loop.exec(); }
#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr, "FAIL at line %d: %s\n", __LINE__, #expr); return 1; } } while(false)
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QTemporaryDir root;
    CHECK(root.isValid());
    QDir dir(root.path());
    CHECK(dir.mkdir(QStringLiteral("child")));
    QFile file(dir.filePath(QStringLiteral("sample.txt")));
    CHECK(file.open(QIODevice::WriteOnly)); file.write("sample"); file.close();
    Explorer window(root.path());
    window.show(); settle();
    auto *address = window.findChild<QLineEdit *>(QStringLiteral("address"));
    auto *view = window.findChild<QTreeView *>(QStringLiteral("details"));
    auto *model = qobject_cast<QFileSystemModel *>(view->model());
    CHECK(address && model);
    CHECK(model->filePath(view->rootIndex()) == root.path());
    window.navigate(dir.filePath(QStringLiteral("child"))); settle();
    CHECK(address->text() == dir.filePath(QStringLiteral("child")));
    for (auto *shortcut : window.findChildren<QShortcut *>()) {
        if (shortcut->key() == QKeySequence(Qt::ALT | Qt::Key_Left)) QMetaObject::invokeMethod(shortcut, "activated");
    }
    settle();
    CHECK(address->text() == root.path());
    const auto index = model->index(file.fileName());
    CHECK(index.isValid());
    view->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    for (auto *action : window.actions()) if (action->shortcut() == QKeySequence::Copy) action->trigger();
    CHECK(QApplication::clipboard()->mimeData()->urls() == QList<QUrl>{QUrl::fromLocalFile(file.fileName())});
    window.navigate(dir.filePath(QStringLiteral("child"))); settle();
    for (auto *action : window.actions()) if (action->shortcut() == QKeySequence::Paste) action->trigger();
    for (int retry = 0; retry < 30 && !QFileInfo::exists(dir.filePath(QStringLiteral("child/sample.txt"))); ++retry) settle();
    QFile copy(dir.filePath(QStringLiteral("child/sample.txt")));
    CHECK(copy.open(QIODevice::ReadOnly));
    CHECK(copy.readAll() == QByteArray("sample"));
    CHECK(QFileInfo::exists(file.fileName()));
    CHECK(QDir(dir.filePath(QStringLiteral("child"))).entryList(QDir::Files) == QStringList{QStringLiteral("sample.txt")});
    return 0;
}
