/* SPDX-License-Identifier: MIT */
#pragma once

#include <QObject>
#include <QStringList>
#include <QTimer>

class QDBusServiceWatcher;

class KeyboardLayoutStatus final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(QString currentLabel READ currentLabel NOTIFY changed)
    Q_PROPERTY(QString currentName READ currentName NOTIFY changed)
    Q_PROPERTY(QStringList layouts READ layouts NOTIFY changed)

public:
    struct Layout {
        QString shortName;
        QString displayName;
        QString longName;
    };

    explicit KeyboardLayoutStatus(QObject *parent = nullptr);

    bool available() const;
    QString currentLabel() const;
    QString currentName() const;
    QStringList layouts() const;
    Q_INVOKABLE void nextLayout();
    Q_INVOKABLE void setLayout(int index);

Q_SIGNALS:
    void changed();

private Q_SLOTS:
    void refresh();
    void onLayoutChanged(uint index);

private:
    QDBusServiceWatcher *m_serviceWatcher = nullptr;
    QTimer m_refreshTimer;
    QStringList m_labels;
    QStringList m_names;
    uint m_currentIndex = 0;
    bool m_available = false;
};
