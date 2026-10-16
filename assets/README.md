# Bundled AeroShell appearance assets

These assets are installed with AeroShell and selected automatically by the Wayland session. AeroThemePlasma is the required first-login appearance; users do not need to install a theme, import assets, or enable widgets.

| Asset | Default behavior |
|---|---|
| `icons/Windows 7 Aero/` | Complete Windows 7 Aero icon theme, including the full installed icon tree plus AeroShell's scalable Start, application, folder, and status icons. AeroShell selects it in the shell process; GTK applications launched in the session also see the private asset path. Missing icons fall back to Adwaita/hicolor. |
| `cursors/aero-drop/` | Aero-style Xcursor theme selected for the Wayland session with `XCURSOR_THEME`, `XCURSOR_PATH`, and a default size of 24. The size can be overridden with `AEROSHELL_CURSOR_SIZE`. |
| `wallpapers/aerothemeplasma-default.jpg` | AeroThemePlasma's default `Wallpaper1.jpg`, applied automatically to new AeroShell profiles and displays. The user may choose a different wallpaper afterward. |
| `color-schemes/AeroColorScheme1.colors` | AeroThemePlasma's silver and Aero blue color scheme, loaded into AeroShell's Qt palette. |
| `themes/AeroShellAero/` | AeroShell's glass-style GTK theme, exposed to session children through `GTK_THEME`; it does not write GTK settings. |
| `sounds/Windows 7/` | Windows 7 event sound theme selected by the private GTK theme and used by AeroShell for login, logout, and notification sounds. The session loads the libcanberra GTK 3 event module only for AeroShell-launched applications. |
| `widgets/aerothemeplasma-start-orb.png` | The AeroThemePlasma Start orb sprite sheet; the panel shows its normal, hover, and pressed frames automatically. |
| `plasma/look-and-feel/org.aeroshell.desktop/` | AeroThemePlasma-derived KScreenLocker package, installed in the private `/opt/aeroshell/share/plasma` tree and selected only by AeroShell's private lock configuration. It includes the lock UI and successful-unlock sound; it does not install a Plasma desktop theme or change KDE's defaults. |
| `fontconfig/fonts.conf` | Gives Noto Sans priority and applies light hinting for AeroShell session children only. AeroShell selects Noto Sans for its own QML interface. |
| `qtquickcontrols2.conf` | Selects Qt Quick Controls' Basic style with Noto Sans and the AeroThemePlasma palette for processes launched from the AeroShell session. The session points Qt Quick Controls to this file without changing application or KDE settings. |
| `LICENSES/CC0-1.0.txt` | License text for original CC0 resources in this directory. The repository license covers AeroShell's own source and identified project-created resources. |

The Windows 7 Aero icon theme and `aero-drop` cursor theme were copied from the user's local desktop asset installations into this repository so a fresh AeroShell session has them by default. Per the user's direction, asset-rights verification is not a completion gate and has not been performed. The repository does not claim to have created those third-party assets.

The default wallpaper, color scheme, Start orb, and lock-screen package were copied from the local AeroThemePlasma source snapshot into AeroShell's private data tree. They are selected automatically; no SDDM or KDE configuration is changed. Asset-rights verification is out of scope per user direction. The Noto Sans font comes from the declared `noto-fonts` runtime dependency. The panel, Start menu, desktop items, notification center, clock/calendar, task list, show-desktop action, audio, battery, network, keyboard-layout, and StatusNotifier controls are created by the shell automatically. The default widget mapping is recorded in `docs/default-appearance.md`.

The Windows 7 sound theme was copied from the local `Windows-Sounds-for-Plasma` checkout at commit `926b5052ccb28cb8f97e26397e55e16ed1ede9e8`. AeroShell bundles it under its private data tree and does not install it into the shared system sound directory or change KDE's sound settings.
