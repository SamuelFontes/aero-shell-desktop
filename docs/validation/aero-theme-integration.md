# AeroThemePlasma integration repair — 2026-10-16

The reported screenshot showed a shell-painted gradient, a small Start orb,
prototype button chrome, centered tray controls, and a launcher that did not use
SevenStart's artwork. Merely selecting the icon theme and palette did not load
the desktop theme.

The panel now renders the original Seven-Black SVG frame with its south-edge
prefix. SevenTasks supplies normal, active, hover and grouped-window frames;
Show Desktop and tray hover states also use their original frames. The panel
height is 40 logical pixels, as in upstream's taskbar layout. The window list
absorbs spare width, while the tray has a fixed content width. Regression checks
exercise 800, 1280 and 1920 pixels. The orb uses its original 54-pixel frame and
retains its pressed appearance while Start is open.

SevenStart supplies the launcher glass frame, list/sidebar highlights, search
field and power-button artwork. The launcher uses the original 254-pixel list
column and adjacent sidebar proportions. Application names elide inside their
column. Original icon bitmaps take precedence; shell-created missing-name icons
live in a separate inherited fallback theme.

An actual compositor screenshot exposed a further missing dependency: the glass
assets expect AeroThemePlasma's compositor effect. The earlier implementation
only enabled stock blur and requested a retired KWayland blur interface. The
original Wayland Aero Glass effect is now bundled with its shaders and reflection
textures, pinned in `src/effects/aeroglassblur/UPSTREAM.md`, built against KWin
6.7.4, and installed below AeroShell's private prefix. KWindowSystem negotiates
`ext-background-effect-v1`. Existing prototype profiles migrate once to Aero
Glass with the upstream Sky preset, preserving existing tint values. Stock blur
is disabled because the two effects conflict.

The adaptation removes the shared-memory connection to Plasma's color KCM.
All effect configuration comes from AeroShell's private KWin profile. No shared
KDE configuration, global plugin or display-manager configuration is changed.
The build explicitly generates the plugin factory metadata and compiles the
Wayland protocol C definitions; unresolved symbols fail linking. A load test
checks both factory metadata and library loading.

## Evidence

The native virtual KWin run records `aeroglassblur` in its loaded effects,
successfully loads the original Aero shaders, and reports the taskbar frame at
`[0,680,1280,40]`. `build/aero-glass-native/start-composite.png` shows the actual
composited Start menu, taskbar, blur, tint and reflections. QML surface grabs
alone cannot prove compositor effects; they intentionally contain transparent
areas. The final package and extracted-package captures are recorded alongside
this report when validation completes.

The asset tests render the original frame prefixes with KSvg and verify that
original folder icons win at 16, 22, 32 and 48 pixels. Session tests check private
plugin discovery and migration while preserving an existing tint and unrelated
custom settings. QML tests open the panel popovers and launcher and verify tray
geometry at three widths.

Vulkan headers were supplied from an isolated temporary checkout to build the
KWin plugin on this host; they are a declared build dependency, not a runtime
requirement. Runtime dependencies were checked by makepkg; the build was then
run with `--nodeps` because those supplied headers are not installed in pacman's
database. No system package was installed during this repair.

This verifies the reported taskbar/theme integration in a virtual compositor.
A real display-manager login, hardware workflows, authentication, lock/unlock,
and multi-monitor/HiDPI acceptance remain separate checks. The fixture has no
audio server or independent PolicyKit login, so its sound/agent messages do not
establish those workflows. Qt also reports malformed unused paths/references
inside the unmodified upstream SVGs; the used frames render and pass checks.
