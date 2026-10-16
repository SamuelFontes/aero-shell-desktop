/* SPDX-License-Identifier: MIT */
#include "applicationmodel.h"

#include <KIO/ApplicationLauncherJob>
#include <KJob>
#include <KApplicationTrader>
#include <KService>

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUrl>

#include <algorithm>

ApplicationModel::ApplicationModel(QObject *parent)
    : QAbstractListModel(parent)
{
    const auto services = KApplicationTrader::query([](const KService::Ptr &service) {
        return service && service->isApplication() && !service->noDisplay();
    });

    m_allEntries.reserve(services.size());
    for (const auto &service : services) {
        if (!service || service->storageId().isEmpty()) {
            continue;
        }
        m_allEntries.push_back({service->name(), service->storageId(), service->icon(), service->comment()});
    }

    std::sort(m_allEntries.begin(), m_allEntries.end(), [](const Entry &left, const Entry &right) {
        const int nameOrder = QString::localeAwareCompare(left.name, right.name);
        return nameOrder == 0 ? left.storageId < right.storageId : nameOrder < 0;
    });
    rebuildVisibleRows();
}

int ApplicationModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_visibleRows.size();
}

QVariant ApplicationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visibleRows.size()) {
        return {};
    }

    const Entry &entry = m_allEntries.at(m_visibleRows.at(index.row()));
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return entry.name;
    case StorageIdRole:
        return entry.storageId;
    case IconNameRole:
        return entry.iconName;
    case CommentRole:
        return entry.comment;
    default:
        return {};
    }
}

QHash<int, QByteArray> ApplicationModel::roleNames() const
{
    return {
        {Qt::DisplayRole, "display"},
        {NameRole, "name"},
        {StorageIdRole, "storageId"},
        {IconNameRole, "iconName"},
        {CommentRole, "comment"},
    };
}

QString ApplicationModel::query() const
{
    return m_query;
}

void ApplicationModel::setQuery(const QString &query)
{
    const QString trimmed = query.trimmed();
    if (m_query == trimmed) {
        return;
    }
    beginResetModel();
    m_query = trimmed;
    rebuildVisibleRows();
    endResetModel();
    Q_EMIT queryChanged();
}

void ApplicationModel::launch(const QString &storageId)
{
    const KService::Ptr service = KService::serviceByStorageId(storageId);
    if (!service || !service->isApplication() || service->noDisplay()) {
        Q_EMIT launchFailed(storageId, tr("The application entry is no longer available."));
        return;
    }

    auto *job = new KIO::ApplicationLauncherJob(service, this);
    const QString applicationName = service->name();
    connect(job, &KJob::result, this, [this, job, applicationName] {
        if (job->error()) {
            Q_EMIT launchFailed(applicationName, job->errorString());
        }
    });
    job->start();
}

void ApplicationModel::openHome()
{
    openLocation(QStringLiteral("home"));
}

bool ApplicationModel::locationAvailable(const QString &location) const
{
    const QString path = locationPath(location);
    return !path.isEmpty() && QFileInfo(path).isDir();
}

void ApplicationModel::openLocation(const QString &location)
{
    const QString path = locationPath(location);
    if (path.isEmpty() || !QFileInfo(path).isDir()) {
        Q_EMIT launchFailed(location, tr("This folder is not available."));
        return;
    }

    const QUrl url = QUrl::fromLocalFile(path);
    const KService::Ptr service = KApplicationTrader::preferredService(QStringLiteral("inode/directory"));
    if (!service) {
        if (!QDesktopServices::openUrl(url)) {
            Q_EMIT launchFailed(location, tr("No file manager is available."));
        }
        return;
    }

    auto *job = new KIO::ApplicationLauncherJob(service, this);
    job->setUrls({url});
    const QString applicationName = service->name();
    connect(job, &KJob::result, this, [this, job, applicationName] {
        if (job->error()) {
            Q_EMIT launchFailed(applicationName, job->errorString());
        }
    });
    job->start();
}

QString ApplicationModel::locationPath(const QString &location) const
{
    if (location == QLatin1String("computer")) {
        return QStringLiteral("/");
    }
    if (location == QLatin1String("home")) {
        return QDir::homePath();
    }

    QStandardPaths::StandardLocation standardLocation;
    if (location == QLatin1String("desktop")) {
        standardLocation = QStandardPaths::DesktopLocation;
    } else if (location == QLatin1String("documents")) {
        standardLocation = QStandardPaths::DocumentsLocation;
    } else if (location == QLatin1String("pictures")) {
        standardLocation = QStandardPaths::PicturesLocation;
    } else if (location == QLatin1String("music")) {
        standardLocation = QStandardPaths::MusicLocation;
    } else if (location == QLatin1String("downloads")) {
        standardLocation = QStandardPaths::DownloadLocation;
    } else if (location == QLatin1String("videos")) {
        standardLocation = QStandardPaths::MoviesLocation;
    } else {
        return {};
    }
    return QStandardPaths::writableLocation(standardLocation);
}

void ApplicationModel::rebuildVisibleRows()
{
    m_visibleRows.clear();
    const QString needle = m_query.toCaseFolded();
    for (qsizetype row = 0; row < m_allEntries.size(); ++row) {
        const Entry &entry = m_allEntries.at(row);
        if (needle.isEmpty() || entry.name.toCaseFolded().contains(needle)
            || entry.comment.toCaseFolded().contains(needle)
            || entry.storageId.toCaseFolded().contains(needle)) {
            m_visibleRows.push_back(static_cast<int>(row));
        }
    }
}
