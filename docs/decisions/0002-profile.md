# Decision 0002: user profile isolation

**Status:** Unresolved; evidence required before T06 implementation.

**Question:** Which session-scoped configuration, cache, state, and data paths isolate AeroShell's desktop state while preserving access to the user's applications and files?

**Constraints:** Never write AeroShell desktop settings to Plasma's profile. Do not change `HOME` or `XDG_RUNTIME_DIR`. Preserve user application discovery deliberately; do not expose KDE desktop assets as writable AeroShell settings by adding broad search paths.

**Evidence to collect:** Actual XDG path use in pinned components, file-write traces, application/icon discovery behavior, fixture results for custom XDG paths and symlinks.

**Alternatives and consequences:** To be completed from source inspection and tests.
