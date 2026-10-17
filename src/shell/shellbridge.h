/* SPDX-License-Identifier: MIT */
#pragma once

#include <QObject>
#include <QHash>
#include <QPointer>
#include <QWindow>
#include <QSet>

namespace KWayland::Client {
class ConnectionThread;
class PlasmaShell;
class PlasmaShellSurface;
class PlasmaWindowManagement;
class PlasmaWindowModel;
class Registry;
}

class ShellBridge final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *windowModel READ windowModel NOTIFY windowModelChanged)
    Q_PROPERTY(bool plasmaShellAvailable READ plasmaShellAvailable NOTIFY plasmaShellAvailableChanged)
    Q_PROPERTY(bool showingDesktop READ showingDesktop NOTIFY showingDesktopChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)

public:
    explicit ShellBridge(QObject *parent = nullptr);

    QObject *windowModel() const;
    bool plasmaShellAvailable() const;
    bool showingDesktop() const;
    bool ready() const;

    Q_INVOKABLE void registerSurface(QWindow *window, const QString &role);
    Q_INVOKABLE void setSurfacePosition(QWindow *window, const QPoint &position);
    Q_INVOKABLE void toggleShowDesktop();
    Q_INVOKABLE void requestLogout();

Q_SIGNALS:
    void windowModelChanged();
    void plasmaShellAvailableChanged();
    void showingDesktopChanged();
    void readyChanged();
    void integrationError(const QString &message);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    KWayland::Client::ConnectionThread *m_connection = nullptr;
    KWayland::Client::Registry *m_registry = nullptr;
    KWayland::Client::PlasmaShell *m_plasmaShell = nullptr;
    KWayland::Client::PlasmaWindowManagement *m_windowManagement = nullptr;
    KWayland::Client::PlasmaWindowModel *m_windowModel = nullptr;
    bool m_ready = false;
    QSet<QWindow *> m_attachingSurfaces;
    QHash<QWindow *, QString> m_windowRoles;
    QHash<QWindow *, QPoint> m_windowPositions;
    QHash<QWindow *, QPointer<KWayland::Client::PlasmaShellSurface>> m_shellSurfaces;

    void attachSurface(QWindow *window);
    void attachBlur(QWindow *window);
    void attachPendingSurfaces();
};
