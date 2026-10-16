#!/usr/bin/env bash
set -Eeuo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
TEMP_ROOT="$(mktemp -d /tmp/aeroshell-session-test.XXXXXX)"
trap 'rm -rf -- "$TEMP_ROOT"' EXIT

fail() {
    printf 'FAIL: %s\n' "$*" >&2
    exit 1
}

assert_equal() {
    local expected="$1" actual="$2" label="$3"
    [[ "$expected" == "$actual" ]] || fail "$label: expected <$expected>, got <$actual>"
}

assert_file_line() {
    local file="$1" line="$2" expected="$3" actual
    actual="$(sed -n "${line}p" "$file")"
    assert_equal "$expected" "$actual" "$file line $line"
}

mkdir -p "$TEMP_ROOT/bin" "$TEMP_ROOT/runtime" "$TEMP_ROOT/prefix with spaces/libexec" \
    "$TEMP_ROOT/prefix with spaces/bin" "$TEMP_ROOT/prefix with spaces/share/aeroshell/defaults" \
    "$TEMP_ROOT/prefix with spaces/share/aeroshell" \
    "$TEMP_ROOT/prefix with spaces/share/aeroshell/aurorae/themes/Se7enAero" \
    "$TEMP_ROOT/prefix with spaces/share/plasma/look-and-feel/org.aeroshell.desktop/contents/lockscreen" \
    "$TEMP_ROOT/prefix with spaces/share/plasma/look-and-feel/org.aeroshell.desktop/contents/sounds" \
    "$TEMP_ROOT/home with spaces"
cp "$ROOT/assets/defaults/kwinrc" "$TEMP_ROOT/prefix with spaces/share/aeroshell/defaults/kwinrc"
cp "$ROOT/assets/defaults/kscreenlockerrc" "$TEMP_ROOT/prefix with spaces/share/aeroshell/defaults/kscreenlockerrc"
cp "$ROOT/assets/qtquickcontrols2.conf" "$TEMP_ROOT/prefix with spaces/share/aeroshell/qtquickcontrols2.conf"
cp "$ROOT/assets/plasma/look-and-feel/org.aeroshell.desktop/metadata.json" \
    "$TEMP_ROOT/prefix with spaces/share/plasma/look-and-feel/org.aeroshell.desktop/metadata.json"
cp "$ROOT/assets/plasma/look-and-feel/org.aeroshell.desktop/contents/lockscreen/LockScreen.qml" \
    "$TEMP_ROOT/prefix with spaces/share/plasma/look-and-feel/org.aeroshell.desktop/contents/lockscreen/LockScreen.qml"
cp "$ROOT/assets/plasma/look-and-feel/org.aeroshell.desktop/contents/lockscreen/LockScreenUi.qml" \
    "$TEMP_ROOT/prefix with spaces/share/plasma/look-and-feel/org.aeroshell.desktop/contents/lockscreen/LockScreenUi.qml"
cp "$ROOT/assets/plasma/look-and-feel/org.aeroshell.desktop/contents/sounds/lockSuccess.ogg" \
    "$TEMP_ROOT/prefix with spaces/share/plasma/look-and-feel/org.aeroshell.desktop/contents/sounds/lockSuccess.ogg"
touch "$TEMP_ROOT/prefix with spaces/share/aeroshell/aurorae/themes/Se7enAero/metadata.desktop" \
    "$TEMP_ROOT/prefix with spaces/share/aeroshell/aurorae/themes/Se7enAero/decoration.svg"
chmod 700 "$TEMP_ROOT/runtime"
cp "$ROOT/session/aeroshell-check-compatibility" \
    "$TEMP_ROOT/prefix with spaces/libexec/aeroshell-check-compatibility"
chmod +x "$TEMP_ROOT/prefix with spaces/libexec/aeroshell-check-compatibility"
touch "$TEMP_ROOT/prefix with spaces/libexec/aeroshell-shell-session"
chmod +x "$TEMP_ROOT/prefix with spaces/libexec/aeroshell-shell-session"

cat > "$TEMP_ROOT/bin/dbus-run-session" <<'FAKE_DBUS_RUN_SESSION'
#!/usr/bin/env bash
[[ "${1:-}" == "--" ]] || exit 93
shift
printf '%s\n' "$@" > "$AEROSHELL_TEST_DBUS_ARGS_FILE"
export DBUS_SESSION_BUS_ADDRESS="$AEROSHELL_TEST_PRIVATE_BUS_ADDRESS"
exec "$@"
FAKE_DBUS_RUN_SESSION
chmod +x "$TEMP_ROOT/bin/dbus-run-session"

cat > "$TEMP_ROOT/bin/kwin_wayland" <<'FAKE_KWIN'
#!/usr/bin/env bash
case "${1:-}" in
    --version)
        printf 'kwin %s\n' "${AEROSHELL_TEST_KWIN_VERSION:-6.7.4}"
        exit 0
        ;;
    --help)
        printf '%s\n' "${AEROSHELL_TEST_KWIN_HELP:---xwayland --desktopfile --exit-with-session --lockscreen}"
        exit 0
        ;;
esac
printf '%s\n' "$@" > "$AEROSHELL_TEST_ARGS_FILE"
printf '%s\n' \
    "$XDG_CONFIG_HOME" "$XDG_DATA_HOME" "$XDG_CACHE_HOME" "$XDG_STATE_HOME" \
    "$XDG_CURRENT_DESKTOP" "$XDG_SESSION_DESKTOP" "$XDG_SESSION_TYPE" "$DESKTOP_SESSION" \
    "$XDG_RUNTIME_DIR" "$DBUS_SESSION_BUS_ADDRESS" "$XDG_DATA_DIRS" "$GTK_THEME" "$FONTCONFIG_FILE" \
    "$QT_QUICK_CONTROLS_CONF" "$XCURSOR_THEME" "$XCURSOR_PATH" "$XCURSOR_SIZE" \
    > "$AEROSHELL_TEST_ENV_FILE"
FAKE_KWIN
chmod +x "$TEMP_ROOT/bin/kwin_wayland"

export HOME="$TEMP_ROOT/home with spaces"
export XDG_RUNTIME_DIR="$TEMP_ROOT/runtime"
export XDG_CONFIG_HOME="$TEMP_ROOT/user config"
export XDG_DATA_HOME="$TEMP_ROOT/user data"
export XDG_CACHE_HOME="$TEMP_ROOT/user cache"
export XDG_STATE_HOME="$TEMP_ROOT/user state"
export XDG_DATA_DIRS="$TEMP_ROOT/system data"
export DBUS_SESSION_BUS_ADDRESS="unix:path=/tmp/aeroshell-test-bus"
export PATH="$TEMP_ROOT/bin:/usr/bin:/bin"
export AEROSHELL_PREFIX="$TEMP_ROOT/prefix with spaces"
export AEROSHELL_TEST_ARGS_FILE="$TEMP_ROOT/kwin-args"
export AEROSHELL_TEST_ENV_FILE="$TEMP_ROOT/kwin-env"
export AEROSHELL_TEST_DBUS_ARGS_FILE="$TEMP_ROOT/dbus-args"
export AEROSHELL_TEST_PRIVATE_BUS_ADDRESS="unix:path=$TEMP_ROOT/private-session-bus"
export AEROSHELL_TEST_KWIN_VERSION="6.7.4"
unset XCURSOR_PATH AEROSHELL_CURSOR_SIZE
user_config_home="$XDG_CONFIG_HOME"
user_data_home="$XDG_DATA_HOME"
user_cache_home="$XDG_CACHE_HOME"
user_state_home="$XDG_STATE_HOME"

bash "$ROOT/session/aeroshell-session-wayland"

assert_file_line "$TEMP_ROOT/kwin-env" 1 "$XDG_CONFIG_HOME/aeroshell/kwin/config"
assert_file_line "$TEMP_ROOT/kwin-env" 2 "$XDG_DATA_HOME/aeroshell/kwin/data"
assert_file_line "$TEMP_ROOT/kwin-env" 3 "$XDG_CACHE_HOME/aeroshell/kwin/cache"
assert_file_line "$TEMP_ROOT/kwin-env" 4 "$XDG_STATE_HOME/aeroshell/kwin/state"
assert_file_line "$TEMP_ROOT/kwin-env" 5 "AeroShell"
assert_file_line "$TEMP_ROOT/kwin-env" 6 "aeroshell"
assert_file_line "$TEMP_ROOT/kwin-env" 7 "wayland"
assert_file_line "$TEMP_ROOT/kwin-env" 8 "aeroshell"
assert_file_line "$TEMP_ROOT/kwin-env" 9 "$XDG_RUNTIME_DIR"
assert_file_line "$TEMP_ROOT/kwin-env" 10 "$AEROSHELL_TEST_PRIVATE_BUS_ADDRESS"
assert_file_line "$TEMP_ROOT/kwin-env" 11 "$AEROSHELL_PREFIX/share:$AEROSHELL_PREFIX/share/aeroshell:$XDG_DATA_DIRS"
assert_file_line "$TEMP_ROOT/kwin-env" 12 "AeroShellAero"
assert_file_line "$TEMP_ROOT/kwin-env" 13 "$AEROSHELL_PREFIX/share/aeroshell/fontconfig/fonts.conf"
assert_file_line "$TEMP_ROOT/kwin-env" 14 "$AEROSHELL_PREFIX/share/aeroshell/qtquickcontrols2.conf"
assert_file_line "$TEMP_ROOT/kwin-env" 15 "aero-drop"
assert_file_line "$TEMP_ROOT/kwin-env" 16 "$AEROSHELL_PREFIX/share/aeroshell/cursors:/usr/share/icons"
assert_file_line "$TEMP_ROOT/kwin-env" 17 "24"

for profile_path in \
    "$XDG_CONFIG_HOME/aeroshell/kwin/config" \
    "$XDG_DATA_HOME/aeroshell/kwin/data" \
    "$XDG_CACHE_HOME/aeroshell/kwin/cache" \
    "$XDG_STATE_HOME/aeroshell/kwin/state"; do
    [[ -d "$profile_path" ]] || fail "profile path was not created: $profile_path"
done
rg -q 'library=org.kde.kwin.aurorae.v2' "$XDG_CONFIG_HOME/aeroshell/kwin/config/kwinrc" \
    || fail "the first AeroShell login did not seed its private Aero window decoration"
rg -q 'theme=__aurorae__svg__Se7enAero' "$XDG_CONFIG_HOME/aeroshell/kwin/config/kwinrc" \
    || fail "the private KWin defaults do not select the Se7en Aero decoration"
rg -q 'LookAndFeel=org.aeroshell.desktop' "$XDG_CONFIG_HOME/aeroshell/kwin/config/kscreenlockerrc" \
    || fail "the private lock-screen defaults do not select the AeroShell LookAndFeel"
rg -q 'LockOnResume=true' "$XDG_CONFIG_HOME/aeroshell/kwin/config/kscreenlockerrc" \
    || fail "AeroShell does not lock after suspend/resume"

printf 'keep this AeroShell customization\n' > "$XDG_CONFIG_HOME/aeroshell/kwin/config/kwinrc"
printf 'keep this lock-screen customization\n' > "$XDG_CONFIG_HOME/aeroshell/kwin/config/kscreenlockerrc"
bash "$ROOT/session/aeroshell-session-wayland"
assert_equal 'keep this AeroShell customization' "$(cat "$XDG_CONFIG_HOME/aeroshell/kwin/config/kwinrc")" "second launch preserves KWin profile data"
assert_equal 'keep this lock-screen customization' "$(cat "$XDG_CONFIG_HOME/aeroshell/kwin/config/kscreenlockerrc")" "second launch preserves lock-screen settings"

export XDG_CONFIG_HOME="$TEMP_ROOT/already-aeroshell"
bash "$ROOT/session/aeroshell-session-wayland"
assert_file_line "$TEMP_ROOT/kwin-env" 1 "$TEMP_ROOT/already-aeroshell/aeroshell/kwin/config"
[[ -d "$TEMP_ROOT/already-aeroshell/aeroshell/kwin/config" ]] || fail "did not append the private profile to a user-selected XDG base named aeroshell"
export XDG_CONFIG_HOME="$user_config_home"

mapfile -t kwin_args < "$TEMP_ROOT/kwin-args"
mapfile -t dbus_args < "$TEMP_ROOT/dbus-args"
assert_equal "--xwayland" "${kwin_args[0]}" "KWin starts XWayland for legacy applications"
assert_equal "--desktopfile" "${kwin_args[1]}" "KWin gets the AeroShell desktop identity"
assert_equal "aeroshell" "${kwin_args[2]}" "KWin desktop identity argument"
assert_equal "--exit-with-session" "${kwin_args[3]}" "KWin owns the shell lifetime"
assert_equal "$TEMP_ROOT/prefix with spaces/libexec/aeroshell-shell-session" "${kwin_args[4]}" "session shell wrapper path"
assert_equal "$TEMP_ROOT/bin/kwin_wayland" "${dbus_args[0]}" "private D-Bus launches the expected compositor"
assert_equal "${kwin_args[*]}" "${dbus_args[*]:1}" "KWin runs inside the isolated D-Bus session"

cat > "$TEMP_ROOT/prefix with spaces/bin/aeroshell-shell" <<'FAKE_SHELL'
#!/usr/bin/env bash
printf '%s\n' \
    "${XDG_CONFIG_HOME-unset}" "${XDG_DATA_HOME-unset}" "${XDG_CACHE_HOME-unset}" "${XDG_STATE_HOME-unset}" \
    "$XDG_CURRENT_DESKTOP" "$XDG_SESSION_DESKTOP" "$XDG_SESSION_TYPE" "$XDG_RUNTIME_DIR" \
    "${AEROSHELL_USER_XDG_CONFIG_HOME-unset}" "$XDG_DATA_DIRS" "$GTK_THEME" "$FONTCONFIG_FILE" \
    "$QT_QUICK_CONTROLS_CONF" "$XCURSOR_THEME" "$XCURSOR_PATH" "$XCURSOR_SIZE" \
    > "$AEROSHELL_TEST_RESTORED_ENV_FILE"
FAKE_SHELL
chmod +x "$TEMP_ROOT/prefix with spaces/bin/aeroshell-shell"
export AEROSHELL_TEST_RESTORED_ENV_FILE="$TEMP_ROOT/restored-env"
export AEROSHELL_USER_XDG_CONFIG_HOME="$user_config_home"
export AEROSHELL_USER_XDG_DATA_HOME="$user_data_home"
export AEROSHELL_USER_XDG_CACHE_HOME="$user_cache_home"
export AEROSHELL_USER_XDG_STATE_HOME="$user_state_home"
XDG_CONFIG_HOME="$XDG_CONFIG_HOME/aeroshell/kwin/config" \
XDG_DATA_HOME="$XDG_DATA_HOME/aeroshell/kwin/data" \
XDG_CACHE_HOME="$XDG_CACHE_HOME/aeroshell/kwin/cache" \
XDG_STATE_HOME="$XDG_STATE_HOME/aeroshell/kwin/state" \
XDG_DATA_DIRS="$AEROSHELL_PREFIX/share:$AEROSHELL_PREFIX/share/aeroshell:$XDG_DATA_DIRS" \
GTK_THEME="AeroShellAero" FONTCONFIG_FILE="$AEROSHELL_PREFIX/share/aeroshell/fontconfig/fonts.conf" \
QT_QUICK_CONTROLS_CONF="$AEROSHELL_PREFIX/share/aeroshell/qtquickcontrols2.conf" \
XCURSOR_THEME="aero-drop" XCURSOR_PATH="$AEROSHELL_PREFIX/share/aeroshell/cursors:/usr/share/icons" \
XCURSOR_SIZE="24" \
XDG_CURRENT_DESKTOP="AeroShell" XDG_SESSION_DESKTOP="aeroshell" XDG_SESSION_TYPE="wayland" \
    bash "$ROOT/session/aeroshell-shell-session"

assert_file_line "$TEMP_ROOT/restored-env" 1 "$user_config_home"
assert_file_line "$TEMP_ROOT/restored-env" 2 "$user_data_home"
assert_file_line "$TEMP_ROOT/restored-env" 3 "$user_cache_home"
assert_file_line "$TEMP_ROOT/restored-env" 4 "$user_state_home"
assert_file_line "$TEMP_ROOT/restored-env" 5 "AeroShell"
assert_file_line "$TEMP_ROOT/restored-env" 6 "aeroshell"
assert_file_line "$TEMP_ROOT/restored-env" 7 "wayland"
assert_file_line "$TEMP_ROOT/restored-env" 8 "$XDG_RUNTIME_DIR"
assert_file_line "$TEMP_ROOT/restored-env" 9 "unset"
assert_file_line "$TEMP_ROOT/restored-env" 10 "$AEROSHELL_PREFIX/share:$AEROSHELL_PREFIX/share/aeroshell:$XDG_DATA_DIRS"
assert_file_line "$TEMP_ROOT/restored-env" 11 "AeroShellAero"
assert_file_line "$TEMP_ROOT/restored-env" 12 "$AEROSHELL_PREFIX/share/aeroshell/fontconfig/fonts.conf"
assert_file_line "$TEMP_ROOT/restored-env" 13 "$AEROSHELL_PREFIX/share/aeroshell/qtquickcontrols2.conf"
assert_file_line "$TEMP_ROOT/restored-env" 14 "aero-drop"
assert_file_line "$TEMP_ROOT/restored-env" 15 "$AEROSHELL_PREFIX/share/aeroshell/cursors:/usr/share/icons"
assert_file_line "$TEMP_ROOT/restored-env" 16 "24"
[[ -d "$user_config_home/aeroshell" ]] || fail "shell wrapper did not create its private settings directory"

# Refuse a symlinked settings root before the shell can ask QSettings to write it.
mkdir -p "$TEMP_ROOT/settings-link-parent" "$TEMP_ROOT/settings-link-sentinel"
ln -s "$TEMP_ROOT/settings-link-sentinel" "$TEMP_ROOT/settings-link-parent/aeroshell"
rm -f "$TEMP_ROOT/restored-env"
if HOME="$TEMP_ROOT/home with spaces" XDG_CONFIG_HOME="$TEMP_ROOT/settings-link-parent" \
    AEROSHELL_USER_XDG_CONFIG_HOME="$TEMP_ROOT/settings-link-parent" \
    bash "$ROOT/session/aeroshell-shell-session" >"$TEMP_ROOT/settings-link-out" 2>&1; then
    fail "shell wrapper accepted a symlinked AeroShell settings directory"
fi
rg -q 'refusing to use a symlinked settings directory' "$TEMP_ROOT/settings-link-out" \
    || fail "symlinked settings directory did not produce a useful error"
[[ ! -e "$TEMP_ROOT/restored-env" ]] || fail "shell started despite a symlinked settings directory"
[[ -z "$(find "$TEMP_ROOT/settings-link-sentinel" -mindepth 1 -print -quit)" ]] \
    || fail "the settings directory symlink target was modified"

# Refuse a symlinked settings file before the shell can write through it.
mkdir -p "$TEMP_ROOT/settings-file-parent/aeroshell" "$TEMP_ROOT/settings-file-sentinel"
ln -s "$TEMP_ROOT/settings-file-sentinel/foreign.ini" \
    "$TEMP_ROOT/settings-file-parent/aeroshell/settings.ini"
rm -f "$TEMP_ROOT/restored-env"
if HOME="$TEMP_ROOT/home with spaces" XDG_CONFIG_HOME="$TEMP_ROOT/settings-file-parent" \
    AEROSHELL_USER_XDG_CONFIG_HOME="$TEMP_ROOT/settings-file-parent" \
    bash "$ROOT/session/aeroshell-shell-session" >"$TEMP_ROOT/settings-file-out" 2>&1; then
    fail "shell wrapper accepted a symlinked AeroShell settings file"
fi
rg -q 'refusing to use a symlinked settings file' "$TEMP_ROOT/settings-file-out" \
    || fail "symlinked settings file did not produce a useful error"
[[ ! -e "$TEMP_ROOT/restored-env" ]] || fail "shell started despite a symlinked settings file"
[[ ! -e "$TEMP_ROOT/settings-file-sentinel/foreign.ini" ]] || fail "settings symlink target was modified"

# Refuse settings files hard-linked to another desktop's configuration.
mkdir -p "$TEMP_ROOT/settings-hardlink-parent/aeroshell" "$TEMP_ROOT/settings-hardlink-sentinel"
printf 'shared configuration\n' > "$TEMP_ROOT/settings-hardlink-sentinel/foreign.ini"
ln "$TEMP_ROOT/settings-hardlink-sentinel/foreign.ini" \
    "$TEMP_ROOT/settings-hardlink-parent/aeroshell/settings.ini"
rm -f "$TEMP_ROOT/restored-env"
if HOME="$TEMP_ROOT/home with spaces" XDG_CONFIG_HOME="$TEMP_ROOT/settings-hardlink-parent" \
    AEROSHELL_USER_XDG_CONFIG_HOME="$TEMP_ROOT/settings-hardlink-parent" \
    bash "$ROOT/session/aeroshell-shell-session" >"$TEMP_ROOT/settings-hardlink-out" 2>&1; then
    fail "shell wrapper accepted a shared hard-linked settings file"
fi
rg -q 'refusing to use a shared settings file' "$TEMP_ROOT/settings-hardlink-out" \
    || fail "hard-linked settings file did not produce a useful error"
[[ ! -e "$TEMP_ROOT/restored-env" ]] || fail "shell started despite a shared settings file"
assert_equal 'shared configuration' "$(cat "$TEMP_ROOT/settings-hardlink-sentinel/foreign.ini")" \
    "hard-linked settings target remains unchanged"

# A symlink at AeroShell's profile root must fail before KWin starts.
mkdir -p "$TEMP_ROOT/link home/config" "$TEMP_ROOT/profile sentinel"
ln -s "$TEMP_ROOT/profile sentinel" "$TEMP_ROOT/link home/config/aeroshell"
rm -f "$AEROSHELL_TEST_ARGS_FILE"
if HOME="$TEMP_ROOT/link home" XDG_CONFIG_HOME="$TEMP_ROOT/link home/config" \
    XDG_DATA_HOME="$TEMP_ROOT/link home/data" XDG_CACHE_HOME="$TEMP_ROOT/link home/cache" \
    XDG_STATE_HOME="$TEMP_ROOT/link home/state" \
    bash "$ROOT/session/aeroshell-session-wayland" >"$TEMP_ROOT/symlink-out" 2>&1; then
    fail "session accepted a symlinked AeroShell profile root"
fi
[[ ! -e "$AEROSHELL_TEST_ARGS_FILE" ]] || fail "KWin started despite a symlinked profile root"
[[ -z "$(find "$TEMP_ROOT/profile sentinel" -mindepth 1 -print -quit)" ]] || fail "symlink target was modified"

# An unsupported compositor release must fail before creating the AeroShell profile.
mkdir -p "$TEMP_ROOT/unsupported-runtime"
chmod 700 "$TEMP_ROOT/unsupported-runtime"
rm -f "$AEROSHELL_TEST_ARGS_FILE"
if HOME="$TEMP_ROOT/unsupported home" XDG_RUNTIME_DIR="$TEMP_ROOT/unsupported-runtime" \
    XDG_CONFIG_HOME="$TEMP_ROOT/unsupported config" XDG_DATA_HOME="$TEMP_ROOT/unsupported data" \
    XDG_CACHE_HOME="$TEMP_ROOT/unsupported cache" XDG_STATE_HOME="$TEMP_ROOT/unsupported state" \
    AEROSHELL_TEST_KWIN_VERSION="6.7.5" bash "$ROOT/session/aeroshell-session-wayland" \
    >"$TEMP_ROOT/unsupported-out" 2>&1; then
    fail "session accepted an unverified KWin release"
fi
rg -q 'outside this build.s current target of 6.7.4' "$TEMP_ROOT/unsupported-out" \
    || fail "unsupported KWin release did not get a useful compatibility error"
[[ ! -e "$TEMP_ROOT/unsupported config/aeroshell" ]] \
    || fail "unsupported KWin release created an AeroShell profile"
[[ ! -e "$AEROSHELL_TEST_ARGS_FILE" ]] || fail "unsupported KWin release started a session"

# A symlink at the private KWin config file must fail before the compositor starts.
mkdir -p "$TEMP_ROOT/kwin-config-home/config/aeroshell/kwin/config" "$TEMP_ROOT/kwin-config-sentinel"
ln -s "$TEMP_ROOT/kwin-config-sentinel" "$TEMP_ROOT/kwin-config-home/config/aeroshell/kwin/config/kwinrc"
rm -f "$AEROSHELL_TEST_ARGS_FILE"
if HOME="$TEMP_ROOT/kwin-config-home" XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" \
    XDG_CONFIG_HOME="$TEMP_ROOT/kwin-config-home/config" XDG_DATA_HOME="$TEMP_ROOT/kwin-config-home/data" \
    XDG_CACHE_HOME="$TEMP_ROOT/kwin-config-home/cache" XDG_STATE_HOME="$TEMP_ROOT/kwin-config-home/state" \
    bash "$ROOT/session/aeroshell-session-wayland" >"$TEMP_ROOT/kwin-config-symlink-out" 2>&1; then
    fail "session accepted a symlinked KWin config file"
fi
rg -q 'Refusing to use a symlinked AeroShell config' "$TEMP_ROOT/kwin-config-symlink-out" \
    || fail "symlinked KWin config did not produce a useful error"
[[ ! -e "$AEROSHELL_TEST_ARGS_FILE" ]] || fail "KWin started with a symlinked config file"
[[ -z "$(find "$TEMP_ROOT/kwin-config-sentinel" -mindepth 1 -print -quit)" ]] \
    || fail "the KWin config symlink target was modified"

# KWin edits kwinrc in place, so reject any config inode shared with KDE.
mkdir -p "$TEMP_ROOT/kwin-config-hardlink-home/config/aeroshell/kwin/config" \
    "$TEMP_ROOT/kwin-config-hardlink-sentinel"
printf 'shared KWin configuration\n' > "$TEMP_ROOT/kwin-config-hardlink-sentinel/kwinrc"
ln "$TEMP_ROOT/kwin-config-hardlink-sentinel/kwinrc" \
    "$TEMP_ROOT/kwin-config-hardlink-home/config/aeroshell/kwin/config/kwinrc"
rm -f "$AEROSHELL_TEST_ARGS_FILE"
if HOME="$TEMP_ROOT/kwin-config-hardlink-home" XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" \
    XDG_CONFIG_HOME="$TEMP_ROOT/kwin-config-hardlink-home/config" \
    XDG_DATA_HOME="$TEMP_ROOT/kwin-config-hardlink-home/data" \
    XDG_CACHE_HOME="$TEMP_ROOT/kwin-config-hardlink-home/cache" \
    XDG_STATE_HOME="$TEMP_ROOT/kwin-config-hardlink-home/state" \
    bash "$ROOT/session/aeroshell-session-wayland" >"$TEMP_ROOT/kwin-hardlink-out" 2>&1; then
    fail "session accepted a shared hard-linked KWin config"
fi
rg -q 'Refusing to use a shared AeroShell config' "$TEMP_ROOT/kwin-hardlink-out" \
    || fail "hard-linked KWin config did not produce a useful error"
[[ ! -e "$AEROSHELL_TEST_ARGS_FILE" ]] || fail "KWin started with a shared config file"
assert_equal 'shared KWin configuration' "$(cat "$TEMP_ROOT/kwin-config-hardlink-sentinel/kwinrc")" \
    "hard-linked KWin target remains unchanged"

# Refuse a symlink at an intermediate private KWin profile path.
mkdir -p "$TEMP_ROOT/kwin-parent-home/config/aeroshell" "$TEMP_ROOT/kwin-parent-sentinel"
ln -s "$TEMP_ROOT/kwin-parent-sentinel" "$TEMP_ROOT/kwin-parent-home/config/aeroshell/kwin"
rm -f "$AEROSHELL_TEST_ARGS_FILE"
if HOME="$TEMP_ROOT/kwin-parent-home" XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" \
    XDG_CONFIG_HOME="$TEMP_ROOT/kwin-parent-home/config" XDG_DATA_HOME="$TEMP_ROOT/kwin-parent-home/data" \
    XDG_CACHE_HOME="$TEMP_ROOT/kwin-parent-home/cache" XDG_STATE_HOME="$TEMP_ROOT/kwin-parent-home/state" \
    bash "$ROOT/session/aeroshell-session-wayland" >"$TEMP_ROOT/kwin-parent-symlink-out" 2>&1; then
    fail "session accepted a symlinked intermediate KWin profile directory"
fi
rg -q 'Refusing to use symlinked AeroShell profile path' "$TEMP_ROOT/kwin-parent-symlink-out" \
    || fail "symlinked KWin profile directory did not produce a useful error"
[[ ! -e "$AEROSHELL_TEST_ARGS_FILE" ]] || fail "KWin started with a symlinked profile parent"
[[ -z "$(find "$TEMP_ROOT/kwin-parent-sentinel" -mindepth 1 -print -quit)" ]] \
    || fail "KWin profile symlink target was modified"

printf 'PASS: session environment, private D-Bus, compatibility gate, KWin arguments, and profile safety\n'
