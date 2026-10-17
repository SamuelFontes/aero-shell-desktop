/* SPDX-License-Identifier: MIT */
#include "taskbarmodel.h"
#include "applicationmodel.h"
#include <KApplicationTrader>
#include <KService>
#include <algorithm>
#include <KWayland/Client/plasmawindowmodel.h>
using WindowModel = KWayland::Client::PlasmaWindowModel;
namespace { constexpr int countRole = Qt::UserRole + 1000; constexpr int pinnedRole = countRole + 1; }
TaskbarModel::TaskbarModel(ApplicationModel *applications, QObject *parent)
    : QAbstractListModel(parent), m_applications(applications)
{
    const auto browser = KApplicationTrader::preferredService(QStringLiteral("text/html"));
    if (browser) m_browser = browser->storageId().chopped(8);
    rebuild();
}
void TaskbarModel::setSource(QAbstractItemModel *source)
{
    if (m_source) disconnect(m_source, nullptr, this, nullptr);
    m_source = source;
    if (source) {
        connect(source, &QAbstractItemModel::modelReset, this, &TaskbarModel::rebuild);
        connect(source, &QAbstractItemModel::rowsInserted, this, &TaskbarModel::rebuild);
        connect(source, &QAbstractItemModel::rowsRemoved, this, &TaskbarModel::rebuild);
        connect(source, &QAbstractItemModel::dataChanged, this, &TaskbarModel::rebuild);
        connect(source, &QAbstractItemModel::layoutChanged, this, &TaskbarModel::rebuild);
        connect(source, &QObject::destroyed, this, &TaskbarModel::rebuild);
    }
    rebuild();
}
void TaskbarModel::rebuild()
{
    beginResetModel();
    m_groups = {{QStringLiteral("org.aeroshell.explorer"), tr("Windows Explorer"), {}, true}};
    if (!m_browser.isEmpty()) m_groups.append({m_browser, tr("Internet Browser"), {}, true});
    if (m_source) for (int row = 0; row < m_source->rowCount(); ++row) {
        const auto index = m_source->index(row, 0);
        if (m_source->data(index, WindowModel::SkipTaskbar).toBool()) continue;
        const auto appId = m_source->data(index, WindowModel::AppId).toString();
        auto group = std::find_if(m_groups.begin(), m_groups.end(), [&appId](const Group &g) { return !appId.isEmpty() && g.appId == appId; });
        if (group == m_groups.end()) {
            m_groups.append({appId, m_source->data(index, Qt::DisplayRole).toString(), {row}, false});
        } else group->rows.append(row);
    }
    endResetModel();
}
int TaskbarModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : m_groups.size(); }
int TaskbarModel::representative(int row) const
{
    if (!m_source || row < 0 || row >= m_groups.size() || m_groups.at(row).rows.isEmpty()) return -1;
    for (const int sourceRow : m_groups.at(row).rows) if (m_source->data(m_source->index(sourceRow, 0), WindowModel::IsActive).toBool()) return sourceRow;
    return m_groups.at(row).rows.first();
}
QVariant TaskbarModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_groups.size()) return {};
    const auto &group = m_groups.at(index.row());
    if (role == WindowModel::AppId) return group.appId;
    if (role == WindowModel::SkipTaskbar) return false;
    if (role == countRole) return group.rows.size();
    if (role == pinnedRole) return group.pinned;
    const int sourceRow = representative(index.row());
    if (role == Qt::DisplayRole && sourceRow < 0) return group.title;
    if (sourceRow >= 0) return m_source->data(m_source->index(sourceRow, 0), role);
    return false;
}
QHash<int, QByteArray> TaskbarModel::roleNames() const
{
    auto roles = m_source ? m_source->roleNames() : QHash<int, QByteArray>{
        {Qt::DisplayRole, "display"}, {WindowModel::AppId, "AppId"}, {WindowModel::SkipTaskbar, "SkipTaskbar"},
        {WindowModel::IsActive, "IsActive"}, {WindowModel::IsMinimized, "IsMinimized"},
        {WindowModel::IsMaximized, "IsMaximized"}, {WindowModel::IsCloseable, "IsCloseable"}};
    roles.insert(countRole, "WindowCount"); roles.insert(pinnedRole, "IsPinned"); return roles;
}
void TaskbarModel::invoke(const char *method, int row)
{
    const int sourceRow = representative(row);
    if (sourceRow >= 0) QMetaObject::invokeMethod(m_source, method, Q_ARG(int, sourceRow));
}
void TaskbarModel::requestActivate(int row)
{
    if (row < 0 || row >= m_groups.size()) return;
    if (m_groups.at(row).rows.isEmpty()) {
        if (!m_applications) return;
        if (m_groups.at(row).appId == QLatin1String("org.aeroshell.explorer")) m_applications->openHome();
        else m_applications->launch(m_groups.at(row).appId + QStringLiteral(".desktop"));
    } else invoke("requestActivate", row);
}
void TaskbarModel::requestClose(int row) { invoke("requestClose", row); }
void TaskbarModel::requestToggleMinimized(int row) { invoke("requestToggleMinimized", row); }
void TaskbarModel::requestToggleMaximized(int row) { invoke("requestToggleMaximized", row); }
QVariantList TaskbarModel::windows(int row) const
{
    QVariantList result;
    if (!m_source || row < 0 || row >= m_groups.size()) return result;
    for (const int sourceRow : m_groups.at(row).rows) result.append(QVariantMap{
        {QStringLiteral("row"), sourceRow}, {QStringLiteral("title"), m_source->data(m_source->index(sourceRow, 0), Qt::DisplayRole)}});
    return result;
}
void TaskbarModel::activateWindow(int sourceRow)
{
    if (m_source && sourceRow >= 0 && sourceRow < m_source->rowCount()) QMetaObject::invokeMethod(m_source, "requestActivate", Q_ARG(int, sourceRow));
}
