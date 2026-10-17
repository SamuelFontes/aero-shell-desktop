#!/usr/bin/env bash
# Real virtual KWin smoke test. No active desktop bus, socket, or profile is used.
set -Eeuo pipefail
if [[ "${1:-}" != --private ]]; then
    [[ $# == 2 ]] || { printf 'usage: %s STAGED_PREFIX OUTPUT_DIRECTORY\n' "$0" >&2; exit 2; }
    exec dbus-run-session -- bash "$0" --private "$(realpath "$1")" "$(realpath -m "$2")"
fi
prefix="$2"
output="$3"
root="$(mktemp -d /tmp/aeroshell-wayland-test.XXXXXX)"
cleanup() {
    # The private Documents portal can mount a FUSE filesystem in this runtime.
    # Unmount it before removing our own temporary directory.
    if mountpoint -q "$root/runtime/doc"; then fusermount3 -u "$root/runtime/doc"; fi
    rm -rf -- "$root"
}
trap cleanup EXIT
export XDG_CONFIG_HOME="$root/config" XDG_DATA_HOME="$root/data" XDG_CACHE_HOME="$root/cache" XDG_STATE_HOME="$root/state" XDG_RUNTIME_DIR="$root/runtime"
mkdir -p "$XDG_CONFIG_HOME" "$XDG_RUNTIME_DIR" "$XDG_DATA_HOME/applications" "$output"
chmod 700 "$XDG_RUNTIME_DIR"
unset WAYLAND_DISPLAY DISPLAY XAUTHORITY
export AEROSHELL_SESSION_BUS_ADDRESS="$DBUS_SESSION_BUS_ADDRESS"
export XDG_CURRENT_DESKTOP=AeroShell XDG_SESSION_DESKTOP=aeroshell PLASMA_DEFAULT_SHELL=org.aeroshell.desktop
export XDG_DATA_DIRS="$prefix/share:$prefix/share/aeroshell:/usr/local/share:/usr/share"
export XCURSOR_THEME=aero-drop XCURSOR_PATH="$prefix/share/aeroshell/cursors:/usr/share/icons" XCURSOR_SIZE=32
export QT_QUICK_CONTROLS_CONF="$prefix/share/aeroshell/qtquickcontrols2.conf"
export QT_PLUGIN_PATH="$prefix/lib/qt6/plugins${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}"
export QT_QUICK_BACKEND=software LIBGL_ALWAYS_SOFTWARE=1 QT_FORCE_STDERR_LOGGING=1
export QT_LOGGING_RULES='kwin_scripting.debug=true'
cp "$prefix/share/aeroshell/defaults/kdeglobals" "$XDG_CONFIG_HOME/kdeglobals"
cp "$prefix/share/aeroshell/defaults/kwinrc" "$XDG_CONFIG_HOME/kwinrc"
# Capture settled frames; animation appearance is outside this fixture.
kwriteconfig6 --file "$XDG_CONFIG_HOME/kdeglobals" --group KDE --key AnimationDurationFactor 0
cp "$prefix/share/aeroshell/defaults/kglobalshortcutsrc" "$XDG_CONFIG_HOME/kglobalshortcutsrc"
# KWin authorizes the taskbar protocol by the actual executable path. The
# production desktop entry is unchanged; this override belongs to the fixture.
sed "s|^Exec=.*|Exec=$prefix/bin/aeroshell-shell|" "$prefix/share/applications/org.aeroshell.shell.desktop" > "$XDG_DATA_HOME/applications/org.aeroshell.shell.desktop"
mkdir -p "$XDG_DATA_HOME/kwin/scripts/aeroshell-audit/contents/code"
cat > "$XDG_DATA_HOME/kwin/scripts/aeroshell-audit/metadata.json" <<'JSON'
{"KPackageStructure":"KWin/Script","KPlugin":{"Id":"aeroshell-audit","Name":"Geometry test","License":"MIT"},"X-Plasma-API":"javascript","X-Plasma-MainScript":"code/main.js"}
JSON
cat > "$XDG_DATA_HOME/kwin/scripts/aeroshell-audit/contents/code/main.js" <<'JS'
workspace.windowAdded.connect(function(window) {
    function report() {
        const g = window.frameGeometry;
        print("AEROSHELL_FRAME", window.caption, JSON.stringify([g.x,g.y,g.width,g.height]));
    }
    window.frameGeometryChanged.connect(report);
    report();
});
JS
kwriteconfig6 --file "$XDG_CONFIG_HOME/kwinrc" --group Plugins --key aeroshell-auditEnabled true
export AEROSHELL_PREFIX="$prefix"
export AEROSHELL_USER_XDG_CONFIG_HOME="$XDG_CONFIG_HOME" AEROSHELL_USER_XDG_DATA_HOME="$XDG_DATA_HOME" AEROSHELL_USER_XDG_CACHE_HOME="$XDG_CACHE_HOME" AEROSHELL_USER_XDG_STATE_HOME="$XDG_STATE_HOME"
printf '#!/usr/bin/env bash\n(sleep 1; qdbus6 org.kde.KWin /Effects org.freedesktop.DBus.Properties.Get org.kde.kwin.Effects loadedEffects > %q) &\n(sleep 1; qdbus6 org.kde.kglobalaccel /component/kwin org.kde.kglobalaccel.Component.shortcutNames > %q) &\n(sleep 1; spectacle -b -f -n -o %q -d 1700) &\nexec %q --smoke-test %q\n' "$output/effects.txt" "$output/shortcuts.txt" "$output/composite.png" "$prefix/libexec/aeroshell-shell-session" "$output" > "$root/child"
chmod +x "$root/child"
timeout 20s kwin_wayland --virtual --width 1280 --height 720 --no-lockscreen --no-kactivities --exit-with-session "$root/child" > "$output/session.log" 2>&1
rg -q 'AEROSHELL_FRAME AeroShell Panel.* \[0,680,1280,40\]' "$output/session.log"
for action in 'AeroShell Explorer' 'AeroShell Run' 'AeroShell Lock' 'AeroShell Start' 'AeroShell Security options'; do
    rg -Fxq "$action" "$output/shortcuts.txt"
done
rg -Fxq 'aeroglassblur' "$output/effects.txt"
rg -q 'Mapped shell window "AeroShell Panel".*exposed true' "$output/session.log"
[[ -s "$output/composite.png" && -s "$output/AeroShell Panel.png" && -s "$output/AeroShell Desktop.png" && -s "$output/AeroShell Start.png" && -s "$output/Run.png" ]]
if rg 'QML.*(Error|Cannot)|ReferenceError|TypeError|SyntaxError|does not expose' "$output/session.log"; then exit 1; fi
printf 'PASS: real KWin desktop, bottom-edge taskbar, original Aero Glass effect, and compositor capture\n'
