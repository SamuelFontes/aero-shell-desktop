/* SPDX-License-Identifier: MIT */
#pragma once

#include <QObject>
#include <QString>

class ShellSounds final : public QObject
{
    Q_OBJECT

public:
    explicit ShellSounds(QString soundsDirectory, QObject *parent = nullptr);

    Q_INVOKABLE void play(const QString &eventName) const;

private:
    QString m_soundsDirectory;
};
