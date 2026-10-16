# AeroThemePlasma is the AeroShell default

AeroThemePlasma is the mandatory, preconfigured appearance of AeroShell. A new account gets its wallpaper, icons, font, cursor, GTK styling, panel, Start menu, and supported widgets automatically. The user does not need to install or activate a theme, select an icon pack, import a wallpaper, or add widgets. AeroShell may offer customization after first login, and all saved preferences stay in AeroShell's private profile.

The Wayland session is a separate desktop entry running AeroShell's Qt Quick shell under KWin. It does not start `plasmashell`, run an AeroThemePlasma setup wizard, change SDDM, write KDE defaults, or replace KDE packages. The package owns its session launcher and private runtime assets; the user's login screen remains under the user's control.

## First-login defaults

| Appearance/component | Installed default | First-login behavior |
|---|---|---|
| Wallpaper | AeroThemePlasma's `Wallpaper1.jpg` | Automatically selected for the desktop on each display unless the AeroShell profile already contains a user-selected wallpaper. |
| Color scheme | AeroThemePlasma's `AeroColorScheme1.colors` | Loaded into AeroShell's Qt palette; GTK receives the matching light silver and Aero blue style. |
| Icons | Complete `Windows 7 Aero` icon tree in `assets/icons/` | Selected within AeroShell; the full pack is bundled, along with scalable Start, application, folder, and status icons. Adwaita/hicolor supply names not present in the theme. |
| Font | Noto Sans | Selected for AeroShell QML; session fontconfig gives it priority for child applications. The package declares `noto-fonts`. |
| Qt Quick applications | Basic Controls style with AeroThemePlasma palette and Noto Sans | The session sets `QT_QUICK_CONTROLS_CONF` to the bundled config for child applications; applications that force their own style may override it. |
| Cursor | `aero-drop` Xcursor theme | Selected for KWin and session applications with `XCURSOR_THEME=aero-drop`; default size is 24 and can be overridden with `AEROSHELL_CURSOR_SIZE`. |
| GTK style | Private `AeroShellAero` CSS | Selected for session children through `GTK_THEME`; the session does not rewrite GTK configuration files. |
| Window decoration | Bundled Se7en Aero Aurorae theme | Seeded into a private `kwinrc` on first login and loaded by the stock KWin Aurorae plugin. Existing AeroShell decoration preferences are preserved. |
| Wayland effects | KWin blur and glide effects | Enabled in the private first-login KWin profile; visual effect availability and rendering still need a live Wayland check. |
| Lock screen | AeroThemePlasma-derived `org.aeroshell.desktop` KScreenLocker package | Selected in the private KScreenLocker config, with the Aero wallpaper, five-minute idle lock, password requirement, and lock-on-resume. The Start menu requests KWin's secure locker over AeroShell's private D-Bus and checks that it became active. |
| Desktop | Wallpaper surface and XDG Desktop folder entries | Created for each display at shell startup; existing desktop files remain in the user's normal Desktop folder. |
| Taskbar and Start menu | Aero glass panel, Start orb, app search/list, window tasks, show-desktop control, user-folder shortcuts, and confirmed suspend/restart/shutdown actions | Created by default. Task buttons support activation, minimize/restore, maximize/restore, and close; power actions are enabled only when logind reports them available. |
| Panel layout | Network, audio, battery, tray, keyboard, notifications, and clock widgets | All supported widgets start enabled. Users can hide or restore them in AeroShell Settings; preferences are stored only in `~/.config/aeroshell/settings.ini`. |
| Appearance settings | AeroShell Settings font and shell icon theme selectors | Users may customize those choices after login and restore the Noto Sans/Windows 7 Aero defaults. Choices affect the AeroShell interface only and are stored in the AeroShell settings file. |
| Clock/calendar | Panel clock and interactive month calendar | Created by default. |
| System tray and notifications | StatusNotifier icon area, notification service, notification center, and toast popup | Created by default. AeroShell claims `org.freedesktop.Notifications` only when no existing owner is present; Plasma or another running service retains ownership. |
| Network | NetworkManager status, saved profiles, Wi-Fi scan and radio, open/WPA2/WPA3 personal connection flow | Created by default when NetworkManager is available. WEP and enterprise authentication are not supported yet. |
| Audio | Volume and mute control through `wpctl` or `pactl` | Created by default; unavailable controls report their state. |
| Battery | Power-supply status and level | Created by default on supported hardware and hides cleanly on systems without a battery. |
| Keyboard layout | Active KWin keyboard layout indicator and selector | Created by default. |
| Desktop sounds | Bundled Windows 7 sound theme in AeroShell's private data tree | GTK 3 event sounds use the session-selected theme and libcanberra module. AeroShell plays login/logout and notification sounds; the lock screen plays its bundled successful-unlock sound. No shared system or KDE sound setting is changed. |

## AeroThemePlasma widget coverage

The built-in widgets listed by the local AeroThemePlasma snapshot are represented by native AeroShell components. Every entry in this table is part of the first-login composition; none needs a Plasma widget host, first-run wizard, or manual add step.

| AeroThemePlasma component | AeroShell default equivalent | First-login state |
|---|---|---|
| `io.gitgud.wackyideas.desktopcontainment` | Per-display wallpaper surface and XDG Desktop folder items | Created on each display |
| `io.gitgud.wackyideas.SevenStart` | Three-state Aero Start orb and searchable application menu | Visible on each panel |
| `org.kde.panel` and `io.gitgud.wackyideas.seventasks` | Aero glass panel, KWin-backed task list, and show-desktop control | Visible on each display |
| `io.gitgud.wackyideas.digitalclocklite` | Panel clock with interactive calendar | Enabled |
| `io.gitgud.wackyideas.networkmanagement` | NetworkManager status, Wi-Fi scan, and connection controls | Enabled when NetworkManager is installed |
| `io.gitgud.wackyideas.volume` | PipeWire/PulseAudio volume and mute controls | Enabled when an audio control provider is available |
| `io.gitgud.wackyideas.battery` | Battery status and level | Enabled when battery hardware is present |
| `io.gitgud.wackyideas.keyboardlayout` | KWin keyboard layout indicator and selector | Enabled when KWin exposes the layout interface |
| `io.gitgud.wackyideas.win7showdesktop` | KWin show-desktop toggle at the end of the panel | Visible |
| `org.kde.plasma.systemtray` | AeroShell StatusNotifier watcher, icons, actions, and remote D-Bus menus | Enabled |
| `org.kde.plasma.notifications` | AeroShell notification service, center, and toast | Enabled; it claims the session-bus name only if unowned |

Hardware/service-dependent widgets stay in the default layout and degrade cleanly where their provider or device is absent. AeroShell provides the corresponding native implementations; installing these Plasma packages alone would not satisfy this mapping.

The shell's palette, GTK styling, panel, Start menu, and widgets are native AeroShell components implementing the AeroThemePlasma desktop design. The installed icon pack, original theme wallpaper, font, and cursor are already selected. These are defaults, not optional theme packages or a future setup checklist.

## Runtime isolation

* AeroShell's KWin config, data, cache, and state roots are under the user's `aeroshell` subdirectories below their XDG roots. The shell restores the original XDG roots before launching the shell and ordinary applications.
* `GTK_THEME`, `FONTCONFIG_FILE`, `XCURSOR_THEME`, `XCURSOR_PATH`, and `XCURSOR_SIZE` are set only by the AeroShell session launcher. No system fontconfig, GTK setting, KDE setting, or global theme default is written.
* QML and GTK resources are read from `/opt/aeroshell/share/aeroshell`; the look-and-feel package is stored under `/opt/aeroshell/share/plasma` and is discoverable through the session-only `XDG_DATA_DIRS`. Mutable shell preferences are stored under AeroShell's own application config path. Existing user applications and files remain shared by design.
* KWin and the shell run inside `dbus-run-session`, so AeroShell compositor, notification, and tray bus names do not claim names on the concurrent Plasma session bus. The system bus remains shared for system services.
* The session entry and portal-selection file are the only shared integration files. There are no SDDM themes, modules, scripts, config writes, or dependencies in the build or package.

## Appearance work still open

The installed defaults above are active in source and staged builds. AeroShell Settings can change the shell font and icon theme and restore the AeroThemePlasma defaults without changing KDE settings. AeroShell includes a session-owned PolicyKit authentication agent and Aero-styled prompt; its live system-bus interaction still needs a benign authentication test. The tray now has an Aero-styled D-BusMenu renderer, but it has not been exercised against a real application. The private KWin profile enables Wayland blur and glide, and KScreenLocker is wired to a bundled AeroThemePlasma look-and-feel package with an Aero lock screen. Static config and QML checks pass, but effect rendering, lock, password authentication, and unlock have not been demonstrated in a live Wayland session. Qt Quick child apps receive Aero colors/font by default; applications that force another style and Qt Widgets/KDE application styling remain open. GTK 3 and AeroShell-native login, notification, and logout sounds now use the bundled Windows 7 sound theme. Qt/KDE application sound behavior remains open. A real graphical Wayland run, multi-display/HiDPI checks, and visual comparison with the upstream theme are outstanding. Do not describe the desktop as fully validated until these rows are closed with evidence.

The workspace contains a local AeroThemePlasma source snapshot and the user's installed Windows 7 Aero icon and cursor themes. GitHub access from the build environment has failed DNS resolution, so full upstream commit metadata could not be retrieved. Per the user's direction, asset-rights verification is out of scope and is not a release blocker.

## Checks

`tests/test-default-appearance.py` asserts that the wallpaper, complete icon theme, cursor, Noto Sans, GTK styling, multi-display shell components, and default widgets are selected. `tests/test-session-launch.sh` checks the cursor, font, GTK, and private KWin environment. `tests/test-theme-assets.cpp` renders the core Aero icons and verifies the desktop wallpaper. Staged-manifest checks ensure integration remains within AeroShell-owned paths plus the session and portal registrations.
