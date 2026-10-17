/* SPDX-License-Identifier: MIT */
#include "screenevents.h"

#include <QGuiApplication>
#include <QScreen>

ScreenEvents::ScreenEvents(QGuiApplication *application, QObject *parent)
    : QObject(parent)
{
    const auto watch = [this](QScreen *screen) {
        connect(screen, &QScreen::geometryChanged, this, [this] { Q_EMIT screensChanged(); });
    };
    for (QScreen *screen : application->screens()) watch(screen);
    connect(application, &QGuiApplication::screenAdded, this, [this, watch](QScreen *screen) {
        watch(screen);
        Q_EMIT screensChanged();
    });
    connect(application, &QGuiApplication::screenRemoved, this, [this](QScreen *) {
        Q_EMIT screensChanged();
    });
}
