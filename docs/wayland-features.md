# Wayland feature matrix

Wayland is AeroShell's only target for the first release. The presence of a Wayland launcher or protocol client does not count as runtime proof; every row below stays unverified until exercised in a disposable graphical session.

## Startup and surface ownership

| Area | Current implementation | Evidence | Status |
|---|---|---|---|
| Session registration | Unique `aeroshell.desktop` and `/usr/bin/aeroshell-session-wayland` entry | `desktop-file-validate`, staging policy | Static pass; no display-manager selection test |
| Compositor and bus | Private `dbus-run-session` runs distro `kwin_wayland --xwayland --desktopfile aeroshell --exit-with-session <shell-wrapper>` | KWin 6.7.4 `--help`, compatibility checker, fake-D-Bus/fake-KWin launcher fixture | Arguments and bus boundary pass fixtures; no real compositor started |
| XDG profile | KWin receives AeroShell-private config/data/cache/state roots; shell restores the user's originals | Fake-KWin fixture including spaces, symlink rejection, and repeated launch | Fixture pass; no real write trace |
| Desktop/panel/popup surfaces | Qt Quick windows register `org_kde_plasma_shell` roles and the panel uses KWin's window model | Build, QML lint, protocol-presence failure paths | No Wayland surface has been created here |
| Window buttons | Activate, minimize, maximize/restore, close, and show-desktop use KWin's window-management protocol | Compile/QML lint | No real KWin model interaction |
| Window decoration | Stock KWin Aurorae plugin loads the bundled Se7en Aero theme from AeroShell's private data path; first login seeds the theme in the private KWin profile | Session fixture checks first-run config creation and symlink refusal | Theme activation and Wayland decoration rendering need a real login |

## Required interaction matrix

| Feature | Current provider | Validation status / release gate |
|---|---|---|
| Native Wayland applications | Distro compositor and application protocols | Unverified; launch an ordinary Qt app and a GTK app |
| XWayland applications | KWin `--xwayland` | CLI option verified only; unverified app launch, input, and window management |
| Keyboard and pointer input | KWin | Unverified on graphical hardware |
| Clipboard, selection, drag and drop | KWin and application protocols | No shell-owned implementation; unverified end-to-end |
| Task switching | KWin Plasma window-management protocol | UI implemented; activation/minimize behavior unverified |
| Multiple displays | One desktop and panel per `QGuiApplication` screen; `QGuiApplication` screen add/remove events reconcile the window set | Hotplug lifecycle is implemented and statically checked; multi-monitor placement, screen removal, and settings behavior still need a graphical test |
| Fractional scaling | Qt 6 and KWin | No supported scaling matrix or screenshots yet |
| Portals and file dialogs | Desktop-specific portal selector plus Qt dialogs | Selector stages; backend activation and broker environment unverified |
| Screenshots and screen sharing | KDE portal backend for capture interfaces | Unverified; confirm Plasma portal configuration stays unchanged |
| Polkit | AeroShell registers a native PolkitQt agent and displays password prompts inside its own shell | Source builds; live system-bus registration, a benign authentication request, and single-agent behavior remain unverified and block release |
| Notifications | Native AeroShell session-bus notification service, center, and toast widget | Model tests cover notification behavior but use a disconnected bus; D-Bus interoperability, owner collision handling, and Wayland rendering are unverified |
| Lock and unlock | Start menu calls KWin's `org.freedesktop.ScreenSaver.Lock`; KScreenLocker loads the bundled `org.aeroshell.desktop` greeter | Static controller/QML and first-login private-config fixtures pass; actual lock activation, password authentication, successful unlock sound, and lock-on-resume remain unverified and block release |
| Wayland effects | Private first-login `kwinrc` enables KWin blur and glide effects | Config fixture passes; confirm KWin advertises/loads them and render correctly in a live session |
| Suspend and resume | Confirmed logind action from the Start menu; system power stack | Capability and action code exists; hardware suspend/resume and lock-on-resume behavior are unverified |
| GPU configurations | Distro KWin | Only the development host has been inspected; no test matrix |

## Isolation constraints

- Never start `plasmashell` or `startplasma-wayland` from this session.
- Do not edit SDDM or another display manager's files/settings; session selection is user-managed.
- Keep KWin's mutable XDG roots under the AeroShell profile. Keep shell-owned settings in the AeroShell profile and never write Plasma theme/layout defaults.
- Do not import private loader variables or XDG profile roots into the persistent user manager.
- Do not claim clipboard, portals, authentication, locking, multi-monitor, scaling, or GPU support from static inspection or mocked tests.

The sandbox denied creation of both filesystem and abstract AF_UNIX sockets, including a private D-Bus session socket under `/tmp`; therefore KWin could not start. The real-session matrix requires a disposable Arch/Manjaro login environment. See [build and runtime evidence](validation/shell-build.md).
