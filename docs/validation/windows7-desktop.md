# Windows 7 desktop repair — 2026-10-16

The reported missing taskbar exposed a startup hang that the earlier offscreen UI mocks could not catch. A real virtual KWin run under GDB stopped in `KeyboardLayoutStatus::parseLayouts`: a mutable `QDBusArgument` selected the write overload of `beginArray`/`beginStructure`, so the incoming iterator never advanced. The same pattern existed in tray pixmap and D-BusMenu readers. All three readers now use const arguments and validate their wire signatures. The keyboard-layout signal also now matches KWin's actual `layoutChanged(uint)` interface.

A second placement problem appeared after the hang was fixed. A Wayland toplevel reports local `(0,0)` independently of its global Plasma-shell position. Feeding the window's reported coordinates back into the positioning protocol moved the panel to the top and put Start above the screen. The desktop, panel, Start and authentication windows now preserve their requested positions separately. A regression fixture simulates the compositor reset. Surface registration retries after mapping and guards reentrant creation. Start uses an applet/tool surface that can open from a global shortcut without an input-serial popup grab. The packaged Start orb now has a `file:` URL. Panel and popup surfaces request KWin blur.

The cursor defaults now agree: the bundled theme contains 32- and 48-pixel cursor sizes; the compositor's private `kcminputrc`, session environment and GTK settings select 32 logical pixels. Standard Wayland cursor names are aliased to the bundled shapes, and movement cursors use the actual movement shape. Existing AeroShell profiles receive the missing compositor and shortcut configuration. The old title-bar all-desktops diamond is removed when the profile still has the prototype default.

The desktop now includes grouped task buttons, pinned Explorer/browser launchers, a right-edge Show Desktop button, Start search keyboard navigation, Run and security windows, native Explorer, Task Manager and Recycle Bin. Folder shortcuts and desktop directories launch AeroShell Explorer directly. The packaged desktop-specific MIME default makes Explorer available as AeroShell's directory handler. The Explorer desktop entry stays hidden from other desktops' application menus. Explorer has breadcrumbs/address entry, Favorites/Libraries/Computer navigation, details/icons, current-folder search, selection, clipboard copy/move, recorded-operation undo, rename, new folder, properties, drag/drop, trash and confirmed permanent deletion. Keyboard defaults are listed in [the shortcut table](../windows7-shortcuts.md).

## Verification

- Build: `cmake -S . -B build` and `cmake --build build -j 4`.
- Fourteen CTests cover the existing services and appearance, a real separate-process D-Bus keyboard reply, grouped task mapping, Explorer navigation and an actual clipboard file-copy operation, shell creation/popovers and the simulated Wayland coordinate reset, and private-profile provisioning.
- `qmllint` on shell QML; desktop-entry validation; shell syntax; install-policy and source-lock fixture suites; staged-file policy audit.
- `tests/test-wayland-session.sh STAGED_PREFIX OUTPUT` runs a real virtual KWin with a private bus, temporary runtime/profile, actual application metadata, native Explorer windows, Start and Run. A KWin script reports the panel frame at `[0,672,1280,48]` on a 1280×720 output. Captures include desktop, taskbar, Start, Run and a compositor screenshot from Spectacle. Applet opening is checked without relying on input from the active desktop.
- `scripts/build-local-package.sh` builds a checksum-pinned package from the current workspace, including uncommitted changes, rather than silently packaging the older Git source.

Local outputs are under `build/wayland-verified/` and the final validation/package directories recorded in README. The virtual test intentionally lacks a real audio server; sound failures and Wayland disconnect messages during compositor teardown are expected. PolicyKit reports that the existing login session already has an agent, so this test does not prove authentication in an independent login.

## Remaining acceptance work

This is a substantial desktop repair, not complete Windows 7 parity. Real display-manager login, lock/unlock, hardware/audio/network workflows, multi-monitor/HiDPI behavior and package installation/removal still require independent validation. Explorer search is currently limited to the current folder, and full indexed search, Flip 3D, projection controls, Magnifier, gadgets and a complete Control Panel remain open. File undo currently covers recorded copy/move/trash jobs; rename and folder creation are not recorded. The source-lock and release ledger retain their earlier outstanding requirements.


## Package evidence

The clean snapshot package is `build/package-final/aeroshell-desktop-0.1.0.r6.gbeb9b90.fix20261016195406-1-x86_64.pkg.tar.zst` (SHA-256 `7ef83aa7db81627acad86502ae3db332ae59ecb9fd3a5fab78a617e096b4262b`). Its build log records 14/14 passing CTests, QML/desktop-entry checks, 17 install-policy fixtures and 11 source-lock fixtures. The extracted archive passes the 8,504-entry install audit and the real compositor smoke test (`build/packaged-wayland/`). Packaged `src/` and `session/` match the reviewed workspace. The standalone virtual-test helper subsequently gained FUSE cleanup and shortcut-registration assertions; these test-helper changes do not change the packaged runtime.
