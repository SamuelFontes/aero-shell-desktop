Aero Glass Blur from https://github.com/aeroshell-desktop/aeroshell-kwin-components
Branch: Plasma/6.7
Commit: 461ba76c05e78680045fd36099addac68df0b50b

The Wayland effect is bundled with its shaders and textures. The build excludes
its global KCM and installs the plugin only below /opt/aeroshell. The resource
path for outputmanagement.frag is adapted to this directory. Copyright and
SPDX notices are retained in the upstream files.

The upstream shared-memory bridge to the Plasma color KCM is removed: color
settings come exclusively from AeroShell's private KWin configuration. The
first-login preset is upstream's Sky (#6b74b8fc).
The shell resource class and Start window caption are recognized alongside the
upstream Plasma identities, so shell glass retains the correct active tint and
Start does not receive a window-corner glow.
