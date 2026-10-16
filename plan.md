# AeroShell Desktop — Independent Desktop Environment

**Status:** Planning  
**Project repository:** [SamuelFontes/aero-shell-desktop](https://github.com/SamuelFontes/aero-shell-desktop)  
**Target:** Arch Linux / Manjaro first; other Linux distributions later  
**Upstream base:** [AeroThemePlasma](https://github.com/aeroshell-desktop/aerothemeplasma), `Plasma/6.7` branch (verify version compatibility before builds)  
**Proposed package:** `aeroshell-desktop-git`  
**Proposed install prefix:** `/opt/aeroshell`  
**Primary session:** Wayland; X11 is optional future work and is not a release prerequisite.  
**Primary objective:** Install AeroShell alongside an existing KDE Plasma installation, and choose either session from the existing login screen without AeroShell installation, use, updates, or removal changing the existing KDE setup.

## 1. Goal and non-negotiable requirements

Build an independently selectable **AeroShell** desktop session based on AeroThemePlasma's Windows 7-style shell. It may use the distro's KDE Plasma, Qt, KDE Frameworks, drivers, and applications as dependencies, but it **must not replace or modify** the distro's KDE packages or the user's KDE desktop configuration.

### Required behavior

- [ ] Register **AeroShell (Wayland)** as a distinct desktop session using the standard session directory; preserve the existing Plasma entry.
- [ ] Never install, configure, restart, enable, disable, theme, or replace SDDM or another display manager. Never change its default session or remembered selection.
- [ ] Login-screen theme selection belongs entirely to the user through their chosen tools; AeroShell supplies no SDDM theme installer, configuration module, or theme-switching hook.
- [ ] AeroShell has its own desktop identity, startup lifecycle, settings profile, and session services. KDE technology may be reused as dependencies without making AeroShell a theme applied to the Plasma session.
- [ ] Existing Plasma packages remain package-manager-owned, unmodified, and usable.
- [ ] AeroShell panels, plasmoids, wallpaper, window decorations, KWin effects, shortcuts, fonts, icons, and shell settings do not overwrite KDE Plasma's settings.
- [ ] Switching between AeroShell and Plasma takes only a logout and login; no theme/application/uninstall scripts are needed.
- [ ] AeroShell runs under the **same Linux user account** and can access the user's files and installed applications.
- [ ] Installing or uninstalling AeroShell never requires reinstalling `libplasma`, `plasma-desktop`, `plasma-workspace`, or `kwin`.
- [ ] No AeroShell-only process, policy agent, or compositor effect is started in a regular KDE session.
- [ ] A broken AeroShell session never prevents login to stock Plasma.
- [ ] Wayland is required for the first release. Native Wayland applications and legacy applications through session-local XWayland must work; an X11 desktop session is optional future work.

### Out of scope for the initial release

- A complete KDE/Qt fork with independent versions of every system library.
- Replacing SDDM, Plymouth, global login branding, system fonts, or other machine-wide appearance.
- Full application sandboxing; apps, home files, kernel, drivers, and system services remain shared.
- Guaranteed compatibility with all future Plasma versions without rebuilding.
- Official packages for every Linux distribution.
- An X11 desktop session or preserving effects that depend on X11-only mechanisms.

## 2. Upstream starting point to verify during inventory

The existing plan identifies the `Plasma/6.7` branch as providing AeroThemePlasma-specific SDDM sessions. Its X11 launcher (`plasma/sddm/login-sessions/startatp.cmake`) sets `XDG_CONFIG_DIRS`, `USE_UAC_AGENT`, and `PLASMA_DEFAULT_SHELL=io.gitgud.wackyideas.desktop`, then starts `startplasma-x11`. **This is a separate session entry, not complete isolation**: `XDG_CONFIG_DIRS` supplies system defaults but does not isolate writable per-user settings.

The upstream `install.sh` builds and installs multiple projects using `-DCMAKE_INSTALL_PREFIX=/usr`, including a fork of `libplasma`. Its installation documentation warns that KDE updates can break the modifications and that uninstalling requires reinstalling the distro's `libplasma`. Do **not** execute the upstream installer on the development host until the installation has been made non-destructive.

### Upstream components to inventory

| Component | Expected role | Initial treatment |
|---|---|---|
| `aerothemeplasma` | Plasma shell themes, plasmoids, default settings, session entries | Relocate into private prefix; replace session launchers |
| `aeroshell-workspace` | Workspace integration and shell components | Audit for hardcoded paths, settings writes, global services |
| `libplasma` fork | Aero-specific Plasma library changes | **Highest risk:** privately load only in AeroShell processes |
| `aeroshell-kwin-components` | Custom KWin effects and integrations | Isolate plugin discovery and compositor configuration |
| `smod` / glow components | Window decoration and effects | Audit Wayland support; port needed effects into supported compositor/decoration interfaces, or defer them |
| `uac-polkit-agent` | Aero-styled authentication agent | Activate only for AeroShell; avoid duplicate agents |
| `aeroshell-sddm-kcm` | SDDM configuration UI | Exclude entirely from builds, packages, dependencies, and setup flows |
| `aerothemeplasma-icons` / `-sounds` | Visual and audio assets | Install privately without changing desktop-wide defaults |

**Source of truth:** Pin commits for all repositories and inspect their real install manifests; the table above is an audit checklist, not evidence that each component already supports a private prefix.

## 3. Proposed architecture

```text
Existing display manager (user-managed, untouched)
├── Plasma (existing)
│   ├── distro startplasma-*
│   ├── distro libplasma + KWin + Qt
│   └── existing ~/.config, ~/.local/share, ~/.cache
└── AeroShell (new)
    ├── /usr/bin/aeroshell-session-wayland
    ├── /opt/aeroshell/{bin,lib,lib64,share,etc,libexec}/...
    ├── private AeroShell libplasma and custom plugins (as needed)
    ├── AeroThemePlasma shell + KWin effects + resources
    └── isolated AeroShell configuration, data, cache, state
```

### Installation layout

```text
/opt/aeroshell/
├── bin/
├── lib/                      # actual library path depends on distro
├── libexec/
├── share/
│   ├── plasma/               # verify KDE-specific subdirectory layout
│   ├── icons/
│   ├── sounds/
│   └── ...
└── etc/xdg/
    └── aerothemeplasma/

/usr/bin/aeroshell-session-wayland
/usr/share/wayland-sessions/aeroshell.desktop
# Optional later: /usr/share/xsessions/aeroshell.desktop

~/.config/aeroshell/         # isolated config home or targeted KDE files
~/.local/share/aeroshell/    # isolated user data, where needed
~/.cache/aeroshell/
~/.local/state/aeroshell/
```

**Ownership rule:** Only uniquely named package integration files may go under `/usr` (for example `/usr/bin/aeroshell-session-wayland` and a standard desktop-session `.desktop` file). No existing system paths may be overwritten, and no file owned by another package may be claimed.

## 4. Implementation phases

### Phase 0 — Inventory, safety checks, and reproducible baseline

- [ ] Use `SamuelFontes/aero-shell-desktop` as the project repository for standalone integration, packaging, patches, and documentation. Track AeroThemePlasma's `Plasma/6.7` branch as a pinned upstream source; record the exact commit SHA.
- [ ] Pin matching SHAs or tags for all repositories currently fetched by `install.sh`.
- [ ] Inventory Wayland startup support and classify every shell feature, decoration, effect, and helper as Wayland-ready, requiring a port, or deferred. Do not assume the upstream X11 launcher establishes Wayland support.
- [ ] Define the minimum Wayland feature set before packaging; X11-only visual extras must not block a usable Wayland desktop.
- [ ] Record host Plasma/Qt/KF6/KWin versions and compare them with upstream's tested versions.
- [ ] Save `pacman -Q`, `pacman -Qkk` results for core KDE packages, and a snapshot of relevant KDE settings **before testing**.
- [ ] Enumerate every `install(...)` in the root and dependent projects, including absolute paths, `KDEInstallDirs`, shell scripts, fonts, MIME files, DBus services, desktop entries, user services, polkit, and SDDM files.
- [ ] Build each component with `DESTDIR` staging; inspect file lists before any host installation.
- [ ] Run the first end-to-end tests in an Arch/Manjaro VM or disposable test user on a test system, **never by overwriting the production KDE install**.
- [ ] Classify each upstream change as: private asset, private library/plugin, session-only setting/service, or unacceptable global mutation.

**Deliverable:** `docs/install-inventory.md`, dependency matrix, pinned versions, and staged-file inventory.

**Exit criterion:** Every installed path and every system-wide side effect has an identified owner and isolation strategy.

### Phase 1 — Separate installation and packaging roots

- [ ] Add a build option such as `AEROSHELL_STANDALONE=ON`.
- [ ] Use an isolated prefix (`/opt/aeroshell`) for upstream projects rather than `/usr`.
- [ ] Replace hardcoded `/usr`, `/etc/xdg`, and `/usr/local/share` outputs with configured relocatable directories where appropriate.
- [ ] Set appropriate `CMAKE_INSTALL_LIBDIR`, `CMAKE_INSTALL_LIBEXECDIR`, QML paths, and KPackage/plugin install destinations per component.
- [ ] Split **desktop session registration** (small `/usr/share/...` integration files) from private runtime installation.
- [ ] Stage installations with `DESTDIR` and automatically fail the build if staged files would overwrite a distro-owned path.
- [ ] Avoid global `update-mime-database`, fontconfig edits, `/etc/environment` edits, and default theme changes.
- [ ] Exclude all upstream SDDM themes, configuration modules, scripts, dependencies, and install hooks. Standard session registration is the only login-screen integration.
- [ ] Define a reproducible build entry point (`scripts/build.sh`) that does not use `sudo` for compilation.

**Deliverable:** Private-prefix builds and an install manifest clean of KDE package conflicts.

**Exit criterion:** All components stage successfully with **zero overwrites** of host KDE files.

### Phase 2 — Isolate `libplasma` and runtime plugins (critical path)

- [ ] First test whether every AeroShell feature actually requires the patched `libplasma`; document the APIs or behavioral patches used.
- [ ] If required, build the patched library into `/opt/aeroshell/lib` without replacing `/usr/lib/libplasma*`.
- [ ] Identify which AeroShell binaries, QML modules, helpers, and KWin plugins must bind to the private library.
- [ ] Configure component-specific ELF `RUNPATH`/`RPATH` (or limited launch wrappers) and plugin loading so **only AeroShell processes** use the private version.
- [ ] Keep `LD_LIBRARY_PATH`, `QT_PLUGIN_PATH`, and QML import path changes out of global environment files. Prefer exact binary/plugin-specific paths; any session-wide overrides must be justified and tested against normal applications.
- [ ] Resolve plugin loading of SMOD, KWin effects, shell packages, QML imports, and custom modules from the private prefix.
- [ ] Validate with `readelf -d`, `ldd`, `/proc/<pid>/maps`, and Qt/KDE plugin debugging (`QT_DEBUG_PLUGINS=1` where appropriate).
- [ ] Confirm stock Plasma resolves distro libraries while AeroShell resolves only the private forks it needs.
- [ ] If unmodified `startplasma-*` cannot reliably load the intended private shell/components, provide dedicated AeroShell wrappers or patch/build the necessary launch components **under the private prefix**; do not replace distro launchers.

**Deliverable:** A test harness starting the AeroShell shell and a Wayland compositor with private dependencies; begin with nested testing, then validate a real login session on test hardware.

**Exit criterion:** Stock KDE and AeroShell can launch in turn without ELF, QML, KWin, or Qt plugin collisions; distro `libplasma` remains unchanged.

**Stop condition:** Do not proceed to an installable package if achieving this requires replacing host `libplasma` or globally injecting an incompatible loader path.

### Phase 3 — Isolate user configuration and session state

- [ ] Define an AeroShell settings policy. Candidate: session-scoped `XDG_CONFIG_HOME=$HOME/.config/aeroshell`, `XDG_CACHE_HOME`, and `XDG_STATE_HOME` with AeroShell-specific defaults in `/opt/aeroshell/etc/xdg`.
- [ ] Audit `plasmashellrc`, `plasma-org.kde.plasma.desktop-appletsrc`, `kwinrc`, `kdeglobals`, `kscreenlockerrc`, `kglobalshortcutsrc`, appearance, power, notifications, and related state; confirm actual paths from source/runtime traces.
- [ ] Evaluate `XDG_DATA_HOME` isolation carefully: globally changing it can hide existing `~/.local/share/applications`, icons, or other user-installed items. Implement explicit shared paths or narrower file-level isolation where needed.
- [ ] Identify programs that ignore XDG paths or hardcode `$HOME/.config` / `$HOME/.local/share`; patch or wrap those only if necessary.
- [ ] Ensure the first-run wizard writes **only** AeroShell settings. It must never import and then overwrite stock KDE configurations without explicit consent.
- [ ] Audit the shared systemd user manager and D-Bus activation. Use uniquely named AeroShell units, session targets, and private service names where possible; do not enable AeroShell services globally or replace KDE units/activation files.
- [ ] Scope environment changes to AeroShell processes. Any unavoidable shared activation-environment change must preserve prior values and restore them after normal logout, failed startup, and crashes; otherwise redesign the startup path.
- [ ] Stop only services owned by the AeroShell session; never stop, restart, or reconfigure unrelated KDE or user services.
- [ ] Avoid simultaneous KDE + AeroShell graphical sessions for the same user during initial testing unless service/lifetime isolation is explicitly verified.
- [ ] Use session-scoped fontconfig and QML font-rendering options, never `/etc/environment` or a shared per-user fontconfig file.
- [ ] Define whether user-installed plasmoids, launchers, and app defaults are shared or isolated; favor protecting KDE settings over silently losing access to apps.

**Deliverable:** Repeatable first-run profile provisioning, documented shared-versus-private settings policy, and session state cleanup.

**Exit criterion:** Customizing AeroShell does not change stock KDE's panel, theme, window behavior, keyboard shortcuts, or wallpaper—and vice versa.

### Phase 4 — Independent desktop startup and lifecycle

- [ ] Add **AeroShell (Wayland)** to `/usr/share/wayland-sessions/` with a unique launcher under `/usr/bin`.
- [ ] Set the Aero-specific shell identifier (`PLASMA_DEFAULT_SHELL=io.gitgud.wackyideas.desktop`) and required configuration/asset paths.
- [ ] Reuse the distro's login/session plumbing where safe; otherwise provide dedicated wrappers for `startplasma-wayland`, D-Bus startup, and session initialization; audit the selected upstream version before choosing which launch components to reuse.
- [ ] Give AeroShell its own session identity (`DesktopNames`, `XDG_CURRENT_DESKTOP`, and `XDG_SESSION_DESKTOP`) and explicitly configure portal/backend selection for that identity. Document and verify any KDE compatibility identifiers required by reused components; never modify the Plasma session entry or its identity.
- [ ] Provide AeroShell-specific settings entry points; exclude modules that mutate shared KDE or display-manager configuration. Any shared application preferences must be documented and must not include KDE desktop settings.
- [ ] Start the Aero polkit agent **only** in AeroShell; verify that it does not run alongside an existing polkit agent.
- [ ] Start Aero-specific services, setup wizard, and compositor effects exclusively in the AeroShell session.
- [ ] Handle abnormal exit, logout, session restart, and environment cleanup without killing unrelated user services.
- [ ] Establish ownership and cleanup of the AeroShell Wayland compositor, display socket, and any XWayland instance without affecting Plasma processes. Audit KWin plugin ABI and upstream session/D-Bus startup helpers.
- [ ] Validate native Wayland input, window management, decorations, shortcuts, clipboard, drag and drop, and session-local XWayland compatibility.
- [ ] Validate portal routing, screen sharing, screenshots, lock/unlock, and suspend/resume in the actual Wayland login session.
- [ ] Port or defer X11-only glow, effects, and helpers; never require an X11 desktop session as a fallback to satisfy Wayland acceptance.

**Deliverable:** Independently selectable AeroShell Wayland desktop with its own lifecycle, no display-manager changes beyond standard session registration, and no residual effects on KDE after logout.

**Exit criterion:** KDE → logout → AeroShell → logout → KDE works repeatedly with original KDE layout intact.

### Phase 5 — Arch/Manjaro packages and safe removal

- [ ] Create `packaging/arch/PKGBUILD` (or split packages) with pinned sources/checksums, accurate `depends`, `makedepends`, and runtime compatibility constraints.
- [ ] Package with `makepkg -si` through pacman; never implement a direct `sudo make install` workflow for end users.
- [ ] Use **unique package names** and no `conflicts`, `provides`, or `replaces` claims against distro `libplasma`, `kwin`, or `plasma-workspace`.
- [ ] Split optional assets/effects if this reduces dependency or upgrade fragility; keep a single main meta-package for end users.
- [ ] Ensure install does not automatically change the default SDDM session, login theme, shell, desktop defaults, or system environment.
- [ ] Uninstall removes only package-owned binaries, session entries, and bundled resources. **Do not delete any user configuration automatically.** Provide an optional documented profile-cleanup command.
- [ ] Test clean install, reinstall, upgrade, dependency failure, rollback, and removal; ensure KDE can launch at each step.
- [ ] Add a preflight compatibility checker for Plasma/KWin/KF6/Qt version mismatches; fail clearly instead of partially installing incompatible plugins.

**Deliverable:** Locally installable `aeroshell-desktop-git` package and user-facing README.

**Exit criterion:** `pacman -Rns aeroshell-desktop-git` (or exact split package set) leaves existing KDE packages/settings untouched and removes the AeroShell login option.

### Phase 6 — Automated validation and release readiness

- [ ] Build in clean Arch and Manjaro test environments.
- [ ] Run static manifest checks rejecting writes to package-owned paths.
- [ ] Validate loader/plugin resolution for all Aero components.
- [ ] Take before/after hashes of KDE configuration and package files.
- [ ] Exercise session switching, first-run setup, appearance changes, logout, crashes, and package uninstall.
- [ ] Test multiple displays, HiDPI, GPU acceleration, compositing/blur, audio, notification area, shortcuts, widgets, and standard applications.
- [ ] Check app launchers, desktop portals, polkit prompts, lock/unlock, suspend/resume, and DBus/systemd user services.
- [ ] Test version upgrades of Plasma/KWin and define a supported compatibility matrix instead of claiming universal rolling-release compatibility.
- [ ] Document recovery using the stock Plasma session if AeroShell fails.
- [ ] Produce install/upgrade/uninstall instructions and a known-issues list.

**Deliverable:** Release candidate validated against the acceptance matrix below.

## 5. Acceptance matrix

| Scenario | Expected result | Required |
|---|---|---|
| Install on existing KDE | No existing package file overwritten | Yes |
| Existing login session picker | Existing Plasma and new AeroShell appear through standard session registration | Yes |
| SDDM and other display managers | Configuration, theme, services, defaults, and remembered selection untouched by AeroShell | Yes |
| Independent desktop identity | AeroShell owns its startup, profile, settings entry points, and session services | Yes |
| Failed startup / lingering user manager | No AeroShell environment, services, or settings leak into Plasma | Yes |
| First AeroShell login | Aero taskbar, shell, widgets, decorations load | Yes |
| Configure Aero appearance | No change in existing KDE profile | Yes |
| Log into stock KDE | Original layout/settings preserved | Yes |
| Standard applications | Work in either session | Yes |
| KWin effects | Aero-only effects do not alter regular KDE | Yes |
| Dependency lookup | Private fork loads only where intended | Yes |
| Remove package | KDE remains launchable without repair | Yes |
| Crash of AeroShell | Can switch to KDE after logout/restart | Yes |
| Repeated upgrades | Incompatible builds rejected safely | Yes |
| Wayland session | Primary desktop starts and exits cleanly with isolated compositor/settings | Yes |
| Native Wayland and XWayland apps | Launch, input, clipboard, and window management work | Yes |
| Wayland portals and security | Screen sharing, screenshots, lock/unlock, and suspend/resume work | Yes |
| Display handling | Multiple displays, hotplug, and fractional scaling pass the supported hardware matrix | Yes |
| X11 desktop session | Optional future work | Later |
| Fedora/openSUSE packages | Outside first release | Later |

## 6. Known risks and mitigation

| Risk | Impact | Mitigation |
|---|---|---|
| Patched `libplasma` shares SONAME/ABI with distro library | Wrong copy loaded, breakage in either desktop | Per-process loading, verified binary/linker paths, no global overrides |
| X11-only upstream effects or helpers | Missing or broken Wayland visuals/features | Audit early; port required features and defer optional extras without blocking Wayland |
| KWin plugin ABI changes frequently | Effects crash or silently fail after upgrades | Pin supported versions; rebuild and compatibility gate |
| KDE services run through a persistent systemd user manager | Settings/environment leak across sessions | Import/scrub environment deliberately; test logout-login cycles and lingering services |
| Upstream installers use absolute `/usr` paths | File conflicts with KDE packages | Audit every CMake install/manifest; patch and stage with `DESTDIR` |
| `XDG_DATA_HOME` separation hides user application entries | Missing launchers, icons, application defaults | Explicitly preserve shared app discovery or use selective isolation |
| First-run customization writes absolute home paths | Existing KDE configuration changes | Instrument writes and patch first-run tools |
| Plugin/QML search paths leak to child applications | Unexpected Qt crashes/compatibility bugs | Scope search paths to Aero processes rather than all programs |
| Fonts, MIME, polkit, or login integration change system behavior | Changes visible in regular KDE | Keep AeroShell customization session-local; exclude all display-manager customization |
| Arch rolling updates move beyond upstream branch | Build or runtime failures | Version detection, strict compatibility matrix, rebuild automation |
| Proprietary Windows 7 artwork/fonts have redistribution constraints | Unusable binaries for public packages | Audit licenses and use lawful sources/user-provided assets; do not redistribute restricted files |

## 7. Repository organization (proposed)

```text
aeroshell-desktop/
├── plan.md
├── README.md
├── cmake/
│   └── StandaloneInstallPaths.cmake
├── scripts/
│   ├── build.sh
│   ├── audit-install-manifests.sh
│   ├── check-compatibility.sh
│   └── verify-library-loading.sh
├── session/
│   ├── aeroshell-session-wayland
│   └── aeroshell-wayland.desktop
├── defaults/
│   └── xdg/
├── patches/
│   ├── aerothemeplasma/
│   ├── aeroshell-workspace/
│   ├── libplasma/
│   ├── kwin-components/
│   └── smod/
├── packaging/
│   └── arch/
│       └── PKGBUILD
├── tests/
│   ├── test-no-conflicts.sh
│   ├── test-session-isolation.sh
│   └── test-uninstall.sh
├── docs/
│   ├── install-inventory.md
│   ├── compatibility.md
│   └── recovery.md
└── upstream/                      # pinned clones, submodules, or fetched sources
```

## 8. Suggested implementation order (critical path)

1. **Inventory all upstream write/install paths** and pin repositories. Do not modify the running system.
2. **Build privately into a staging root** and resolve paths until there are no file conflicts.
3. **Prove private `libplasma` loading** and controlled KWin/QML plugin discovery before investing in UI polish.
4. **Isolate AeroShell user settings and user-session services.** Verify no settings leak to KDE.
5. **Make the independent Wayland desktop session work** and repeat the Plasma ↔ AeroShell switch test.
6. **Create installable Arch/Manjaro packages** with safe removal and version checks.
7. **Consider an optional X11 session and additional distributions** after the Wayland package passes all tests.

### Milestone definitions

- **M1 — Build isolation:** private-prefix install staged without overwriting KDE.
- **M2 — Runtime isolation:** the shell, Wayland compositor, private library, and plugins run together; stock KDE remains unaffected.
- **M3 — Usable separate desktop:** both sessions selectable; configurations stay independent.
- **M4 — Installable release:** pacman-managed install, upgrade and removal pass the acceptance matrix.

## 9. Definition of done

The project is done for its Wayland MVP when a user with an existing Manjaro/Arch KDE system can install the new `aeroshell-desktop-git` package, select **AeroShell (Wayland)** from their existing login screen, customize and use it, return to **Plasma** with their existing settings unchanged, and uninstall AeroShell **without having to repair or reinstall KDE**. SDDM and other display managers remain user-managed: AeroShell registers its session entry but never changes their configuration, theme, service state, or default session. AeroShell must have its own desktop identity and lifecycle, with no writes to KDE desktop settings or globally enabled AeroShell services. Native Wayland and XWayland applications, portal screen sharing, screenshots, lock/unlock, suspend/resume, and supported display/scaling configurations must pass the acceptance matrix. All of this must be demonstrated on a clean test system and supported by automated install-path, configuration-isolation, and rollback tests.

## 10. Upstream references

- [AeroThemePlasma repository (GitHub mirror)](https://github.com/aeroshell-desktop/aerothemeplasma)
- [Upstream installation guide, `Plasma/6.7`](https://github.com/aeroshell-desktop/aerothemeplasma/blob/Plasma/6.7/INSTALL.md)
- [Upstream installer, `Plasma/6.7`](https://github.com/aeroshell-desktop/aerothemeplasma/blob/Plasma/6.7/install.sh)
- [Upstream X11 session launcher](https://github.com/aeroshell-desktop/aerothemeplasma/blob/Plasma/6.7/plasma/sddm/login-sessions/startatp.cmake)
- [Upstream CMake root](https://github.com/aeroshell-desktop/aerothemeplasma/blob/Plasma/6.7/CMakeLists.txt)

> This document is a development plan, not a claim that the port has already been implemented or tested. Audit results and compatibility findings should be recorded as implementation proceeds.
