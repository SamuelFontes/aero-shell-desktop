/* SPDX-License-Identifier: MIT */
#include "networkstatus.h"

#include <QDir>
#include <QFileDevice>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QUuid>
#include <QSet>
#include <QStandardPaths>

#include <functional>
#include <cctype>
#include <memory>
#include <utility>

NetworkStatus::NetworkStatus(QObject *parent)
    : QObject(parent)
{
    m_refreshTimer.setInterval(5000);
    connect(&m_refreshTimer, &QTimer::timeout, this, &NetworkStatus::refresh);
    m_refreshTimer.start();
    refresh();
}

bool NetworkStatus::available() const
{
    return m_available;
}

bool NetworkStatus::connected() const
{
    return m_connected;
}

bool NetworkStatus::wifiEnabled() const
{
    return m_wifiEnabled;
}

bool NetworkStatus::busy() const
{
    return m_busy;
}

QString NetworkStatus::summary() const
{
    return m_summary;
}

QString NetworkStatus::error() const
{
    return m_error;
}

QVariantList NetworkStatus::connections() const
{
    return m_connections;
}

QVariantList NetworkStatus::wifiNetworks() const
{
    return m_wifiNetworks;
}

QStringList NetworkStatus::splitTerse(const QString &line)
{
    QStringList fields;
    QString field;
    bool escaped = false;
    for (const QChar character : line) {
        if (escaped) {
            field += character;
            escaped = false;
        } else if (character == QLatin1Char('\\')) {
            escaped = true;
        } else if (character == QLatin1Char(':')) {
            fields.push_back(field);
            field.clear();
        } else {
            field += character;
        }
    }
    if (escaped) {
        field += QLatin1Char('\\');
    }
    fields.push_back(field);
    return fields;
}

QVariantList NetworkStatus::parseConnections(const QString &output, const QString &activeOutput)
{
    QSet<QString> activeUuids;
    for (const QString &line : activeOutput.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        const QStringList fields = splitTerse(line);
        if (fields.size() >= 2) {
            activeUuids.insert(fields.at(1));
        }
    }

    QVariantList connections;
    for (const QString &line : output.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        const QStringList fields = splitTerse(line);
        if (fields.size() < 3) {
            continue;
        }
        const bool active = activeUuids.contains(fields.at(1));
        connections.push_back(QVariantMap{
            {QStringLiteral("name"), fields.at(0)},
            {QStringLiteral("uuid"), fields.at(1)},
            {QStringLiteral("type"), fields.at(2)},
            {QStringLiteral("active"), active},
        });
    }
    return connections;
}

QVariantList NetworkStatus::parseWifiNetworks(const QString &output)
{
    QVariantList networks;
    QSet<QString> seen;
    for (const QString &line : output.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        const QStringList fields = splitTerse(line);
        if (fields.size() < 4 || fields.at(1).isEmpty() || seen.contains(fields.at(1))) {
            continue;
        }
        seen.insert(fields.at(1));
        bool ok = false;
        const int signal = fields.at(2).toInt(&ok);
        networks.push_back(QVariantMap{
            {QStringLiteral("inUse"), fields.at(0) == QLatin1String("*")},
            {QStringLiteral("ssid"), fields.at(1)},
            {QStringLiteral("signal"), ok ? signal : 0},
            {QStringLiteral("security"), fields.at(3) == QLatin1String("--") ? QString() : fields.at(3)},
        });
    }
    return networks;
}

void NetworkStatus::startCommand(const QStringList &arguments, CommandCallback callback, int timeoutMs)
{
    auto *process = new QProcess(this);
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    process->setProcessEnvironment(environment);
    auto completed = std::make_shared<bool>(false);
    const auto complete = [process, completed, callback](int exitCode, const QString &standardOutput,
                                                         const QString &standardError) {
        if (*completed) {
            return;
        }
        *completed = true;
        callback(exitCode, standardOutput, standardError);
        process->deleteLater();
    };
    connect(process, &QProcess::finished, this,
            [process, complete](int exitCode, QProcess::ExitStatus status) {
                complete(status == QProcess::NormalExit ? exitCode : -1,
                         QString::fromLocal8Bit(process->readAllStandardOutput()),
                         QString::fromLocal8Bit(process->readAllStandardError()));
            });
    connect(process, &QProcess::errorOccurred, this,
            [complete](QProcess::ProcessError error) {
                if (error == QProcess::FailedToStart) {
                    complete(-1, {}, QStringLiteral("NetworkManager command could not be started."));
                }
            });
    process->start(QStringLiteral("nmcli"), arguments);
    QTimer::singleShot(timeoutMs, process, [process, complete] {
        if (process->state() != QProcess::NotRunning) {
            process->kill();
            complete(-1, {}, QStringLiteral("NetworkManager command timed out."));
        }
    });
}

void NetworkStatus::refresh()
{
    if (m_busy) {
        m_refreshPending = true;
        return;
    }
    if (m_pendingQueries > 0) {
        m_refreshPending = true;
        return;
    }
    if (QStandardPaths::findExecutable(QStringLiteral("nmcli")).isEmpty()) {
        if (m_available || m_connected || !m_summary.contains(QStringLiteral("unavailable"), Qt::CaseInsensitive)) {
            m_available = false;
            m_connected = false;
            m_wifiEnabled = false;
            m_connections.clear();
            m_summary = QStringLiteral("NetworkManager is not installed");
            Q_EMIT changed();
        }
        return;
    }

    m_pendingQueries = 4;
    m_allConnectionOutput.clear();
    m_activeConnectionOutput.clear();
    m_activeConnectionName.clear();
    const auto done = [this] {
        --m_pendingQueries;
        if (m_pendingQueries == 0) {
            finishRefresh();
        }
    };
    startCommand({QStringLiteral("--terse"), QStringLiteral("--fields"), QStringLiteral("STATE,CONNECTIVITY"),
                  QStringLiteral("general"), QStringLiteral("status")},
                 [this, done](int exitCode, const QString &output, const QString &) {
                     const bool available = exitCode == 0;
                     if (!available) {
                         m_available = false;
                         m_connected = false;
                         m_summary = QStringLiteral("NetworkManager is unavailable");
                     } else {
                         const QStringList fields = splitTerse(output.trimmed());
                         const QString state = fields.value(0).toLower();
                         m_available = true;
                         m_connected = state.startsWith(QStringLiteral("connected"));
                         m_summary = m_connected ? QStringLiteral("Connected")
                                                 : state.startsWith(QStringLiteral("connecting")) ? QStringLiteral("Connecting…")
                                                                                               : QStringLiteral("Disconnected");
                     }
                     done();
                 });
    startCommand({QStringLiteral("--terse"), QStringLiteral("--escape"), QStringLiteral("yes"),
                  QStringLiteral("--fields"), QStringLiteral("NAME,UUID,TYPE"),
                  QStringLiteral("connection"), QStringLiteral("show")},
                 [this, done](int exitCode, const QString &output, const QString &) {
                     if (exitCode == 0) {
                         m_allConnectionOutput = output;
                         m_connections = parseConnections(m_allConnectionOutput, m_activeConnectionOutput);
                     }
                     done();
                 });
    startCommand({QStringLiteral("--terse"), QStringLiteral("--escape"), QStringLiteral("yes"),
                  QStringLiteral("--fields"), QStringLiteral("NAME,UUID,TYPE"),
                  QStringLiteral("connection"), QStringLiteral("show"), QStringLiteral("--active")},
                 [this, done](int exitCode, const QString &output, const QString &) {
                     if (exitCode == 0) {
                         m_activeConnectionOutput = output;
                         m_connections = parseConnections(m_allConnectionOutput, m_activeConnectionOutput);
                         const QStringList activeFields = splitTerse(output.trimmed().section(QLatin1Char('\n'), 0, 0));
                         m_activeConnectionName = activeFields.value(0);
                     } else {
                         m_activeConnectionOutput.clear();
                         m_activeConnectionName.clear();
                     }
                     done();
                 });
    startCommand({QStringLiteral("--terse"), QStringLiteral("radio"), QStringLiteral("wifi")},
                 [this, done](int exitCode, const QString &output, const QString &) {
                     if (exitCode == 0) {
                         m_wifiEnabled = output.trimmed().compare(QStringLiteral("enabled"), Qt::CaseInsensitive) == 0;
                     }
                     done();
                 });
}

void NetworkStatus::finishRefresh()
{
    if (!m_available) {
        m_connected = false;
        m_wifiEnabled = false;
        m_connections.clear();
        m_activeConnectionName.clear();
    } else if (m_connected && !m_activeConnectionName.isEmpty()) {
        m_summary = m_activeConnectionName;
    }
    Q_EMIT changed();
    if (m_refreshPending) {
        m_refreshPending = false;
        QTimer::singleShot(0, this, &NetworkStatus::refresh);
    }
}

void NetworkStatus::runAction(const QStringList &arguments)
{
    if (!m_available || m_busy) {
        return;
    }
    m_busy = true;
    m_error.clear();
    Q_EMIT changed();
    startCommand(arguments, [this](int exitCode, const QString &, const QString &errorOutput) {
        m_busy = false;
        if (exitCode != 0) {
            m_error = errorOutput.trimmed().isEmpty()
                ? QStringLiteral("NetworkManager could not complete that request.")
                : errorOutput.trimmed();
        }
        Q_EMIT changed();
        refresh();
    });
}

void NetworkStatus::setWifiEnabled(bool enabled)
{
    runAction({QStringLiteral("radio"), QStringLiteral("wifi"), enabled ? QStringLiteral("on") : QStringLiteral("off")});
}

void NetworkStatus::scanWifi()
{
    if (!m_available || m_busy) {
        return;
    }
    m_busy = true;
    m_error.clear();
    m_wifiNetworks.clear();
    Q_EMIT changed();
    startCommand({QStringLiteral("--terse"), QStringLiteral("--escape"), QStringLiteral("yes"),
                  QStringLiteral("--fields"), QStringLiteral("IN-USE,SSID,SIGNAL,SECURITY"),
                  QStringLiteral("device"), QStringLiteral("wifi"), QStringLiteral("list"),
                  QStringLiteral("--rescan"), QStringLiteral("yes")},
                 [this](int exitCode, const QString &output, const QString &errorOutput) {
                     m_busy = false;
                     if (exitCode == 0) {
                         m_wifiNetworks = parseWifiNetworks(output);
                     } else {
                         m_error = errorOutput.trimmed().isEmpty()
                             ? QStringLiteral("Could not scan for Wi-Fi networks.") : errorOutput.trimmed();
                     }
                     Q_EMIT changed();
                     refresh();
                 },
                 15000);
}

void NetworkStatus::joinWifi(const QString &ssid, const QString &password, const QString &security)
{
    if (!m_available || m_busy || ssid.isEmpty() || ssid.toUtf8().size() > 32
        || password.contains(QLatin1Char('\n')) || password.contains(QLatin1Char('\r'))
        || password.contains(QChar::Null)) {
        return;
    }
    const QString securityLower = security.toLower();
    if (securityLower.contains(QStringLiteral("wep")) || securityLower.contains(QStringLiteral("802.1x"))
        || securityLower.contains(QStringLiteral("eap"))) {
        m_error = QStringLiteral("This network needs WEP or enterprise authentication, which is not supported by this widget yet.");
        Q_EMIT changed();
        return;
    }
    const QByteArray secret = password.toUtf8();
    const bool validHexKey = secret.size() == 64
        && std::all_of(secret.cbegin(), secret.cend(), [](unsigned char character) {
               return std::isxdigit(character) != 0;
           });
    if (!secret.isEmpty() && (secret.size() < 8 || (secret.size() > 63 && !validHexKey))) {
        m_error = QStringLiteral("Wi-Fi passphrases must have 8–63 characters or be a 64-digit hexadecimal key.");
        Q_EMIT changed();
        return;
    }

    QStringList arguments{QStringLiteral("--wait"), QStringLiteral("30")};
    if (!secret.isEmpty()) {
        QString runtimeDirectory = qEnvironmentVariable("XDG_RUNTIME_DIR");
        if (runtimeDirectory.isEmpty() || !QFileInfo(runtimeDirectory).isDir()
            || !QFileInfo(runtimeDirectory).isWritable()) {
            runtimeDirectory = QDir::tempPath();
        }
        const QString fileTemplate = QDir(runtimeDirectory).filePath(QStringLiteral("aeroshell-nm-secret-XXXXXX"));
        m_passwordFile = std::make_unique<QTemporaryFile>(fileTemplate);
        if (!m_passwordFile->open()) {
            m_passwordFile.reset();
            m_error = QStringLiteral("Could not create a temporary Wi-Fi credential file.");
            Q_EMIT changed();
            return;
        }
        if (!m_passwordFile->setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner)) {
            m_passwordFile.reset();
            m_error = QStringLiteral("Could not protect the temporary Wi-Fi credential file.");
            Q_EMIT changed();
            return;
        }
        const QByteArray entry = QByteArrayLiteral("802-11-wireless-security.psk:") + secret + '\n';
        if (m_passwordFile->write(entry) != entry.size() || !m_passwordFile->flush()) {
            m_passwordFile.reset();
            m_error = QStringLiteral("Could not write the temporary Wi-Fi credential file.");
            Q_EMIT changed();
            return;
        }
    }

    m_busy = true;
    m_error.clear();
    Q_EMIT changed();
    const auto finish = [this](const QString &errorMessage = QString()) {
        m_passwordFile.reset();
        m_busy = false;
        m_error = errorMessage;
        Q_EMIT changed();
        refresh();
    };

    if (secret.isEmpty()) {
        arguments << QStringLiteral("device") << QStringLiteral("wifi") << QStringLiteral("connect") << ssid
                  << QStringLiteral("private") << QStringLiteral("yes");
        startCommand(arguments, [finish](int exitCode, const QString &, const QString &errorOutput) {
            finish(exitCode == 0 ? QString() : errorOutput.trimmed().isEmpty()
                        ? QStringLiteral("Could not connect to that Wi-Fi network.") : errorOutput.trimmed());
        }, 35000);
        return;
    }

    const QString user = qEnvironmentVariable("USER");
    if (user.isEmpty()) {
        finish(QStringLiteral("Cannot create a private Wi-Fi profile without the login user name."));
        return;
    }
    const QString profileName = QStringLiteral("AeroShell Wi-Fi %1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    const bool saeOnly = securityLower.contains(QStringLiteral("wpa3"))
        && !securityLower.contains(QStringLiteral("wpa2"));
    auto removeProfile = [this, finish, profileName](const QString &uuid, const QString &errorMessage) {
        QStringList removeArguments{QStringLiteral("connection"), QStringLiteral("delete")};
        if (uuid.isEmpty()) {
            removeArguments << QStringLiteral("id") << profileName;
        } else {
            removeArguments << QStringLiteral("uuid") << uuid;
        }
        startCommand(removeArguments, [finish, errorMessage](int, const QString &, const QString &) {
            finish(errorMessage);
        });
    };
    QStringList createArguments{QStringLiteral("--wait"), QStringLiteral("15"), QStringLiteral("connection"),
                                QStringLiteral("add"), QStringLiteral("type"), QStringLiteral("wifi"),
                                QStringLiteral("ifname"), QStringLiteral("*"), QStringLiteral("con-name"), profileName,
                                QStringLiteral("ssid"), ssid, QStringLiteral("wifi-sec.key-mgmt"),
                                saeOnly ? QStringLiteral("sae") : QStringLiteral("wpa-psk"),
                                QStringLiteral("connection.autoconnect"), QStringLiteral("no"),
                                QStringLiteral("connection.permissions"), QStringLiteral("user:") + user};
    if (saeOnly) {
        createArguments << QStringLiteral("wifi-sec.pmf") << QStringLiteral("3");
    }
    startCommand(createArguments,
                 [this, profileName, removeProfile, finish](int addExitCode, const QString &, const QString &addError) {
                     if (addExitCode != 0) {
                         finish(addError.trimmed().isEmpty()
                                    ? QStringLiteral("Could not create a private Wi-Fi connection profile.")
                                    : addError.trimmed());
                         return;
                     }
                     startCommand({QStringLiteral("--get-values"), QStringLiteral("UUID"),
                                   QStringLiteral("connection"), QStringLiteral("show"),
                                   QStringLiteral("id"), profileName},
                                  [this, profileName, removeProfile, finish](int queryExitCode,
                                                                             const QString &uuidOutput,
                                                                             const QString &queryError) {
                                      const QString uuid = uuidOutput.trimmed();
                                      if (queryExitCode != 0 || uuid.isEmpty()) {
                                          removeProfile({}, queryError.trimmed().isEmpty()
                                                             ? QStringLiteral("Could not find the new Wi-Fi connection profile.")
                                                             : queryError.trimmed());
                                          return;
                                      }
                                      const QString passwordFile = m_passwordFile->fileName();
                                      startCommand({QStringLiteral("--wait"), QStringLiteral("30"),
                                                    QStringLiteral("connection"), QStringLiteral("up"),
                                                    QStringLiteral("uuid"), uuid, QStringLiteral("passwd-file"), passwordFile},
                                                   [removeProfile, finish, uuid](int upExitCode, const QString &,
                                                                                const QString &upError) {
                                                       if (upExitCode == 0) {
                                                           finish();
                                                       } else {
                                                           removeProfile(uuid, upError.trimmed().isEmpty()
                                                                                  ? QStringLiteral("Could not connect to that Wi-Fi network.")
                                                                                  : upError.trimmed());
                                                       }
                                                   },
                                                   35000);
                                  });
                 },
                 20000);
}

void NetworkStatus::toggleConnection(const QString &uuid, bool active)
{
    if (uuid.isEmpty()) {
        return;
    }
    runAction(active ? QStringList{QStringLiteral("connection"), QStringLiteral("down"), QStringLiteral("uuid"), uuid}
                     : QStringList{QStringLiteral("--wait"), QStringLiteral("15"), QStringLiteral("connection"),
                                   QStringLiteral("up"), QStringLiteral("uuid"), uuid});
}
