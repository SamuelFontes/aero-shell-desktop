/* SPDX-License-Identifier: MIT */
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTemporaryFile>
#include <QTimer>
#include <QVariantList>

#include <functional>
#include <memory>

class NetworkStatus final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(bool connected READ connected NOTIFY changed)
    Q_PROPERTY(bool wifiEnabled READ wifiEnabled NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString summary READ summary NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(QVariantList connections READ connections NOTIFY changed)
    Q_PROPERTY(QVariantList wifiNetworks READ wifiNetworks NOTIFY changed)

public:
    explicit NetworkStatus(QObject *parent = nullptr);

    bool available() const;
    bool connected() const;
    bool wifiEnabled() const;
    bool busy() const;
    QString summary() const;
    QString error() const;
    QVariantList connections() const;
    QVariantList wifiNetworks() const;
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void scanWifi();
    Q_INVOKABLE void joinWifi(const QString &ssid, const QString &password, const QString &security);
    Q_INVOKABLE void setWifiEnabled(bool enabled);
    Q_INVOKABLE void toggleConnection(const QString &uuid, bool active);

Q_SIGNALS:
    void changed();

private:
    using CommandCallback = std::function<void(int, const QString &, const QString &)>;

    QTimer m_refreshTimer;
    QString m_summary = QStringLiteral("Network unavailable");
    QString m_error;
    QVariantList m_connections;
    QVariantList m_wifiNetworks;
    QString m_allConnectionOutput;
    QString m_activeConnectionOutput;
    QString m_activeConnectionName;
    int m_pendingQueries = 0;
    bool m_available = false;
    bool m_connected = false;
    bool m_wifiEnabled = false;
    bool m_busy = false;
    bool m_refreshPending = false;
    std::unique_ptr<QTemporaryFile> m_passwordFile;

    static QStringList splitTerse(const QString &line);
    static QVariantList parseConnections(const QString &output, const QString &activeOutput);
    static QVariantList parseWifiNetworks(const QString &output);
    void startCommand(const QStringList &arguments, CommandCallback callback, int timeoutMs = 5000);
    void finishRefresh();
    void runAction(const QStringList &arguments);
};
