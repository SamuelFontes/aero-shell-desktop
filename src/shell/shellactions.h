/* SPDX-License-Identifier: MIT */
#pragma once
#include <QObject>
class ShellActions final : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.aeroshell.Shell")
public:
    using QObject::QObject;
public Q_SLOTS:
    void toggleLauncher() { Q_EMIT actionRequested(QStringLiteral("start")); }
    void dispatch(const QString &action) { Q_EMIT actionRequested(action); }
Q_SIGNALS:
    void actionRequested(const QString &action);
};
