/* SPDX-License-Identifier: MIT */
#include "../src/shell/batterystatus.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <cstdio>

namespace
{
bool writeFile(const QString &path, const QByteArray &contents)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    return file.write(contents) == contents.size();
}
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir temporaryDirectory;
    if (!temporaryDirectory.isValid()) {
        std::fprintf(stderr, "FAIL: cannot create temporary power-supply tree\n");
        return 1;
    }

    const QString battery = temporaryDirectory.filePath(QStringLiteral("BAT0"));
    if (!QDir().mkpath(battery)
        || !writeFile(battery + QStringLiteral("/type"), QByteArrayLiteral("Battery\n"))
        || !writeFile(battery + QStringLiteral("/present"), QByteArrayLiteral("1\n"))
        || !writeFile(battery + QStringLiteral("/capacity"), QByteArrayLiteral("73\n"))
        || !writeFile(battery + QStringLiteral("/energy_full"), QByteArrayLiteral("100000\n"))
        || !writeFile(battery + QStringLiteral("/status"), QByteArrayLiteral("Discharging\n"))) {
        std::fprintf(stderr, "FAIL: cannot prepare fake power-supply tree\n");
        return 1;
    }

    BatteryStatus status(temporaryDirectory.path(), nullptr);
    status.refresh();
    if (!status.present() || status.percentage() != 73 || status.summary() != QLatin1String("On battery")) {
        std::fprintf(stderr, "FAIL: battery percentage/status was not read correctly\n");
        return 1;
    }

    if (!writeFile(battery + QStringLiteral("/capacity"), QByteArrayLiteral("100\n"))
        || !writeFile(battery + QStringLiteral("/status"), QByteArrayLiteral("Full\n"))) {
        std::fprintf(stderr, "FAIL: cannot update fake battery status\n");
        return 1;
    }
    status.refresh();
    if (status.percentage() != 100 || status.summary() != QLatin1String("Fully charged")) {
        std::fprintf(stderr, "FAIL: full battery state was not refreshed\n");
        return 1;
    }

    if (!QDir(battery).removeRecursively()) {
        std::fprintf(stderr, "FAIL: cannot remove fake battery\n");
        return 1;
    }
    status.refresh();
    if (status.present() || status.percentage() != 0) {
        std::fprintf(stderr, "FAIL: absent battery did not disable the widget\n");
        return 1;
    }

    std::puts("PASS: battery detection, charging state, and no-battery degradation");
    return 0;
}
