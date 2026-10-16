# AeroShell settings scope

**Status:** The Qt Quick shell saves per-display wallpaper selections and panel-widget visibility in one AeroShell-owned settings file. A new profile enables every supported panel widget by default. KWin and KScreenLocker receive private defaults under AeroShell's KWin profile. Bundled defaults are read-only package resources. Appearance controls beyond wallpaper/widget visibility, tray, notification, display, and service settings are not complete.

## Path ownership

| State | Path/owner | Behavior |
|---|---|---|
| KWin config/data/cache/state | Under the original XDG base paths at aeroshell/kwin/{config,data,cache,state} | Set only in the KWin compositor process; session launcher refuses symlinked AeroShell profile leaves |
| AeroShell shell settings | $XDG_CONFIG_HOME/aeroshell/settings.ini; normally ~/.config/aeroshell/settings.ini | QSettings INI file; shell uses the user's original XDG_CONFIG_HOME after the session wrapper restores it |
| Wallpaper choice | appearance/wallpaper/<encoded-output-name> in settings.ini | Stores a local file path per monitor; leaves user-owned image files in place; reset removes the private override and returns to the packaged Aero default |
| Panel widget visibility | widgets/panel/<widget> in settings.ini | Seven supported taskbar widgets default to enabled; a known-key allowlist rejects arbitrary settings names; reset removes only the panel widget subgroup |
| Default wallpaper/icons/fonts/cursor/decoration/theme/widget assets | /opt/aeroshell/share/aeroshell/ | Immutable package data; never write changes into this tree |
| Default KWin appearance | `$XDG_CONFIG_HOME/aeroshell/kwin/config/kwinrc`; normally `~/.config/aeroshell/kwin/config/kwinrc` | Se7en Aero decoration is copied only when this private file is absent; an existing custom KWin config is preserved; symlinked KWin config is refused |
| Screen lock | `$XDG_CONFIG_HOME/aeroshell/kwin/config/kscreenlockerrc` | On first launch, selects the bundled AeroThemePlasma greeter and wallpaper, requires password, locks after five minutes idle, and locks on resume. Existing user-owned AeroShell lock settings are preserved; symlinked settings are refused. The lock request uses KWin on the private session bus. |
| GTK theme/font/cursor selection | Session child environment: XDG_DATA_DIRS, GTK_THEME, FONTCONFIG_FILE, XCURSOR_THEME, XCURSOR_PATH, XCURSOR_SIZE | Selects bundled CSS, font preferences, and cursor only in AeroShell session children; no GTK/KDE config is edited. KDE Qt application theming is still unresolved. |
| Qt Quick Controls defaults | Session child environment: `QT_QUICK_CONTROLS_CONF` points to `/opt/aeroshell/share/aeroshell/qtquickcontrols2.conf` | Sets Basic Controls style, Noto Sans, and the AeroThemePlasma palette for Qt Quick apps that use runtime style selection. It does not write app or KDE settings; Qt Widgets/KDE style coverage remains open. |
| Application preferences | Existing user XDG roots | Shared intentionally, so app settings remain available in both desktops |

The session launcher does not change HOME or XDG_RUNTIME_DIR. It does not import environment variables into systemd's user manager or persistent D-Bus activation environment. KWin and AeroShell run on the `dbus-run-session` bus; the system bus remains shared. `XDG_DATA_DIRS`, `GTK_THEME`, and `FONTCONFIG_FILE` are inherited by the compositor and session children only.

## Current settings UI

The Start menu has an AeroShell Settings entry. The current page supports choosing a local wallpaper per output, resetting it to the AeroThemePlasma default, hiding or showing the supported taskbar widgets, and restoring the default widget layout. The values are stored only in settings.ini. Widget keys default to enabled, so first-login composition is complete without a settings visit. A missing or unreadable saved wallpaper falls back to the bundled default resource URL.

The default wallpaper is installed at `/opt/aeroshell/share/aeroshell/wallpapers/aerothemeplasma-default.jpg` and is selected on a new profile. Asset-rights verification is not a completion gate per user direction.

## Required settings work

- First login must seed the full AeroThemePlasma widget layout and appearance defaults only when no AeroShell profile exists. The current settings UI controls wallpaper and supported panel widget visibility; other appearance controls remain incomplete.
- Later setting changes must affect AeroShell only. Wallpaper and supported taskbar widget visibility are configurable; icon, font, cursor, sound, and effect choices still need controls when their backing implementations are available.
- Font/theme overrides for shell processes are application-local. Toolkit appearance for apps launched from AeroShell must be session-scoped and must not rewrite GTK/KDE config files or global defaults.
- Window-manager settings remain below the AeroShell-specific KWin XDG roots. Verify exact KWin and plugin paths from the pinned components.
- Settings reset/migrations must preserve unrelated keys and never read then rewrite Plasma settings.
- Package removal preserves ~/.config/aeroshell and all other user state. Any optional cleanup is a separate user-invoked command with a preview.

## Evidence still required

Use a disposable account. Capture XDG paths and file writes while first-run setup creates defaults, each setting is changed, the shell is forcibly terminated, and AeroShell is removed. Compare the existing Plasma profile before and after. Do not infer safety from the current path construction or a mocked compositor alone.
