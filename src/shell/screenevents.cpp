/* SPDX-License-Identifier: MIT */
#include "screenevents.h"

#include <QGuiApplication>
#include <QScreen>

ScreenEvents::ScreenEvents(QGuiApplication *application, QObject *parent)
    : QObject(parent)
{
    connect(application, &QGuiApplication::screenAdded, this, [this](QScreen *) {
        Q_EMIT screensChanged();
    });
    connect(application, &QGuiApplication::screenRemoved, this, [this](QScreen *) {
        Q_EMIT screensChanged();
    });
}
