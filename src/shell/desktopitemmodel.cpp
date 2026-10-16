/* SPDX-License-Identifier: MIT */
#include "desktopitemmodel.h"

#include <KIO/ApplicationLauncherJob>
#include <KJob>
#include <KService>

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QStandardPaths>

DesktopItemModel::DesktopItemModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_desktopDirectory(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation))
{
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &DesktopItemModel::reload);
    reload();
}

int DesktopItemModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant DesktopItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return {};
    }

    const Entry &entry = m_entries.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return entry.name;
    case UrlRole:
        return entry.url;
    case IconNameRole:
        return entry.iconName;
    default:
        return {};
    }
}

QHash<int, QByteArray> DesktopItemModel::roleNames() const
{
    return {
        {Qt::DisplayRole, "display"},
        {UrlRole, "url"},
        {IconNameRole, "iconName"},
    };
}

void DesktopItemModel::open(const QUrl &url)
{
    if (!url.isLocalFile() || !QFileInfo::exists(url.toLocalFile())) {
        return;
    }

    const QFileInfo file(url.toLocalFile());
    if (file.suffix().compare(QStringLiteral("desktop"), Qt::CaseInsensitive) == 0) {
        const KService::Ptr service = KService::serviceByDesktopPath(file.absoluteFilePath());
        if (!service || !service->isApplication() || service->noDisplay()) {
            Q_EMIT openFailed(file.fileName(), tr("This desktop launcher is not a visible application."));
            return;
        }
        auto *job = new KIO::ApplicationLauncherJob(service, this);
        const QString applicationName = service->name();
        connect(job, &KJob::result, this, [this, job, applicationName] {
            if (job->error()) {
                Q_EMIT openFailed(applicationName, job->errorString());
            }
        });
        job->start();
        return;
    }

    if (!QDesktopServices::openUrl(url)) {
        Q_EMIT openFailed(file.fileName(), tr("No application is available to open this item."));
    }
}

void DesktopItemModel::reload()
{
    if (!QFileInfo(m_desktopDirectory).isDir()) {
        const QString parentDirectory = QFileInfo(m_desktopDirectory).absolutePath();
        const QStringList watchedDirectories = m_watcher.directories();
        for (const QString &watched : watchedDirectories) {
            if (watched != parentDirectory) {
                m_watcher.removePath(watched);
            }
        }
        if (QFileInfo(parentDirectory).isDir() && !m_watcher.directories().contains(parentDirectory)) {
            m_watcher.addPath(parentDirectory);
        }
        beginResetModel();
        m_entries.clear();
        endResetModel();
        return;
    }

    const QStringList watchedDirectories = m_watcher.directories();
    for (const QString &watched : watchedDirectories) {
        if (watched != m_desktopDirectory) {
            m_watcher.removePath(watched);
        }
    }
    if (!m_watcher.directories().contains(m_desktopDirectory)) {
        m_watcher.addPath(m_desktopDirectory);
    }

    const QDir desktopDir(m_desktopDirectory);
    const QFileInfoList files = desktopDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Readable,
                                                        QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
    QMimeDatabase mimeDatabase;
    QVector<Entry> entries;
    entries.reserve(files.size());
    for (const QFileInfo &file : files) {
        if (file.isHidden()) {
            continue;
        }
        const QMimeType mime = mimeDatabase.mimeTypeForFile(file.filePath(), QMimeDatabase::MatchExtension);
        const QString iconName = mime.isValid() ? mime.iconName() : QStringLiteral("unknown");
        entries.push_back({file.fileName(), QUrl::fromLocalFile(file.filePath()), iconName});
    }

    beginResetModel();
    m_entries = std::move(entries);
    endResetModel();
}
