# Upstream source audit

**Audit date:** 2026-10-16  
**Audit status:** Partial; public mirror documentation inspected, source repositories not cloned.  
**Do not use this report as a build lock or full install manifest.**

## Findings

1. AeroThemePlasma describes itself as a Windows 7 style shell for KDE Plasma. Its `Plasma/6.7` README says Wayland support is incomplete and that proper support is planned for Plasma 6.8. It also describes AeroThemePlasma and its components as read-only GitHub mirrors. The README lists tested Plasma 6.7.4, Frameworks 6.28.0, and Qt 6.11.1; these are upstream's claims, not versions tested by this project.
2. The `Plasma/6.7` install guide currently recommends installing KDE Plasma and SDDM related packages, lists a broad set of shell dependencies, and documents both AUR and source installs. This project must derive its own dependencies from audited build/runtime requirements and must not add an SDDM dependency solely for session registration.
3. The upstream installer clones/builds `libplasma`, `uac-polkit-agent`, `smod`, `aeroshell-workspace`, `aeroshell-kwin-components`, `aeroshell-sddm-kcm`, icons, sounds, and the main AeroThemePlasma project. It configures most CMake projects with `/usr`, installs with elevated privileges, runs `update-mime-database` against `/usr/local/share/mime`, and builds X11 variants unless skipped. This installer is reference material only; never execute it.
4. The installer explicitly builds Aero KWin components with `KWIN_BUILD_WAYLAND=ON`, so some effects have a Wayland build mode. That does not demonstrate a usable independent Wayland desktop, complete feature support, or safe private-prefix installation.
5. The public GitHub organization describes its repositories as mirrors of sources on GitGud. The source branches shown publicly include `Plasma/6.7`. A mirror branch name is not a commit pin. Do not build from a moving branch or use an abbreviated SHA.
6. The upstream AeroShell Workspace README states that staging with `DESTDIR` requires exporting `DESTDIR` before the initial CMake configure so its Segoe UI font configuration symlink observes staging. This is a concrete example of why staging behavior must be inspected per component rather than inferred from a generic CMake pattern.

## Component list from the upstream installer

| Component | Canonical source location shown by installer | Initial project status | Evidence still needed |
|---|---|---|---|
| AeroThemePlasma | `https://gitgud.io/wackyideas/aerothemeplasma.git` | Required visual/shell base; Wayland support incomplete | Full source pin, CMake install tree, launcher/session behavior, settings writes |
| AeroShell libplasma | `https://gitgud.io/aeroshell/libplasma.git` | Optional pending proof of need; highest ABI risk | Full source pin, patched APIs, SONAME/ABI, private runtime feasibility |
| AeroShell workspace | `https://gitgud.io/aeroshell/aeroshell-workspace.git` | Required candidates; shared libraries/assets | Full pin, CMake and custom install behavior, symlink and MIME actions, service/settings writes |
| AeroShell KWin components | `https://gitgud.io/aeroshell/aeroshell-kwin-components.git` | Candidate Wayland effects/scripts; each feature reviewed separately | Full pin, every plugin's ABI/install destination, Wayland feature behavior |
| SMOD | `https://gitgud.io/aeroshell/smod.git` | Candidate decoration; verify compositor support | Full pin, KDecoration ABI, install path and private loading |
| UAC polkit agent | `https://gitgud.io/aeroshell/uac-polkit-agent.git` | Candidate session-only auth UI | Full pin, service/autostart/bus names, Wayland behavior, desktop identity |
| AeroThemePlasma SDDM KCM | `https://gitgud.io/aeroshell/aeroshell-sddm-kcm.git` | **Excluded** by project requirements | Must not be fetched as a runtime/build dependency or installed |
| AeroThemePlasma icons | `https://gitgud.io/aeroshell/atp/aerothemeplasma-icons` | Optional user-selectable visual assets | Full pin, license/assets, install paths, defaults must stay private |
| AeroThemePlasma sounds | `https://gitgud.io/aeroshell/atp/aerothemeplasma-sounds` | Optional user-selectable sound assets | Full pin, license/assets, install paths, defaults must stay private |

The installer clones AeroThemePlasma components and checks out `Plasma/6.7` where shown. Icons and sounds use branch updates without the same explicit checkout in the inspected script. That pattern is not suitable for reproducible pins.

## Immediate implications

- Wayland is an upstream gap, not a feature we can assume is already complete. The first milestone is to identify a minimal viable compositor/session chain and required Wayland ports.
- Treat `Plasma/6.7` as an audited candidate only. Before selecting it, inspect whether a newer upstream branch supports the targeted Plasma/KWin ABI and Wayland features. Do not move the runtime base to an unverified version merely because it is newer.
- Keep user-selectable theming in AeroShell's own settings/profile. Do not install or activate a machine-wide/global Plasma theme as the core desktop startup mechanism.
- Keep standard session entry creation separate from display-manager configuration. No SDDM package, KCM, theme, service operation, or default change is part of AeroShell.
- No source commit has been pinned by this report. Shell `git ls-remote` attempts failed because the environment could not resolve `github.com` or `gitgud.io`; GitHub's web view did not expose full commit SHAs in this audit.

## Source references

- [AeroThemePlasma README, Plasma/6.7](https://github.com/aeroshell-desktop/aerothemeplasma/blob/Plasma/6.7/README.md)
- [AeroThemePlasma install guide, Plasma/6.7](https://github.com/aeroshell-desktop/aerothemeplasma/blob/Plasma/6.7/INSTALL.md)
- [AeroThemePlasma installer, Plasma/6.7](https://github.com/aeroshell-desktop/aerothemeplasma/blob/Plasma/6.7/install.sh)
- [AeroShell Workspace README, Plasma/6.7](https://github.com/aeroshell-desktop/aeroshell-workspace/tree/Plasma/6.7)
- [AeroShell KWin Components README, Plasma/6.7](https://github.com/aeroshell-desktop/aeroshell-kwin-components/tree/Plasma/6.7)
