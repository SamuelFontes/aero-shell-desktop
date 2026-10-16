/* SPDX-License-Identifier: MIT */
#include "dbusmenumodel.h"

#include <QDBusArgument>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QDateTime>
#include <QVariantList>

#include <algorithm>
#include <utility>

namespace
{
constexpr auto menuInterface = "com.canonical.dbusmenu";

QVariant unpack(const QVariant &value)
{
    if (value.metaType() == QMetaType::fromType<QDBusVariant>()) {
        return value.value<QDBusVariant>().variant();
    }
    return value;
}

QVariant property(const QVariantMap &properties, const QString &key)
{
    return unpack(properties.value(key));
}

QString displayLabel(QString label)
{
    QString result;
    result.reserve(label.size());
    for (qsizetype i = 0; i < label.size(); ++i) {
        if (label.at(i) == QLatin1Char('_')) {
            if (i + 1 < label.size() && label.at(i + 1) == QLatin1Char('_')) {
                result.append(QLatin1Char('_'));
                ++i;
            }
            continue;
        }
        result.append(label.at(i));
    }
    return result;
}

bool readLayoutNode(const QVariant &value, int *id, QVariantMap *properties, QVariantList *children)
{
    const QVariant unwrapped = unpack(value);
    if (!unwrapped.canConvert<QDBusArgument>()) {
        return false;
    }

    QDBusArgument argument = unwrapped.value<QDBusArgument>();
    argument.beginStructure();
    argument >> *id;
    argument >> *properties;
    argument >> *children;
    argument.endStructure();
    return true;
}

QVariantMap menuEntryMap(const QVariant &value)
{
    int id = 0;
    QVariantMap properties;
    QVariantList children;
    if (!readLayoutNode(value, &id, &properties, &children)) {
        return {};
    }

    const QString type = property(properties, QStringLiteral("type")).toString();
    const QString toggleType = property(properties, QStringLiteral("toggle-type")).toString();
    const QString childrenDisplay = property(properties, QStringLiteral("children-display")).toString();
    const QString rawLabel = property(properties, QStringLiteral("label")).toString();

    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("label"), displayLabel(rawLabel)},
        {QStringLiteral("iconName"), property(properties, QStringLiteral("icon-name")).toString()},
        {QStringLiteral("enabled"), !properties.contains(QStringLiteral("enabled"))
             || property(properties, QStringLiteral("enabled")).toBool()},
        {QStringLiteral("visible"), !properties.contains(QStringLiteral("visible"))
             || property(properties, QStringLiteral("visible")).toBool()},
        {QStringLiteral("separator"), type == QLatin1String("separator")},
        {QStringLiteral("checkable"), toggleType == QLatin1String("checkmark")
             || toggleType == QLatin1String("radio")},
        {QStringLiteral("checked"), property(properties, QStringLiteral("toggle-state")).toInt() == 1},
        {QStringLiteral("submenu"), !children.isEmpty() || childrenDisplay == QLatin1String("submenu")},
    };
}

}

DBusMenuModel::DBusMenuModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_bus(QDBusConnection::sessionBus())
{
}

int DBusMenuModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant DBusMenuModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return {};
    }
    const Entry &entry = m_entries.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case LabelRole:
        return entry.label;
    case ItemIdRole:
        return entry.id;
    case ItemEnabledRole:
        return entry.enabled;
    case SeparatorRole:
        return entry.separator;
    case ItemCheckableRole:
        return entry.checkable;
    case ItemCheckedRole:
        return entry.checked;
    case SubmenuRole:
        return entry.submenu;
    case IconNameRole:
        return entry.iconName;
    default:
        return {};
    }
}

QHash<int, QByteArray> DBusMenuModel::roleNames() const
{
    return {
        {Qt::DisplayRole, "display"},
        {ItemIdRole, "itemId"},
        {LabelRole, "label"},
        {ItemEnabledRole, "itemEnabled"},
        {SeparatorRole, "separator"},
        {ItemCheckableRole, "itemCheckable"},
        {ItemCheckedRole, "itemChecked"},
        {SubmenuRole, "submenu"},
        {IconNameRole, "iconName"},
    };
}

QVector<DBusMenuModel::Entry> DBusMenuModel::entriesFromLayout(const QVariant &layout)
{
    int rootId = 0;
    QVariantMap rootProperties;
    QVariantList children;
    if (!readLayoutNode(layout, &rootId, &rootProperties, &children)) {
        return {};
    }

    QVector<Entry> entries;
    entries.reserve(children.size());
    for (const QVariant &child : children) {
        const QVariantMap map = menuEntryMap(child);
        if (map.isEmpty() || !map.value(QStringLiteral("visible")).toBool()) {
            continue;
        }
        Entry entry;
        entry.id = map.value(QStringLiteral("id")).toInt();
        entry.label = map.value(QStringLiteral("label")).toString();
        entry.iconName = map.value(QStringLiteral("iconName")).toString();
        entry.enabled = map.value(QStringLiteral("enabled")).toBool();
        entry.separator = map.value(QStringLiteral("separator")).toBool();
        entry.checkable = map.value(QStringLiteral("checkable")).toBool();
        entry.checked = map.value(QStringLiteral("checked")).toBool();
        entry.submenu = map.value(QStringLiteral("submenu")).toBool();
        entries.push_back(std::move(entry));
    }
    return entries;
}

bool DBusMenuModel::loading() const
{
    return m_loading;
}

bool DBusMenuModel::isOpen() const
{
    return !m_service.isEmpty() && !m_path.isEmpty();
}

int DBusMenuModel::depth() const
{
    return m_parentIds.size();
}

int DBusMenuModel::itemCount() const
{
    return m_entries.size();
}

QString DBusMenuModel::error() const
{
    return m_error;
}

void DBusMenuModel::openMenu(const QString &serviceAndItemPath, const QString &menuPath)
{
    const qsizetype slash = serviceAndItemPath.indexOf(QLatin1Char('/'));
    if (slash <= 0 || !menuPath.startsWith(QLatin1Char('/'))) {
        close();
        setError(QStringLiteral("This tray item has no usable remote menu."));
        return;
    }

    const bool wasOpen = isOpen();
    if (wasOpen) {
        m_bus.disconnect(m_service, m_path, QString::fromLatin1(menuInterface),
                         QStringLiteral("LayoutUpdated"), this, SLOT(onLayoutUpdated(uint,int)));
    }
    m_service = serviceAndItemPath.left(slash);
    m_path = menuPath;
    m_bus.connect(m_service, m_path, QString::fromLatin1(menuInterface),
                   QStringLiteral("LayoutUpdated"), this, SLOT(onLayoutUpdated(uint,int)));
    m_parentIds = {0};
    setError({});
    beginResetModel();
    m_entries.clear();
    endResetModel();
    if (!wasOpen) {
        Q_EMIT openChanged();
    }
    Q_EMIT menuChanged();
    requestLayout(0);
}

void DBusMenuModel::openSubmenu(int itemId)
{
    const auto found = std::find_if(m_entries.cbegin(), m_entries.cend(), [itemId](const Entry &entry) {
        return entry.id == itemId;
    });
    if (found == m_entries.cend() || !found->submenu || !isOpen()) {
        return;
    }
    m_parentIds.push_back(itemId);
    Q_EMIT menuChanged();
    requestLayout(itemId);
}

void DBusMenuModel::goBack()
{
    if (m_parentIds.size() <= 1) {
        close();
        return;
    }
    m_parentIds.removeLast();
    Q_EMIT menuChanged();
    requestLayout(m_parentIds.constLast());
}

void DBusMenuModel::trigger(int itemId)
{
    const auto found = std::find_if(m_entries.cbegin(), m_entries.cend(), [itemId](const Entry &entry) {
        return entry.id == itemId;
    });
    if (found == m_entries.cend() || !found->enabled || found->separator) {
        return;
    }
    if (found->submenu) {
        openSubmenu(itemId);
        return;
    }
    sendEvent(itemId);
    close();
}

void DBusMenuModel::close()
{
    const bool wasOpen = isOpen();
    ++m_requestGeneration;
    if (wasOpen) {
        m_bus.disconnect(m_service, m_path, QString::fromLatin1(menuInterface),
                         QStringLiteral("LayoutUpdated"), this, SLOT(onLayoutUpdated(uint,int)));
    }
    m_service.clear();
    m_path.clear();
    m_parentIds.clear();
    setLoading(false);
    setError({});
    beginResetModel();
    m_entries.clear();
    endResetModel();
    if (wasOpen) {
        Q_EMIT openChanged();
    }
    Q_EMIT menuChanged();
}

void DBusMenuModel::setLoading(bool loading)
{
    if (m_loading == loading) {
        return;
    }
    m_loading = loading;
    Q_EMIT loadingChanged();
}

void DBusMenuModel::setError(const QString &error)
{
    if (m_error == error) {
        return;
    }
    m_error = error;
    Q_EMIT menuChanged();
}

void DBusMenuModel::requestLayout(int parentId, bool notifyAboutToShow)
{
    if (!isOpen() || !m_bus.isConnected()) {
        setError(QStringLiteral("The session bus is unavailable."));
        setLoading(false);
        return;
    }

    setLoading(true);
    setError({});
    const quint64 generation = ++m_requestGeneration;
    if (!notifyAboutToShow) {
        fetchLayout(parentId, generation);
        return;
    }
    QDBusMessage aboutToShow = QDBusMessage::createMethodCall(m_service, m_path,
                                                               QString::fromLatin1(menuInterface),
                                                               QStringLiteral("AboutToShow"));
    aboutToShow << parentId;
    auto *showWatcher = new QDBusPendingCallWatcher(m_bus.asyncCall(aboutToShow, 1800), this);
    connect(showWatcher, &QDBusPendingCallWatcher::finished, this,
            [this, showWatcher, generation, parentId] {
        QDBusPendingReply<bool> showReply = *showWatcher;
        showWatcher->deleteLater();
        if (generation != m_requestGeneration || !isOpen()) {
            return;
        }
        if (showReply.isError()) {
            setError(QStringLiteral("The application menu could not be opened."));
            setLoading(false);
            return;
        }

        fetchLayout(parentId, generation);
    });
}

void DBusMenuModel::fetchLayout(int parentId, quint64 generation)
{
    QDBusMessage getLayout = QDBusMessage::createMethodCall(m_service, m_path,
                                                             QString::fromLatin1(menuInterface),
                                                             QStringLiteral("GetLayout"));
    getLayout << parentId << 1 << QStringList{};
    auto *layoutWatcher = new QDBusPendingCallWatcher(m_bus.asyncCall(getLayout, 2500), this);
    connect(layoutWatcher, &QDBusPendingCallWatcher::finished, this,
            [this, layoutWatcher, generation] {
        const QDBusMessage reply = layoutWatcher->reply();
        layoutWatcher->deleteLater();
        if (generation != m_requestGeneration || !isOpen()) {
            return;
        }
        if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().size() < 2) {
            setError(QStringLiteral("The application menu could not be loaded."));
            setLoading(false);
            return;
        }

        QVector<Entry> entries = entriesFromLayout(reply.arguments().at(1));
        beginResetModel();
        m_entries = std::move(entries);
        endResetModel();
        setError({});
        setLoading(false);
        Q_EMIT menuChanged();
    });
}

void DBusMenuModel::onLayoutUpdated(uint revision, int parentId)
{
    Q_UNUSED(revision);
    if (isOpen() && !m_parentIds.isEmpty() && m_parentIds.constLast() == parentId) {
        requestLayout(parentId, false);
    }
}

void DBusMenuModel::sendEvent(int itemId)
{
    if (!isOpen() || !m_bus.isConnected()) {
        return;
    }
    QDBusMessage event = QDBusMessage::createMethodCall(m_service, m_path,
                                                         QString::fromLatin1(menuInterface),
                                                         QStringLiteral("Event"));
    event << itemId << QStringLiteral("clicked") << QVariant::fromValue(QDBusVariant(0))
          << static_cast<uint>(QDateTime::currentMSecsSinceEpoch() & 0xffffffffU);
    m_bus.asyncCall(event, 1800);
}
