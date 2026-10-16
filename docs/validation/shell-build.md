# AeroShell build and validation evidence

**Date:** 2026-10-16  
**Environment:** Manjaro development host, Qt 6.11.2, KWin 6.7.4, KWayland 6.7.4. Build and staged files stayed in the workspace and `/tmp`; no live installation occurred.

The subsequent [preflight review](preflight-review.md) found and fixed runtime QML, taskbar authorization, popup, activation, notification, and lock-screen package selection bugs. The previous package below is superseded. Current evidence includes eleven CTests and the installed greeter selecting AeroShell's `Plasma/Shell` package offscreen; it does not include a real Wayland login.

## Build and package staging

- `cmake -S . -B build -G Ninja -DCMAKE_INSTALL_PREFIX=/opt/aeroshell` — configured successfully.
- `cmake --build build -j2` — passed, including the in-process PolkitQt agent and Qt Quick prompt.
- `qmllint -I build/qmltypes -I /usr/lib/qt6/qml src/shell/qml/*.qml` — passed.
- `DESTDIR="$PWD/build/stage-final-20261016a" cmake --install build` — staged successfully with AeroShell's widgets, sound theme, private settings, screen hotplug handling, and bundled AeroThemePlasma assets.
- `python scripts/audit-install-manifests.py --stage-root build/stage-final-20261016a` — passed for 8,474 staged entries. `--check-target /` found no existing target ownership collisions. The stage contains only AeroShell's private `/opt/aeroshell` tree, its Wayland session launcher and entry, and its desktop-specific portal selector.
- `bash scripts/verify-library-loading.sh build/stage-final-20261016a/opt/aeroshell/bin/aeroshell-shell` — passed. The binary resolves distro shared libraries and has only its private `$ORIGIN` RUNPATH.
- `python tests/test-install-policy.py` — 17 fixtures passed.
- `python tests/test-source-lock.py` — 11 structural fixtures passed; no complete upstream source lock is present yet.
- `makepkg --printsrcinfo` — parsed the Arch package scaffold after removing the unavailable `qt6-quickcontrols2` dependency (Qt Quick Controls is supplied by `qt6-declarative` on this host).
- A local, unpublished Manjaro package was built from the current workspace snapshot: `build/local-package/aeroshell-desktop-0.1.0.r1.g8a440d9-1-x86_64.pkg.tar.zst`. `pacman -Qp` recognizes it. Its archive contains the AeroShell Wayland session entry, private AeroShell assets, and desktop-specific portal selector; an archive-path check found no SDDM, global Plasma/KWin configuration, or home-profile paths. SHA-256: `c52a6d94eab21a45f24781b046909c8c553ecfc9e3a1652baeb5837014d5246e`.
- The package is not installed: this workspace can write only inside the repository and `/tmp`. Package install/upgrade/removal and booting the installed session therefore remain unverified.

## Automated checks

- `ctest --test-dir build --output-on-failure` — all eleven CTests passed (including the new runtime QML startup check): audio controls, battery status, network status, StatusNotifier icon conversion, offscreen Aero icons/wallpaper/color-scheme checks, disconnected-bus notification model, AeroShell-only settings storage/defaults, KWin compatibility, isolated session startup, and mandatory first-login appearance.
- `bash tests/test-session-launch.sh` — passed. The fake compositor fixture checks session identity, compositor arguments, theme environment, first-login decoration setup, existing-profile preservation, unsupported KWin refusal, XDG restoration, and refusal of symlinked or hard-linked settings/KWin config paths.
- `python tests/test-default-appearance.py` — passed. It checks the full icon tree, AeroThemePlasma wallpaper/color scheme/Start orb, cursor, Noto Sans, GTK defaults, enabled default widgets and widget settings, screen add/remove reconciliation, remote StatusNotifier menu wiring, confirmed power actions, notification service, and AeroShell-owned Polkit prompt/password handling.
- Session fixtures confirm Qt Quick children receive the bundled Aero color/font configuration path. The CTest appearance fixture validates the expected Basic style, Noto Sans, Aero palette, and session-only environment selection.
- `qmllint -I build/qmltypes -I /usr/lib/qt6/qml assets/plasma/shells/org.aeroshell.desktop/contents/lockscreen/*.qml` — passed. The static appearance check confirms the lock-screen package uses its bundled success sound and does not inspect KDE's shared `kdeglobals` file.
- Audio and battery fixtures use fake providers. Network tests use fake `nmcli`, including private credential-file handling. Notification tests use a deliberately disconnected D-Bus connection. None of these proves interaction with live services or physical hardware.
- `desktop-file-validate session/aeroshell.desktop`, shell syntax checks, and `git diff --check` pass.

## Runtime and release limitations

- A private Wayland/KWin smoke run cannot start in this environment. The sandbox rejects both filesystem and abstract AF_UNIX socket creation with `Operation not permitted`, including the private D-Bus socket needed before KWin can start. No real graphical surface, login/logout, display hotplug, scaling, lock/unlock, portal capture, or KDE-to-AeroShell-to-KDE comparison has been demonstrated.
- GitHub/GitGud DNS resolution failed during source retrieval. Remote component pins and a reproducible clean package checkout remain incomplete. Per user direction, asset-rights verification is not required and is not a completion gate.
- The shell starts a session-owned PolkitQt listener and Aero-styled prompt, but live system-bus registration, a benign authentication request, collision behavior, and multi-session behavior remain unverified.
- The KScreenLocker lock action, AeroThemePlasma lock-screen UI/wallpaper, five-minute idle lock, password requirement, lock-on-resume, and private Wayland blur/glide defaults are implemented and fixture-tested. Live lock/authentication/unlock and effect rendering remain unverified. The bundled Windows 7 sound theme is selected for GTK 3 and used by AeroShell-native login, logout, and notification events. AeroShell Settings persists shell font/icon choices and restores the required defaults. Qt Widgets/KDE application styling and sound behavior remain open work. The StatusNotifier D-BusMenu renderer is implemented but still needs live interoperability testing. Do not call the desktop release complete until required features and runtime evidence are closed.
- No SDDM configuration, theme, service, or default was changed. No Plasma configuration, package, or service was written by this work. Real-system comparison is still pending.
