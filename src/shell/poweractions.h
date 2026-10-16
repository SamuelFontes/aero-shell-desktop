/* SPDX-License-Identifier: MIT */
#pragma once

#include <QObject>
#include <QString>

class PowerActions final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool serviceAvailable READ serviceAvailable NOTIFY capabilitiesChanged)
    Q_PROPERTY(bool suspendAvailable READ suspendAvailable NOTIFY capabilitiesChanged)
    Q_PROPERTY(bool rebootAvailable READ rebootAvailable NOTIFY capabilitiesChanged)
    Q_PROPERTY(bool powerOffAvailable READ powerOffAvailable NOTIFY capabilitiesChanged)
    Q_PROPERTY(bool pending READ pending NOTIFY pendingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit PowerActions(QObject *parent = nullptr);

    bool serviceAvailable() const;
    bool suspendAvailable() const;
    bool rebootAvailable() const;
    bool powerOffAvailable() const;
    bool pending() const;
    QString error() const;

    Q_INVOKABLE void request(const QString &action);
    Q_INVOKABLE void clearError();

Q_SIGNALS:
    void capabilitiesChanged();
    void pendingChanged();
    void errorChanged();

private:
    bool m_serviceAvailable = false;
    bool m_suspendAvailable = false;
    bool m_rebootAvailable = false;
    bool m_powerOffAvailable = false;
    bool m_pending = false;
    bool m_refreshing = false;
    QString m_error;

    void refreshCapabilities();
    void setError(const QString &error);
};
