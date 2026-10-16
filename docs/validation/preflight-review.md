# Review before the first real login

Date: 2026-10-16. Host target: Manjaro, KWin/KScreenLocker/KWayland 6.7.4, Qt 6.11.2.

This review supersedes the earlier package and lock-screen selection evidence. No system package was installed, no SDDM or KDE user configuration was changed, and nothing was published.

## Problems fixed

| Area | Finding and correction |
|---|---|
| QML startup | `QtObject` could not contain the root's child objects. The panel also overrode the final `Button.display` property and declared the unsupported QML `uint` type. The root now uses `Item`, task text uses its model, and notification IDs use an exact numeric representation for UINT32. |
| Taskbar integration | KWin hid its restricted window-management interface because the shell had no application metadata declaring it. A hidden application entry under `/opt/aeroshell/share/applications` now declares the permission. Its executable, interface list, and visibility are parsed with the installed KService library in the startup test. The task delegate now reads the actual case-sensitive KWayland roles (`AppId`, `IsActive`, etc.). |
| Desktop and screens | QML screen objects expose `virtualX`, `virtualY`, `width`, and `height`, rather than the `geometry` object previously used. Fullscreen initialization followed by `show()` also restored a 1×1 desktop. Explicit desktop bounds now remain stable, surface positions follow changes, and screen reconciliation is deferred until Qt updates its screen list. |
| Popovers | Item popovers were confined to the 48-pixel panel surface. The seven panel popovers and task context menus now use separate popup windows. Power dialogs have explicit implicit widths to break layout loops. |
| Private service activation | The private bus started before KWin created its display sockets. Its activation environment never received the final display/profile settings. A marker trampoline identifies the private bus, and the child wrapper updates only that bus, without touching the systemd user manager. Missing/mismatched markers and failed activation initialization prevent shell startup. Nested invocation from an active Wayland desktop is refused. |
| Icons | Refresh queries were passed to `QIcon::fromTheme`, causing generic fallback icons. Provider IDs now discard the cache query, and zero-size requests use a valid size. Pixel comparisons cover the query form. |
| Tray | `QDBusContext` was inherited by the adaptor, where Qt documents that accessing it is invalid. It now belongs to the registered model object, and caller access is checked. Tray-menu activation data is wrapped as a D-Bus variant to match the protocol. |
| Notifications | Default and persistent expiration were reversed. `-1` now uses the server default and `0` persists. Updating an older notification promotes it to the latest position, action labels cannot be submitted as keys, and QML receives correctly named close signals while the external D-Bus signal names are preserved. Summaries use plain text. Application-launch errors appear in the notification UI. |
| Lock-screen selection | The installed KScreenLocker 6.7.4 reads a `Plasma/Shell` package, not the old LookAndFeel setting. The bundled UI is now a self-contained shell package selected through `PLASMA_DEFAULT_SHELL` in KWin's private profile. Package contents stay within their own root because KPackage rejects cross-package content symlinks. |
| Lock-screen resources | The OSD imported an absent directory. It now implements the greeter's OSD interface locally. An unused loader referenced another missing file and was removed. Private compositor/greeter `kdeglobals` selects Noto Sans and Aero icons without writing the user's KDE file. |
| Audio and packaging | Audio queries use the C locale so translated command output does not break parsing. Direct Wayland/XWayland dependencies are declared. The package requires `libpulse` for the compatible control client, rather than suggesting installation of the PulseAudio server alongside an existing PipeWire setup. |

## Evidence

The current build succeeds. All eleven CTests, QML lint (shell, lock-screen, and component files), 17 install-policy fixtures, 11 source-lock fixtures, shell syntax checks, desktop-file validation, and `git diff --check` pass. The fresh `build/review-stage-final` audits 8,478 entries and passes the host ownership-collision check; the staged binary passes the distro-library/RUNPATH check.

- `test-qml-startup` instantiates the real desktop QML offscreen with isolated XDG directories, a disconnected notification bus, populated mock services, and a task model using the installed KWayland role definitions. It creates the desktop/panel, opens all named popovers and task menus, opens the launcher, and loads the lock-screen OSD. QML warnings fail the check.
- `tests/test-session-launch.sh` checks the private-bus trampoline, activation address/environment, missing marker, failed activation, nested invocation refusal, seeded private font/icon defaults, and unchanged KDE configuration sentinels.
- Notification tests cover protocol lifetime semantics, updates of older notifications, and action-key validation. Theme tests compare actual queried icons to their unqueried originals.
- `build/review-preview/desktop-0.png` and `panel-0.png` were generated by the offscreen fixture and visually inspected. They are previews with test data, not screenshots of a real Wayland login.
- The installed `/usr/lib/kscreenlocker_greet --testing` was run offscreen with temporary XDG roots, an unreachable session bus, `PLASMA_DEFAULT_SHELL=org.aeroshell.desktop`, and the bundled assets in its data path. Import tracing confirms the AeroShell lock-screen source is selected without `--shell`. The test was terminated after five seconds; no authentication or real locking was attempted. Logs: `build/review-lockscreen.log`. D-Bus/audio/offscreen platform errors are expected in this sandbox.

## First-login limits

A real compositor login, graphics drivers, physical displays, lock/authentication/unlock, portals, live tray interoperability, system authentication, and KDE-to-AeroShell-to-KDE comparison remain unverified. The sandbox cannot create the required private session sockets or install files into system paths. Qt Widgets/KDE application appearance and event sounds remain outside the completed shell appearance implementation. This package is for the requested first local test; these checks do not establish a fully validated desktop release.

## Primary references

- [KWin 6.7.4 interface authorization](https://github.com/KDE/kwin/blob/v6.7.4/src/wayland_server.cpp) and [executable-to-service metadata lookup](https://github.com/KDE/kwin/blob/v6.7.4/src/utils/serviceutils.h).
- [Qt QDBusContext guidance](https://doc.qt.io/qt-6/qdbuscontext.html).
- [Desktop notification lifetime/action protocol](https://specifications.freedesktop.org/notification/latest/protocol.html).
- [KScreenLocker 6.7.4 default shell selection](https://github.com/KDE/kscreenlocker/blob/v6.7.4/settings/shell_integration.cpp).

## Reviewed package

The fresh local build passed all eleven CTests again. Artifact: `build/local-package/aeroshell-desktop-0.1.0.r5.g55050aa.review1-1-x86_64.pkg.tar.zst`. All declared dependencies are satisfied by the current host. SHA-256: `0f54adf4ac644968a3b9b274309a0e3e5dbc1cd6df1626524af0aa7f15e28a39`. The extracted archive passes the install policy and host ownership check for 8,477 entries, ELF dependency/RUNPATH inspection, and desktop-entry validation. The one-entry difference from the CMake stage is an empty, untracked icon `animations` directory; all implementation/test/asset files and symlinks in the source snapshot match the working tree.

The stable local alias is `build/local-package/aeroshell-desktop-reviewed.pkg.tar.zst`. It supersedes the previous package. The package is not installed and has not been boot-tested.
