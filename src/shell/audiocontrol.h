/* SPDX-License-Identifier: MIT */
#pragma once

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QTimer>

class AudioControl final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY stateChanged)
    Q_PROPERTY(int volume READ volume NOTIFY stateChanged)
    Q_PROPERTY(bool muted READ muted NOTIFY stateChanged)
    Q_PROPERTY(QString backend READ backend NOTIFY stateChanged)

public:
    explicit AudioControl(QObject *parent = nullptr);

    bool available() const;
    int volume() const;
    bool muted() const;
    QString backend() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setVolume(int percent);
    Q_INVOKABLE void toggleMute();

Q_SIGNALS:
    void stateChanged();

private:
    enum class RefreshStep {
        Volume,
        Mute,
    };

    QString m_wpctl;
    QString m_pactl;
    QString m_backend;
    QTimer m_refreshTimer;
    QProcess *m_queryProcess = nullptr;
    bool m_available = false;
    int m_volume = 0;
    int m_pendingVolume = 0;
    bool m_muted = false;

    void startQuery(RefreshStep step);
    void handleQuery(QProcess *process, RefreshStep step, int exitCode, QProcess::ExitStatus status);
    void runControlCommand(const QStringList &arguments);
    void setAvailable(bool available);
};
