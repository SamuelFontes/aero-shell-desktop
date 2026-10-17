/* SPDX-License-Identifier: MIT */
#include "recyclebin.h"
#include <KIO/ListJob>
#include <KIO/EmptyTrashJob>
#include <KIO/JobUiDelegateFactory>
#include <KIO/RestoreJob>
#include <KFileItem>
#include <QHeaderView>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>
#include <QTreeWidget>
RecycleBin::RecycleBin()
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Recycle Bin"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("user-trash")));
    resize(850, 560);
    m_items = new QTreeWidget;
    m_items->setHeaderLabels({tr("Name"), tr("Size"), tr("Type")});
    m_items->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_items->setRootIsDecorated(false);
    m_items->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    setCentralWidget(m_items);
    auto *tools = addToolBar(tr("Recycle Bin"));
    tools->setMovable(false);
    tools->addAction(tr("Restore selected items"), this, [this] {
        QList<QUrl> urls;
        for (auto *item : m_items->selectedItems()) urls.append(item->data(0, Qt::UserRole).toUrl());
        if (!urls.isEmpty()) watch(KIO::restoreFromTrash(urls));
    });
    tools->addAction(tr("Empty Recycle Bin…"), this, [this] {
        if (QMessageBox::question(this, tr("Empty Recycle Bin"), tr("Permanently delete every item in the Recycle Bin? This cannot be undone."),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) watch(KIO::emptyTrash());
    });
    tools->addAction(tr("Refresh"), this, &RecycleBin::refresh);
    refresh();
}
void RecycleBin::watch(KJob *job)
{
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
    connect(job, &KJob::result, this, [this, job] { if (!job->error()) refresh(); });
}
void RecycleBin::refresh()
{
    m_items->clear();
    auto *job = KIO::listDir(QUrl(QStringLiteral("trash:/")), KIO::HideProgressInfo);
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
    connect(job, &KIO::ListJob::entries, this, [this](KIO::Job *, const KIO::UDSEntryList &entries) {
        for (const auto &entry : entries) {
            const QString name = entry.stringValue(KIO::UDSEntry::UDS_NAME);
            if (name == QLatin1String(".") || name == QLatin1String("..")) continue;
            QUrl url(QStringLiteral("trash:/")); url.setPath(QLatin1Char('/') + name);
            KFileItem file(entry, url);
            auto *row = new QTreeWidgetItem(m_items, {file.text(), QString::number(file.size()), file.mimeComment()});
            row->setIcon(0, QIcon::fromTheme(file.iconName()));
            row->setData(0, Qt::UserRole, url);
        }
        statusBar()->showMessage(tr("%1 items").arg(m_items->topLevelItemCount()));
    });
}
