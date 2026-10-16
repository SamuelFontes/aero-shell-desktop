# AeroShell Desktop — Independent Desktop Environment

**Status:** Implementation in progress
**Project repository:** [SamuelFontes/aero-shell-desktop](https://github.com/SamuelFontes/aero-shell-desktop)  
**Target:** Arch Linux / Manjaro first; other Linux distributions later  
**Required appearance source:** [AeroThemePlasma](https://github.com/aeroshell-desktop/aerothemeplasma), `Plasma/6.7` mirror inspected; its appearance, widgets, and assets are AeroShell's required default. Its Plasma session and machine-wide installer are not the AeroShell desktop/session implementation.
**Proposed package:** `aeroshell-desktop-git`  
**Proposed install prefix:** `/opt/aeroshell`  
**Primary session:** Wayland; X11 is optional future work and is not a release prerequisite.  
**Primary objective:** Install AeroShell alongside an existing KDE Plasma installation, and choose either session from the existing login screen without AeroShell installation, use, updates, or removal changing the existing KDE setup. AeroThemePlasma is the required, complete default appearance of AeroShell, adapted to AeroShell's own shell and session.

## 1. Goal and non-negotiable requirements

Build an independently selectable **AeroShell** Wayland desktop environment with its own Qt Quick shell, session lifecycle, settings, and services. KWin and KDE libraries may be used as compositor/runtime dependencies, but AeroShell must not start Plasma's shell session or replace or modify the distro's KDE packages or the user's KDE desktop configuration. AeroThemePlasma is the required complete default visual design and widget source; its theme session is not the independent desktop implementation.

### Required behavior

- [ ] Register **AeroShell (Wayland)** as a distinct desktop session using the standard session directory; preserve the existing Plasma entry.
- [ ] Never install, configure, restart, enable, disable, theme, or replace SDDM or another display manager. Never programmatically change its default session or remembered selection; the display manager may remember a session the user selects through its normal UI.
- [ ] Login-screen theme selection belongs entirely to the user through their chosen tools; AeroShell supplies no SDDM theme installer, configuration module, or theme-switching hook.
- [ ] AeroShell has its own desktop identity, startup lifecycle, settings profile, and session services. KDE technology may be reused as dependencies without making AeroShell a theme applied to the Plasma session.
- [ ] AeroThemePlasma's full desktop appearance is set up on first login: wallpaper, icons, fonts, cursor, colors, shell style, taskbar, Start menu, widgets/gadgets, window decoration/effects where Wayland-safe, sounds, notifications, and lock-screen appearance. The user does not need to install or activate a theme after login.
- [ ] AeroThemePlasma's selected appearance assets and shell components are packaged as AeroShell-owned resources. Plasma-only pieces are ported to AeroShell's native shell or hosted in a private AeroShell process; installing a plasmoid alone does not count as working widget support.
- [ ] Every required first-login resource is bundled and selected automatically; the user has explicitly directed that asset-rights verification is not a completion gate. Do not make fonts, wallpaper, icons, or any other required appearance item depend on user import or setup.
- [ ] Existing Plasma packages remain package-manager-owned, unmodified, and usable.
- [ ] AeroShell panels, plasmoids, wallpaper, window decorations, KWin effects, shortcuts, fonts, icons, and shell settings do not overwrite KDE Plasma's settings.
- [ ] AeroShell presents its default AeroThemePlasma desktop composition and widgets without first-run manual theming. All appearance and widget customization stays in AeroShell's profile.
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

The `Plasma/6.7` README describes AeroThemePlasma as a Windows 7 appearance for KDE Plasma, says its Wayland support is incomplete, and points to Plasma 6.8 for planned support. It also carries a hiatus notice with an unknown resumption date. The KWin Components project defaults its Wayland build option off, and an open upstream issue reports a deprecated Wayland startup helper. AeroThemePlasma is nevertheless the required design and component source for AeroShell's default appearance; it is not the independent DE implementation or an accepted Wayland session baseline. Port or isolate its needed shell components and widgets. The public GitHub repositories are read-only mirrors; record exact full source commits and canonical source URLs before incorporating upstream assets or components. See `docs/upstream-audit.md` for the partial web audit. The `Plasma/6.7` branch's Wayland launcher sets KDE shell/config variables and starts the distro `startplasma-wayland`; its entry identifies itself as `KDE`. AeroShell's own launcher and shell must not use that session chain.

The upstream Wayland CMake rules install a launcher into the bindir and a desktop entry into `wayland-sessions`. The upstream `install.sh` builds and installs multiple projects using `-DCMAKE_INSTALL_PREFIX=/usr`, runs `update-mime-database` on a global path, and builds an SDDM configuration module alongside a fork of `libplasma`. Its installation documentation warns that KDE updates can break the modifications and that uninstalling requires reinstalling the distro's `libplasma`. Do **not** execute the upstream installer on the development host until the installation has been made non-destructive.

### Upstream components to inventory

| Component | Expected role | Initial treatment |
|---|---|---|
| In-repository Qt Quick shell | AeroShell desktop surface, panel, launcher, task list, widgets, and app launch | Required core; use KWin Wayland and KWayland's Plasma shell/window-management protocols without starting `startplasma-wayland`; port default AeroThemePlasma widgets here |
| `aerothemeplasma` | Theme design, shell visuals, widgets, and appearance assets | Required default design/source; audit and adapt component-by-component; never import its Plasma session launcher or shared Plasma defaults |
| `aeroshell-workspace` | Shared AeroShell widgets, artwork, configuration, and workspace integration | Required source candidate for default appearance/widgets; privately install and audit each install action and config write |
| `libplasma` fork | Aero-specific Plasma APIs used by upstream applets/components | Required only for selected widgets that cannot yet be ported; isolate in a private widget host loaded only by AeroShell processes |
| `aeroshell-kwin-components` | Custom KWin effects and integrations | Required for each effect that forms part of the default appearance and passes Wayland/ABI/isolation audits; replace unsafe effects with an AeroShell-native equivalent |
| `smod` / glow components | Window decoration and effects | Required default appearance component where it passes Wayland and private KWin plugin-loading checks; otherwise implement an equivalent Aero decoration/effect |
| `uac-polkit-agent` | Aero-styled authentication UI | Required if authentication and single-agent ownership pass; otherwise adapt a safe session-owned agent to the Aero appearance |
| `aeroshell-sddm-kcm` | SDDM configuration UI | Exclude entirely from builds, packages, dependencies, and setup flows |
| `aerothemeplasma-icons` / `-sounds` | Default icon and audio assets | Required defaults; selection stays inside AeroShell |
| AeroShell GTK style | GTK application styling that matches AeroThemePlasma | Required appearance component; selected only for apps launched from AeroShell |

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
    ├── distro kwin_wayland compositor and xwayland
    ├── independent Qt Quick AeroShell shell; KWin receives desktop/panel roles
    ├── distro Qt/KDE/KWayland dependencies; no private libplasma in the core
    ├── preconfigured AeroThemePlasma appearance, icons, fonts, wallpaper, widgets, and session-only services
    └── isolated KWin and AeroShell profile; user apps keep original XDG roots
```

### Installation layout

```text
/opt/aeroshell/
├── bin/aeroshell-shell
├── libexec/aeroshell-shell-session
└── share/aeroshell/          # required, audited AeroThemePlasma visuals and widget resources

/usr/bin/aeroshell-session-wayland
/usr/share/wayland-sessions/aeroshell.desktop
# Optional later: /usr/share/xsessions/aeroshell.desktop

~/.config/aeroshell/kwin/config/
~/.local/share/aeroshell/kwin/data/
~/.cache/aeroshell/kwin/cache/
~/.local/state/aeroshell/kwin/state/
# AeroShell appearance and widget layout live below ~/.config/aeroshell/.
```

**Ownership rule:** Only uniquely named package integration files may go under `/usr` (for example `/usr/bin/aeroshell-session-wayland` and a standard desktop-session `.desktop` file). No existing system paths may be overwritten, and no file owned by another package may be claimed.

## 4. Implementation phases

### Phase 0 — Inventory, safety checks, and reproducible baseline

- [ ] Use `SamuelFontes/aero-shell-desktop` as the project repository for standalone integration, packaging, patches, and documentation. Track AeroThemePlasma's `Plasma/6.7` branch as a pinned upstream source; record the exact commit SHA.
- [ ] Pin full SHAs for all repositories currently fetched by `install.sh` that provide a required default visual or shell component.
- [ ] Inventory Wayland startup support and classify every shell feature, decoration, effect, and helper as Wayland-ready, requiring a port, or deferred. Do not assume the upstream X11 launcher establishes Wayland support.
- [ ] Define the minimum Wayland feature set before packaging; X11-only visual extras must not block a usable Wayland desktop.
- [ ] Record host Plasma/Qt/KF6/KWin versions and compare them with upstream's tested versions.
- [ ] Save `pacman -Q`, `pacman -Qkk` results for core KDE packages, and a snapshot of relevant KDE settings **before testing**.
- [ ] Enumerate every `install(...)` in the root and dependent projects, including absolute paths, `KDEInstallDirs`, shell scripts, fonts, MIME files, DBus services, desktop entries, user services, polkit, and SDDM files.
- [ ] Build each component with `DESTDIR` staging; inspect file lists before any host installation.
- [ ] Run the first end-to-end tests in an Arch/Manjaro VM or disposable test user on a test system, **never by overwriting the production KDE install**.
- [ ] Classify each upstream change as: required private appearance/widget asset, private library/plugin, session-only setting/service, or unacceptable global mutation. Maintain a checklist of what must be visible at first login.

**Deliverable:** `docs/install-inventory.md`, dependency matrix, pinned versions, and staged-file inventory.

**Exit criterion:** Every installed path and every system-wide side effect has an identified owner and isolation strategy.

### Phase 1 — Separate installation and packaging roots

- [ ] Add a build option such as `AEROSHELL_STANDALONE=ON`.
- [ ] Build and install the repository-owned Qt Quick shell as the desktop surface; do not start `startplasma-wayland` or `plasmashell`.
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

### Phase 2 — Prove stock runtime loading and keep the core independent of `libplasma`

- [ ] The core shell uses distro Qt, KWaylandClient, KIO, KService, and KWin. Verify this dependency set and do not add a private `libplasma` fork to the core.
- [ ] Record direct and transitive ELF dependencies and prove they resolve from the distro package paths.
- [ ] Keep `LD_LIBRARY_PATH`, `QT_PLUGIN_PATH`, and QML import path changes out of global environment files. The current shell embeds its QML module and needs no private loader-path injection.
- [ ] For each required AeroThemePlasma widget/effect using patched `libplasma`, map the exact API need and prove only an AeroShell-owned process loads it. Otherwise port the component into the native shell.
- [ ] Resolve discovery for every default SMOD/KWin effect per AeroShell KWin process; never install effects where stock KDE KWin loads them automatically.
- [ ] Validate with `readelf -d`, `ldd`, `/proc/<pid>/maps`, and Qt/KDE plugin debugging (`QT_DEBUG_PLUGINS=1` where appropriate).
- [ ] Confirm ordinary applications launched from AeroShell retain the user's existing XDG settings paths; KWin alone receives the private XDG profile.
- [ ] Use KWin directly as the session compositor; do not route startup through `startplasma-*`.

**Deliverable:** A test harness starting the AeroShell Qt Quick shell under distro KWin and a report showing distro ELF/plugin paths. Begin with a nested session, then validate a real login session on a disposable system.

**Exit criterion:** AeroShell and stock KDE resolve the distro Qt/KWin runtime; no private library or plugin override reaches stock Plasma; distro `libplasma` remains untouched.

**Stop condition:** Do not proceed to an installable package if core desktop behavior requires replacing host KDE packages or globally injecting an incompatible loader path.

### Phase 3 — Isolate user configuration and session state

- [ ] Define an AeroShell settings policy. KWin gets session-scoped `XDG_CONFIG_HOME`, `XDG_DATA_HOME`, `XDG_CACHE_HOME`, and `XDG_STATE_HOME` below the user's `aeroshell/kwin` roots; the Qt Quick shell and user apps restore the original XDG roots. AeroShell-owned settings use only the private profile.
- [ ] Audit `plasmashellrc`, `plasma-org.kde.plasma.desktop-appletsrc`, `kwinrc`, `kdeglobals`, `kscreenlockerrc`, `kglobalshortcutsrc`, appearance, power, notifications, and related state; confirm actual paths from source/runtime traces.
- [ ] Evaluate `XDG_DATA_HOME` isolation carefully: globally changing it can hide existing `~/.local/share/applications`, icons, or other user-installed items. Implement explicit shared paths or narrower file-level isolation where needed.
- [ ] Identify programs that ignore XDG paths or hardcode `$HOME/.config` / `$HOME/.local/share`; patch or wrap those only if necessary.
- [ ] Ensure the first-run wizard writes **only** AeroShell settings. It must never overwrite stock KDE configurations. Any future import must copy selected values into the AeroShell profile without modifying the source.
- [ ] Audit the shared systemd user manager and D-Bus activation. Use uniquely named AeroShell units, session targets, and private service names where possible; do not enable AeroShell services globally or replace KDE units/activation files.
- [ ] Scope environment changes to AeroShell processes. Do not rely on an exit trap to restore shared activation state after a crash. Any unavoidable shared activation-environment change requires a documented recovery owner and tested recovery after forced termination; otherwise redesign the startup path.
- [ ] Stop only services owned by the AeroShell session; never stop, restart, or reconfigure unrelated KDE or user services.
- [ ] Avoid simultaneous KDE + AeroShell graphical sessions for the same user during initial testing unless service/lifetime isolation is explicitly verified.
- [ ] Select the default AeroShell font in the session with scoped fontconfig and QML font-rendering options, never `/etc/environment` or shared per-user fontconfig. Noto Sans is bundled as an operating-system dependency and selected automatically; a missing font or first-login import is not an accepted default.
- [ ] Define whether user-installed plasmoids, launchers, and app defaults are shared or isolated; favor protecting KDE settings over silently losing access to apps.

**Deliverable:** Repeatable first-run profile provisioning, documented shared-versus-private settings policy, and session state cleanup.

**Exit criterion:** Customizing AeroShell does not change stock KDE's panel, theme, window behavior, keyboard shortcuts, or wallpaper—and vice versa.

### Phase 4 — Independent desktop startup and lifecycle

- [ ] Add **AeroShell (Wayland)** to `/usr/share/wayland-sessions/` with a unique launcher under `/usr/bin`.
- [ ] Start `kwin_wayland --xwayland --exit-with-session /opt/aeroshell/libexec/aeroshell-shell-session` from the unique Wayland session entry. Do not start Plasma's session manager or shell.
- [ ] Give AeroShell its own session identity with `XDG_CURRENT_DESKTOP=AeroShell`, `XDG_SESSION_DESKTOP=aeroshell`, and `DESKTOP_SESSION=aeroshell`; configure portal/backend selection for that identity without changing Plasma's entry or user-manager environment.
- [ ] Use the KWin Wayland shell and window-management protocols to create desktop, panel, popup, and task-list surfaces. Fail clearly if required compositor protocols are absent.
- [ ] Start with AeroThemePlasma's full default desktop composition: wallpaper on every monitor, icon theme, Aero font, taskbar, Start menu, system tray, clock, show-desktop control, and shipped widgets/gadgets. Persist layout and appearance only in AeroShell's profile.
- [ ] Inventory every upstream widget/gadget intended for the default desktop. Port it to the native Qt Quick shell or run it in a private audited host; show enabled widgets on first login and keep their settings separate from Plasma applets.
- [ ] Select icon theme, font, cursor, and shell colors for AeroShell processes and toolkit applications where supported. Do not change global defaults or rewrite shared GTK/KDE settings.
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
- [ ] Package the appearance assets and dependencies so installation produces the complete AeroThemePlasma default setup automatically. Keep runtime resources under AeroShell-owned paths.
- [ ] Ensure install does not automatically change the default SDDM session, login theme, shell, desktop defaults, or system environment.
- [ ] Uninstall removes only package-owned binaries, session entries, and bundled resources. **Do not delete any user configuration automatically.** Provide an optional documented profile-cleanup command.
- [ ] Test clean install, reinstall, upgrade, dependency failure, rollback, and removal; ensure KDE can launch at each step.
- [ ] Add a preflight compatibility checker for Plasma/KWin/KF6/Qt version mismatches; fail clearly instead of partially installing incompatible plugins.

**Deliverable:** Locally installable `aeroshell-desktop-git` package and user-facing README.

**Exit criterion:** Removing only the AeroShell package set leaves existing KDE packages/settings untouched and removes the AeroShell login option. Test plain `pacman -R` first; recursive dependency removal is not the documented default because existing KDE dependencies must remain installed.

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
| SDDM and other display managers | No AeroShell writes to configuration, theme, services, defaults, or remembered selection; normal user-driven selection is allowed | Yes |
| Independent desktop identity | AeroShell owns its startup, profile, settings entry points, and session services | Yes |
| Failed startup / lingering user manager | No AeroShell environment, services, or settings leak into Plasma | Yes |
| First AeroShell login | Complete AeroThemePlasma default: taskbar, Start menu, widgets, wallpaper, icons, fonts, cursor, sounds, notifications, and Wayland-safe decorations/effects are configured | Yes |
| Configure Aero appearance/widgets | Defaults are active on first login; later changes affect only AeroShell's profile | Yes |
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
| X11-only upstream effects or helpers | Missing or broken Wayland visuals/features | Audit early; port every feature included in the default desktop and defer only non-default X11 extras without blocking Wayland |
| KWin plugin ABI changes frequently | Effects crash or silently fail after upgrades | Pin supported versions; rebuild and compatibility gate |
| KDE services run through a persistent systemd user manager | Settings/environment leak across sessions | Import/scrub environment deliberately; test logout-login cycles and lingering services |
| Upstream installers use absolute `/usr` paths | File conflicts with KDE packages | Audit every CMake install/manifest; patch and stage with `DESTDIR` |
| `XDG_DATA_HOME` separation hides user application entries | Missing launchers, icons, application defaults | Explicitly preserve shared app discovery or use selective isolation |
| First-run customization writes absolute home paths | Existing KDE configuration changes | Instrument writes and patch first-run tools |
| Plugin/QML search paths leak to child applications | Unexpected Qt crashes/compatibility bugs | Scope search paths to Aero processes rather than all programs |
| Fonts, MIME, polkit, or login integration change system behavior | Changes visible in regular KDE | Keep AeroShell customization session-local; exclude all display-manager customization |
| Arch rolling updates move beyond upstream branch | Build or runtime failures | Version detection, strict compatibility matrix, rebuild automation |
| A local appearance asset is unavailable on a clean install | Incomplete first-login desktop | Bundle the selected icons, cursor, wallpaper, font dependency, GTK style, and widget resources with AeroShell; user-directed rights verification is not a completion gate |

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

## 11. Implementation guide for bounded model tasks

This section turns the phases into executable work items. It specifies intended interfaces, not verified upstream capabilities. Do not invent upstream paths, flags, package names, commit IDs, or successful test results to satisfy it. Record discoveries with source commit and file/line evidence. If a discovery invalidates a proposed interface, update its decision record before implementing consumers.

### 11.1 Working rules

1. Read this plan, repository instructions, `docs/tasks.md`, and the decision record needed by the selected task.
2. Choose the first pending task whose dependencies are complete. Complete one task at a time; split work if it spans unrelated components.
3. Inspect the affected pinned upstream sources before editing. Keep upstream changes as ordered patches; do not silently edit fetched trees.
4. Implement only the selected task and its necessary documentation. Do not run upstream installers, use privileged host installation, alter SDDM, or apply a Plasma global theme.
5. Run the task's specified checks in its specified environment. Host-safe checks use fixtures or staging roots; login and package lifecycle tests belong in disposable systems.
6. Record changed files, commands, exit codes, relevant output, and remaining limitations. Mark a task done only when its acceptance evidence exists.
7. If a required tool, network connection, source, or graphical environment is missing, mark the task blocked with the exact prerequisite. Continue an independent task if available. Never report a simulated login as a real login test.

Task states: `pending`, `in-progress`, `blocked`, `done`. A blocked task includes a reason and an unblocking action. Missing test facilities mean untested, not passed.

Use this handoff template in `docs/tasks.md`:

```text
Task: Txx — title
Status: pending | in-progress | blocked | done
Dependencies:
Inputs inspected: paths and pinned revisions
Files changed:
Checks: command, environment, exit code, evidence path
Result: behavior demonstrated
Limitations / blocker:
Next task:
```

Use one implementation commit per completed task where practical. Do not commit generated build trees, private user profiles, machine identifiers, credentials, or raw home-directory dumps. Git/network restrictions do not justify changing repository metadata through an alternate location; record the restriction.

### 11.2 Fixed constraints and decisions requiring evidence

| Topic | Fixed requirement | Decision to resolve before dependent work |
|---|---|---|
| Desktop protocol | Wayland is the first release | Exact pinned startup chain and compositor invocation |
| Compositor | Reuse KWin technology initially | Distro binary versus a privately built binary, based on actual plugin/linking needs |
| Runtime prefix | `/opt/aeroshell` | Actual architecture-specific library/plugin directories from build outputs |
| Session registration | `/usr/share/wayland-sessions/aeroshell.desktop` | Version-supported desktop-entry fields and launcher behavior |
| Entry launcher | `/usr/bin/aeroshell-session-wayland` | Whether a private supervisor or audited upstream startup component owns lifecycle |
| Session identity | AeroShell owns its identity | Proposed `DesktopNames=AeroShell`, `XDG_CURRENT_DESKTOP=AeroShell`, `XDG_SESSION_DESKTOP=aeroshell`; verify compatibility before freezing |
| Configuration | AeroShell desktop settings never write into the Plasma profile | Exact XDG paths, service activation, and settings-module isolation |
| Libraries | No distro library replacement | Which private binaries must be rebuilt to bind reliably to a private fork |
| Application access | Existing files/apps remain accessible | Read-only discovery of user desktop entries without exposing KDE desktop assets as writable AeroShell state |
| Display manager | No configuration, theme, service, or default changes | None; adding the unique standard session entry is the only integration |
| Concurrent sessions | No promise of simultaneous same-user graphical sessions in MVP | Safe detection and refusal before modifying shared activation state |

Create `docs/decisions/0001-runtime.md`, `0002-profile.md`, and `0003-services.md`. Each records the question, inspected source revisions, alternatives tested, selected approach, evidence, and consequences. Unresolved decisions are explicitly labeled unresolved. A model must not choose global environment injection merely because it is easier to implement.

### 11.3 Minimum desktop versus deferred visual work

Required: independent login/logout; complete AeroThemePlasma default composition and widgets (see `docs/default-appearance.md`); usable panel/taskbar, launcher, desktop background, window decorations and window controls; notifications and tray; keyboard/mouse input; application launch; network/audio controls through existing system services; display configuration; lock/unlock; polkit prompts; portals; and a settings surface limited to AeroShell-owned desktop preferences. Existing system network/audio services may be consumed without replacing their system configuration.

Required compatibility: native Wayland and XWayland applications; clipboard and drag and drop; multi-monitor hotplug; supported scaling configurations; screenshots and screen sharing; suspend/resume. Record the exact tested hardware and versions. A VM pass cannot establish GPU or physical hotplug support.

Deferred: X11 desktop login, exact reproduction of every Windows 7 animation, and X11-only effects without a working Wayland port. Sounds, default widgets, wallpaper/icons/font selection, safe configuration, authentication, screen locking, and KDE isolation are required, not optional polish.

## 12. Ordered implementation tasks

### T00 — Establish repository and task ledger

**Dependencies:** None. **Scope:** Documentation and repository scaffolding only.

- Preserve this plan in `SamuelFontes/aero-shell-desktop`; inspect remote contents before merging local work. Never overwrite an existing remote history.
- Create `README.md` with project purpose, Wayland focus, current planning status, and no installation claims.
- Create `.gitignore` for `build/`, `stage/`, downloaded sources, and local evidence containing user data. Track source pins and patches.
- Create `docs/tasks.md` with T00–T16 and the handoff template. Create decision records with unresolved questions.

**Acceptance:** All task IDs exist with dependencies; no executable installer or host mutation is introduced. If Git is unavailable/read-only, create writable documentation and record repository setup as blocked rather than claiming connection.

### T01 — Discover and pin upstream sources

**Dependencies:** T00 documentation available.

**Outputs:** `upstream/sources.lock.json`, `scripts/validate-sources-lock.py`, `docs/source-lock-format.md`, `docs/upstream-audit.md`.

- Inspect the actual upstream branch, installer, build files, and recursively fetched projects without executing installer code.
- Each source record contains a unique component name, canonical fetch URL, full commit SHA, required/optional status, patch directory, and build dependencies. An optional license path may be retained as provenance but does not gate completion. Do not store credentials in URLs.
- Record branch/tag as explanatory metadata only; build selection uses the full pinned SHA. Reject missing sources, duplicate names, mutable-only refs, and dependency cycles.
- Validate the completed lock before any fetch or configure step. Excluded SDDM components must have no fetchable source metadata and cannot appear as a dependency.
- Record SDDM-related components as excluded; do not fetch/build them as runtime dependencies.

**Acceptance:** Every selected source resolves to a real revision; dependency graph is acyclic; upstream assertions in section 2 are either supported by evidence or corrected. Network failure blocks source verification.

### T02 — Inventory installation and Wayland feasibility

**Dependencies:** T01.

**Outputs:** `docs/install-inventory.md`, `docs/wayland-features.md`, initial `docs/compatibility.md`.

- For every component list build system, install outputs, absolute paths, custom commands, post-install actions, services, D-Bus names, plugins, and configuration writes.
- Trace actual Wayland startup from login entry to compositor, shell, agents, and shutdown. Identify environment imports and user-manager activation explicitly.
- Map every minimum desktop feature to its provider, source evidence, Wayland status, private-prefix support, and required patch or test.
- List supported candidate Plasma/KWin/Qt/KF versions as unverified until built. Do not infer compatibility from a branch label.

**Acceptance:** No selected component has unknown install behavior; required Wayland gaps are explicit tasks/subtasks. If a necessary feature requires a host KDE replacement, block that approach and document an alternative.

### T03 — Define staged-file policy and checker

**Dependencies:** T02.

**Outputs:** `packaging/install-policy.json`, `scripts/audit-install-manifests.py`, `tests/test-install-policy.py`.

- Default deny all destinations. Initially allow only files below `/opt/aeroshell/` and exact integration files `/usr/bin/aeroshell-session-wayland` and `/usr/share/wayland-sessions/aeroshell.desktop`.
- Represent additional uniquely named integration files as explicit reviewed policy entries. Private service/portal assets remain inside the prefix unless source evidence requires an integration path.
- Checker takes a staging root and policy. Check regular files, directories, executable scripts, symlinks, and canonical destination paths. Reject traversal, escaping symlink targets, forbidden `/etc` writes, SDDM assets, and unknown integration files.
- In a target Arch test environment, also compare final file destinations against the package ownership database. Shared existing directories are allowed; existing files owned by another package are not.
- Reject privileged script actions or install hooks that bypass the staged manifest. Audit CMake custom install commands; `DESTDIR` alone is not proof of safety.

**Acceptance:** Fixtures reject a KDE library overwrite, SDDM config, escaping symlink, unexpected `/usr` executable, and harmful install hook; accept a valid private tree and exact session files. Checker never follows a fixture symlink to modify the host.

### T04 — Build the smallest dependency closure privately

**Dependencies:** T02–T03. Upstream asset pins from T01 are not required to build the repository-owned shell core.

**Outputs:** root `CMakeLists.txt`, `src/shell/`, staged Qt Quick shell and session integration, build evidence.

- Build the shell against stock Qt 6, KWaylandClient, KIO, KService, and KWin runtime dependencies. Do not vendor or replace distro KDE libraries.
- Use private-prefix CMake paths and explicit session integration destinations. A representative configure/stage sequence is `cmake -S . -B <build> -DCMAKE_INSTALL_PREFIX=/opt/aeroshell`, `cmake --build <build>`, then `DESTDIR=<stage> cmake --install <build>`.
- Keep upstream appearance/widget builds as separately audited components after exact pins and their file manifests are verified; they are required contents of the complete first-login desktop, not optional customization.
- No `sudo`, `make install` to live root, implicit host package installation, or automatic interactive login. Print missing prerequisites and stop.

**Acceptance:** A clean local build and a second clean build produce equivalent staged file inventories; the current staged tree passes T03 and contains only `/opt/aeroshell` runtime files plus the two reviewed session integration entries. Bit-for-bit binary reproducibility is a separate claim.

### T05 — Prove library and plugin selection

**Dependencies:** T04.

**Outputs:** completed `docs/decisions/0001-runtime.md`, `scripts/verify-library-loading.sh`, `docs/runtime-loading.md`.

- Inventory direct/transitive ELF dependencies, Qt plugins, KWayland protocols, and KWin integration per process. Record actual binary/plugin paths and loaded mappings.
- The core shell must use the distro's KWin/Qt/KWayland/KF libraries and must not load AeroThemePlasma's `libplasma` fork.
- If optional private libraries are later necessary, rebuild only the exact AeroShell binary/plugin that needs them and verify every dynamic lookup path.
- Test a nested Wayland compositor/shell in an isolated test account with a private session bus and profile. Discover flags from installed KWin help; do not hardcode guessed flags.
- Launch representative unrelated Qt/KDE applications from AeroShell and confirm they do not inherit private library/plugin overrides.

**Acceptance:** Evidence identifies distro libraries for the shell and normal applications; no incompatible duplicate library is loaded into one process. A real session still requires T09; mocked startup or nested operation alone is insufficient.

### T06 — Implement private profile provisioning

**Dependencies:** T02, T05.

**Outputs:** session XDG profile provisioning, completed `docs/decisions/0002-profile.md`, `tests/test-session-launch.sh`.

- Compute AeroShell roots from the user's original absolute XDG base directories, falling back to standard home paths, then append `aeroshell` once. Set only KWin's XDG roots to the private `kwin/{config,data,cache,state}` subdirectories; restore the original roots before starting the shell and applications.
- Never change `HOME` or `XDG_RUNTIME_DIR`. Keep runtime files in a uniquely owned subdirectory of the existing user runtime directory with restrictive permissions.
- Create private directories without following an existing malicious/accidental symlink into the KDE profile. Copy initial defaults only when absent; keep a profile version marker.
- Repeated initialization preserves customization. Failed provisioning leaves either a complete initial profile or a retryable state. Future migrations affect only private files and preserve user values.
- Do not add all of the original user data home to generic plugin/data search paths as a shortcut: that can expose Plasma assets and activation files. Implement and test application-entry/icon discovery deliberately.

**Acceptance:** Temporary-home fixtures cover first run, second run, custom XDG bases, paths containing spaces, partial failure, and symlinks. A sentinel stock `kwinrc`, `kdeglobals`, panel layout, and user app entry remain byte-identical and accessible as intended.

### T07 — Resolve service, activation, and environment ownership

**Dependencies:** T05, T06.

**Outputs:** completed `docs/decisions/0003-services.md`, `docs/service-map.md`, private service/supervisor definitions.

- Table every service: owner, executable, activation mechanism, bus name, environment, configuration root, shutdown action, and collision behavior. Include shell, compositor, shortcuts, notifications, polkit agent, portals, power integration, and screen locker.
- Determine which reused components require the shared user bus or systemd user manager. Do not assume `dbus-run-session` alone isolates all user-manager-activated services.
- Choose explicit lifecycle ownership. Avoid importing private XDG/loader variables into the persistent user manager. If unavoidable, prove a recovery mechanism independent of the dying launcher; an EXIT trap is insufficient for SIGKILL or a compositor crash.
- Never kill by broad process name, restart global user targets, or globally enable private units. Detect unsupported same-user concurrent graphical sessions before changing shared state and fail with a useful message.

**Acceptance:** The decision includes forced termination and lingering-manager recovery tests. If required services still write into KDE or leak shared environment, T08 and packaging remain blocked.

### T08 — Implement the Wayland session launcher

**Dependencies:** T05–T07.

**Outputs:** `session/aeroshell-session-wayland`, `session/aeroshell-wayland.desktop`, private startup helpers.

Launcher sequence:

1. Verify runtime compatibility and required executables before starting shell/services.
2. Reject unsupported concurrent sessions safely; do not disturb the other session.
3. Establish the private profile and session identity using the frozen decisions.
4. Start the audited compositor/session chain with explicit child ownership and a log in private state storage.
5. Start only required AeroShell agents; report startup failures and clean up owned children/resources.
6. Wait for the session owner; propagate meaningful exit status, handle termination, and remove only resources owned by this session.

The desktop entry uses `Type=Application`, `Name=AeroShell (Wayland)`, an absolute `Exec` pointing to the unique launcher, and a validated `TryExec`/`DesktopNames` where supported. Do not embed shell pipelines in `Exec` or manipulate SDDM configuration.

**Acceptance:** Fixture tests with fake child processes cover missing dependency, failed compositor, termination, repeated launch, and unrelated process survival. Validate desktop-entry syntax. Fake processes demonstrate orchestration only, not Wayland functionality.

### T09 — Prove a real minimal Wayland desktop

**Dependencies:** T08; disposable Arch test system with an existing working Plasma session.

**Outputs:** `docs/validation/minimal-wayland.md`, required narrowly scoped fixes.

- Capture a stock Plasma baseline with the desktop logged out to avoid background settings writes confusing the comparison.
- Install only the audited staged package candidate on the disposable system and select AeroShell manually from its existing login screen.
- Exercise the login, AeroShell desktop/panel/launcher, KWin window management, keyboard/mouse, application launch, XWayland startup, and normal logout. Tray, notifications, network/audio controls, authentication, lock, and portals are completed and validated by T10/T11 after their service ownership is resolved.
- Return to Plasma and compare package ownership/files, relevant desktop settings, enabled units, activation environment, and active processes against baseline. Explain unavoidable benign differences rather than silently excluding changed files.
- Repeat switching at least three complete cycles. Record actual test output and observations.

**Acceptance:** The minimum shell works on a real Wayland login, stock Plasma retains its layout/settings, and no AeroShell process or loader override remains after logout. No SDDM writes originate from AeroShell.

### T10 — Port and enable the complete AeroThemePlasma appearance and settings

**Dependencies:** T09, T01, T02.

**Outputs:** pinned/default assets and widget ports, first-run AeroShell layout, `docs/default-appearance.md`, `docs/settings-scope.md`, feature-status updates.

- Deliver the complete widget/resource inventory in `docs/default-appearance.md`, not a placeholder blue gradient. Map every packaged upstream widget and relevant default feature to a working AeroShell implementation.
- Copy the upstream default layout/assets into the AeroShell profile on first login only. Persist later user changes, including wallpaper/font/icon choices, under AeroShell's private settings paths.
- Add one decoration/effect/settings category per subtask. Record private install locations and configuration keys before enabling it.
- Ship only settings modules demonstrated to write to the private profile. Do not expose an unfiltered generic KDE System Settings entry as the AeroShell desktop settings UI.
- Verify icon/font/theme changes stay session-local and ordinary applications remain usable. Scope toolkit appearance environment to AeroShell-launched apps, and keep it separate from library/plugin search paths.
- Treat shared application preferences such as a browser profile as shared by design; AeroShell setup must not rewrite them to enforce desktop appearance.

**Acceptance:** The complete supported AeroThemePlasma appearance and widget layout are active at first login. Each settings control changes only documented AeroShell state. Plasma appearance remains unchanged after all categories are exercised. Any Wayland-incompatible part has a named shell-native replacement or an explicit failed acceptance row; it is not silently omitted.

### T11 — Complete Wayland integration and security behavior

**Dependencies:** T09, T07.

**Outputs:** `docs/validation/wayland-integration.md`, private portal/agent integration fixes.

- Verify native and XWayland app launch, keyboard input, clipboard, drag and drop, task switching, and shortcut routing.
- Verify actual portal backend selection for AeroShell, a screen-sharing client, screenshots, and file pickers; confirm Plasma's portal configuration remains unchanged.
- Test polkit with a benign authentication request; ensure one session-appropriate agent and unchanged policy rules.
- Verify screen locking protects the session and unlocking requires authentication. Test suspend/resume and lock-on-resume according to the declared default policy.
- Test display hotplug, multiple displays, supported fractional scaling, and at least the GPU configurations claimed for release. Record unsupported hardware explicitly.

**Acceptance:** All mandatory integration rows pass with actual graphical evidence. Missing screen locking or broken authentication blocks release; passing mocked tests cannot waive this gate.

### T12 — Implement version compatibility checks

**Dependencies:** T05, T09; update after T10–T11.

**Outputs:** `compatibility/versions.json`, `scripts/check-compatibility.sh`, `tests/test-compatibility.sh`.

- Record supported distro package versions and build-time Qt/KF/Plasma/KWin identifiers, including plugin ABI requirements discovered from actual builds.
- Check both package build prerequisites and session startup runtime versions. Unknown/incompatible combinations fail before loading plugins with a diagnostic naming the component and supported combination.
- Test version comparison using the distro's version semantics; do not use plain lexical string comparison.
- Do not assume an install hook can safely veto an upgrade transaction. Encode package constraints where supported and retain a runtime gate for later dependency upgrades; document recovery/rebuild instructions.

**Acceptance:** Fixtures cover supported, older, newer, missing, and malformed version data. A disposable-system incompatible dependency test refuses AeroShell startup while Plasma remains available.

### T13 — Create managed Arch packages

**Dependencies:** T03–T12 passed for the initial release candidate.

**Outputs:** `packaging/arch/PKGBUILD`, generated `.SRCINFO` when publishing packaging metadata, `docs/install.md`.

- Build from pins plus committed patch series. Keep dependency names grounded in actual target package databases; no speculative package names.
- Stage every component into the package root, run manifest policy checks, and inspect the built archive before installing it in the test VM.
- Use unique package identity; no distro KDE replacements, SDDM dependency, theme hook, global user-service enablement, or profile writes during package installation.
- Keep uninstall limited to package-owned files; preserve user profiles. Document plain AeroShell-only removal, and warn against automatically removing pre-existing KDE dependencies through recursive cleanup.
- Fail package construction for missing required components. Do not ship partial output as a completed desktop.

**Acceptance:** Clean build succeeds; archive contains only approved paths; package dependency metadata and install scripts satisfy policy. Installation/removal in a disposable existing KDE system pass T14.

### T14 — Exercise installation and failure recovery

**Dependencies:** T13.

**Outputs:** `tests/test-uninstall.sh`, `tests/test-session-isolation.sh`, `docs/validation/lifecycle.md`, `docs/recovery.md`.

Run on disposable systems only:

| Case | Required observation |
|---|---|
| Fresh install / reinstall | KDE files and profile unchanged; exactly one AeroShell session entry |
| Upgrade with customized AeroShell profile | Customization survives; migrations touch private state only |
| Build failure / missing dependency | No partial live install or forced KDE repair |
| Unsupported runtime versions | AeroShell fails clearly before incompatible plugin load; Plasma starts |
| Shell or compositor forced termination | Owned services exit/recover as documented; next Plasma login is clean |
| Logout with lingering user manager | No private activation environment or agents remain |
| Remove package while logged out of AeroShell | Only AeroShell package files disappear; profile retained; Plasma works |
| Manual rollback to prior supported package | Documented compatibility/profile behavior; no KDE changes |

**Acceptance:** Each row records before/after evidence and pass/fail; failures become tasks and block release. Do not automate destructive tests against the developer's current desktop.

### T15 — Add repeatable build and static checks

**Dependencies:** T03, T04, T12, T13; graphical jobs depend on a real test facility.

**Outputs:** repository CI configuration, `docs/validation/README.md`.

- Separate source/policy checks, clean package builds, fixture tests, and graphical tests. Each job says what it proves.
- Run manifest, shell syntax/static checks where applicable, desktop-entry validation, profile fixtures, compatibility fixtures, and package archive inspection.
- Save sanitized reports as artifacts. A graphical test that lacks a display/GPU/login facility is skipped or blocked, never passed.
- Do not publish packages or push releases automatically as a side effect of validation.

**Acceptance:** A deliberately forbidden file fails the manifest job; a bad profile fixture fails its job; valid builds pass. CI documentation identifies which release gates require manual test-system evidence.

### T16 — Release candidate review and documentation

**Dependencies:** T10–T15; every required acceptance row passed on declared supported versions.

**Outputs:** `README.md`, `docs/install.md`, `docs/compatibility.md`, `docs/recovery.md`, `docs/release-checklist.md`.

- State supported versions/hardware and unverified combinations; separate Wayland support from XWayland application compatibility.
- Provide installation, manual session selection, settings locations, logout, upgrade/rebuild, AeroShell-only removal, and optional private-profile cleanup instructions.
- Explain that SDDM theme selection is entirely user-managed and outside this project. Do not include a theme-changing command or bundled SDDM setup step.
- Link evidence for all required acceptance rows. Clearly list deferred effects and known limitations.

**Acceptance:** A reviewer can reproduce the supported install/switch/remove flow from the documentation and trace each safety claim to evidence. Preparing a release candidate does not itself authorize publication.

## 13. Task dependencies and execution prompts

```text
T00 → T02 → T03 → T04 → T05 → T06 → T07 → T08 → T09
T01 → required upstream appearance/assets and widget inventory (T10)
T09 → T10     
T09 + T07 → T11
T05 + T09 → T12
T03–T12 → T13 → T14
T03 + T04 + T12 + T13 → T15 
T10–T15 → T16      
```

The task headings are authoritative for dependencies; the sketch shows the main sequence. T01 pins only upstream content that is actually selected, and does not block the repository-owned Wayland shell core. T12 starts from the minimal working session and is updated for features added in T10/T11. Never bypass an isolation gate to start packaging early.

Reusable implementation prompt:

```text
Implement task Txx from plan.md in SamuelFontes/aero-shell-desktop.
Read repository instructions, the task ledger, and its dependency evidence first.
Keep Wayland primary. Never modify SDDM or existing KDE packages/settings/services.
Use pinned upstream sources; do not execute upstream install scripts.
Implement the task's listed outputs and run its scoped acceptance checks.
If blocked, record the actual failure and prerequisite; do not invent a workaround
that changes shared KDE state. Update docs/tasks.md with evidence and next work.
Stop after completing this bounded task and report files, checks, and limitations.
```

For tasks requiring architectural decisions (T05/T07), first produce the evidence-backed decision record, then implement the selected approach. If a model cannot resolve an ABI, activation, or lifecycle issue, preserve its minimal reproduction and escalate that specific decision for review; other completed task work should remain usable.

## 14. Evidence and definition-of-done discipline

Store shareable reports under `docs/validation/` and generated local artifacts under ignored `build/evidence/`. Each report records source pins, patches, package/kernel/GPU versions where relevant, environment type, commands, outcomes, and remaining gaps. Sanitize usernames, paths, bus dumps, and logs before committing evidence.

Use explicit evidence labels: `source-inspected`, `fixture-tested`, `staged-build-tested`, `nested-session-tested`, `real-login-tested`, and `physical-hardware-tested`. A weaker label cannot stand in for a stronger acceptance requirement.

Before declaring the desktop ready, all of the following must hold:

- Every required acceptance row links to evidence from the declared supported environment.
- No unresolved required Wayland feature, KDE write, package conflict, stale activation variable, or orphaned AeroShell agent remains.
- No install/uninstall hook touches display-manager configuration or the Plasma profile.
- Stock Plasma has been exercised after install, AeroShell customization, forced failure, upgrade, and removal.
- The package remains an independent desktop session with the required AeroThemePlasma appearance active; display-manager branding remains user-managed.
