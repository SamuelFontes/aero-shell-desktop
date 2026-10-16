/* SPDX-License-Identifier: MIT */
#include "statusnotifier.h"
#include "statusnotifiericon.h"

#include <QCoreApplication>
#include <QBuffer>
#include <QDBusArgument>
#include <QDBusAbstractAdaptor>
#include <QDBusConnectionInterface>
#include <QDBusContext>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#include <QDBusServiceWatcher>
#include <QDBusVariant>
#include <QImage>
#include <QTimer>

#include <algorithm>
#include <utility>

namespace
{
constexpr auto watcherService = "org.kde.StatusNotifierWatcher";
constexpr auto watcherPath = "/StatusNotifierWatcher";
constexpr auto watcherInterface = "org.kde.StatusNotifierWatcher";
constexpr auto itemInterface = "org.kde.StatusNotifierItem";
constexpr auto propertiesInterface = "org.freedesktop.DBus.Properties";

QVariant unpack(const QVariant &value)
{
    if (value.metaType() == QMetaType::fromType<QDBusVariant>()) {
        return value.value<QDBusVariant>().variant();
    }
    return value;
}

QString propertyString(const QVariantMap &properties, const QString &name)
{
    return unpack(properties.value(name)).toString();
}

QString iconPixmapDataUrl(const QVariant &value)
{
    const QVariant unpacked = unpack(value);
    if (!unpacked.canConvert<QDBusArgument>()) {
        return {};
    }

    QDBusArgument argument = unpacked.value<QDBusArgument>();
    int bestArea = 0;
    QImage bestImage;
    argument.beginArray();
    while (!argument.atEnd()) {
        int width = 0;
        int height = 0;
        QByteArray pixels;
        argument.beginStructure();
        argument >> width >> height >> pixels;
        argument.endStructure();
        if (width <= 0 || height <= 0 || width > 512 || height > 512
            || pixels.size() < width * height * 4 || width * height <= bestArea) {
            continue;
        }

        QImage image = decodeStatusNotifierIconPixmap(width, height, pixels);
        if (!image.isNull()) {
            bestImage = image.copy();
            bestArea = width * height;
        }
    }
    argument.endArray();
    if (bestImage.isNull()) {
        return {};
    }

    QByteArray png;
    QBuffer buffer(&png);
    if (!buffer.open(QIODevice::WriteOnly) || !bestImage.save(&buffer, "PNG")) {
        return {};
    }
    return QStringLiteral("data:image/png;base64,") + QString::fromLatin1(png.toBase64());
}
}

class StatusNotifierWatcherAdaptor final : public QDBusAbstractAdaptor, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.StatusNotifierWatcher")
    Q_PROPERTY(QStringList RegisteredStatusNotifierItems READ registeredItems)
    Q_PROPERTY(bool IsStatusNotifierHostRegistered READ hostRegistered)
    Q_PROPERTY(int ProtocolVersion READ protocolVersion)

public:
    explicit StatusNotifierWatcherAdaptor(StatusNotifierModel *parent)
        : QDBusAbstractAdaptor(parent)
        , m_model(parent)
    {
        connect(m_model, &StatusNotifierModel::StatusNotifierItemRegistered,
                this, &StatusNotifierWatcherAdaptor::StatusNotifierItemRegistered);
        connect(m_model, &StatusNotifierModel::StatusNotifierItemUnregistered,
                this, &StatusNotifierWatcherAdaptor::StatusNotifierItemUnregistered);
        connect(m_model, &StatusNotifierModel::StatusNotifierHostRegistered,
                this, &StatusNotifierWatcherAdaptor::StatusNotifierHostRegistered);
        connect(m_model, &StatusNotifierModel::StatusNotifierHostUnregistered,
                this, &StatusNotifierWatcherAdaptor::StatusNotifierHostUnregistered);
    }

    QStringList registeredItems() const
    {
        return m_model->registeredItems();
    }

    bool hostRegistered() const
    {
        return m_model->hostRegistered();
    }

    int protocolVersion() const
    {
        return 0;
    }

public Q_SLOTS:
    void RegisterStatusNotifierItem(const QString &serviceOrPath)
    {
        QString service = serviceOrPath;
        QString path = QStringLiteral("/StatusNotifierItem");
        const qsizetype slash = serviceOrPath.indexOf(QLatin1Char('/'));
        if (serviceOrPath.startsWith(QLatin1Char('/'))) {
            service = message().service();
            path = serviceOrPath;
        } else if (slash >= 0) {
            service = serviceOrPath.left(slash);
            path = serviceOrPath.mid(slash);
        }
        m_model->registerItem(service, path);
    }

    void RegisterStatusNotifierHost(const QString &service)
    {
        m_model->registerHost(service);
    }

Q_SIGNALS:
    void StatusNotifierItemRegistered(const QString &serviceId);
    void StatusNotifierItemUnregistered(const QString &serviceId);
    void StatusNotifierHostRegistered();
    void StatusNotifierHostUnregistered();

private:
    StatusNotifierModel *m_model;
};

StatusNotifierModel::StatusNotifierModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_bus(QDBusConnection::sessionBus())
{
    m_plasmaShellWatcher = new QDBusServiceWatcher(QStringLiteral("org.kde.plasmashell"),
                                                   QDBusConnection::sessionBus(),
                                                   QDBusServiceWatcher::WatchForOwnerChange,
                                                   this);
    connect(m_plasmaShellWatcher, &QDBusServiceWatcher::serviceOwnerChanged, this,
            [this](const QString &, const QString &, const QString &newOwner) {
                if (!newOwner.isEmpty()) {
                    stopTray();
                } else {
                    startTray();
                }
            });

    m_refreshTimer.setInterval(3000);
    connect(&m_refreshTimer, &QTimer::timeout, this, &StatusNotifierModel::refresh);
    startTray();
}

StatusNotifierModel::~StatusNotifierModel()
{
    stopTray();
}

int StatusNotifierModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant StatusNotifierModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return {};
    }

    const Item &item = m_items.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case TitleRole:
        return item.title;
    case ServiceIdRole:
        return item.serviceId;
    case IconNameRole:
        return item.iconName;
    case IconDataUrlRole:
        return item.iconDataUrl;
    case MenuPathRole:
        return item.menuPath;
    case StatusRole:
        return item.status;
    case ItemIsMenuRole:
        return item.itemIsMenu;
    default:
        return {};
    }
}

QHash<int, QByteArray> StatusNotifierModel::roleNames() const
{
    return {
        {Qt::DisplayRole, "display"},
        {ServiceIdRole, "serviceId"},
        {TitleRole, "title"},
        {IconNameRole, "iconName"},
        {IconDataUrlRole, "iconDataUrl"},
        {MenuPathRole, "menuPath"},
        {StatusRole, "status"},
        {ItemIsMenuRole, "itemIsMenu"},
    };
}

bool StatusNotifierModel::active() const
{
    return m_active;
}

QStringList StatusNotifierModel::registeredItems() const
{
    return m_registeredItems;
}

bool StatusNotifierModel::hostRegistered() const
{
    return !m_hosts.isEmpty();
}

void StatusNotifierModel::startTray()
{
    if (m_active || !m_busConnectionName.isEmpty()) {
        return;
    }
    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    if (!sessionBus.interface()) {
        return;
    }
    const QDBusReply<bool> plasmaOwnsShell = sessionBus.interface()->isServiceRegistered(
        QStringLiteral("org.kde.plasmashell"));
    if (!plasmaOwnsShell.isValid() || plasmaOwnsShell.value()) {
        return;
    }

    m_busConnectionName = QStringLiteral("aeroshell-statusnotifier-%1-%2")
        .arg(QCoreApplication::applicationPid())
        .arg(++m_generation);
    m_bus = QDBusConnection::connectToBus(QDBusConnection::SessionBus, m_busConnectionName);
    if (!m_bus.isConnected() || !m_bus.interface()) {
        QDBusConnection::disconnectFromBus(m_busConnectionName);
        m_busConnectionName.clear();
        m_bus = QDBusConnection::sessionBus();
        return;
    }

    const QDBusReply<bool> watcherOwnsName = m_bus.interface()->isServiceRegistered(QString::fromLatin1(watcherService));
    if (!watcherOwnsName.isValid()) {
        stopTray();
        return;
    }

    if (!watcherOwnsName.value()) {
        m_watcherAdaptor = new StatusNotifierWatcherAdaptor(this);
        const bool objectRegistered = m_bus.registerObject(QString::fromLatin1(watcherPath), this,
                                                            QDBusConnection::ExportAdaptors);
        const QDBusReply<QDBusConnectionInterface::RegisterServiceReply> serviceReply =
            m_bus.interface()->registerService(QString::fromLatin1(watcherService),
                                               QDBusConnectionInterface::DontQueueService,
                                               QDBusConnectionInterface::DontAllowReplacement);
        if (!objectRegistered || !serviceReply.isValid()
            || serviceReply.value() != QDBusConnectionInterface::ServiceRegistered) {
            if (serviceReply.isValid() && serviceReply.value() == QDBusConnectionInterface::ServiceRegistered) {
                m_bus.interface()->unregisterService(QString::fromLatin1(watcherService));
            }
            m_bus.unregisterObject(QString::fromLatin1(watcherPath));
            delete m_watcherAdaptor;
            m_watcherAdaptor = nullptr;
            const QDBusReply<bool> retryWatcher = m_bus.interface()->isServiceRegistered(QString::fromLatin1(watcherService));
            if (!retryWatcher.isValid() || !retryWatcher.value()) {
                stopTray();
                return;
            }
        } else {
            m_ownsWatcher = true;
        }
    }

    if (!m_ownsWatcher) {
        m_bus.connect(QString::fromLatin1(watcherService), QString::fromLatin1(watcherPath),
                      QString::fromLatin1(watcherInterface), QStringLiteral("StatusNotifierItemRegistered"),
                      this, SLOT(onWatcherItemRegistered(QString)));
        m_bus.connect(QString::fromLatin1(watcherService), QString::fromLatin1(watcherPath),
                      QString::fromLatin1(watcherInterface), QStringLiteral("StatusNotifierItemUnregistered"),
                      this, SLOT(onWatcherItemUnregistered(QString)));
    }

    m_hostService = m_bus.baseService();
    if (m_hostService.isEmpty()) {
        stopTray();
        return;
    }
    QDBusMessage registration = QDBusMessage::createMethodCall(QString::fromLatin1(watcherService),
                                                               QString::fromLatin1(watcherPath),
                                                               QString::fromLatin1(watcherInterface),
                                                               QStringLiteral("RegisterStatusNotifierHost"));
    registration << m_hostService;
    const quint64 generation = m_generation;
    auto *watcher = new QDBusPendingCallWatcher(m_bus.asyncCall(registration, 2000), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, generation] {
        QDBusPendingReply<> reply = *watcher;
        watcher->deleteLater();
        if (generation != m_generation) {
            return;
        }
        if (reply.isError()) {
            stopTray();
            return;
        }
        setActive(true);
        refresh();
        m_refreshTimer.start();
    });
}

void StatusNotifierModel::stopTray()
{
    ++m_generation;
    m_refreshTimer.stop();
    if (!m_busConnectionName.isEmpty()) {
        if (m_ownsWatcher && m_bus.isConnected()) {
            m_bus.unregisterService(QString::fromLatin1(watcherService));
            m_bus.unregisterObject(QString::fromLatin1(watcherPath));
        }
        delete m_watcherAdaptor;
        m_watcherAdaptor = nullptr;
        QDBusConnection::disconnectFromBus(m_busConnectionName);
        m_busConnectionName.clear();
        m_ownsWatcher = false;
        m_hostService.clear();
        m_registeredItems.clear();
        m_hosts.clear();
        for (QDBusServiceWatcher *watcher : std::as_const(m_itemServiceWatchers)) {
            watcher->deleteLater();
        }
        for (QDBusServiceWatcher *watcher : std::as_const(m_hostServiceWatchers)) {
            watcher->deleteLater();
        }
        m_itemServiceWatchers.clear();
        m_hostServiceWatchers.clear();
        m_pendingItemReads.clear();
        m_bus = QDBusConnection::sessionBus();
        beginResetModel();
        m_items.clear();
        endResetModel();
    }
    setActive(false);
}

void StatusNotifierModel::registerItem(const QString &service, const QString &path)
{
    if (service.isEmpty() || path.isEmpty()) {
        return;
    }
    const QString serviceId = service + path;
    if (m_registeredItems.contains(serviceId)) {
        return;
    }
    m_registeredItems.push_back(serviceId);
    Q_EMIT StatusNotifierItemRegistered(serviceId);
    if (m_ownsWatcher) {
        QTimer::singleShot(0, this, &StatusNotifierModel::refresh);
    }

    if (!m_itemServiceWatchers.contains(service)) {
        auto *watcher = new QDBusServiceWatcher(service, m_bus, QDBusServiceWatcher::WatchForUnregistration, this);
        m_itemServiceWatchers.insert(service, watcher);
        connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, [this, service, watcher] {
            QStringList removed;
            for (const QString &serviceId : std::as_const(m_registeredItems)) {
                if (serviceId.startsWith(service + QLatin1Char('/'))) {
                    removed.push_back(serviceId);
                }
            }
            for (const QString &serviceId : removed) {
                removeItem(serviceId);
            }
            m_itemServiceWatchers.remove(service);
            watcher->deleteLater();
        });
    }
}

void StatusNotifierModel::registerHost(const QString &service)
{
    if (service.isEmpty() || m_hosts.contains(service)) {
        return;
    }
    const bool wasEmpty = m_hosts.isEmpty();
    m_hosts.insert(service);
    if (wasEmpty) {
        Q_EMIT StatusNotifierHostRegistered();
    }
    auto *watcher = new QDBusServiceWatcher(service, m_bus, QDBusServiceWatcher::WatchForUnregistration, this);
    m_hostServiceWatchers.insert(service, watcher);
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, [this, service, watcher] {
        const bool removed = m_hosts.remove(service);
        m_hostServiceWatchers.remove(service);
        watcher->deleteLater();
        if (removed && m_hosts.isEmpty()) {
            Q_EMIT StatusNotifierHostUnregistered();
        }
    });
}

void StatusNotifierModel::refresh()
{
    if (!m_active || !m_bus.isConnected()) {
        return;
    }
    QDBusInterface watcher(QString::fromLatin1(watcherService), QString::fromLatin1(watcherPath),
                           QString::fromLatin1(watcherInterface), m_bus);
    const QStringList serviceIds = watcher.property("RegisteredStatusNotifierItems").toStringList();
    synchronizeItems(serviceIds);
}

void StatusNotifierModel::onWatcherItemRegistered(const QString &serviceId)
{
    if (!m_registeredItems.contains(serviceId)) {
        m_registeredItems.push_back(serviceId);
    }
    QTimer::singleShot(0, this, &StatusNotifierModel::refresh);
}

void StatusNotifierModel::onWatcherItemUnregistered(const QString &serviceId)
{
    removeItem(serviceId);
}

void StatusNotifierModel::synchronizeItems(const QStringList &serviceIds)
{
    QVector<Item> items;
    QSet<QString> seen;
    items.reserve(serviceIds.size());
    for (const QString &rawId : serviceIds) {
        if (rawId.isEmpty() || seen.contains(rawId)) {
            continue;
        }
        seen.insert(rawId);
        Item item;
        item.serviceId = rawId;
        const qsizetype slash = rawId.indexOf(QLatin1Char('/'));
        if (slash < 0) {
            item.service = rawId;
            item.path = QStringLiteral("/StatusNotifierItem");
            item.serviceId = item.service + item.path;
        } else {
            item.service = rawId.left(slash);
            item.path = rawId.mid(slash);
        }

        const auto existing = std::find_if(m_items.cbegin(), m_items.cend(), [&item](const Item &candidate) {
            return candidate.serviceId == item.serviceId;
        });
        if (existing != m_items.cend()) {
            item.title = existing->title;
            item.iconName = existing->iconName;
            item.iconDataUrl = existing->iconDataUrl;
            item.menuPath = existing->menuPath;
            item.status = existing->status;
            item.itemIsMenu = existing->itemIsMenu;
        }
        items.push_back(item);
    }

    if (items.size() != m_items.size()
        || !std::equal(items.cbegin(), items.cend(), m_items.cbegin(), m_items.cend(), [](const Item &left, const Item &right) {
               return left.serviceId == right.serviceId;
           })) {
        beginResetModel();
        m_items = std::move(items);
        endResetModel();
    } else {
        m_items = std::move(items);
    }

    for (const Item &item : std::as_const(m_items)) {
        requestItemProperties(item);
    }
}

void StatusNotifierModel::requestItemProperties(const Item &item)
{
    if (m_pendingItemReads.contains(item.serviceId)) {
        return;
    }
    m_pendingItemReads.insert(item.serviceId);
    QDBusMessage message = QDBusMessage::createMethodCall(item.service, item.path,
                                                          QString::fromLatin1(propertiesInterface),
                                                          QStringLiteral("GetAll"));
    message << QString::fromLatin1(itemInterface);
    const quint64 generation = m_generation;
    auto *watcher = new QDBusPendingCallWatcher(m_bus.asyncCall(message, 2000), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, item, generation] {
        QDBusPendingReply<QVariantMap> reply = *watcher;
        watcher->deleteLater();
        m_pendingItemReads.remove(item.serviceId);
        if (generation != m_generation || reply.isError()) {
            return;
        }
        applyItemProperties(item.serviceId, reply.value());
    });
}

void StatusNotifierModel::applyItemProperties(const QString &serviceId, const QVariantMap &properties)
{
    const auto found = std::find_if(m_items.begin(), m_items.end(), [&serviceId](const Item &item) {
        return item.serviceId == serviceId;
    });
    if (found == m_items.end()) {
        return;
    }

    QString title = propertyString(properties, QStringLiteral("Title"));
    if (title.isEmpty()) {
        title = propertyString(properties, QStringLiteral("Id"));
    }
    if (title.isEmpty()) {
        title = found->service;
    }
    const QString status = propertyString(properties, QStringLiteral("Status"));
    QString iconName = propertyString(properties, QStringLiteral("IconName"));
    QString iconDataUrl;
    if (status == QLatin1String("NeedsAttention")) {
        const QString attentionIcon = propertyString(properties, QStringLiteral("AttentionIconName"));
        if (!attentionIcon.isEmpty()) {
            iconName = attentionIcon;
        }
        iconDataUrl = iconPixmapDataUrl(properties.value(QStringLiteral("AttentionIconPixmap")));
    }
    if (iconDataUrl.isEmpty()) {
        iconDataUrl = iconPixmapDataUrl(properties.value(QStringLiteral("IconPixmap")));
    }
    const bool itemIsMenu = unpack(properties.value(QStringLiteral("ItemIsMenu"))).toBool();
    const QVariant menuValue = unpack(properties.value(QStringLiteral("Menu")));
    const QString menuPath = menuValue.metaType() == QMetaType::fromType<QDBusObjectPath>()
        ? menuValue.value<QDBusObjectPath>().path() : menuValue.toString();
    const QString activeMenuPath = menuPath == QLatin1String("/") ? QString() : menuPath;
    if (found->title == title && found->iconName == iconName && found->iconDataUrl == iconDataUrl
        && found->menuPath == activeMenuPath && found->status == status && found->itemIsMenu == itemIsMenu) {
        return;
    }

    found->title = title;
    found->iconName = iconName;
    found->iconDataUrl = iconDataUrl;
    found->menuPath = activeMenuPath;
    found->status = status;
    found->itemIsMenu = itemIsMenu;
    const int row = static_cast<int>(std::distance(m_items.begin(), found));
    Q_EMIT dataChanged(index(row), index(row),
                       {TitleRole, IconNameRole, IconDataUrlRole, MenuPathRole, StatusRole, ItemIsMenuRole});
}

void StatusNotifierModel::removeItem(const QString &serviceId)
{
    m_registeredItems.removeAll(serviceId);
    const auto found = std::find_if(m_items.cbegin(), m_items.cend(), [&serviceId](const Item &item) {
        return item.serviceId == serviceId;
    });
    if (found != m_items.cend()) {
        const int row = static_cast<int>(std::distance(m_items.cbegin(), found));
        beginRemoveRows({}, row, row);
        m_items.removeAt(row);
        endRemoveRows();
    }
    m_pendingItemReads.remove(serviceId);
    Q_EMIT StatusNotifierItemUnregistered(serviceId);
}

void StatusNotifierModel::callItem(const QString &serviceId, const QString &method, int x, int y)
{
    const auto found = std::find_if(m_items.cbegin(), m_items.cend(), [&serviceId](const Item &item) {
        return item.serviceId == serviceId;
    });
    if (found == m_items.cend() || !m_active) {
        return;
    }
    QDBusMessage message = QDBusMessage::createMethodCall(found->service, found->path,
                                                          QString::fromLatin1(itemInterface), method);
    message << x << y;
    m_bus.asyncCall(message, 2000);
}

void StatusNotifierModel::activate(const QString &serviceId, int x, int y)
{
    callItem(serviceId, QStringLiteral("Activate"), x, y);
}

void StatusNotifierModel::contextMenu(const QString &serviceId, int x, int y)
{
    callItem(serviceId, QStringLiteral("ContextMenu"), x, y);
}

void StatusNotifierModel::secondaryActivate(const QString &serviceId, int x, int y)
{
    callItem(serviceId, QStringLiteral("SecondaryActivate"), x, y);
}

void StatusNotifierModel::setActive(bool active)
{
    if (m_active == active) {
        return;
    }
    m_active = active;
    Q_EMIT activeChanged();
}

#include "statusnotifier.moc"
