/* SPDX-License-Identifier: MIT */
#pragma once

#include <QAbstractListModel>
#include <QDBusConnection>
#include <QDBusContext>
#include <QHash>
#include <QSet>
#include <QString>
#include <QVariantMap>
#include <QTimer>
#include <QVector>

class QDBusServiceWatcher;
class StatusNotifierWatcherAdaptor;

class StatusNotifierModel final : public QAbstractListModel, public QDBusContext
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)

public:
    enum Role {
        ServiceIdRole = Qt::UserRole + 1,
        TitleRole,
        IconNameRole,
        IconDataUrlRole,
        MenuPathRole,
        StatusRole,
        ItemIsMenuRole,
    };
    Q_ENUM(Role)

    explicit StatusNotifierModel(QObject *parent = nullptr);
    ~StatusNotifierModel() override;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool active() const;

    Q_INVOKABLE void activate(const QString &serviceId, int x, int y);
    Q_INVOKABLE void contextMenu(const QString &serviceId, int x, int y);
    Q_INVOKABLE void secondaryActivate(const QString &serviceId, int x, int y);

Q_SIGNALS:
    void activeChanged();
    void StatusNotifierItemRegistered(const QString &serviceId);
    void StatusNotifierItemUnregistered(const QString &serviceId);
    void StatusNotifierHostRegistered();
    void StatusNotifierHostUnregistered();

private Q_SLOTS:
    void refresh();
    void onWatcherItemRegistered(const QString &serviceId);
    void onWatcherItemUnregistered(const QString &serviceId);

private:
    struct Item {
        QString serviceId;
        QString service;
        QString path;
        QString title;
        QString iconName;
        QString iconDataUrl;
        QString menuPath;
        QString status;
        bool itemIsMenu = false;
    };

    QDBusConnection m_bus;
    QDBusServiceWatcher *m_plasmaShellWatcher = nullptr;
    StatusNotifierWatcherAdaptor *m_watcherAdaptor = nullptr;
    QHash<QString, QDBusServiceWatcher *> m_itemServiceWatchers;
    QHash<QString, QDBusServiceWatcher *> m_hostServiceWatchers;
    QTimer m_refreshTimer;
    QVector<Item> m_items;
    QStringList m_registeredItems;
    QSet<QString> m_pendingItemReads;
    QString m_hostService;
    QString m_busConnectionName;
    bool m_active = false;
    bool m_ownsWatcher = false;
    quint64 m_generation = 0;
    QSet<QString> m_hosts;

    QStringList registeredItems() const;
    bool hostRegistered() const;
    void startTray();
    void stopTray();
    void registerItem(const QString &service, const QString &path);
    void registerHost(const QString &service);
    void synchronizeItems(const QStringList &serviceIds);
    void requestItemProperties(const Item &item);
    void applyItemProperties(const QString &serviceId, const QVariantMap &properties);
    void removeItem(const QString &serviceId);
    void callItem(const QString &serviceId, const QString &method, int x, int y);
    void setActive(bool active);

    friend class StatusNotifierWatcherAdaptor;
};
