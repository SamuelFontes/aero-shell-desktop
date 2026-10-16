# Decision 0003: service and session lifecycle

**Status:** Partial. The KWin-to-shell process chain runs inside a private D-Bus session, and the shell has a KScreenLocker-backed lock action. Tray-menu, portal, PolicyKit, forced-termination, and shared user-manager behavior still need live validation.

**Question:** How are the compositor, shell, portals, policy agent, screen locker, and other required services started and stopped without leaking environment or affecting KDE?

**Constraints:** Never globally enable AeroShell services, kill processes by broad name, restart shared user targets, or rely only on an exit trap to undo shared activation state after forced termination. Do not modify SDDM or other display-manager settings.

**Evidence to collect:** Pinned service/activation definitions, D-Bus names, systemd user-manager interactions, process ownership, normal logout and forced-termination results, same-user concurrent-session behavior.

**Selected approach:** The unique session launcher starts `dbus-run-session -- kwin_wayland --xwayland --desktopfile aeroshell --exit-with-session <shell-wrapper>`. KWin receives private XDG roots and its own session bus. Its child wrapper restores the user's original XDG roots before launching AeroShell and KIO applications. This separates KWin, notifications, StatusNotifier, and D-Bus activation names from a concurrent KDE session. The system bus and user files remain shared. AeroShell does not import environment into systemd's persistent user manager or enable global user units.

**Evidence:** `tests/test-session-launch.sh` checks the private bus wrapper, compositor arguments, XDG boundary, and session identity. The KWin 6.7.4 binary links KScreenLocker and exposes `--lockscreen`; the Start menu calls its `org.freedesktop.ScreenSaver` lock method and confirms the active state. Fake-session coverage passes. Real compositor startup, locking, portals, and shutdown have not been tested because the sandbox denies private D-Bus and Wayland socket creation.

**Tradeoffs:** The private bus prevents name collisions with KDE but does not expose user-manager-only session services; D-Bus-activated services and desktop portals must work on the private bus. Test portal capture, app activation, notifications, tray menus, and same-user concurrent KDE sessions on a disposable graphical login. Starting Plasma's session stack would violate the independent-DE requirement.
