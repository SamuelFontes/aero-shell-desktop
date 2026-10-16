/* SPDX-License-Identifier: MIT */
#include "../src/shell/audiocontrol.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <QThread>

#include <cstdio>

namespace
{
bool waitUntil(const std::function<bool()> &predicate, int timeoutMs = 3000)
{
    QElapsedTimer timer;
    timer.start();
    while (!predicate() && timer.elapsed() < timeoutMs) {
        QCoreApplication::processEvents();
        QThread::msleep(10);
    }
    QCoreApplication::processEvents();
    return predicate();
}

bool writeFile(const QString &path, const QByteArray &contents, QFileDevice::Permissions permissions = QFileDevice::ReadOwner | QFileDevice::WriteOwner)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    if (file.write(contents) != contents.size()) {
        return false;
    }
    file.close();
    return file.setPermissions(permissions);
}

QByteArray readFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir temporaryDirectory;
    if (!temporaryDirectory.isValid()) {
        std::fprintf(stderr, "FAIL: cannot create a temporary test directory\n");
        return 1;
    }

    const QString toolPath = temporaryDirectory.filePath(QStringLiteral("wpctl"));
    const QString statePath = temporaryDirectory.filePath(QStringLiteral("audio-state"));
    const QString callsPath = temporaryDirectory.filePath(QStringLiteral("audio-calls"));
    const QByteArray fakeWpctl = QByteArrayLiteral(
        "#!/bin/sh\n"
        "if [ \"$1\" = get-volume ]; then\n"
        "  [ \"$LC_ALL\" = C ] || exit 99\n"
        "  /bin/cat \"$AEROSHELL_TEST_AUDIO_STATE\"\n"
        "else\n"
        "  printf '%s\\n' \"$*\" >> \"$AEROSHELL_TEST_AUDIO_CALLS\"\n"
        "fi\n");

    if (!writeFile(toolPath, fakeWpctl, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner)
        || !writeFile(statePath, QByteArrayLiteral("Volume: 0.42\n"))
        || !writeFile(callsPath, {})) {
        std::fprintf(stderr, "FAIL: cannot prepare fake wpctl\n");
        return 1;
    }

    const QByteArray previousPath = qgetenv("PATH");
    qputenv("PATH", temporaryDirectory.path().toLocal8Bit());
    qputenv("AEROSHELL_TEST_AUDIO_STATE", statePath.toLocal8Bit());
    qputenv("AEROSHELL_TEST_AUDIO_CALLS", callsPath.toLocal8Bit());
    qputenv("LC_ALL", "C.UTF-8");

    AudioControl audio;
    if (audio.backend() != QLatin1String("wpctl")) {
        std::fprintf(stderr, "FAIL: AudioControl did not select the injected wpctl backend\n");
        return 1;
    }
    if (!waitUntil([&audio] { return audio.available(); }) || audio.volume() != 42 || audio.muted()) {
        std::fprintf(stderr, "FAIL: wpctl volume state was not parsed correctly\n");
        return 1;
    }

    if (!writeFile(statePath, QByteArrayLiteral("Volume: 0.42 [MUTED]\n"))) {
        std::fprintf(stderr, "FAIL: cannot update fake audio state\n");
        return 1;
    }
    audio.refresh();
    if (!waitUntil([&audio] { return audio.muted(); })) {
        std::fprintf(stderr, "FAIL: wpctl mute state was not parsed correctly\n");
        return 1;
    }

    audio.setVolume(55);
    audio.toggleMute();
    if (!waitUntil([&callsPath] {
            const QByteArray calls = readFile(callsPath);
            return calls.contains("set-volume @DEFAULT_AUDIO_SINK@ 55%")
                && calls.contains("set-mute @DEFAULT_AUDIO_SINK@ toggle");
        })) {
        std::fprintf(stderr, "FAIL: volume controls issued unexpected wpctl arguments\n");
        return 1;
    }

    const QString pactlPath = temporaryDirectory.filePath(QStringLiteral("pactl"));
    const QByteArray fakePactl = QByteArrayLiteral(
        "#!/bin/sh\n"
        "case \"$1\" in\n"
        "  get-sink-volume) printf 'Volume: front-left: 32768 / 50%% / -18.00 dB\\n';;\n"
        "  get-sink-mute) printf 'Mute: no\\n';;\n"
        "  *) printf '%s\\n' \"$*\" >> \"$AEROSHELL_TEST_AUDIO_CALLS\";;\n"
        "esac\n");
    QFile::remove(toolPath);
    QFile::remove(callsPath);
    if (!writeFile(pactlPath, fakePactl, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner)
        || !writeFile(callsPath, {})) {
        std::fprintf(stderr, "FAIL: cannot prepare fake pactl\n");
        return 1;
    }

    AudioControl pulseAudio;
    if (pulseAudio.backend() != QLatin1String("pactl")
        || !waitUntil([&pulseAudio] { return pulseAudio.available(); })
        || pulseAudio.volume() != 50 || pulseAudio.muted()) {
        std::fprintf(stderr, "FAIL: pactl volume/mute state was not parsed correctly\n");
        return 1;
    }
    pulseAudio.setVolume(63);
    pulseAudio.toggleMute();
    if (!waitUntil([&callsPath] {
            const QByteArray calls = readFile(callsPath);
            return calls.contains("set-sink-volume @DEFAULT_SINK@ 63%")
                && calls.contains("set-sink-mute @DEFAULT_SINK@ toggle");
        })) {
        std::fprintf(stderr, "FAIL: volume controls issued unexpected pactl arguments\n");
        return 1;
    }

    qputenv("PATH", previousPath);
    std::puts("PASS: wpctl and pactl volume/mute parsing and control arguments");
    return 0;
}
