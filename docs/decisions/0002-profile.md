# Decision 0002: user profile isolation

**Status:** Initial XDG isolation selected and fixture-tested; real-session write tracing pending.

**Question:** Which session-scoped paths isolate KWin/AeroShell state while preserving the user's application settings, files, and installed application entries?

**Constraints:** Never write AeroShell desktop settings to Plasma's profile. Do not change `HOME` or `XDG_RUNTIME_DIR`. Preserve user application discovery deliberately; do not expose KDE desktop assets as writable AeroShell settings by adding broad search paths.

**Evidence to collect:** Actual XDG path use in pinned components, file-write traces, application/icon discovery behavior, fixture results for custom XDG paths and symlinks.

**Selected initial approach:** Create `aeroshell/kwin/{config,data,cache,state}` below the user's absolute XDG base roots (or standard home defaults). Pass those roots to KWin only. The KWin-launched shell wrapper restores the original user XDG roots before starting the Qt Quick shell, so KIO application launches use the existing application configuration and `.desktop` data. Never change `HOME` or `XDG_RUNTIME_DIR`; refuse symlinks at the AeroShell profile roots.

**Evidence:** `tests/test-session-launch.sh` covers paths containing spaces, KWin-private roots, restoration of original application roots, and symlink refusal using a fake compositor. This is fixture evidence only; no file-write trace from a real KWin/AeroShell login exists yet.

**Consequences:** KWin profile settings are isolated, while applications intentionally continue to share their existing application preferences. The Qt Quick shell keeps its own settings at `$XDG_CONFIG_HOME/aeroshell/settings.ini` (normally `~/.config/aeroshell/settings.ini`) using `QSettings::IniFormat`; it must not change process-wide XDG roots. The current wallpaper picker writes only this file. Real-session write tracing is still required.
