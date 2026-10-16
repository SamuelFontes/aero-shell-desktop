# Compatibility matrix

This is a deliberately narrow development target, not a claim of broad Plasma compatibility. The KWin session launcher fails before creating AeroShell profiles if the detected compositor version is outside the checked range or lacks a required command-line option.

| Component | Build/runtime constraint | Observed environment | Evidence and status |
|---|---|---|---|
| Distro | Arch Linux or Manjaro | Manjaro development host | Other distributions are outside the first package target |
| KWin Wayland | Exactly 6.7.4; versions outside the range are rejected | 6.7.4 | Version and option checker, plus fake-compositor fixtures; real session unverified |
| Qt | Qt 6.8 or newer | Qt 6.11.2 | Build and offscreen theme tests pass on host; other versions unverified |
| KWaylandClient | 6.7.4 or newer in package metadata | 6.7.4 | Build pass; Wayland protocol runtime unverified |
| KDE Frameworks | KIO and KService from target distribution | KF6 from Manjaro host | Build pass; no pinned full package set |
| Architecture | x86_64 and aarch64 package metadata | x86_64 | aarch64 build and graphics behavior unverified |
| Plasma | No Plasma session is launched by AeroShell | Plasma installation present on host | The user profile and a Plasma-to-AeroShell-to-Plasma cycle have not been exercised |

Do not widen the KWin range based on version-string comparison alone. Each newly supported KWin release needs a real compositor session, protocol interaction, package build, and the relevant compatibility fixture. The current checker does not prove KWin plugin ABI stability or AeroThemePlasma component compatibility.

The supported runtime is only the installed Manjaro host's observed KWin 6.7.4 configuration. This workspace cannot start an isolated compositor because the sandbox denies AF_UNIX socket binds. The release requires the real-session matrix in [Wayland feature validation](wayland-features.md).
