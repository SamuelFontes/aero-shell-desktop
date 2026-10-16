#!/usr/bin/env bash
set -Eeuo pipefail

usage() {
    printf 'Usage: %s <aeroshell-binary> [running-pid]\n' "$0" >&2
    exit 2
}

fail() {
    printf 'FAIL: %s\n' "$*" >&2
    exit 1
}

[[ $# -ge 1 && $# -le 2 ]] || usage
binary="$1"
[[ -f "$binary" && -x "$binary" ]] || fail "not an executable file: $binary"

for tool in readelf ldd; do
    command -v "$tool" >/dev/null 2>&1 || fail "required inspection tool is missing: $tool"
done

dynamic_section="$(readelf -d "$binary" 2>/dev/null)" || fail "cannot read ELF metadata: $binary"
runpath="$(sed -n 's/.*(RUNPATH).*\[\(.*\)\].*/\1/p' <<<"$dynamic_section")"
[[ "$runpath" == '$ORIGIN:$ORIGIN/../lib' ]] || fail "unexpected RUNPATH: ${runpath:-<missing>}"

dependencies="$(ldd "$binary" 2>&1)" || fail "ldd could not inspect $binary"
[[ "$dependencies" != *"not found"* ]] || fail "one or more shared libraries are missing"

while IFS= read -r dependency_line; do
    [[ "$dependency_line" == *"=>"* ]] || continue
    dependency_path="$(awk '{print $3}' <<<"$dependency_line")"
    [[ -f "$dependency_path" ]] || continue
    resolved_path="$(readlink -f "$dependency_path")"
    case "$resolved_path" in
        /usr/lib/*|/lib/*) ;;
        *) fail "shared library resolved outside distro library directories: $resolved_path" ;;
    esac
done <<<"$dependencies"

[[ "$dependencies" != *"libplasma"* ]] || fail "the shell unexpectedly depends on AeroThemePlasma's libplasma fork"

if [[ $# -eq 2 ]]; then
    pid="$2"
    [[ "$pid" =~ ^[0-9]+$ && -r "/proc/$pid/maps" ]] || fail "cannot inspect process mappings for PID $pid"
    expected_binary="$(readlink -f "$binary")"
    running_binary="$(readlink -f "/proc/$pid/exe")"
    [[ "$running_binary" == "$expected_binary" ]] || fail "PID $pid is running $running_binary, not $expected_binary"
    process_environment="$(tr '\0' '\n' <"/proc/$pid/environ")"
    for variable in LD_LIBRARY_PATH LD_PRELOAD QT_PLUGIN_PATH QML2_IMPORT_PATH; do
        if grep -q "^${variable}=." <<<"$process_environment"; then
            fail "PID $pid has a nonempty library/plugin override: $variable"
        fi
    done
    process_maps="$(cat "/proc/$pid/maps")"
    [[ "$process_maps" != *"/opt/aeroshell/lib/"* ]] || fail "process loaded a private shared library"
    while IFS= read -r mapping; do
        [[ "$mapping" == *libQt6* || "$mapping" == *libKF6* || "$mapping" == *libKWaylandClient* ]] || continue
        mapping_path="${mapping##* }"
        case "$mapping_path" in
            /usr/lib/*|/lib/*) ;;
            *) fail "Qt/KF/KWayland mapping is outside distro library directories: $mapping_path" ;;
        esac
    done <<<"$process_maps"
    printf 'PASS: PID %s maps Qt/KF/KWayland from distro library directories.\n' "$pid"
fi

printf 'PASS: %s uses only distro shared libraries and its private $ORIGIN RUNPATH.\n' "$binary"
