/* SPDX-License-Identifier: MIT */
#include "../src/shell/notifications.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QThread>

#include <cstdio>

namespace
{
bool waitUntil(const std::function<bool()> &predicate, int timeoutMs = 2000)
{
    QElapsedTimer timer;
    timer.start();
    while (!predicate() && timer.elapsed() < timeoutMs) {
        QCoreApplication::processEvents();
        QThread::msleep(5);
    }
    QCoreApplication::processEvents();
    return predicate();
}
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const QDBusConnection disconnectedBus(QStringLiteral("aeroshell-test-disconnected"));
    NotificationsModel notifications(disconnectedBus);

    if (notifications.active()) {
        std::fputs("Notifications test unexpectedly acquired a session D-Bus service.\n", stderr);
        return 1;
    }
    const uint id = notifications.notify(QStringLiteral("Mail"), 0, QStringLiteral("mail"),
                                         QStringLiteral("New message"), QStringLiteral("<b>Safe</b><br>body"),
                                         {QStringLiteral("open"), QStringLiteral("Open")},
                                         {{QStringLiteral("urgency"), 2U}}, -1);
    if (id == 0 || notifications.count() != 1 || notifications.rowCount() != 1
        || notifications.data(notifications.index(0), NotificationsModel::UrgencyRole).toUInt() != 2
        || !notifications.data(notifications.index(0), NotificationsModel::BodyRole).toString().contains(QStringLiteral("Safe"))) {
        std::fputs("Notification fields were not stored or markup was not converted to safe text.\n", stderr);
        return 2;
    }

    const uint replacedId = notifications.notify(QStringLiteral("Mail"), id, QStringLiteral("mail"),
                                                  QStringLiteral("Updated"), QStringLiteral("Replacement"),
                                                  {QStringLiteral("open"), QStringLiteral("Open")}, {}, -1);
    if (replacedId != id || notifications.count() != 1
        || notifications.latestNotification().value(QStringLiteral("summary")).toString() != QStringLiteral("Updated")) {
        std::fputs("Replacing a notification did not update the existing entry.\n", stderr);
        return 3;
    }

    bool actionInvoked = false;
    bool closedByAction = false;
    QObject::connect(&notifications, &NotificationsModel::ActionInvoked,
                     [&actionInvoked](uint actionId, const QString &key) {
                         actionInvoked = actionId != 0 && key == QStringLiteral("open");
                     });
    QObject::connect(&notifications, &NotificationsModel::NotificationClosed,
                     [&closedByAction](uint closedId, uint reason) {
                         closedByAction = closedId != 0 && reason == 2;
                     });
    notifications.invokeAction(id, QStringLiteral("open"));
    if (!actionInvoked || !closedByAction || notifications.count() != 0) {
        std::fputs("Notification action activation or dismissal failed.\n", stderr);
        return 4;
    }

    bool expired = false;
    QObject::connect(&notifications, &NotificationsModel::NotificationClosed,
                     [&expired](uint, uint reason) { expired = reason == 1; });
    notifications.notify(QStringLiteral("Timer"), 0, {}, QStringLiteral("Short-lived"), {}, {}, {}, 20);
    if (!waitUntil([&notifications] { return notifications.count() == 0; }) || !expired) {
        std::fputs("Notification expiry did not close the entry.\n", stderr);
        return 5;
    }

    std::puts("PASS: safe notification text, replacement, actions, dismissal, and expiry without claiming D-Bus");
    return 0;
}
