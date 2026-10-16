# Decision 0001: runtime and compositor

**Status:** Core runtime selected; real-session validation pending. Required AeroThemePlasma appearance components remain unpinned and unaudited.

**Question:** Which Wayland startup chain and compositor arrangement can run an independent AeroShell shell while keeping stock Plasma libraries and plugins unchanged?

**Constraints:** Wayland is the MVP. Do not replace distro KDE packages, globally inject incompatible loader paths, or change Plasma launchers. Reuse the distro compositor only if the source and runtime audit proves that required AeroShell components work without modifying its shared installation or configuration.

**Evidence to collect:** Pinned upstream revisions; startup source paths; ELF dependencies and plugin lookup paths; nested Wayland results; ordinary application loading results; supported Plasma/KWin/Qt versions.

**Selected approach for the core:** Launch the distro `kwin_wayland` compositor directly and use KWaylandClient's `org_kde_plasma_shell` and `org_kde_plasma_window_management` protocols from the repository-owned Qt Quick shell. Do not invoke `startplasma-wayland`, `plasmashell`, or `PLASMA_DEFAULT_SHELL`. The core links to distro Qt/KF/KWayland libraries; no patched `libplasma` is currently required or packaged.

**Alternatives and consequences:** The upstream `startplasma-wayland` path would run Plasma's session lifecycle and use a KDE identity, so it does not meet the independent-DE requirement. A private KWin build is not justified by current code. The chosen core now depends on KWin exposing the Plasma Wayland shell/window-management protocols; startup must fail clearly if those globals are unavailable. AeroThemePlasma's default appearance/widgets are required; each component needs ABI and process-isolation evidence, with a native AeroShell port if the upstream Plasma implementation cannot safely run here.

## Partial source evidence (2026-10-16)

The public `Plasma/6.7` mirror's Wayland launcher sets `XDG_CONFIG_DIRS` to include `/etc/xdg/aerothemeplasma`, exports shell and QML options, then calls `plasma-dbus-run-session-if-needed` with `startplasma-wayland`. Its session desktop entry uses `DesktopNames=KDE`. This confirms that the existing upstream session is a Plasma startup session with Aero defaults, not yet proof of a separate DE identity or private profile.

References:

- [Wayland launcher](https://github.com/aeroshell-desktop/aerothemeplasma/blob/Plasma/6.7/plasma/sddm/login-sessions/startatp-wayland.cmake)
- [Wayland session entry](https://github.com/aeroshell-desktop/aerothemeplasma/blob/Plasma/6.7/plasma/sddm/login-sessions/aerothemeplasmawayland.desktop.cmake)
- [Session install rules](https://github.com/aeroshell-desktop/aerothemeplasma/blob/Plasma/6.7/plasma/sddm/login-sessions/CMakeLists.txt)

**Evidence:** The Qt Quick shell builds and stages; `scripts/verify-library-loading.sh` confirms the staged binary resolves Qt/KF/KWayland libraries from `/usr/lib`, has only the private-prefix `$ORIGIN` RUNPATH, and has no `libplasma` dependency. Fake-compositor session fixtures prove the launcher passes the expected KWin arguments and XDG environment. A real compositor session could not be started because the sandbox rejected the private D-Bus socket bind before KWin ran. See `docs/runtime-loading.md` and `docs/validation/shell-build.md`.

**Remaining decision scope:** Runtime behavior and plugin loading need a real disposable Wayland session. Upstream component behavior remains subject to T01/T02 pinning and recursive audit.
