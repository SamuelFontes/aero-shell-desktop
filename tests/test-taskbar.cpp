/* SPDX-License-Identifier: MIT */
#include "../src/shell/taskbarmodel.h"
#include <KWayland/Client/plasmawindowmodel.h>
#include <QCoreApplication>
#include <QStandardItemModel>
#include <QDebug>
using W = KWayland::Client::PlasmaWindowModel;
class Windows final : public QStandardItemModel
{
    Q_OBJECT
public:
    int activated = -1;
    Q_INVOKABLE void requestActivate(int row) { activated = row; }
};
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    Windows source;
    source.setColumnCount(1);
    source.setItemRoleNames({{Qt::DisplayRole, "display"}, {W::AppId, "AppId"}, {W::SkipTaskbar, "SkipTaskbar"},
        {W::IsActive, "IsActive"}, {W::IsMinimized, "IsMinimized"}, {W::IsMaximized, "IsMaximized"}, {W::IsCloseable, "IsCloseable"}});
    source.setRowCount(4);
    for (int row = 0; row < 4; ++row) {
        const auto i = source.index(row, 0);
        source.setData(i, row == 3 ? QStringLiteral("hidden") : QStringLiteral("org.aeroshell.explorer"), W::AppId);
        source.setData(i, QStringLiteral("Window %1").arg(row), Qt::DisplayRole);
        source.setData(i, row == 3, W::SkipTaskbar);
        source.setData(i, row == 1, W::IsActive);
    }
    TaskbarModel tasks(nullptr);
    tasks.setSource(&source);
    const int countRole = tasks.roleNames().key("WindowCount");
    if (tasks.data(tasks.index(0), countRole).toInt() != 3 || tasks.windows(0).size() != 3) return 1;
    tasks.requestActivate(0);
    if (source.activated != 1) return 2;
    source.removeRow(0);
    if (tasks.data(tasks.index(0), countRole).toInt() != 2) return 3;
    source.removeRows(0, 2);
    if (tasks.data(tasks.index(0), countRole).toInt() != 0 || !tasks.windows(0).isEmpty()) return 4;
    return 0;
}
#include "test-taskbar.moc"
