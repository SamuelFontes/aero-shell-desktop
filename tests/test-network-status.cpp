/* SPDX-License-Identifier: MIT */
#include "../src/shell/networkstatus.h"

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QTemporaryDir>
#include <QTimer>

#include <cstdio>

namespace
{
bool waitFor(const std::function<bool()> &predicate, int timeoutMs)
{
    QElapsedTimer elapsed;
    elapsed.start();
    while (!predicate() && elapsed.elapsed() < timeoutMs) {
        QEventLoop loop;
        QTimer::singleShot(10, &loop, &QEventLoop::quit);
        loop.exec();
    }
    return predicate();
}

bool fail(const char *message)
{
    std::fprintf(stderr, "Network status fixture failed: %s\n", message);
    return false;
}
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir temporary;
    if (!temporary.isValid()) {
        return fail("temporary directory could not be created") ? 0 : 1;
    }
    const QString binPath = temporary.filePath(QStringLiteral("bin"));
    if (!QDir().mkpath(binPath)) {
        return fail("fake nmcli directory could not be created") ? 0 : 1;
    }
    const QString logPath = temporary.filePath(QStringLiteral("nmcli.log"));
    const QString fakeNmcliPath = QDir(binPath).filePath(QStringLiteral("nmcli"));
    QFile fakeNmcli(fakeNmcliPath);
    const QByteArray fakeNmcliScript = R"(#!/bin/sh
printf 'ARGS:%s\n' "$*" >> "$AEROSHELL_TEST_NMCLI_LOG"
case "$*" in
  *"connection up uuid uuid-new passwd-file"*)
    previous=
    for argument in "$@"; do
      if [ "$previous" = passwd-file ]; then password_file="$argument"; fi
      previous="$argument"
    done
    mode=$(stat -c '%a' "$password_file") || exit 9
    if grep -qF '802-11-wireless-security.psk:Correct horse battery 42' "$password_file" \
      || grep -qF '802-11-wireless-security.psk:another valid passphrase' "$password_file"; then
      printf 'SECRET_FILE:%s:present\n' "$mode" >> "$AEROSHELL_TEST_NMCLI_LOG"
      exit 0
    fi
    exit 8
    ;;
  *"device wifi connect Guest private yes"*) exit 0 ;;
  *"connection add type wifi"*) exit 0 ;;
  *"--get-values UUID connection show id AeroShell Wi-Fi"*) printf 'uuid-new\n'; exit 0 ;;
  *"connection delete"*) exit 0 ;;
  *"radio wifi off"*) printf 'disabled\n'; exit 0 ;;
  *"radio wifi"*) printf 'enabled\n'; exit 0 ;;
  *"general status"*) printf 'connected:full\n'; exit 0 ;;
  *"connection show --active"*) printf 'Wired:uuid-active:802-3-ethernet\n'; exit 0 ;;
  *"connection show"*) printf 'Wired:uuid-active:802-3-ethernet\nHome\\:West:uuid-saved:802-11-wireless\n'; exit 0 ;;
  *"wifi list --rescan yes"*) printf '*:Home\\:West:82:WPA2\n:Guest:45:--\n'; exit 0 ;;
esac
exit 0
)";
    if (!fakeNmcli.open(QIODevice::WriteOnly) || fakeNmcli.write(fakeNmcliScript) != fakeNmcliScript.size()) {
        return fail("fake nmcli script could not be written") ? 0 : 1;
    }
    fakeNmcli.close();
    if (!fakeNmcli.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner)) {
        return fail("fake nmcli script could not be made executable") ? 0 : 1;
    }

    qputenv("PATH", (binPath + QLatin1Char(':') + qEnvironmentVariable("PATH")).toLocal8Bit());
    qputenv("AEROSHELL_TEST_NMCLI_LOG", logPath.toLocal8Bit());
    qputenv("USER", "aeroshell-test-user");
    qputenv("XDG_RUNTIME_DIR", temporary.filePath(QStringLiteral("missing-runtime-dir")).toLocal8Bit());

    NetworkStatus network;
    if (!waitFor([&network] { return network.available() && network.connections().size() == 2; }, 3000)) {
        return fail("NetworkManager status and saved profiles did not load") ? 0 : 1;
    }
    if (!network.connected() || network.summary() != QLatin1String("Wired")) {
        return fail("active connection status was parsed incorrectly") ? 0 : 1;
    }
    const QVariantMap savedConnection = network.connections().at(1).toMap();
    if (savedConnection.value(QStringLiteral("name")).toString() != QLatin1String("Home:West")
        || savedConnection.value(QStringLiteral("active")).toBool()) {
        return fail("escaped saved connection fields were parsed incorrectly") ? 0 : 1;
    }

    network.scanWifi();
    if (!waitFor([&network] { return !network.busy() && network.wifiNetworks().size() == 2; }, 3000)) {
        return fail("Wi-Fi scan did not return access points") ? 0 : 1;
    }
    if (network.wifiNetworks().at(0).toMap().value(QStringLiteral("ssid")).toString() != QLatin1String("Home:West")) {
        return fail("escaped SSID was parsed incorrectly") ? 0 : 1;
    }

    network.joinWifi(QStringLiteral("Guest"), {}, {});
    if (!waitFor([&network] { return !network.busy(); }, 3000)) {
        return fail("open network connection did not finish") ? 0 : 1;
    }
    network.joinWifi(QStringLiteral("Home:West"), QStringLiteral("Correct horse battery 42"), QStringLiteral("WPA2"));
    if (!network.busy()) {
        std::fprintf(stderr, "Wi-Fi join did not start: %s\n", network.error().toLocal8Bit().constData());
        return fail("secured Wi-Fi connection was rejected before activation") ? 0 : 1;
    }
    if (!waitFor([&network] { return !network.busy(); }, 4000)) {
        return fail("secured Wi-Fi connection did not finish") ? 0 : 1;
    }
    network.joinWifi(QStringLiteral("New WPA3 network"), QStringLiteral("another valid passphrase"), QStringLiteral("WPA3"));
    if (!network.busy()) {
        std::fprintf(stderr, "WPA3 join did not start: %s\n", network.error().toLocal8Bit().constData());
        return fail("WPA3 connection was rejected before activation") ? 0 : 1;
    }
    if (!waitFor([&network] { return !network.busy(); }, 4000)) {
        return fail("WPA3 Wi-Fi connection did not finish") ? 0 : 1;
    }

    QFile logFile(logPath);
    if (!logFile.open(QIODevice::ReadOnly)) {
        return fail("nmcli command log could not be read") ? 0 : 1;
    }
    const QByteArray log = logFile.readAll();
    if (!log.contains("device wifi connect Guest private yes")) {
        return fail("open network activation did not request a private profile") ? 0 : 1;
    }
    if (log.count("SECRET_FILE:600:present") != 2 || log.contains("Correct horse battery 42")
        || log.contains("another valid passphrase")) {
        return fail("Wi-Fi secret was not kept in a mode-0600 password file") ? 0 : 1;
    }
    if (!log.contains("wifi-sec.key-mgmt wpa-psk") || !log.contains("wifi-sec.key-mgmt sae")
        || !log.contains("wifi-sec.pmf 3")) {
        return fail("WPA2 and WPA3 profiles did not select their matching key management") ? 0 : 1;
    }
    const QList<QByteArray> logLines = log.split('\n');
    QString lastPasswordFile;
    for (const QByteArray &line : logLines) {
        if (line.startsWith("ARGS:") && line.contains("connection up uuid") && line.contains("passwd-file ")) {
            lastPasswordFile = QString::fromLocal8Bit(line.split(' ').last());
        }
    }
    if (lastPasswordFile.isEmpty() || QFile::exists(lastPasswordFile)) {
        return fail("temporary Wi-Fi credential file was not removed after activation") ? 0 : 1;
    }
    return 0;
}
