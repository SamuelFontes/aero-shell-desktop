/* SPDX-License-Identifier: MIT */
#pragma once
#include <QMainWindow>
class QTreeWidget;
class RecycleBin final : public QMainWindow
{
public:
    RecycleBin();
private:
    QTreeWidget *m_items;
    void refresh();
    void watch(class KJob *job);
};
