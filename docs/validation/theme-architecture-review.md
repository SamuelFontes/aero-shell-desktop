# Theme architecture review — 2026-10-17

The current implementation is a custom desktop using selected AeroThemePlasma
assets. It does not reproduce the original theme's runtime composition. This
explains why loading the artwork successfully is insufficient to establish
equivalent appearance or behavior.

This review compares repository source with the local reference snapshot at
`/home/sfontes/Downloads/aerothemeplasma`. That snapshot is evidence of the
reference implementation, not a verified current upstream release. No changes
to the installed desktop or user settings were made.

## Findings, in priority order

### 1. Original widget behavior has been replaced rather than integrated

`src/shell/qml/LauncherWindow.qml` uses a custom application list backed by
`src/shell/applicationmodel.cpp`. The reference SevenStart `contents/ui/main.qml`
uses Kicker's RootModel, RecentUsageModel, RunnerModel and DragHelper. The
replacement's sorted application list and text filtering cannot provide those
features simply by displaying SevenStart's SVGs.

Similarly, `src/shell/qml/PanelWindow.qml` and `src/shell/taskbarmodel.cpp` replace
SevenTasks. The reference uses TaskManager.TasksModel, its applet backend,
thumbnail tooltips, manual sorting and desktop/activity integration. AeroShell
implements its own grouping and menus. Behavior parity has to be implemented
and tested explicitly; artwork reuse does not transfer it.

The decision to avoid plasmashell is explicit in
`docs/decisions/0001-runtime.md`. A separate session/profile does not, by itself,
prove that every widget needs a replacement. The feasibility of privately
hosting the reference components needs to be revisited before further broad
UI rewrites. Hosting is a proposal, not a verified compatible solution.

### 2. Application controls use different style engines

Reference `plasma/look-and-feel/authui7/contents/defaults` selects
`widgetStyle=kvantum`. AeroShell instead selects Qt Quick Controls Basic in
`assets/qtquickcontrols2.conf`; `src/explorer/main.cpp:17` explicitly forces
Fusion for Explorer, Recycle Bin and Task Manager. Changing colors and icons
does not give these controls Kvantum's rendering or metrics. The Quick Controls
configuration also does not configure Qt Widgets applications.

The original application style must be packaged, selected and validated through
an application-specific configuration strategy before claiming desktop-wide
theme parity. Removing the Fusion override alone is not a complete fix.

### 3. Window decoration is a substitute

The same reference defaults select `library=org.smod.smod` and `theme=SMOD`.
`assets/defaults/kwinrc` selects Aurorae with `Se7enAero`. The bundled original
Aero Glass effect does not make the substitute decoration equivalent to SMOD.
The reference also selects `thumbnail_seven` as the window switcher, which the
project's KWin defaults do not select.

SMOD and the reference switcher need dependency/ABI and private installation
validation, or their substitutes must be documented and evaluated as gaps.

### 4. Theme configuration stops at the compositor profile

`session/aeroshell-session-wayland:166` gives KWin private XDG roots and seeds
its `kdeglobals`. `session/aeroshell-shell-session:19` restores the user's
ordinary XDG roots before launching the shell, and publishes that environment
to D-Bus activation. Applications consequently do not inherit the private
KWin `kdeglobals`. The shell explicitly sets its own palette/icons/font, which
can hide this split in shell screenshots.

This establishes a configuration inconsistency, not proof that every child
application is visibly broken. Actual styling depends on each application's
toolkit and platform integration. A correction must preserve normal user data
and application preferences while providing session-scoped theme settings;
blindly redirecting all application XDG roots is not an adequate fix.

### 5. Passing checks do not establish the requested equivalence

`ctest --test-dir build --output-on-failure` passed all 15 tests during this
review, using the existing build. No source rebuild or real display-manager
login was performed.

`tests/test-default-appearance.py` checks source strings and asset presence,
including explicitly requiring the substitute Aurorae decoration.
`tests/test-qml-startup.cpp` supplies stub backends and runs offscreen.
These are useful regression checks but do not compare against reference
appearance or verify complete live workflows.

`tests/test-wayland-session.sh` constructs its own environment rather than
executing the production session launcher. It sets the saved user XDG roots
to the same test roots as KWin, masking the production configuration split.
It disables locking and activities and checks geometry, effect loading and
capture existence. It is not full session acceptance evidence.

## Recommended correction order

1. Establish a reference component inventory from the working KDE setup:
   widgets/backends, application style, decoration, switcher, effects and their
   settings. Record intentional differences explicitly.
2. Validate reuse of reference widget/runtime components under the project's
   separate session and profile requirements. If a component needs porting,
   retain its behavior and dependencies deliberately rather than replacing it
   with a smaller lookalike without parity criteria.
3. Resolve application theme configuration and package/select the intended
   style and decoration. Keep the existing KDE profile untouched.
4. Exercise the production launcher in isolation, including the real split
   between compositor and application profiles. Compare Qt Widgets, Qt Quick,
   GTK, menus and window decorations against the reference.
5. Verify actual interaction workflows: launcher search/recent items, task
   grouping/activation/previews, tray menus, application launches, lock/unlock,
   scaling and multiple displays. Keep screenshot and unit checks, but stop
   treating them as evidence of complete theme integration.

The README's claim of a “complete AeroThemePlasma appearance” is stronger than
the implementation and validation support. This review supersedes that claim;
the earlier integration report remains evidence for its narrower SVG and
compositor-effect work.
