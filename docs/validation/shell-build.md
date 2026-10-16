# AeroShell build and validation evidence

**Date:** 2026-10-16  
**Environment:** Manjaro development host, Qt 6.11.2, KWin 6.7.4, KWayland 6.7.4. Build and staged files stayed in the workspace and `/tmp`; no live installation occurred.

## Build and package staging

- `cmake -S . -B build -G Ninja -DCMAKE_INSTALL_PREFIX=/opt/aeroshell` — configured successfully.
- `cmake --build build -j2` — passed, including the in-process PolkitQt agent and Qt Quick prompt.
- `qmllint -I build/qmltypes src/shell/qml/*.qml` — passed.
- `DESTDIR=/tmp/aeroshell-stage-menu.t72GGl cmake --install build` — staged successfully with the D-BusMenu renderer, private widget settings, and screen hotplug handling.
- The earlier stage at `/tmp/aeroshell-stage-menu.t72GGl` passed for 8,326 entries (5,392 regular files, 2,800 symlinks, and 134 directories) before the AeroThemePlasma lock-screen package was added. The latest 8,442-entry stage, including the session-only Qt Quick Controls config, is recorded in `docs/validation/install-policy.md`.
- `bash scripts/verify-library-loading.sh /tmp/aeroshell-stage-menu.t72GGl/opt/aeroshell/bin/aeroshell-shell` — passed. The binary resolves distro shared libraries and has only its private `$ORIGIN` RUNPATH.
- `python tests/test-install-policy.py` — 17 fixtures passed.
- `python tests/test-source-lock.py` — 11 structural fixtures passed; no complete upstream source lock is present yet.
- `makepkg --printsrcinfo` — parsed the Arch package scaffold, including the PolkitQt dependency. Package build/install/upgrade/removal on a disposable Arch system remains untested.

## Automated checks

- `ctest --test-dir build --output-on-failure` — all ten CTests passed: audio controls, battery status, network status, StatusNotifier icon conversion, offscreen Aero icons/wallpaper/color-scheme checks, disconnected-bus notification model, AeroShell-only settings storage/defaults, KWin compatibility, isolated session startup, and mandatory first-login appearance.
- `bash tests/test-session-launch.sh` — passed. The fake compositor fixture checks session identity, compositor arguments, theme environment, first-login decoration setup, existing-profile preservation, unsupported KWin refusal, XDG restoration, and refusal of symlinked or hard-linked settings/KWin config paths.
- `python tests/test-default-appearance.py` — passed. It checks the full icon tree, AeroThemePlasma wallpaper/color scheme/Start orb, cursor, Noto Sans, GTK defaults, enabled default widgets and widget settings, screen add/remove reconciliation, remote StatusNotifier menu wiring, confirmed power actions, notification service, and AeroShell-owned Polkit prompt/password handling.
- Session fixtures confirm Qt Quick children receive the bundled Aero color/font configuration path. The CTest appearance fixture validates the expected Basic style, Noto Sans, Aero palette, and session-only environment selection.
- `qmllint -I build/qmltypes -I /usr/lib/qt6/qml assets/plasma/look-and-feel/org.aeroshell.desktop/contents/lockscreen/*.qml` — passed. The static appearance check confirms the lock-screen package uses its bundled success sound and does not inspect KDE's shared `kdeglobals` file.
- Audio and battery fixtures use fake providers. Network tests use fake `nmcli`, including private credential-file handling. Notification tests use a deliberately disconnected D-Bus connection. None of these proves interaction with live services or physical hardware.
- `desktop-file-validate session/aeroshell.desktop`, shell syntax checks, and `git diff --check` pass.

## Runtime and release limitations

- A private Wayland/KWin smoke run cannot start in this environment. The sandbox rejects both filesystem and abstract AF_UNIX socket creation with `Operation not permitted`, including the private D-Bus socket needed before KWin can start. No real graphical surface, login/logout, display hotplug, scaling, lock/unlock, portal capture, or KDE-to-AeroShell-to-KDE comparison has been demonstrated.
- GitHub/GitGud DNS resolution failed during source retrieval. Remote component pins and a reproducible clean package checkout remain incomplete. Per user direction, asset-rights verification is not required and is not a completion gate.
- The shell starts a session-owned PolkitQt listener and Aero-styled prompt, but live system-bus registration, a benign authentication request, collision behavior, and multi-session behavior remain unverified.
- The KScreenLocker lock action, AeroThemePlasma lock-screen UI/wallpaper, five-minute idle lock, password requirement, lock-on-resume, and private Wayland blur/glide defaults are implemented and fixture-tested. Live lock/authentication/unlock and effect rendering remain unverified. The full desktop sound theme/events, complete appearance settings, and broader Qt application styling remain open work. The StatusNotifier D-BusMenu renderer is implemented but still needs live interoperability testing. Do not call the desktop release complete until required features and runtime evidence are closed.
- No SDDM configuration, theme, service, or default was changed. No Plasma configuration, package, or service was written by this work. Real-system comparison is still pending.
