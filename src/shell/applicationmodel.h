/* SPDX-License-Identifier: MIT */
#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

class KService;

class ApplicationModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        StorageIdRole,
        IconNameRole,
        CommentRole,
    };
    Q_ENUM(Role)

    explicit ApplicationModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString query() const;
    void setQuery(const QString &query);

    Q_INVOKABLE void launch(const QString &storageId);
    Q_INVOKABLE void openHome();
    Q_INVOKABLE bool run(const QString &command);
    Q_INVOKABLE bool locationAvailable(const QString &location) const;
    Q_INVOKABLE void openLocation(const QString &location);

Q_SIGNALS:
    void queryChanged();
    void launchFailed(const QString &applicationName, const QString &message);

private:
    struct Entry {
        QString name;
        QString storageId;
        QString iconName;
        QString comment;
    };

    QVector<Entry> m_allEntries;
    QVector<int> m_visibleRows;
    QString m_query;

    void rebuildVisibleRows();
    QString locationPath(const QString &location) const;
};
