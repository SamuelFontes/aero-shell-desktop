/* SPDX-License-Identifier: MIT */
#pragma once

#include <QAbstractListModel>
#include <QDBusConnection>
#include <QHash>
#include <QStringList>
#include <QVector>
#include <QVariantMap>

class QDBusServiceWatcher;
class QTimer;
class NotificationsAdaptor;

class NotificationsModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QVariantMap latestNotification READ latestNotification NOTIFY latestNotificationChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        AppNameRole,
        AppIconRole,
        SummaryRole,
        BodyRole,
        ActionsRole,
        UrgencyRole,
        TimestampRole,
    };
    Q_ENUM(Role)

    explicit NotificationsModel(const QDBusConnection &bus, QObject *parent = nullptr);
    ~NotificationsModel() override;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool active() const;
    int count() const;
    QVariantMap latestNotification() const;
    QString status() const;

    uint notify(const QString &appName, uint replacesId, const QString &appIcon,
                const QString &summary, const QString &body, const QStringList &actions,
                const QVariantMap &hints, int expireTimeout);
    void closeNotification(uint id, uint reason = 3);
    Q_INVOKABLE void invokeAction(uint id, const QString &actionKey);
    Q_INVOKABLE void dismiss(uint id);

Q_SIGNALS:
    void activeChanged();
    void countChanged();
    void latestNotificationChanged();
    void statusChanged();
    void notificationAdded(uint id);
    void NotificationClosed(uint id, uint reason);
    void ActionInvoked(uint id, const QString &actionKey);

private Q_SLOTS:
    void onPlasmaOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner);
    void onNotificationOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner);

private:
    struct Notification {
        uint id = 0;
        QString appName;
        QString appIcon;
        QString summary;
        QString body;
        QStringList actions;
        uint urgency = 1;
        qint64 timestamp = 0;
    };

    QDBusConnection m_bus;
    QDBusServiceWatcher *m_plasmaWatcher = nullptr;
    QDBusServiceWatcher *m_notificationWatcher = nullptr;
    NotificationsAdaptor *m_adaptor = nullptr;
    QVector<Notification> m_notifications;
    QHash<uint, QTimer *> m_expiryTimers;
    QString m_status;
    uint m_nextId = 1;
    bool m_active = false;

    void startService();
    void stopService();
    void setActive(bool active);
    void setStatus(const QString &status);
    int rowForId(uint id) const;
    QVariantMap toVariantMap(const Notification &notification) const;
    void expire(uint id);
};
