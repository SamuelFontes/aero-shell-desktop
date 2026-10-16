/* SPDX-License-Identifier: MIT */
#pragma once

#include <QAbstractListModel>
#include <QDBusConnection>
#include <QVariantMap>
#include <QVector>

class DBusMenuModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(bool open READ isOpen NOTIFY openChanged)
    Q_PROPERTY(int depth READ depth NOTIFY menuChanged)
    Q_PROPERTY(int itemCount READ itemCount NOTIFY menuChanged)
    Q_PROPERTY(QString error READ error NOTIFY menuChanged)

public:
    enum Role {
        ItemIdRole = Qt::UserRole + 1,
        LabelRole,
        ItemEnabledRole,
        SeparatorRole,
        ItemCheckableRole,
        ItemCheckedRole,
        SubmenuRole,
        IconNameRole,
    };
    Q_ENUM(Role)

    explicit DBusMenuModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool loading() const;
    bool isOpen() const;
    int depth() const;
    int itemCount() const;
    QString error() const;

    Q_INVOKABLE void openMenu(const QString &serviceAndItemPath, const QString &menuPath);
    Q_INVOKABLE void openSubmenu(int itemId);
    Q_INVOKABLE void goBack();
    Q_INVOKABLE void trigger(int itemId);
    Q_INVOKABLE void close();

Q_SIGNALS:
    void loadingChanged();
    void openChanged();
    void menuChanged();

private Q_SLOTS:
    void onLayoutUpdated(uint revision, int parentId);

private:
    struct Entry {
        int id = 0;
        QString label;
        QString iconName;
        bool enabled = true;
        bool separator = false;
        bool checkable = false;
        bool checked = false;
        bool submenu = false;
    };

    QDBusConnection m_bus;
    QString m_service;
    QString m_path;
    QVector<int> m_parentIds;
    QVector<Entry> m_entries;
    QString m_error;
    bool m_loading = false;
    quint64 m_requestGeneration = 0;

    void setLoading(bool loading);
    void setError(const QString &error);
    static QVector<Entry> entriesFromLayout(const QVariant &layout);
    void requestLayout(int parentId, bool notifyAboutToShow = true);
    void fetchLayout(int parentId, quint64 generation);
    void sendEvent(int itemId);
};
