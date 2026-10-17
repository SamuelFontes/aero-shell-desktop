# AeroShell Desktop

AeroShell runs the original AeroThemePlasma desktop in a separate, preconfigured
Wayland login session. It reuses the original shell package, SevenStart,
SevenTasks, desktop containment, tray and status widgets, Kvantum theme, SMOD
decoration, Aero effects, and Windows-7-Better GTK theme.

The implementation now builds those components privately under `/opt/aeroshell`.
It uses the distro Plasma shell and KWin with the pinned upstream Plasma library
and plugins inside that session. It does not replace KDE packages, apply a theme
to the existing Plasma profile, install an SDDM theme, or import its environment
into the shared systemd user manager.

All four XDG profile roots remain private for the complete session, including
applications and activated services: `~/.config/aeroshell/native`,
`~/.local/share/aeroshell/native`, `~/.cache/aeroshell/native`, and
`~/.local/state/aeroshell/native`. HOME and personal documents remain normal.
Application preferences in this session are consequently separate too. Existing
prototype profiles are preserved; the original desktop starts in a new profile.

First login seeds the upstream style, effects, decoration, wallpaper, cursor,
icons and original widget layout automatically. Later customizations are kept.
The upstream setup wizard is excluded because its display-manager/system
changes are unnecessary; its desktop defaults are supplied directly to the
private profile. Upstream requests Segoe UI; on systems without that font,
fontconfig substitutes an available font.

## Build and validate

The supported baseline is the installed Plasma/KWin 6.7.4 stack. Build requirements
and runtime packages are listed in `packaging/arch/PKGBUILD`. Upstream revisions
are pinned in `packaging/native-sources.json`; `patches/native` records the
private-integration and build fixes. The builder fetches those exact revisions,
never runs upstream installers, and stages every upstream file under the private
prefix.

```sh
cmake -S . -B build-original -G Ninja -DCMAKE_INSTALL_PREFIX=/opt/aeroshell
cmake --build build-original
ctest --test-dir build-original --output-on-failure
DESTDIR="$PWD/stage-original" cmake --install build-original
python scripts/audit-install-manifests.py --stage-root stage-original
python tests/test-native-wayland.py stage-original/opt/aeroshell build-original/wayland
```

`tests/test-native-wayland.py` runs the production launcher in a disposable
virtual compositor, loads the original widgets, and captures the desktop and
Start menu. It does not log out or reconfigure the active desktop. Real login,
authentication, hardware, and multi-monitor acceptance remain distinct from
this test.

The retired custom Qt Quick shell remains in the repository for reference and
can be built with `-DAEROSHELL_BUILD_PROTOTYPE=ON`. It is not the default desktop
or packaged by the default build. Earlier reports about its tests and appearance
do not describe the original-component runtime.

See [the architecture decision](docs/decisions/0004-original-desktop.md) and
[the review that prompted this correction](docs/validation/theme-architecture-review.md).
