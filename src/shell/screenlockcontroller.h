/* SPDX-License-Identifier: MIT */
#pragma once

#include <QObject>
#include <QString>

class ScreenLockController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit ScreenLockController(QObject *parent = nullptr);

    QString error() const;
    Q_INVOKABLE bool requestLock();

signals:
    void errorChanged();

private:
    void setError(const QString &error);

    QString m_error;
};
