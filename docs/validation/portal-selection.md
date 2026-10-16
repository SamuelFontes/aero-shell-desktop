# AeroShell portal selection

**Status:** Static selector and install scope implemented; runtime activation is unverified.

## Selection design

- `session/aeroshell-session-wayland` exports `XDG_CURRENT_DESKTOP=AeroShell` before starting KWin. The shell child inherits that value.
- `packaging/aeroshell-portals.conf` installs as `/usr/share/xdg-desktop-portal/aeroshell-portals.conf`, the desktop-specific selector for that identity.
- Generic requests select GTK. Screen cast, screenshots, remote desktop, input capture, and global shortcuts prefer the KDE portal backend because the selected KWin compositor provides the relevant Wayland integration. GTK is a fallback only for screen cast and screenshots.
- The selector does not install or edit `kde-portals.conf`, and it does not select the Plasma notification backend or KWallet. It never writes to a user's KDE config.
- This is a package-owned file under `/usr/share`, selected by the AeroShell desktop name. The staged-file policy allows this exact file only. No package installation was performed on the development host.

## Host audit

On the current Manjaro host, `/usr/share/xdg-desktop-portal/kde-portals.conf` selects the KDE backend by default, routes notifications to `plasmanotify`, and routes secrets to `kwallet`. The GTK selector defaults to GTK. The AeroShell selector prevents AeroShell from relying on Plasma's desktop-specific file, provided the portal frontend is started with AeroShell's desktop identity.

The portal D-Bus service is activated through `org.freedesktop.portal.Desktop.service`, which delegates to `xdg-desktop-portal.service`. Session or systemd user-manager activation can supply an environment different from the process that first requested the portal, and a broker may already be running. The project currently avoids importing AeroShell variables into the shared user manager because that could leak into Plasma. Therefore this static configuration does **not** prove which backend will actually be selected.

## Required runtime proof

On a disposable Arch/Manjaro machine, in separate AeroShell and Plasma logins:

1. Verify the portal frontend sees `XDG_CURRENT_DESKTOP=AeroShell` in AeroShell and `KDE` in Plasma without changing persistent shared user-manager variables.
2. Query the selected backend for file chooser, notification, screen cast, screenshot, remote desktop, and secrets interfaces.
3. Exercise screen sharing and screenshot flows through KWin, plus Flatpak file access and URI opening.
4. Confirm AeroShell logout does not stop a broker or backend still owned by another session, and Plasma continues using its KDE selector, KWallet, and Plasma notifications.

The project still needs to select a safe per-session portal activation strategy before T07 can be marked done.

## References

- [XDG Desktop Portal system integration](https://flatpak.github.io/xdg-desktop-portal/docs/system-integration.html) assigns the desktop identity and portal configuration to the desktop environment.
- [XDG Desktop Portal configuration](https://flatpak.github.io/xdg-desktop-portal/docs/configuration-file.html) documents desktop-specific backend selection.
- [`portals.conf(5)`](https://man.archlinux.org/man/portals.conf.5) documents the desktop-name matching and search locations used by the current Arch-family host.
