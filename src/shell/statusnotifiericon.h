/* SPDX-License-Identifier: MIT */
#pragma once

#include <QByteArray>
#include <QImage>

#include <limits>

inline QImage decodeStatusNotifierIconPixmap(int width, int height, const QByteArray &argb)
{
    if (width <= 0 || height <= 0) {
        return {};
    }
    const qint64 pixelCount = qint64(width) * qint64(height);
    if (pixelCount <= 0 || pixelCount > std::numeric_limits<int>::max() / 4
        || argb.size() < pixelCount * 4) {
        return {};
    }

    QImage image(width, height, QImage::Format_ARGB32);
    if (image.isNull()) {
        return {};
    }
    const auto *source = reinterpret_cast<const uchar *>(argb.constData());
    for (int y = 0; y < height; ++y) {
        auto *row = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < width; ++x) {
            const qint64 offset = (qint64(y) * width + x) * 4;
            row[x] = qRgba(source[offset + 1], source[offset + 2], source[offset + 3], source[offset]);
        }
    }
    return image;
}
