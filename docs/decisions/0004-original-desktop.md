# Decision 0004: reuse the original desktop in an isolated session

The user's clarified requirement is the behavior and appearance of KDE with
AeroThemePlasma fully configured, offered as a separate session without changing
KDE Plasma. Reimplementing the shell is not a requirement. This decision
supersedes decision 0001's rejection of plasmashell and the prototype's settings
scope, service map, and default-appearance documentation.

The default build uses the original upstream components at exact source pins.
Distro plasmashell and KWin host the private patched libplasma, widgets and
plugins. All installed upstream files stay below `/opt/aeroshell`; only the
session launcher, session entry and portal selection are system integration
files. Loader paths are exported only to the new session process tree.

The entire session retains private XDG config/data/cache/state roots. Restoring
shared configuration for child applications was incompatible with both uniform
theming and isolated settings, so application preferences are separate as well.
Normal HOME, user directories and documents remain accessible. No existing KDE
or prototype configuration is migrated or rewritten.

The launcher creates its own D-Bus session and starts the original desktop
services directly. It does not use startplasma's shared systemd startup or call
`systemctl --user import-environment`. KWin owns the compositor lifetime;
plasmashell and supporting services are supervised together. Upstream's
non-systemd shutdown path uses that session's KSMServer and KWin interfaces.

The theme's setup wizard and SDDM integration are excluded. Defaults are taken
from its XDG configuration, look-and-feel defaults and setup code and seeded
once. Its SDDM background accessor is adapted to read the private lock-screen
wallpaper. First-login layout retains the original widgets and explicitly
enables the original status applets and wallpaper.

The source patches also fix SevenStart's CMake imported library targets, stop
the tray subproject from forcing tests on, and allow workspace components to
build against the installed 6.7.4 Activities libraries. Original application
behavior and artwork remain upstream-owned. GTK uses the original recommended
Windows-7-Better companion. Segoe UI is selected as upstream requests, but is
not bundled; font substitution remains a visual difference when unavailable.

Validation must exercise the production launcher, original native plugins,
private application settings and compositor rendering. The prototype's 15
passing tests are not evidence for this runtime. Virtual tests cannot establish
real display-manager login, hardware integration or authentication correctness.
