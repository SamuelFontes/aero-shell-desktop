/* SPDX-License-Identifier: MIT */
#include "notifications.h"

#include <QCoreApplication>
#include <QDBusAbstractAdaptor>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QDateTime>
#include <QTextDocument>
#include <QTimer>

#include <algorithm>

namespace
{
constexpr auto notificationService = "org.freedesktop.Notifications";
constexpr auto notificationPath = "/org/freedesktop/Notifications";
constexpr int maximumNotifications = 20;
constexpr int defaultExpiryMs = 5000;

uint urgencyFromHints(const QVariantMap &hints)
{
    const QVariant raw = hints.value(QStringLiteral("urgency"));
    bool ok = false;
    uint urgency = raw.toUInt(&ok);
    if (!ok && raw.canConvert<QByteArray>()) {
        const QByteArray bytes = raw.toByteArray();
        if (!bytes.isEmpty()) {
            urgency = static_cast<uchar>(bytes.at(0));
            ok = true;
        }
    }
    return ok ? std::min(urgency, 2U) : 1U;
}
}

class NotificationsAdaptor final : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")

public:
    explicit NotificationsAdaptor(NotificationsModel *parent)
        : QDBusAbstractAdaptor(parent)
        , m_model(parent)
    {
        connect(m_model, &NotificationsModel::NotificationClosed,
                this, &NotificationsAdaptor::NotificationClosed);
        connect(m_model, &NotificationsModel::ActionInvoked,
                this, &NotificationsAdaptor::ActionInvoked);
    }

public Q_SLOTS:
    QStringList GetCapabilities() const
    {
        return {QStringLiteral("actions"), QStringLiteral("body"), QStringLiteral("body-markup")};
    }

    QString GetServerInformation(QString &vendor, QString &version, QString &specVersion) const
    {
        vendor = QStringLiteral("AeroShell");
        version = QCoreApplication::applicationVersion().isEmpty()
            ? QStringLiteral("0.1.0") : QCoreApplication::applicationVersion();
        specVersion = QStringLiteral("1.2");
        return QStringLiteral("AeroShell Notifications");
    }

    uint Notify(const QString &appName, uint replacesId, const QString &appIcon,
                const QString &summary, const QString &body, const QStringList &actions,
                const QVariantMap &hints, int expireTimeout)
    {
        return m_model->notify(appName, replacesId, appIcon, summary, body, actions, hints, expireTimeout);
    }

    void CloseNotification(uint id)
    {
        m_model->closeNotification(id, 3);
    }

Q_SIGNALS:
    void NotificationClosed(uint id, uint reason);
    void ActionInvoked(uint id, const QString &actionKey);

private:
    NotificationsModel *m_model;
};

NotificationsModel::NotificationsModel(const QDBusConnection &bus, QObject *parent)
    : QAbstractListModel(parent)
    , m_bus(bus)
{
    if (m_bus.isConnected()) {
        m_plasmaWatcher = new QDBusServiceWatcher(QStringLiteral("org.kde.plasmashell"), m_bus,
                                                  QDBusServiceWatcher::WatchForOwnerChange, this);
        connect(m_plasmaWatcher, &QDBusServiceWatcher::serviceOwnerChanged,
                this, &NotificationsModel::onPlasmaOwnerChanged);
        m_notificationWatcher = new QDBusServiceWatcher(QString::fromLatin1(notificationService), m_bus,
                                                         QDBusServiceWatcher::WatchForOwnerChange, this);
        connect(m_notificationWatcher, &QDBusServiceWatcher::serviceOwnerChanged,
                this, &NotificationsModel::onNotificationOwnerChanged);
    }
    startService();
}

NotificationsModel::~NotificationsModel()
{
    stopService();
}

int NotificationsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_notifications.size();
}

QVariant NotificationsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_notifications.size()) {
        return {};
    }
    const Notification &item = m_notifications.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case SummaryRole: return item.summary;
    case IdRole: return item.id;
    case AppNameRole: return item.appName;
    case AppIconRole: return item.appIcon;
    case BodyRole: return item.body;
    case ActionsRole: return item.actions;
    case UrgencyRole: return item.urgency;
    case TimestampRole: return item.timestamp;
    default: return {};
    }
}

QHash<int, QByteArray> NotificationsModel::roleNames() const
{
    return {{Qt::DisplayRole, "display"}, {IdRole, "notificationId"}, {AppNameRole, "appName"},
            {AppIconRole, "appIcon"}, {SummaryRole, "summary"}, {BodyRole, "body"},
            {ActionsRole, "actions"}, {UrgencyRole, "urgency"}, {TimestampRole, "timestamp"}};
}

bool NotificationsModel::active() const { return m_active; }
int NotificationsModel::count() const { return m_notifications.size(); }
QString NotificationsModel::status() const { return m_status; }

QVariantMap NotificationsModel::latestNotification() const
{
    return m_notifications.isEmpty() ? QVariantMap{} : toVariantMap(m_notifications.first());
}

uint NotificationsModel::notify(const QString &appName, uint replacesId, const QString &appIcon,
                                const QString &summary, const QString &body, const QStringList &actions,
                                const QVariantMap &hints, int expireTimeout)
{
    int row = rowForId(replacesId);
    const bool replacing = row >= 0;
    const uint id = replacing ? replacesId : m_nextId++;
    QTextDocument bodyDocument;
    bodyDocument.setHtml(body);
    Notification item{id, appName.left(256), appIcon.left(1024), summary.left(2048),
                      bodyDocument.toPlainText().left(8192), actions.mid(0, 20),
                      urgencyFromHints(hints), QDateTime::currentMSecsSinceEpoch()};
    if (replacing) {
        m_notifications[row] = item;
        Q_EMIT dataChanged(index(row), index(row));
        if (row == 0) {
            Q_EMIT latestNotificationChanged();
        }
    } else {
        beginInsertRows({}, 0, 0);
        m_notifications.prepend(item);
        endInsertRows();
        if (m_notifications.size() > maximumNotifications) {
            const uint oldest = m_notifications.last().id;
            closeNotification(oldest, 2);
        }
        Q_EMIT countChanged();
        Q_EMIT latestNotificationChanged();
    }
    Q_EMIT notificationAdded(id);

    if (QTimer *timer = m_expiryTimers.take(id)) {
        timer->stop();
        timer->deleteLater();
    }
    if (expireTimeout != -1) {
        const int timeout = expireTimeout == 0 ? defaultExpiryMs : std::max(0, expireTimeout);
        auto *timer = new QTimer(this);
        timer->setSingleShot(true);
        m_expiryTimers.insert(id, timer);
        connect(timer, &QTimer::timeout, this, [this, id] { expire(id); });
        timer->start(timeout);
    }
    return id;
}

void NotificationsModel::closeNotification(uint id, uint reason)
{
    const int row = rowForId(id);
    if (row < 0) {
        return;
    }
    if (QTimer *timer = m_expiryTimers.take(id)) {
        timer->stop();
        timer->deleteLater();
    }
    beginRemoveRows({}, row, row);
    m_notifications.removeAt(row);
    endRemoveRows();
    Q_EMIT countChanged();
    if (row == 0) {
        Q_EMIT latestNotificationChanged();
    }
    Q_EMIT NotificationClosed(id, reason);
}

void NotificationsModel::invokeAction(uint id, const QString &actionKey)
{
    const int row = rowForId(id);
    if (row < 0 || !m_notifications.at(row).actions.contains(actionKey)) {
        return;
    }
    Q_EMIT ActionInvoked(id, actionKey);
    closeNotification(id, 2);
}

void NotificationsModel::dismiss(uint id)
{
    closeNotification(id, 2);
}

void NotificationsModel::onPlasmaOwnerChanged(const QString &, const QString &, const QString &newOwner)
{
    if (!newOwner.isEmpty()) {
        stopService();
        setStatus(QStringLiteral("Plasma owns this session; AeroShell notifications are paused."));
    } else {
        startService();
    }
}

void NotificationsModel::onNotificationOwnerChanged(const QString &, const QString &, const QString &newOwner)
{
    if (!newOwner.isEmpty()) {
        if (!m_active) {
            setStatus(QStringLiteral("Another notification service is already active."));
        }
        return;
    }
    startService();
}

void NotificationsModel::startService()
{
    if (m_active) {
        return;
    }
    if (!m_bus.isConnected() || !m_bus.interface()) {
        setStatus(QStringLiteral("Session D-Bus is unavailable."));
        return;
    }
    const QDBusReply<bool> plasmaOwnsShell = m_bus.interface()->isServiceRegistered(QStringLiteral("org.kde.plasmashell"));
    if (!plasmaOwnsShell.isValid() || plasmaOwnsShell.value()) {
        setStatus(QStringLiteral("Plasma owns this session; AeroShell notifications are paused."));
        return;
    }
    const QDBusReply<bool> existingOwner = m_bus.interface()->isServiceRegistered(QString::fromLatin1(notificationService));
    if (!existingOwner.isValid() || existingOwner.value()) {
        setStatus(QStringLiteral("Another notification service is already active."));
        return;
    }

    m_adaptor = new NotificationsAdaptor(this);
    if (!m_bus.registerObject(QString::fromLatin1(notificationPath), this, QDBusConnection::ExportAdaptors)) {
        delete m_adaptor;
        m_adaptor = nullptr;
        setStatus(QStringLiteral("Could not register AeroShell notifications."));
        return;
    }
    const auto reply = m_bus.interface()->registerService(QString::fromLatin1(notificationService),
                                                           QDBusConnectionInterface::DontQueueService);
    if (reply != QDBusConnectionInterface::ServiceRegistered) {
        m_bus.unregisterObject(QString::fromLatin1(notificationPath));
        delete m_adaptor;
        m_adaptor = nullptr;
        setStatus(QStringLiteral("Another notification service is already active."));
        return;
    }
    setActive(true);
    setStatus(QStringLiteral("AeroShell notifications are active."));
}

void NotificationsModel::stopService()
{
    if (m_active) {
        m_bus.unregisterService(QString::fromLatin1(notificationService));
        m_bus.unregisterObject(QString::fromLatin1(notificationPath));
        setActive(false);
    }
    delete m_adaptor;
    m_adaptor = nullptr;
}

void NotificationsModel::setActive(bool active)
{
    if (m_active == active) return;
    m_active = active;
    Q_EMIT activeChanged();
}

void NotificationsModel::setStatus(const QString &status)
{
    if (m_status == status) return;
    m_status = status;
    Q_EMIT statusChanged();
}

int NotificationsModel::rowForId(uint id) const
{
    for (int i = 0; i < m_notifications.size(); ++i) {
        if (m_notifications.at(i).id == id) return i;
    }
    return -1;
}

QVariantMap NotificationsModel::toVariantMap(const Notification &item) const
{
    return {{QStringLiteral("notificationId"), item.id}, {QStringLiteral("appName"), item.appName},
            {QStringLiteral("appIcon"), item.appIcon}, {QStringLiteral("summary"), item.summary},
            {QStringLiteral("body"), item.body}, {QStringLiteral("actions"), item.actions},
            {QStringLiteral("urgency"), item.urgency}, {QStringLiteral("timestamp"), item.timestamp}};
}

void NotificationsModel::expire(uint id)
{
    closeNotification(id, 1);
}

#include "notifications.moc"
