#!/usr/bin/env python3
"""Check that a new AeroShell session selects its bundled first-login defaults."""

from pathlib import Path
import struct


ROOT = Path(__file__).resolve().parents[1]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


main = (ROOT / "src/shell/main.cpp").read_text()
root_qml = (ROOT / "src/shell/qml/ShellRoot.qml").read_text()
panel_qml = (ROOT / "src/shell/qml/PanelWindow.qml").read_text()
menu_model = (ROOT / "src/shell/dbusmenumodel.cpp").read_text()
tray_model = (ROOT / "src/shell/statusnotifier.cpp").read_text()
launcher_qml = (ROOT / "src/shell/qml/LauncherWindow.qml").read_text()
auth_qml = (ROOT / "src/shell/qml/PolkitAuthWindow.qml").read_text()
lock_controller = (ROOT / "src/shell/screenlockcontroller.cpp").read_text()
settings_qml = (ROOT / "src/shell/qml/SettingsWindow.qml").read_text()
settings_model = (ROOT / "src/shell/shellsettings.cpp").read_text()
asset_docs = (ROOT / "assets/README.md").read_text()
appearance_doc = (ROOT / "docs/default-appearance.md").read_text()

wallpaper = ROOT / "assets/wallpapers/aerothemeplasma-default.jpg"
icon_theme = ROOT / "assets/icons/Windows 7 Aero/index.theme"
cursor_theme = ROOT / "assets/cursors/aero-drop/index.theme"
aurorae_theme = ROOT / "assets/aurorae/themes/Se7enAero"
kwin_defaults = ROOT / "assets/defaults/kwinrc"
lock_defaults = ROOT / "assets/defaults/kscreenlockerrc"
lock_theme = ROOT / "assets/plasma/shells/org.aeroshell.desktop"
gtk_theme = ROOT / "assets/themes/AeroShellAero/gtk-3.0/gtk.css"
font_config = ROOT / "assets/fontconfig/fonts.conf"
start_orb = ROOT / "assets/widgets/aerothemeplasma-start-orb.png"
color_scheme = ROOT / "assets/color-schemes/AeroColorScheme1.colors"
qt_quick_controls = ROOT / "assets/qtquickcontrols2.conf"
sound_theme = ROOT / "assets/sounds/Windows 7"

for path in (wallpaper, icon_theme, cursor_theme, gtk_theme, font_config, start_orb, color_scheme,
             qt_quick_controls, sound_theme / "index.theme"):
    require(path.is_file(), f"required first-login appearance asset missing: {path.relative_to(ROOT)}")

wallpaper_bytes = wallpaper.read_bytes()
require(wallpaper_bytes[:3] == b"\xff\xd8\xff", "default wallpaper is not a JPEG")

require('"Windows 7 Aero"' in main, "the shell does not select its bundled Aero icon theme")
require('"cursors/aero-drop/cursors/left_ptr"' in main,
        "the shell startup does not require its bundled Aero cursor")
require((aurorae_theme / "metadata.desktop").is_file() and (aurorae_theme / "decoration.svg").is_file(),
        "the Aero window decoration is not bundled")
require(start_orb.read_bytes()[:8] == b"\x89PNG\r\n\x1a\n", "the AeroThemePlasma Start orb is not a PNG")
orb_data = start_orb.read_bytes()
orb_width, orb_height = struct.unpack(">II", orb_data[16:24])
require((orb_width, orb_height) == (54, 162), "the AeroThemePlasma Start orb must contain three 54-pixel state frames")
require("theme=__aurorae__svg__Se7enAero" in kwin_defaults.read_text(),
        "the first-login KWin profile does not select the Aero window decoration")
require("blurEnabled=true" in kwin_defaults.read_text() and "glideEnabled=true" in kwin_defaults.read_text(),
        "the first-login KWin profile does not enable Aero glass blur and window animations")
require('"KPackageStructure": "Plasma/Shell"' in (lock_theme / "metadata.json").read_text()
        and "LockOnResume=true" in lock_defaults.read_text()
        and "Autolock=true" in lock_defaults.read_text(),
        "the AeroShell lock-screen defaults must use the Aero look, lock after idle, and lock on resume")
require((lock_theme / "metadata.json").is_file()
        and (lock_theme / "contents/lockscreen/LockScreen.qml").is_file()
        and (lock_theme / "contents/lockscreen/LockScreenUi.qml").is_file()
        and (lock_theme / "contents/sounds/lockSuccess.ogg").is_file(),
        "the first-login Aero lock screen must include its QML and success sound")
lock_qml = (lock_theme / "contents/lockscreen/LockScreenUi.qml").read_text()
require("kreadconfig" not in lock_qml and "kdeglobals" not in lock_qml,
        "AeroShell lock screen does not read Plasma's shared sound/theme settings")
icon_paths = list(icon_theme.parent.rglob("*"))
require(sum(path.is_file() for path in icon_paths) >= 5000,
        "the full Windows 7 Aero icon theme is not bundled")
icon_index = icon_theme.read_text()
require("Inherits=Adwaita,hicolor" in icon_index and "scalable/status" in icon_index,
        "Aero icons do not register shell scalable icons and standard fallbacks")
require('"Noto Sans"' in main and "required Noto Sans font is unavailable" in main,
        "Noto Sans must be selected and missing-font fallback must fail")
require("aerothemeplasma-default.jpg" in main, "the bundled AeroThemePlasma wallpaper is not the first-login default")
require("AeroColorScheme1.colors" in main and "schemeColor" in main,
        "the AeroThemePlasma color scheme is not applied to the shell palette")
sound_index = (sound_theme / "index.theme").read_text()
require("Name=Windows 7" in sound_index and "Directories=stereo" in sound_index,
        "the bundled AeroThemePlasma event sound theme is not registered")
for sound in ("desktop-login.ogg", "desktop-logout.ogg", "dialog-information.ogg",
              "dialog-warning.ogg", "dialog-error.ogg", "device-added.ogg",
              "device-removed.ogg", "message-new-instant.ogg"):
    sound_path = sound_theme / "stereo" / sound
    require(sound_path.is_file() and sound_path.read_bytes()[:4] == b"OggS",
            f"the default AeroThemePlasma sound is missing or invalid: {sound}")
require("ShellSounds shellSounds" in main and "shellsounds.cpp" in (ROOT / "CMakeLists.txt").read_text(),
        "the shell event sound controller is not built")
require('shellSounds.play("desktop-login")' in root_qml
        and 'shellSounds.play("desktop-logout")' in root_qml
        and 'shellSounds.play(notification.urgency >= 2 ? "dialog-warning" : "dialog-information")' in root_qml,
        "login, logout, and notification events do not use the bundled sound theme")
qt_controls_config = qt_quick_controls.read_text()
require("Style=Basic" in qt_controls_config and "Font\\Family=Noto Sans" in qt_controls_config
        and "Palette\\Highlight=#418bd4" in qt_controls_config
        and "Palette\\Window=#e0dfde" in qt_controls_config,
        "Qt Quick apps need AeroThemePlasma colors and Noto Sans by default")
require("QFileInfo(requiredAsset).isFile()" in main, "missing bundled appearance assets are not rejected")
session_launcher = (ROOT / "session/aeroshell-session-wayland").read_text()
require("GTK_THEME" in session_launcher and "AeroShellAero" in session_launcher,
        "the Wayland session does not select the private GTK theme")
require('canberra-gtk-module' in session_launcher and "GTK_MODULES" in session_launcher,
        "the Wayland session does not enable GTK 3 event sounds privately")
require("gtk-sound-theme-name=Windows 7" in
        (ROOT / "assets/themes/AeroShellAero/gtk-3.0/settings.ini").read_text(),
        "GTK 3 applications do not receive the bundled Windows 7 sound theme")
require('export QT_QUICK_CONTROLS_CONF="$qt_quick_controls_config"' in session_launcher,
        "Qt Quick apps launched in AeroShell do not receive its private default appearance")
require('export XCURSOR_THEME="aero-drop"' in session_launcher
        and 'export XCURSOR_PATH="$current_cursor_paths"' in session_launcher,
        "the Wayland session does not select its bundled Aero cursor theme and path")
require('install -m 600 -- "$default_kwinrc" "$kwinrc_path"' in session_launcher,
        "the Wayland session does not seed the private Aero decoration config on first login")
require('dbus-run-session' in session_launcher and ' "$dbus_run_session" -- "$bus_wrapper" "$kwin"' in session_launcher,
        "AeroShell and KDE do not get independent session buses")
require("default_screenlockerrc" in session_launcher and '"$screenlockerrc_path"' in session_launcher,
        "the Wayland session does not seed the private Aero lock-screen defaults")
require("default lock-screen package is incomplete" in session_launcher
        and '"$lock_shell_dir/contents/lockscreen/LockScreenUi.qml"' in session_launcher,
        "the Wayland session does not reject an incomplete Aero lock-screen package")
require("Noto Sans" in font_config.read_text(), "the session fontconfig does not prioritize Noto Sans")
require('export XCURSOR_THEME="aero-drop"' in session_launcher,
        "the session does not select its bundled Aero cursor theme")
require("gtk-icon-theme-name=Windows 7 Aero" in
        (ROOT / "assets/themes/AeroShellAero/gtk-3.0/settings.ini").read_text(),
        "GTK defaults do not select the Aero icon theme")
require("AeroColorScheme1.colors" in asset_docs and "aerothemeplasma-start-orb.png" in asset_docs,
        "the default AeroThemePlasma wallpaper, palette, and Start orb are not documented")

for component in ("desktopComponent", "panelComponent", "launcherComponent", "settingsComponent"):
    require(component in root_qml, f"first-login composition is missing {component}")
require("for (const targetScreen of screens)" in root_qml,
        "the default shell composition is not created for each display")
require("screenEvents" in root_qml and "onScreensChanged" in root_qml
        and "function reconcileScreens()" in root_qml and "function removeScreen" in root_qml
        and "ScreenEvents screenEvents" in main,
        "screen add/remove events do not reconcile AeroShell windows for display hotplug")
require('m_settings.value(panelWidgetKey(widget), true).toBool()' in settings_model,
        "new AeroShell profiles must enable all supported panel widgets by default")
require("availableFonts" in settings_model and "setFontFamily" in settings_model
        and "availableIconThemes" in settings_model and "setIconTheme" in settings_model
        and "resetInterfaceAppearance" in settings_qml,
        "AeroShell Settings must offer font and icon theme selection with an AeroThemePlasma reset")
require("iconThemeRevision" in settings_model
        and "shellSettings.iconThemeRevision" in launcher_qml
        and "shellSettings.iconThemeRevision" in panel_qml
        and "shellSettings.iconThemeRevision" in (ROOT / "src/shell/qml/DesktopWindow.qml").read_text(),
        "shell icon images must refresh when the user changes the AeroShell icon theme")
for widget in ("network", "audio", "battery", "tray", "keyboard", "notifications", "clock"):
    require(f'shellSettings.panelWidgets.{widget}' in panel_qml
            and f'setPanelWidgetVisible("{widget}"' in settings_qml,
            f"the AeroShell-only settings page is missing the {widget} widget preference")
require("resetPanelWidgets()" in settings_qml,
        "the settings page cannot restore the complete default widget layout")

for widget in ("startButton", "tasks", "networkButton", "audioButton", "batteryButton", "notificationButton",
               "notificationPopup", "notificationToast", "statusTray", "keyboardLayoutButton", "clockButton",
               "calendarPopup"):
    require(f"id: {widget}" in panel_qml, f"the default panel is missing {widget}")
require("shellBridge.toggleShowDesktop()" in panel_qml
        and 'setRole(PlasmaShellSurface::Role::Desktop)' in (ROOT / "src/shell/shellbridge.cpp").read_text(),
        "the default Aero taskbar must include show-desktop and desktop surfaces")
require("desktopItemModel" in (ROOT / "src/shell/qml/DesktopWindow.qml").read_text(),
        "the desktopcontainment equivalent must show the user's desktop entries by default")
require("remoteMenuModel.openMenu" in panel_qml and "statusNotifierModel" in main,
        "the default system-tray widget must show StatusNotifier icons and menus")
for source_id in (
        "io.gitgud.wackyideas.desktopcontainment", "io.gitgud.wackyideas.SevenStart",
        "io.gitgud.wackyideas.seventasks", "io.gitgud.wackyideas.digitalclocklite",
        "io.gitgud.wackyideas.networkmanagement", "io.gitgud.wackyideas.volume",
        "io.gitgud.wackyideas.battery", "io.gitgud.wackyideas.keyboardlayout",
        "io.gitgud.wackyideas.win7showdesktop", "org.kde.panel", "org.kde.plasma.systemtray",
        "org.kde.plasma.notifications"):
    require(source_id in appearance_doc,
            f"default appearance docs must map required AeroThemePlasma widget {source_id}")
require("applicationModel" in launcher_qml, "the default Start menu has no installed-app model")
require("aerothemeplasma-start-orb.png" in panel_qml and "sourceClipRect" in panel_qml,
        "the default Start button must use the AeroThemePlasma three-state orb")
require("DBusMenuModel remoteMenu" in main and "remoteMenuModel" in main,
        "remote StatusNotifier menus are not owned by the AeroShell process")
require("MenuPathRole" in tray_model and "QDBusObjectPath" in tray_model,
        "tray items do not expose their remote D-Bus menu path")
require('QStringLiteral("GetLayout")' in menu_model and 'QStringLiteral("Event")' in menu_model
        and 'QStringLiteral("AboutToShow")' in menu_model,
        "remote tray menus do not load layouts and send menu events")
require("remoteMenuModel.openMenu" in panel_qml and "remoteMenuModel.trigger(itemId)" in panel_qml
        and "remoteMenuModel.goBack()" in panel_qml and "openSubmenu(itemId)" in menu_model
        and "textFormat: Text.PlainText" in panel_qml,
        "the panel does not display and navigate remote StatusNotifier menus")
require("NotificationsModel notifications" in main and "notificationModel" in main,
        "the AeroShell notification service is not started by default")
require("PolkitAgentModel polkitAgent" in main and "polkitAgentModel" in main,
        "the session-owned PolicyKit agent is not registered by the shell")
require("ScreenLockController screenLockController" in main and "screenLockController" in main
        and 'QStringLiteral("org.freedesktop.ScreenSaver")' in lock_controller
        and 'QStringLiteral("Lock")' in lock_controller and 'QStringLiteral("GetActive")' in lock_controller,
        "the lock UI must request and confirm KWin's compositor-owned session lock")
require('text: "Lock"' in launcher_qml and "screenLockController.requestLock()" in launcher_qml,
        "the default Start menu must offer KWin's secure session lock")
require("PowerActions powerActions" in main and "powerActions" in main,
        "system power actions are not available from the default desktop shell")
require('text: "Power…"' in launcher_qml and "Suspend" in launcher_qml and "Restart" in launcher_qml
        and "Shut down" in launcher_qml and "powerActions.request" in launcher_qml
        and "Confirm power action" in launcher_qml,
        "the Start menu must offer confirmed, capability-checked power actions")
require("PolkitAuthWindow" in root_qml and "polkitAuthComponent.createObject" in root_qml,
        "the PolicyKit authentication prompt is not included in the default shell composition")
require("polkitAgentModel.submitResponse(response)" in auth_qml
        and "polkitAgentModel.cancelRequest()" in auth_qml
        and "TextInput.Password" in auth_qml,
        "the authentication prompt must submit and cancel responses with password echo disabled")
require(auth_qml.count("textFormat: Text.PlainText") >= 3,
        "PolicyKit-controlled prompt text must be displayed as plain text")
require("System D-Bus is unavailable" in (ROOT / "src/shell/polkitagent.cpp").read_text(),
        "PolicyKit registration must report missing system D-Bus")

print("PASS: mandatory first-login AeroThemePlasma appearance, event sounds, interface settings, Qt Quick colors/font, all mapped widgets, secure-lock defaults, confirmed power actions, notifications, and session-owned authentication prompt are configured")
