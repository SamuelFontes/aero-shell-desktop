/* SPDX-License-Identifier: MIT */
#include "batterystatus.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTimer>

namespace
{
bool readText(const QString &path, QString *value)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    *value = QString::fromLocal8Bit(file.readAll()).trimmed();
    return true;
}

bool readNumber(const QString &path, double *value)
{
    QString text;
    if (!readText(path, &text)) {
        return false;
    }
    bool ok = false;
    const double parsed = text.toDouble(&ok);
    if (!ok || parsed < 0) {
        return false;
    }
    *value = parsed;
    return true;
}

struct BatteryReading {
    double percentage = 0;
    double weight = 1;
    QString status;
};

bool readBattery(const QFileInfo &entry, BatteryReading *reading)
{
    QString type;
    if (!readText(entry.filePath() + QStringLiteral("/type"), &type)
        || type.compare(QStringLiteral("Battery"), Qt::CaseInsensitive) != 0) {
        return false;
    }

    double presentValue = 1;
    if (readNumber(entry.filePath() + QStringLiteral("/present"), &presentValue) && presentValue == 0) {
        return false;
    }

    double percentage = 0;
    double full = 0;
    double now = 0;
    double capacity = 0;
    if (readNumber(entry.filePath() + QStringLiteral("/capacity"), &capacity)) {
        percentage = capacity;
        readNumber(entry.filePath() + QStringLiteral("/energy_full"), &full);
        if (full <= 0) {
            readNumber(entry.filePath() + QStringLiteral("/charge_full"), &full);
        }
    } else if (readNumber(entry.filePath() + QStringLiteral("/energy_now"), &now)
               && readNumber(entry.filePath() + QStringLiteral("/energy_full"), &full) && full > 0) {
        percentage = now * 100.0 / full;
    } else if (readNumber(entry.filePath() + QStringLiteral("/charge_now"), &now)
               && readNumber(entry.filePath() + QStringLiteral("/charge_full"), &full) && full > 0) {
        percentage = now * 100.0 / full;
    } else {
        return false;
    }

    if (full <= 0) {
        full = 1;
    }
    reading->percentage = qBound(0.0, percentage, 100.0);
    reading->weight = full;
    readText(entry.filePath() + QStringLiteral("/status"), &reading->status);
    return true;
}
}

BatteryStatus::BatteryStatus(QObject *parent)
    : BatteryStatus(QStringLiteral("/sys/class/power_supply"), parent)
{
}

BatteryStatus::BatteryStatus(const QString &powerSupplyRoot, QObject *parent)
    : QObject(parent)
    , m_powerSupplyRoot(powerSupplyRoot)
{
    m_refreshTimer.setInterval(30000);
    connect(&m_refreshTimer, &QTimer::timeout, this, &BatteryStatus::refresh);
    m_refreshTimer.start();
    QTimer::singleShot(0, this, &BatteryStatus::refresh);
}

bool BatteryStatus::present() const
{
    return m_present;
}

int BatteryStatus::percentage() const
{
    return m_percentage;
}

QString BatteryStatus::summary() const
{
    return m_summary;
}

void BatteryStatus::refresh()
{
    const QFileInfo rootInfo(m_powerSupplyRoot);
    if (!rootInfo.isDir()) {
        if (m_present) {
            m_present = false;
            m_percentage = 0;
            m_summary.clear();
            Q_EMIT stateChanged();
        }
        return;
    }

    const QFileInfoList entries = QDir(m_powerSupplyRoot).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    double weightedPercentage = 0;
    double totalWeight = 0;
    bool anyCharging = false;
    bool anyDischarging = false;
    bool allFull = !entries.isEmpty();
    int batteryCount = 0;

    for (const QFileInfo &entry : entries) {
        BatteryReading reading;
        if (!readBattery(entry, &reading)) {
            continue;
        }
        ++batteryCount;
        weightedPercentage += reading.percentage * reading.weight;
        totalWeight += reading.weight;
        anyCharging = anyCharging || reading.status.compare(QStringLiteral("Charging"), Qt::CaseInsensitive) == 0;
        anyDischarging = anyDischarging || reading.status.compare(QStringLiteral("Discharging"), Qt::CaseInsensitive) == 0;
        allFull = allFull && (reading.status.compare(QStringLiteral("Full"), Qt::CaseInsensitive) == 0
                              || reading.percentage >= 100.0);
    }

    const bool newPresent = batteryCount > 0 && totalWeight > 0;
    const int newPercentage = newPresent ? qBound(0, qRound(weightedPercentage / totalWeight), 100) : 0;
    QString newSummary;
    if (newPresent) {
        if (anyCharging) {
            newSummary = tr("Charging");
        } else if (anyDischarging) {
            newSummary = tr("On battery");
        } else if (allFull) {
            newSummary = tr("Fully charged");
        } else {
            newSummary = tr("Connected to power");
        }
    }

    if (m_present != newPresent || m_percentage != newPercentage || m_summary != newSummary) {
        m_present = newPresent;
        m_percentage = newPercentage;
        m_summary = newSummary;
        Q_EMIT stateChanged();
    }
}
