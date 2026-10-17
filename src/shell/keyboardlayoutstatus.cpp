/* SPDX-License-Identifier: MIT */
#include "keyboardlayoutstatus.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusServiceWatcher>

namespace
{
constexpr auto keyboardService = "org.kde.keyboard";
constexpr auto keyboardPath = "/Layouts";
constexpr auto keyboardInterface = "org.kde.KeyboardLayouts";

QList<KeyboardLayoutStatus::Layout> parseLayouts(const QVariant &value)
{
    QList<KeyboardLayoutStatus::Layout> layouts;
    if (!value.canConvert<QDBusArgument>()) {
        return layouts;
    }

    const QDBusArgument argument = value.value<QDBusArgument>();
    // The const overloads read; the non-const overloads attempt to marshal and
    // never advance the incoming iterator, hanging startup on a live KWin bus.
    if (argument.currentSignature() != QLatin1String("a(sss)")) return layouts;
    argument.beginArray();
    while (!argument.atEnd()) {
        KeyboardLayoutStatus::Layout layout;
        argument.beginStructure();
        argument >> layout.shortName >> layout.displayName >> layout.longName;
        argument.endStructure();
        if (!layout.shortName.isEmpty()) {
            layouts.push_back(std::move(layout));
        }
    }
    argument.endArray();
    return layouts;
}
}

KeyboardLayoutStatus::KeyboardLayoutStatus(QObject *parent)
    : QObject(parent)
{
    m_serviceWatcher = new QDBusServiceWatcher(QString::fromLatin1(keyboardService),
                                               QDBusConnection::sessionBus(),
                                               QDBusServiceWatcher::WatchForOwnerChange,
                                               this);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceOwnerChanged, this,
            [this](const QString &, const QString &, const QString &newOwner) {
                if (newOwner.isEmpty()) {
                    m_refreshTimer.stop();
                }
                refresh();
            });
    QDBusConnection::sessionBus().connect(QString(), QString::fromLatin1(keyboardPath),
                                          QString::fromLatin1(keyboardInterface),
                                          QStringLiteral("layoutChanged"), this,
                                          SLOT(onLayoutChanged(uint)));
    QDBusConnection::sessionBus().connect(QString(), QString::fromLatin1(keyboardPath),
                                          QString::fromLatin1(keyboardInterface),
                                          QStringLiteral("layoutListChanged"), this,
                                          SLOT(refresh()));
    m_refreshTimer.setInterval(2000);
    connect(&m_refreshTimer, &QTimer::timeout, this, &KeyboardLayoutStatus::refresh);
    refresh();
}

bool KeyboardLayoutStatus::available() const
{
    return m_available;
}

QString KeyboardLayoutStatus::currentLabel() const
{
    return m_currentIndex < static_cast<uint>(m_labels.size()) ? m_labels.at(static_cast<qsizetype>(m_currentIndex))
                                                               : QStringLiteral("--");
}

QString KeyboardLayoutStatus::currentName() const
{
    return m_currentIndex < static_cast<uint>(m_names.size()) ? m_names.at(static_cast<qsizetype>(m_currentIndex))
                                                              : QStringLiteral("Keyboard layout unavailable");
}

QStringList KeyboardLayoutStatus::layouts() const
{
    return m_labels;
}

void KeyboardLayoutStatus::refresh()
{
    const QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.interface()) {
        if (m_available || !m_labels.isEmpty()) {
            m_available = false;
            m_labels.clear();
            m_names.clear();
            m_currentIndex = 0;
            Q_EMIT changed();
        }
        m_refreshTimer.stop();
        return;
    }
    const QDBusReply<bool> serviceRegistered = bus.interface()->isServiceRegistered(QString::fromLatin1(keyboardService));
    if (!serviceRegistered.isValid() || !serviceRegistered.value()) {
        if (m_available || !m_labels.isEmpty()) {
            m_available = false;
            m_labels.clear();
            m_names.clear();
            m_currentIndex = 0;
            Q_EMIT changed();
        }
        m_refreshTimer.stop();
        return;
    }

    QDBusInterface keyboard(QString::fromLatin1(keyboardService), QString::fromLatin1(keyboardPath),
                            QString::fromLatin1(keyboardInterface), bus);
    const QDBusMessage listReply = keyboard.call(QStringLiteral("getLayoutsList"));
    const bool listValid = listReply.type() == QDBusMessage::ReplyMessage && !listReply.arguments().isEmpty();
    const QList<Layout> parsed = listValid ? parseLayouts(listReply.arguments().constFirst()) : QList<Layout>{};
    const QDBusReply<uint> currentReply = keyboard.call(QStringLiteral("getLayout"));
    const bool available = listValid && currentReply.isValid() && !parsed.isEmpty();

    QStringList labels;
    QStringList names;
    for (const Layout &layout : parsed) {
        labels.push_back(layout.displayName.isEmpty() ? layout.shortName.toUpper() : layout.displayName);
        names.push_back(layout.longName.isEmpty() ? layout.shortName : layout.longName);
    }
    const uint current = available ? currentReply.value() : 0;
    if (m_available == available && m_labels == labels && m_names == names && m_currentIndex == current) {
        return;
    }
    m_available = available;
    m_labels = std::move(labels);
    m_names = std::move(names);
    m_currentIndex = current;
    if (m_available) {
        m_refreshTimer.start();
    } else {
        m_refreshTimer.stop();
    }
    Q_EMIT changed();
}

void KeyboardLayoutStatus::onLayoutChanged(uint)
{
    refresh();
}

void KeyboardLayoutStatus::nextLayout()
{
    if (!m_available) {
        return;
    }
    QDBusInterface keyboard(QString::fromLatin1(keyboardService), QString::fromLatin1(keyboardPath),
                            QString::fromLatin1(keyboardInterface), QDBusConnection::sessionBus());
    keyboard.asyncCall(QStringLiteral("switchToNextLayout"));
}

void KeyboardLayoutStatus::setLayout(int index)
{
    if (!m_available || index < 0 || index >= m_labels.size()) {
        return;
    }
    QDBusInterface keyboard(QString::fromLatin1(keyboardService), QString::fromLatin1(keyboardPath),
                            QString::fromLatin1(keyboardInterface), QDBusConnection::sessionBus());
    keyboard.asyncCall(QStringLiteral("setLayout"), static_cast<uint>(index));
}
