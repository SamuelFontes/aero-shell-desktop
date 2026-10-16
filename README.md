# AeroShell Desktop

AeroShell is an independent Wayland desktop environment whose default desktop is the complete AeroThemePlasma appearance, adapted to AeroShell's own shell and isolated from KDE Plasma. The first release targets Arch Linux and Manjaro and must coexist with an existing KDE Plasma installation.

**Project status: implementation and validation are in progress. AeroThemePlasma is the required first-login appearance, never an optional theme. The session automatically selects the full Windows 7 Aero icon theme, Aero cursor, Noto Sans, wallpaper, GTK style, sound theme, lock-screen appearance, and supported panel widgets. The shell includes native login/logout and notification sounds; GTK 3 event sounds use the same bundled theme. A local unpublished Manjaro package now builds, but it has not been installed or boot-tested. A real Wayland login and Qt/KDE application styling and sound behavior are not yet validated.** Asset-rights verification is not a completion gate per user direction; upstream source pinning remains incomplete.

AeroShell must keep its desktop settings, processes, and services separate from Plasma. The project registers a standard Wayland session entry; display-manager configuration and login-screen themes remain user-managed. See [plan.md](plan.md) for requirements, task order, evidence rules, and acceptance criteria.

## Development status

The latest [preflight review](docs/validation/preflight-review.md) records the startup/integration fixes, eleven passing CTests, offscreen desktop/popover rendering, automatic Aero lock-screen package selection on this host, and the rebuilt local package. A real Wayland login and authentication test remain necessary.

Task progress and evidence are tracked in [docs/tasks.md](docs/tasks.md). The required first-login composition and active defaults are in [docs/default-appearance.md](docs/default-appearance.md). Upstream source pins and install behavior remain tracked independently from asset-rights verification.

The staged-file policy checker is in `scripts/audit-install-manifests.py`; source metadata validation is in `scripts/validate-sources-lock.py`. The session launcher currently permits KWin 6.7.4 and checks required launch options before creating an AeroShell profile. This preflight is not proof that a real AeroShell session works.

The in-repository shell targets distro KWin and Wayland, with its own desktop, panel, application launcher, task list, clock/calendar, volume, battery, network, keyboard-layout, StatusNotifier items and remote menus, notification controls, and logind power menu, plus XDG profile handling. The first-login composition is automatic: the panel and every mapped AeroThemePlasma widget are created by the shell, and it selects the bundled icon theme, cursor, Noto Sans, GTK style, wallpaper, sound theme, and window decoration without a setup step. Qt Quick child apps receive the AeroThemePlasma palette and Noto Sans through session-only Qt Controls configuration. The private KWin profile enables Wayland blur/glide effects and uses a bundled AeroThemePlasma-derived KScreenLocker theme; the Start menu requests secure KWin locking on AeroShell's private D-Bus. All panel widgets start enabled; AeroShell Settings can hide or restore supported status widgets, change wallpaper, and select the shell font/icon theme inside AeroShell's profile. The Start menu provides app search plus existing Home, Computer, and user-folder shortcuts. The network widget can scan and join open, WPA2, and WPA3 personal networks through NetworkManager. AeroShell includes a notification center/service and a native in-process PolicyKit agent with an Aero-styled prompt. Live D-Bus and graphical behavior remain unverified. Qt Widgets/KDE application styling and sound behavior, real-session integration, and package lifecycle remain required. AeroThemePlasma's Plasma-only startup and all SDDM integrations are excluded; components are adapted to AeroShell's shell or hosted privately without writing into KDE's profile.

First login must already have the full supported AeroThemePlasma appearance and default widget layout enabled, including wallpaper, icons, font, cursor, and panel widgets. Users may customize these after login, but no required piece may be left for manual installation, import, selection, or activation. Asset-rights verification is not a completion gate per the user's instruction.

To build and audit a local staging tree (this does not install anything):

```sh
cmake -S . -B build -G Ninja -DCMAKE_INSTALL_PREFIX=/opt/aeroshell
cmake --build build
DESTDIR="$PWD/stage" cmake --install build
python scripts/audit-install-manifests.py --stage-root stage --print-manifest
```

The current implementation and validation gaps are tracked in [docs/tasks.md](docs/tasks.md) and [docs/validation/shell-build.md](docs/validation/shell-build.md).
