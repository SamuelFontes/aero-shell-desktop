/* SPDX-License-Identifier: MIT */
#include "screenlockcontroller.h"

#include <QDBusConnection>
#include <QDBusMessage>

ScreenLockController::ScreenLockController(QObject *parent)
    : QObject(parent)
{
}

QString ScreenLockController::error() const
{
    return m_error;
}

bool ScreenLockController::requestLock()
{
    const QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        setError(QStringLiteral("AeroShell's private D-Bus session is unavailable."));
        return false;
    }

    QDBusMessage lockMessage = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.ScreenSaver"),
        QStringLiteral("/ScreenSaver"),
        QStringLiteral("org.freedesktop.ScreenSaver"),
        QStringLiteral("Lock"));
    const QDBusMessage lockReply = bus.call(lockMessage, QDBus::BlockWithGui, 5000);
    if (lockReply.type() == QDBusMessage::ErrorMessage) {
        setError(QStringLiteral("KWin could not lock this AeroShell session: %1").arg(lockReply.errorMessage()));
        return false;
    }

    QDBusMessage activeMessage = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.ScreenSaver"),
        QStringLiteral("/ScreenSaver"),
        QStringLiteral("org.freedesktop.ScreenSaver"),
        QStringLiteral("GetActive"));
    const QDBusMessage activeReply = bus.call(activeMessage, QDBus::BlockWithGui, 5000);
    if (activeReply.type() == QDBusMessage::ErrorMessage || activeReply.arguments().isEmpty()
        || !activeReply.arguments().constFirst().toBool()) {
        setError(QStringLiteral("KWin did not confirm that the AeroShell session is locked."));
        return false;
    }

    setError(QString());
    return true;
}

void ScreenLockController::setError(const QString &error)
{
    if (m_error == error) {
        return;
    }
    m_error = error;
    emit errorChanged();
}
