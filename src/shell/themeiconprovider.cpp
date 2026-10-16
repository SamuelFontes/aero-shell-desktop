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
    const QSize targetSize = requestedSize.isValid() ? requestedSize : QSize(32, 32);
    if (size) {
        *size = targetSize;
    }

    QPixmap pixmap;
    if (QFileInfo(id).isAbsolute()) {
        pixmap = QPixmap(id).scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    } else {
        QIcon icon = QIcon::fromTheme(id);
        if (icon.isNull()) {
            icon = QIcon::fromTheme(QStringLiteral("application-x-generic"));
        }
        pixmap = icon.pixmap(targetSize);
    }
    return pixmap;
}
