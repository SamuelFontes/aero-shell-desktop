/* SPDX-License-Identifier: MIT */
#pragma once
#include <QMainWindow>
#include <QUrl>
class QFileSystemModel;
class QTreeView;
class QListView;
class QLineEdit;
class QStackedWidget;
class QLabel;
class QHBoxLayout;
class QAction;
class QAbstractItemView;
class Explorer final : public QMainWindow
{
    Q_OBJECT
public:
    explicit Explorer(const QString &path = {});
    void navigate(const QString &path, bool record = true);
private:
    QFileSystemModel *m_files;
    QTreeView *m_details;
    QListView *m_icons;
    QStackedWidget *m_views;
    QLineEdit *m_address;
    QStackedWidget *m_addressViews;
    QHBoxLayout *m_breadcrumbs;
    void showAddress();
    QLineEdit *m_search;
    QLabel *m_status;
    QAction *m_back;
    QAction *m_forward;
    QString m_path;
    QStringList m_history;
    int m_historyIndex = -1;
    QAbstractItemView *view() const;
    QList<QUrl> selection() const;
    void copy(bool cut);
    void paste();
    void remove(bool permanent);
    void newFolder();
    void properties();
    void updateStatus();
    void openSelection();
    void watchJob(class KJob *job);
};
