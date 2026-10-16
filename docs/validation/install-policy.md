# Install policy checker evidence

**Date:** 2026-10-16  
**Environment:** Current Manjaro development host; no package was installed and no KDE or display-manager files were changed.

## Checks

- `python tests/test-install-policy.py` — 17 fixture checks passed after adding the exact AeroShell-specific portal selector and its parent directory. Fixtures accept the private prefix and the unique Wayland entry, launcher, and portal selector; reject KDE and SDDM destinations (including SDDM-named paths under `/opt`), non-directory private-prefix roots, unexpected `/usr` files, escaping and cross-prefix symlinks, symlinked session integration entries, package hooks, target-root symlink escapes, and unsafe ownership outcomes.
- `python scripts/audit-install-manifests.py --help` — command-line entry point loads.
- `git diff --check` — passed.
- The earlier stage at `/tmp/aeroshell-stage-menu.t72GGl` passed for 8,326 entries before the lock-screen package was included. The latest 8,442-entry audit is recorded below.
- The latest refreshed stage at `/tmp/aeroshell-stage-verified.ztnFMB` passed `python scripts/audit-install-manifests.py --stage-root /tmp/aeroshell-stage-verified.ztnFMB --check-target /` for 8,442 entries, including the private AeroThemePlasma-derived lock-screen package, Qt Quick Controls defaults, and current launcher. No staged file collides with a package-owned path on the host.
- Read-only pacman ownership check for `/usr/bin/pacman` returned owner `pacman`. The checker reported `/usr/bin/pacman: already owned by package 'pacman'` when given that staged destination.

## Scope limits

The ownership behavior was exercised against the current Manjaro pacman database, using an existing unrelated system executable as a collision fixture. The complete current CMake stage was audited, but an Arch package archive has not been built. Repeat package ownership and lifecycle checks in a disposable Arch/Manjaro release-test system once source pins, complete resources, and package scripts are ready.

The complete current CMake stage was audited, but an Arch package archive has not been built. Repeat package ownership and lifecycle checks in a disposable Arch/Manjaro release-test system after upstream source pins and package lifecycle checks are ready. Target-root symlink escape and missing-root behavior use fixture directories. No package was installed and no host files were written.
