# Upstream source audit

**Audit date:** 2026-10-16  
**Audit status:** Partial; public mirror documentation inspected, source repositories not cloned.  
**Do not use this report as a build lock or full install manifest.**

## Findings

1. AeroThemePlasma describes itself as a Windows 7 style for KDE Plasma, not as an independent desktop environment. Its `Plasma/6.7` README says Wayland support is incomplete and proper support is planned for Plasma 6.8. The same README carries a hiatus notice saying development will slow significantly and the resumption date is unknown. It also describes AeroThemePlasma and its components as read-only GitHub mirrors. The README lists tested Plasma 6.7.4, Frameworks 6.28.0, and Qt 6.11.1; these are upstream's claims, not versions tested by this project.
2. The `Plasma/6.7` install guide currently recommends installing KDE Plasma and SDDM related packages, lists a broad set of shell dependencies, and documents both AUR and source installs. This project must derive its own dependencies from audited build/runtime requirements and must not add an SDDM dependency solely for session registration.
3. The upstream installer clones/builds `libplasma`, `uac-polkit-agent`, `smod`, `aeroshell-workspace`, `aeroshell-kwin-components`, `aeroshell-sddm-kcm`, icons, sounds, and the main AeroThemePlasma project. It configures most CMake projects with `/usr`, installs with elevated privileges, runs `update-mime-database` against `/usr/local/share/mime`, and builds X11 variants unless skipped. This installer is reference material only; never execute it.
4. The installer explicitly builds Aero KWin components with `KWIN_BUILD_WAYLAND=ON`, so some effects have a Wayland build mode. That does not demonstrate a usable independent Wayland desktop, complete feature support, or safe private-prefix installation.
5. The public GitHub organization describes its repositories as mirrors of sources on GitGud. The source branches shown publicly include `Plasma/6.7`. A mirror branch name is not a commit pin. Do not build from a moving branch or use an abbreviated SHA.
6. The upstream AeroShell Workspace README states that staging with `DESTDIR` requires exporting `DESTDIR` before the initial CMake configure so its Segoe UI font configuration symlink observes staging. This is a concrete example of why staging behavior must be inspected per component rather than inferred from a generic CMake pattern.
7. AeroThemePlasma contains a Wayland session entry and launcher. The desktop entry uses `DesktopNames=KDE`; its launcher adds `/etc/xdg/aerothemeplasma` to `XDG_CONFIG_DIRS`, sets `QML_DISABLE_DISTANCEFIELD`, `USE_UAC_AGENT`, and `PLASMA_DEFAULT_SHELL`, then calls `plasma-dbus-run-session-if-needed` with `startplasma-wayland`. The CMake file installs that launcher in the bindir and the desktop entry in `wayland-sessions`. This is a separate login entry but still runs the Plasma startup stack and publishes shared KDE identity/config defaults. It is not evidence of an independently isolated desktop environment.
8. The `Plasma/6.7` KWin Components root CMake defaults `KWIN_BUILD_WAYLAND` to `OFF`; its README says the flag must be enabled for Wayland builds. The AeroThemePlasma README plans proper Wayland support for Plasma 6.8, while an open issue reports that deprecation of `plasma-dbus-run-session-if-needed` breaks its Wayland session. Treat the present branch as an unverified and potentially broken baseline, not as the Wayland-ready implementation. Recheck the launcher behavior against the selected full source pins before building.

## Component list from the upstream installer

| Component | Canonical source location shown by installer | Initial project status | Evidence still needed |
|---|---|---|---|
| AeroThemePlasma | `https://gitgud.io/aeroshell/atp/aerothemeplasma` | Required default design, appearance assets, and widget inventory; its Plasma session is excluded | Full source pin, CMake install tree, setup wizard behavior, config destinations, Wayland component status |
| AeroShell libplasma | `https://gitgud.io/aeroshell/libplasma.git` | Needed only to host selected upstream widgets that have not yet been ported; highest ABI risk | Full source pin, patched APIs, SONAME/ABI, private widget-host feasibility |
| AeroShell workspace | `https://gitgud.io/aeroshell/aeroshell-workspace.git` | Required candidate for shared widgets, artwork, default layout, and resources | Full pin, CMake/custom install behavior, symlink and MIME actions, service/settings writes |
| AeroShell KWin components | `https://gitgud.io/aeroshell/aeroshell-kwin-components.git` | Required for each default Wayland-safe effect; unsafe effects need a native replacement or block acceptance | Full pin, every plugin ABI/install destination, Wayland feature behavior |
| SMOD | `https://gitgud.io/aeroshell/smod.git` | Required default decoration candidate; verify compositor/Wayland support | Full pin, KDecoration ABI, install path and private loading |
| UAC polkit agent | `https://gitgud.io/aeroshell/uac-polkit-agent.git` | Not selected; AeroShell implements its own in-process PolkitQt listener and Aero prompt | Do not install its global service/autostart artifacts; native agent still needs live D-Bus testing |
| AeroThemePlasma SDDM KCM | `https://gitgud.io/aeroshell/aeroshell-sddm-kcm.git` | **Excluded** by project requirements | Must not be fetched as a runtime/build dependency or installed |
| AeroThemePlasma icons | `https://gitgud.io/aeroshell/atp/aerothemeplasma-icons` | Required default icon theme; user's installed theme is bundled | Full pin, install paths, private application selection |
| AeroThemePlasma sounds | `https://gitgud.io/aeroshell/atp/aerothemeplasma-sounds` | Required default sound theme candidate | Full pin, install paths, private sound selection |
| Windows 7 sound theme | `https://github.com/hyperballic/Windows-Sounds-for-Plasma` | Local checkout is pinned to `926b5052ccb28cb8f97e26397e55e16ed1ede9e8`; its Windows 7 theme is bundled and selected for GTK 3 and AeroShell-native events | Compare its exact contents against AeroThemePlasma's separate sound repository when that source is reachable; Qt/KDE event playback remains unverified |
| Windows 7 Better GTK companion | `https://gitgud.io/Gamer95875/Windows-7-Better` (recommended by upstream install guide) | GTK appearance candidate; AeroShell currently uses its own session-scoped style | GitGud returned HTTP 403; full source, install paths, and session-scoped GTK selection |

The installer clones AeroThemePlasma components and checks out `Plasma/6.7` where shown. Icons and sounds use branch updates without the same explicit checkout in the inspected script. That pattern is not suitable for reproducible pins.

## Immediate implications

- Wayland is an upstream gap, not a feature we can assume is already complete. The first milestone is to identify a minimal viable compositor/session chain and required Wayland ports.
- Treat `Plasma/6.7` as the required design source, but not a proven Wayland implementation. Before selecting pins, inspect whether a newer upstream revision supports the targeted KWin ABI and Wayland features. Do not move the runtime base to an unverified version merely because it is newer.
- AeroThemePlasma is not an optional theme picker in this project: it is the required default look and widget set. Install/select audited resources as AeroShell-owned defaults and store customization under AeroShell's profile; do not apply them to the Plasma session.
- The upstream install guide says a setup wizard applies the look on first run, lists icon/sound/component packages, and directs users to extract Windows fonts from a Windows 7 ISO. Reimplement relevant setup behavior against AeroShell's profile. Never run upstream migration, uninstall, installer, or wizard scripts against a stock KDE profile.
- The user's instruction is to skip asset-rights verification. The local AeroThemePlasma snapshot and installed icon/cursor themes are used as implementation inputs; required assets are bundled and selected without an import step.
- Keep standard session entry creation separate from display-manager configuration. No SDDM package, KCM, theme, service operation, or default change is part of AeroShell.
- No source lock has been produced. A GitHub commit-history page cached about two months before this audit listed the July 13, 2026 merge commit `6746c83e05b718cda8b28a8e25921b0d73a950f0` at the top of AeroThemePlasma's `Plasma/6.7` history. The raw files and commit detail were cache misses, so this is a candidate pin only, not a verified checkout or audited dependency. All component revisions remain unknown. Shell `git ls-remote` attempts for both the user project and AeroThemePlasma failed because the environment could not resolve `github.com`; the local AeroThemePlasma source snapshot also has no Git metadata or root build files. The project install-path checker can optionally query the target pacman database and rejects collisions with packages other than an explicitly allowed AeroShell package; see `scripts/audit-install-manifests.py`.
- Project scope implication: AeroThemePlasma is a themed Plasma shell, while AeroShell must own its separate DE identity, session lifecycle, settings profile, package boundaries, and validation. Its appearance/widgets are mandatory inputs; its Plasma session is not. Upstream's hiatus notice means this project must do the Wayland ports and cannot depend on upstream completing them.
- Wayland implication: do not package the upstream session launcher as-is. T01 must identify a supported startup chain; T02 must test its compatibility and record any required private launcher or source patch. The upstream README's Plasma 6.8 roadmap and the open deprecation-related startup issue are source-risk evidence, not proof that either problem has been resolved.

## Source references

- [AeroThemePlasma README, Plasma/6.7](https://github.com/aeroshell-desktop/aerothemeplasma/blob/Plasma/6.7/README.md)
- [AeroThemePlasma Wayland launcher, Plasma/6.7](https://github.com/aeroshell-desktop/aerothemeplasma/blob/Plasma/6.7/plasma/sddm/login-sessions/startatp-wayland.cmake)
- [AeroThemePlasma Wayland session entry, Plasma/6.7](https://github.com/aeroshell-desktop/aerothemeplasma/blob/Plasma/6.7/plasma/sddm/login-sessions/aerothemeplasmawayland.desktop.cmake)
- [AeroThemePlasma session install rules, Plasma/6.7](https://github.com/aeroshell-desktop/aerothemeplasma/blob/Plasma/6.7/plasma/sddm/login-sessions/CMakeLists.txt)
- [AeroThemePlasma issue: Wayland session startup helper deprecation](https://github.com/aeroshell-desktop/aerothemeplasma/issues/63)
- [AeroShell KWin Components README](https://github.com/aeroshell-desktop/aeroshell-kwin-components/blob/Plasma/6.7/README.md)
- [AeroShell KWin Components CMake configuration](https://github.com/aeroshell-desktop/aeroshell-kwin-components/blob/Plasma/6.7/CMakeLists.txt)
- [AeroThemePlasma install guide, Plasma/6.7](https://github.com/aeroshell-desktop/aerothemeplasma/blob/Plasma/6.7/INSTALL.md)
- [AeroThemePlasma installer, Plasma/6.7](https://github.com/aeroshell-desktop/aerothemeplasma/blob/Plasma/6.7/install.sh)
- [AeroShell Workspace README, Plasma/6.7](https://github.com/aeroshell-desktop/aeroshell-workspace/tree/Plasma/6.7)
- [AeroShell KWin Components README, Plasma/6.7](https://github.com/aeroshell-desktop/aeroshell-kwin-components/tree/Plasma/6.7)
