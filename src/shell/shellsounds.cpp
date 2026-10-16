/* SPDX-License-Identifier: MIT */
#include "shellsounds.h"

#include <QDebug>
#include <QDir>
#include <QHash>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

#include <utility>

namespace
{
const QHash<QString, QString> soundFiles{
    {QStringLiteral("desktop-login"), QStringLiteral("desktop-login.ogg")},
    {QStringLiteral("desktop-logout"), QStringLiteral("desktop-logout.ogg")},
    {QStringLiteral("dialog-information"), QStringLiteral("dialog-information.ogg")},
    {QStringLiteral("dialog-warning"), QStringLiteral("dialog-warning.ogg")},
    {QStringLiteral("dialog-error"), QStringLiteral("dialog-error.ogg")},
    {QStringLiteral("dialog-question"), QStringLiteral("dialog-question.ogg")},
    {QStringLiteral("device-added"), QStringLiteral("device-added.ogg")},
    {QStringLiteral("device-removed"), QStringLiteral("device-removed.ogg")},
    {QStringLiteral("message-new-instant"), QStringLiteral("message-new-instant.ogg")},
    {QStringLiteral("trash-empty"), QStringLiteral("trash-empty.ogg")},
};
}

ShellSounds::ShellSounds(QString soundsDirectory, QObject *parent)
    : QObject(parent)
    , m_soundsDirectory(std::move(soundsDirectory))
{
}

void ShellSounds::play(const QString &eventName) const
{
    const auto event = soundFiles.constFind(eventName);
    if (event == soundFiles.cend()) {
        qWarning().noquote() << "AeroShell ignored an unknown sound event:" << eventName;
        return;
    }

    const QString soundPath = QDir(m_soundsDirectory).filePath(*event);
    if (!QFileInfo(soundPath).isFile()) {
        qWarning().noquote() << "AeroShell sound asset is missing:" << soundPath;
        return;
    }

    const QString player = QStandardPaths::findExecutable(QStringLiteral("canberra-gtk-play"));
    if (player.isEmpty()) {
        qWarning("AeroShell event sounds require the libcanberra canberra-gtk-play utility.");
        return;
    }

    if (!QProcess::startDetached(player, {QStringLiteral("--file=") + soundPath,
                                          QStringLiteral("--description=AeroShell desktop event")})) {
        qWarning("AeroShell could not start its event sound player.");
    }
}
