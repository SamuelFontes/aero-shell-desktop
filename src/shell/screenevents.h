/* SPDX-License-Identifier: MIT */
#pragma once

#include <QObject>

class QGuiApplication;

class ScreenEvents final : public QObject
{
    Q_OBJECT

public:
    explicit ScreenEvents(QGuiApplication *application, QObject *parent = nullptr);

Q_SIGNALS:
    void screensChanged();
};
