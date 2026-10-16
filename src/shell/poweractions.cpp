/* SPDX-License-Identifier: MIT */
#include "poweractions.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QTimer>

namespace
{
constexpr auto loginService = "org.freedesktop.login1";
constexpr auto loginPath = "/org/freedesktop/login1";
constexpr auto loginInterface = "org.freedesktop.login1.Manager";

bool actionAvailable(const QString &capability)
{
    return capability == QLatin1String("yes") || capability == QLatin1String("challenge");
}
}

PowerActions::PowerActions(QObject *parent)
    : QObject(parent)
{
    refreshCapabilities();
    auto *timer = new QTimer(this);
    timer->setInterval(15000);
    connect(timer, &QTimer::timeout, this, &PowerActions::refreshCapabilities);
    timer->start();
}

bool PowerActions::serviceAvailable() const
{
    return m_serviceAvailable;
}

bool PowerActions::suspendAvailable() const
{
    return m_suspendAvailable;
}

bool PowerActions::rebootAvailable() const
{
    return m_rebootAvailable;
}

bool PowerActions::powerOffAvailable() const
{
    return m_powerOffAvailable;
}

bool PowerActions::pending() const
{
    return m_pending;
}

QString PowerActions::error() const
{
    return m_error;
}

void PowerActions::request(const QString &action)
{
    if (m_pending) {
        return;
    }

    QString method;
    bool available = false;
    if (action == QLatin1String("suspend")) {
        method = QStringLiteral("Suspend");
        available = m_suspendAvailable;
    } else if (action == QLatin1String("reboot")) {
        method = QStringLiteral("Reboot");
        available = m_rebootAvailable;
    } else if (action == QLatin1String("poweroff")) {
        method = QStringLiteral("PowerOff");
        available = m_powerOffAvailable;
    } else {
        setError(tr("AeroShell received an unsupported power action."));
        return;
    }
    if (!available) {
        setError(tr("This power action is not available on this system."));
        return;
    }

    QDBusInterface login(loginService, loginPath, loginInterface, QDBusConnection::systemBus());
    if (!login.isValid()) {
        setError(tr("The system power service is unavailable."));
        return;
    }

    setError({});
    m_pending = true;
    Q_EMIT pendingChanged();
    auto *watcher = new QDBusPendingCallWatcher(login.asyncCall(method, true), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] {
        const QDBusPendingReply<> reply = *watcher;
        watcher->deleteLater();
        m_pending = false;
        Q_EMIT pendingChanged();
        if (reply.isError()) {
            setError(reply.error().message());
        }
        refreshCapabilities();
    });
}

void PowerActions::clearError()
{
    setError({});
}

void PowerActions::refreshCapabilities()
{
    if (m_refreshing) {
        return;
    }
    QDBusInterface login(loginService, loginPath, loginInterface, QDBusConnection::systemBus());
    if (!login.isValid()) {
        if (m_serviceAvailable || m_suspendAvailable || m_rebootAvailable || m_powerOffAvailable) {
            m_serviceAvailable = false;
            m_suspendAvailable = false;
            m_rebootAvailable = false;
            m_powerOffAvailable = false;
            Q_EMIT capabilitiesChanged();
        }
        return;
    }

    m_refreshing = true;
    auto *watcher = new QDBusPendingCallWatcher(login.asyncCall(QStringLiteral("CanSuspend")), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] {
        const QDBusPendingReply<QString> reply = *watcher;
        watcher->deleteLater();
        if (reply.isError()) {
            m_refreshing = false;
            return;
        }
        const bool suspend = actionAvailable(reply.value());

        QDBusInterface login(loginService, loginPath, loginInterface, QDBusConnection::systemBus());
        auto *rebootWatcher = new QDBusPendingCallWatcher(login.asyncCall(QStringLiteral("CanReboot")), this);
        connect(rebootWatcher, &QDBusPendingCallWatcher::finished, this, [this, rebootWatcher, suspend] {
            const QDBusPendingReply<QString> rebootReply = *rebootWatcher;
            rebootWatcher->deleteLater();
            if (rebootReply.isError()) {
                m_refreshing = false;
                return;
            }
            const QString rebootCapability = rebootReply.value();

            QDBusInterface login(loginService, loginPath, loginInterface, QDBusConnection::systemBus());
            auto *powerOffWatcher = new QDBusPendingCallWatcher(login.asyncCall(QStringLiteral("CanPowerOff")), this);
            connect(powerOffWatcher, &QDBusPendingCallWatcher::finished, this,
                    [this, powerOffWatcher, suspend, rebootCapability] {
                        const QDBusPendingReply<QString> powerOffReply = *powerOffWatcher;
                        powerOffWatcher->deleteLater();
                        m_refreshing = false;
                        if (powerOffReply.isError()) {
                            return;
                        }
                        const bool reboot = actionAvailable(rebootCapability);
                        const bool powerOff = actionAvailable(powerOffReply.value());
                        if (m_serviceAvailable && m_suspendAvailable == suspend && m_rebootAvailable == reboot
                            && m_powerOffAvailable == powerOff) {
                            return;
                        }
                        m_serviceAvailable = true;
                        m_suspendAvailable = suspend;
                        m_rebootAvailable = reboot;
                        m_powerOffAvailable = powerOff;
                        Q_EMIT capabilitiesChanged();
                    });
        });
    });
}

void PowerActions::setError(const QString &error)
{
    if (m_error == error) {
        return;
    }
    m_error = error;
    Q_EMIT errorChanged();
}
