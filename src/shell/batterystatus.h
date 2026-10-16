/* SPDX-License-Identifier: MIT */
#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

class BatteryStatus final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool present READ present NOTIFY stateChanged)
    Q_PROPERTY(int percentage READ percentage NOTIFY stateChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY stateChanged)

public:
    explicit BatteryStatus(QObject *parent = nullptr);
    BatteryStatus(const QString &powerSupplyRoot, QObject *parent);

    bool present() const;
    int percentage() const;
    QString summary() const;

    Q_INVOKABLE void refresh();

Q_SIGNALS:
    void stateChanged();

private:
    QString m_powerSupplyRoot;
    QTimer m_refreshTimer;
    bool m_present = false;
    int m_percentage = 0;
    QString m_summary;
};
