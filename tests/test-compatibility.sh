#!/usr/bin/env bash
set -Eeuo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
TEMP_ROOT="$(mktemp -d /tmp/aeroshell-compat-test.XXXXXX)"
trap 'rm -rf -- "$TEMP_ROOT"' EXIT

fail() {
    printf 'FAIL: %s\n' "$*" >&2
    exit 1
}

cat > "$TEMP_ROOT/kwin_wayland" <<'FAKE_KWIN'
#!/usr/bin/env bash
case "${1:-}" in
    --version)
        printf 'kwin %s\n' "${AEROSHELL_TEST_KWIN_VERSION:-6.7.4}"
        ;;
    --help)
        printf '%s\n' "${AEROSHELL_TEST_KWIN_HELP:---xwayland --desktopfile --exit-with-session --lockscreen}"
        ;;
    *)
        exit 64
        ;;
esac
FAKE_KWIN
chmod +x "$TEMP_ROOT/kwin_wayland"

checker="$ROOT/session/aeroshell-check-compatibility"
AEROSHELL_TEST_KWIN_VERSION="6.7.4" bash "$checker" "$TEMP_ROOT/kwin_wayland"

for version in 6.7.3 6.7.5 6.8.0 unknown; do
    if AEROSHELL_TEST_KWIN_VERSION="$version" bash "$checker" "$TEMP_ROOT/kwin_wayland" \
        >"$TEMP_ROOT/version-out" 2>&1; then
        fail "unsupported KWin version $version was accepted"
    fi
    if [[ "$version" == "unknown" ]]; then
        rg -q 'could not parse the installed KWin version' "$TEMP_ROOT/version-out" \
            || fail "unparseable KWin version did not report a compatibility failure"
    else
        rg -q 'outside this build.s current target of 6.7.4' "$TEMP_ROOT/version-out" \
            || fail "KWin version $version did not report a compatibility failure"
    fi
done

if AEROSHELL_TEST_KWIN_HELP="--xwayland --desktopfile --lockscreen" bash "$checker" "$TEMP_ROOT/kwin_wayland" \
    >"$TEMP_ROOT/options-out" 2>&1; then
    fail "KWin missing a required session option was accepted"
fi
rg -q 'does not support required option --exit-with-session' "$TEMP_ROOT/options-out" \
    || fail "missing KWin option did not report the required option"

printf 'PASS: KWin version and required Wayland session options are checked before launch\n'
