/* SPDX-License-Identifier: MIT */
#pragma once

#include <QAbstractListModel>
#include <QFileSystemWatcher>
#include <QUrl>
#include <QVector>

class DesktopItemModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        UrlRole = Qt::UserRole + 1,
        IconNameRole,
    };
    Q_ENUM(Role)

    explicit DesktopItemModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void open(const QUrl &url);
    Q_INVOKABLE void newFolder();
    Q_INVOKABLE void refresh() { reload(); }

Q_SIGNALS:
    void openFailed(const QString &name, const QString &message);

private:
    struct Entry {
        QString name;
        QUrl url;
        QString iconName;
    };

    QString m_desktopDirectory;
    QFileSystemWatcher m_watcher;
    QVector<Entry> m_entries;

    void reload();
};
