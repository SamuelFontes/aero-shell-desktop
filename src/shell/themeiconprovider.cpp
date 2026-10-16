/* SPDX-License-Identifier: MIT */
#include "themeiconprovider.h"

#include <QIcon>
#include <QFileInfo>
#include <QPixmap>

ThemeIconProvider::ThemeIconProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}

QPixmap ThemeIconProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    // The QML cache-busting query is part of the provider id, not the theme
    // icon name (or local filename).
    const QString iconId = id.section(QLatin1Char('?'), 0, 0);
    const QSize targetSize = requestedSize.width() > 0 && requestedSize.height() > 0
        ? requestedSize : QSize(32, 32);
    if (size) {
        *size = targetSize;
    }

    QPixmap pixmap;
    if (QFileInfo(iconId).isAbsolute()) {
        pixmap = QPixmap(iconId).scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    } else {
        QIcon icon = QIcon::fromTheme(iconId);
        if (icon.isNull()) {
            icon = QIcon::fromTheme(QStringLiteral("application-x-generic"));
        }
        pixmap = icon.pixmap(targetSize);
    }
    return pixmap;
}
