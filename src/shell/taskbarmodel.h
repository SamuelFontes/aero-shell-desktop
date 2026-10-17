/* SPDX-License-Identifier: MIT */
#pragma once
#include <QAbstractListModel>
#include <QPointer>
class ApplicationModel;
class TaskbarModel final : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit TaskbarModel(ApplicationModel *applications, QObject *parent = nullptr);
    void setSource(QAbstractItemModel *source);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE void requestActivate(int row);
    Q_INVOKABLE void requestClose(int row);
    Q_INVOKABLE void requestToggleMinimized(int row);
    Q_INVOKABLE void requestToggleMaximized(int row);
    Q_INVOKABLE QVariantList windows(int row) const;
    Q_INVOKABLE void activateWindow(int sourceRow);
private:
    struct Group { QString appId, title; QList<int> rows; bool pinned = false; };
    QList<Group> m_groups;
    QPointer<QAbstractItemModel> m_source;
    ApplicationModel *m_applications;
    QString m_browser;
    void rebuild();
    int representative(int row) const;
    void invoke(const char *method, int row);
};
