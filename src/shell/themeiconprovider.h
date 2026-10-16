/* SPDX-License-Identifier: MIT */
#pragma once

#include <QQuickImageProvider>

class ThemeIconProvider final : public QQuickImageProvider
{
public:
    ThemeIconProvider();

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
};
