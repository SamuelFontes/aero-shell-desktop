/* SPDX-License-Identifier: MIT */
#include "audiocontrol.h"

#include <QRegularExpression>
#include <QProcessEnvironment>
#include <QStandardPaths>

AudioControl::AudioControl(QObject *parent)
    : QObject(parent)
    , m_wpctl(QStandardPaths::findExecutable(QStringLiteral("wpctl")))
    , m_pactl(QStandardPaths::findExecutable(QStringLiteral("pactl")))
{
    if (!m_wpctl.isEmpty()) {
        m_backend = QStringLiteral("wpctl");
    } else if (!m_pactl.isEmpty()) {
        m_backend = QStringLiteral("pactl");
    }

    m_refreshTimer.setInterval(3000);
    connect(&m_refreshTimer, &QTimer::timeout, this, &AudioControl::refresh);
    m_refreshTimer.start();
    QTimer::singleShot(0, this, &AudioControl::refresh);
}

bool AudioControl::available() const
{
    return m_available;
}

int AudioControl::volume() const
{
    return m_volume;
}

bool AudioControl::muted() const
{
    return m_muted;
}

QString AudioControl::backend() const
{
    return m_backend;
}

void AudioControl::refresh()
{
    if (m_queryProcess || m_backend.isEmpty()) {
        return;
    }
    startQuery(RefreshStep::Volume);
}

void AudioControl::setVolume(int percent)
{
    const int safePercent = qBound(0, percent, 100);
    if (m_backend == QLatin1String("wpctl")) {
        runControlCommand({QStringLiteral("set-volume"), QStringLiteral("@DEFAULT_AUDIO_SINK@"),
                           QStringLiteral("%1%").arg(safePercent)});
    } else if (m_backend == QLatin1String("pactl")) {
        runControlCommand({QStringLiteral("set-sink-volume"), QStringLiteral("@DEFAULT_SINK@"),
                           QStringLiteral("%1%").arg(safePercent)});
    }
    QTimer::singleShot(180, this, &AudioControl::refresh);
}

void AudioControl::toggleMute()
{
    if (m_backend == QLatin1String("wpctl")) {
        runControlCommand({QStringLiteral("set-mute"), QStringLiteral("@DEFAULT_AUDIO_SINK@"), QStringLiteral("toggle")});
    } else if (m_backend == QLatin1String("pactl")) {
        runControlCommand({QStringLiteral("set-sink-mute"), QStringLiteral("@DEFAULT_SINK@"), QStringLiteral("toggle")});
    }
    QTimer::singleShot(180, this, &AudioControl::refresh);
}

void AudioControl::startQuery(RefreshStep step)
{
    auto *process = new QProcess(this);
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    process->setProcessEnvironment(environment);
    m_queryProcess = process;

    connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this, process, step](int exitCode, QProcess::ExitStatus status) {
                handleQuery(process, step, exitCode, status);
            });
    connect(process, &QProcess::errorOccurred, this, [this, process, step](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart && m_queryProcess == process) {
            handleQuery(process, step, -1, QProcess::CrashExit);
        }
    });

    if (m_backend == QLatin1String("wpctl")) {
        process->start(m_wpctl, {QStringLiteral("get-volume"), QStringLiteral("@DEFAULT_AUDIO_SINK@")});
    } else if (step == RefreshStep::Volume) {
        process->start(m_pactl, {QStringLiteral("get-sink-volume"), QStringLiteral("@DEFAULT_SINK@")});
    } else {
        process->start(m_pactl, {QStringLiteral("get-sink-mute"), QStringLiteral("@DEFAULT_SINK@")});
    }
}

void AudioControl::handleQuery(QProcess *process, RefreshStep step, int exitCode, QProcess::ExitStatus status)
{
    if (m_queryProcess != process) {
        process->deleteLater();
        return;
    }

    const QByteArray output = process->readAllStandardOutput();
    m_queryProcess = nullptr;
    process->deleteLater();

    if (status != QProcess::NormalExit || exitCode != 0) {
        setAvailable(false);
        return;
    }

    const QString text = QString::fromLocal8Bit(output).trimmed();
    if (m_backend == QLatin1String("wpctl") || step == RefreshStep::Volume) {
        const QRegularExpression volumePattern(m_backend == QLatin1String("wpctl")
                ? QStringLiteral("^Volume:\\s*(\\d+(?:\\.\\d+)?)")
                : QStringLiteral("(\\d{1,3})%"));
        const QRegularExpressionMatch match = volumePattern.match(text);
        if (!match.hasMatch()) {
            setAvailable(false);
            return;
        }
        bool ok = false;
        const double raw = match.captured(1).toDouble(&ok);
        if (!ok) {
            setAvailable(false);
            return;
        }
        const int newVolume = qBound(0, qRound(m_backend == QLatin1String("wpctl") ? raw * 100.0 : raw), 100);
        if (m_backend == QLatin1String("wpctl")) {
            const bool newMuted = text.contains(QStringLiteral("[MUTED]"), Qt::CaseInsensitive);
            const bool changed = !m_available || m_volume != newVolume || m_muted != newMuted;
            m_volume = newVolume;
            m_muted = newMuted;
            m_available = true;
            if (changed) {
                Q_EMIT stateChanged();
            }
        } else {
            m_pendingVolume = newVolume;
            startQuery(RefreshStep::Mute);
        }
        return;
    }

    const bool newMuted = text.contains(QStringLiteral("yes"), Qt::CaseInsensitive);
    const bool changed = !m_available || m_volume != m_pendingVolume || m_muted != newMuted;
    m_volume = m_pendingVolume;
    m_muted = newMuted;
    m_available = true;
    if (changed) {
        Q_EMIT stateChanged();
    }
}

void AudioControl::runControlCommand(const QStringList &arguments)
{
    const QString program = m_backend == QLatin1String("wpctl") ? m_wpctl : m_pactl;
    if (!program.isEmpty()) {
        QProcess::startDetached(program, arguments);
    }
}

void AudioControl::setAvailable(bool available)
{
    if (m_available == available) {
        return;
    }
    m_available = available;
    Q_EMIT stateChanged();
}
