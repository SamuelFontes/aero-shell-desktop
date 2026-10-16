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
    QObject::connect(&notifications, &NotificationsModel::actionInvoked,
                     [&actionInvoked](uint actionId, const QString &key) {
                         actionInvoked = actionId != 0 && key == QStringLiteral("open");
                     });
    QObject::connect(&notifications, &NotificationsModel::notificationClosed,
                     [&closedByAction](uint closedId, uint reason) {
                         closedByAction = closedId != 0 && reason == 2;
                     });
    notifications.invokeAction(id, QStringLiteral("Open"));
    if (actionInvoked || notifications.count() != 1) {
        std::fputs("Notification action labels were accepted as action keys.\n", stderr);
        return 4;
    }
    notifications.invokeAction(id, QStringLiteral("open"));
    if (!actionInvoked || !closedByAction || notifications.count() != 0) {
        std::fputs("Notification action activation or dismissal failed.\n", stderr);
        return 4;
    }

    bool expired = false;
    QObject::connect(&notifications, &NotificationsModel::notificationClosed,
                     [&expired](uint, uint reason) { expired = reason == 1; });
    notifications.notify(QStringLiteral("Timer"), 0, {}, QStringLiteral("Short-lived"), {}, {}, {}, 20);
    if (!waitUntil([&notifications] { return notifications.count() == 0; }) || !expired) {
        std::fputs("Notification expiry did not close the entry.\n", stderr);
        return 5;
    }

    const uint permanent = notifications.notify(QStringLiteral("Persistent"), 0, {}, QStringLiteral("Keep"), {}, {}, {}, 0);
    notifications.notify(QStringLiteral("Newer"), 0, {}, QStringLiteral("Newest"), {}, {}, {}, 0);
    notifications.notify(QStringLiteral("Persistent"), permanent, {}, QStringLiteral("Updated oldest"), {}, {}, {}, 0);
    if (notifications.latestNotification().value(QStringLiteral("notificationId")).toUInt() != permanent) {
        std::fputs("An updated older notification did not become the displayed latest notification.\n", stderr);
        return 6;
    }
    const uint automatic = notifications.notify(QStringLiteral("Default"), 0, {}, QStringLiteral("Expire"), {}, {}, {}, -1);
    if (!waitUntil([&notifications, automatic] {
        for (int row = 0; row < notifications.count(); ++row) {
            if (notifications.data(notifications.index(row), NotificationsModel::IdRole).toUInt() == automatic)
                return false;
        }
        return true;
    }, 7000) || notifications.count() != 2) {
        std::fputs("Default expiry (-1) or persistent lifetime (0) does not follow the notification protocol.\n", stderr);
        return 7;
    }
    std::puts("PASS: notification text, replacement ordering, action keys, dismissal, and protocol expiry without claiming D-Bus");
    return 0;
}
