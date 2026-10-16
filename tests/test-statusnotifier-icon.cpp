/* SPDX-License-Identifier: MIT */
#include "../src/shell/statusnotifiericon.h"

#include <cstdio>

int main()
{
    // The StatusNotifier protocol transports pixels as network-order A,R,G,B.
    const QByteArray argb = QByteArray::fromHex("ffff00008000ff00");
    const QImage image = decodeStatusNotifierIconPixmap(2, 1, argb);
    if (image.size() != QSize(2, 1) || image.pixel(0, 0) != qRgba(255, 0, 0, 255)
        || image.pixel(1, 0) != qRgba(0, 255, 0, 128)) {
        std::fprintf(stderr, "StatusNotifier ARGB pixels were decoded with incorrect channel order.\n");
        return 1;
    }
    if (!decodeStatusNotifierIconPixmap(0, 1, argb).isNull()
        || !decodeStatusNotifierIconPixmap(1, 1, QByteArray::fromHex("ff00")).isNull()) {
        std::fprintf(stderr, "Invalid StatusNotifier icon data was accepted.\n");
        return 1;
    }
    return 0;
}
