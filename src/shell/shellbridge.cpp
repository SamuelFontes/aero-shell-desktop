/* SPDX-License-Identifier: MIT */
#include "shellbridge.h"

#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/plasmawindowmanagement.h>
#include <KWayland/Client/plasmawindowmodel.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/surface.h>

#include <QEvent>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QPlatformSurfaceEvent>

using namespace KWayland::Client;

ShellBridge::ShellBridge(QObject *parent)
    : QObject(parent)
    , m_connection(ConnectionThread::fromApplication(this))
    , m_registry(new Registry(this))
{
    if (!m_connection) {
        QMetaObject::invokeMethod(this, [this] {
            Q_EMIT integrationError(tr("AeroShell requires a Qt Wayland client connection."));
        }, Qt::QueuedConnection);
        return;
    }

    m_registry->create(m_connection);
    connect(m_registry, &Registry::plasmaShellAnnounced, this, [this](quint32 name, quint32 version) {
        if (m_plasmaShell) {
            return;
        }
        m_plasmaShell = m_registry->createPlasmaShell(name, version, this);
        Q_EMIT plasmaShellAvailableChanged();
        attachPendingSurfaces();
    });
    connect(m_registry, &Registry::plasmaWindowManagementAnnounced, this, [this](quint32 name, quint32 version) {
        if (m_windowManagement) {
            return;
        }
        m_windowManagement = m_registry->createPlasmaWindowManagement(name, version, this);
        if (m_windowManagement && m_windowManagement->isValid()) {
            m_windowModel = m_windowManagement->createWindowModel();
            connect(m_windowManagement, &PlasmaWindowManagement::showingDesktopChanged, this, &ShellBridge::showingDesktopChanged);
            Q_EMIT windowModelChanged();
        }
    });
    connect(m_registry, &Registry::interfacesAnnounced, this, [this] {
        if (!plasmaShellAvailable()) {
            Q_EMIT integrationError(tr("KWin does not expose the Plasma Wayland shell protocol required by AeroShell."));
            return;
        }
        if (!m_windowManagement || !m_windowManagement->isValid() || !m_windowModel) {
            Q_EMIT integrationError(tr("KWin does not expose the Plasma window-management protocol required by AeroShell's taskbar."));
            return;
        }
        m_ready = true;
        Q_EMIT readyChanged();
    });
    m_registry->setup();
}

QObject *ShellBridge::windowModel() const
{
    return m_windowModel;
}

bool ShellBridge::plasmaShellAvailable() const
{
    return m_plasmaShell && m_plasmaShell->isValid();
}

bool ShellBridge::showingDesktop() const
{
    return m_windowManagement && m_windowManagement->isShowingDesktop();
}

bool ShellBridge::ready() const
{
    return m_ready;
}

void ShellBridge::toggleShowDesktop()
{
    if (m_windowManagement && m_windowManagement->isValid()) {
        m_windowManagement->setShowingDesktop(!m_windowManagement->isShowingDesktop());
    }
}

void ShellBridge::requestLogout()
{
    QCoreApplication::quit();
}

void ShellBridge::registerSurface(QWindow *window, const QString &role)
{
    if (!window) {
        return;
    }
    m_windowRoles.insert(window, role);
    window->installEventFilter(this);
    connect(window, &QObject::destroyed, this, [this, window] {
        m_windowRoles.remove(window);
        m_windowPositions.remove(window);
        m_shellSurfaces.remove(window);
    });
    if (window->isVisible()) {
        attachSurface(window);
    }
}

void ShellBridge::setSurfacePosition(QWindow *window, const QPoint &position)
{
    if (!window) {
        return;
    }
    m_windowPositions.insert(window, position);
    const auto surface = m_shellSurfaces.value(window);
    if (surface && surface->isValid()) {
        surface->setPosition(position);
    }
}

bool ShellBridge::eventFilter(QObject *watched, QEvent *event)
{
    auto *window = qobject_cast<QWindow *>(watched);
    if (window && m_windowRoles.contains(window) && event->type() == QEvent::PlatformSurface) {
        const auto *surfaceEvent = static_cast<QPlatformSurfaceEvent *>(event);
        if (surfaceEvent->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated) {
            attachSurface(window);
        } else if (surfaceEvent->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
            const auto shellSurface = m_shellSurfaces.take(window);
            if (shellSurface) {
                if (shellSurface->isValid()) {
                    shellSurface->release();
                }
                shellSurface->deleteLater();
            }
        }
    }
    return QObject::eventFilter(watched, event);
}

void ShellBridge::attachPendingSurfaces()
{
    for (auto it = m_windowRoles.cbegin(); it != m_windowRoles.cend(); ++it) {
        attachSurface(it.key());
    }
}

void ShellBridge::attachSurface(QWindow *window)
{
    if (!window || !m_plasmaShell || !m_plasmaShell->isValid() || m_shellSurfaces.value(window)) {
        return;
    }
    Surface *surface = Surface::fromWindow(window);
    if (!surface || !surface->isValid()) {
        return;
    }

    PlasmaShellSurface *shellSurface = m_plasmaShell->createSurface(surface, window);
    if (!shellSurface || !shellSurface->isValid()) {
        Q_EMIT integrationError(tr("The compositor rejected an AeroShell surface role."));
        return;
    }

    const QString role = m_windowRoles.value(window);
    if (role == QLatin1String("desktop")) {
        shellSurface->setRole(PlasmaShellSurface::Role::Desktop);
    } else if (role == QLatin1String("panel")) {
        shellSurface->setRole(PlasmaShellSurface::Role::Panel);
        shellSurface->setPanelBehavior(PlasmaShellSurface::PanelBehavior::AlwaysVisible);
    } else if (role == QLatin1String("popup")) {
        shellSurface->setRole(PlasmaShellSurface::Role::AppletPopup);
    } else {
        shellSurface->setRole(PlasmaShellSurface::Role::Normal);
    }
    shellSurface->setSkipTaskbar(true);
    shellSurface->setSkipSwitcher(true);
    if (m_windowPositions.contains(window)) {
        shellSurface->setPosition(m_windowPositions.value(window));
    }
    m_shellSurfaces.insert(window, shellSurface);
}
