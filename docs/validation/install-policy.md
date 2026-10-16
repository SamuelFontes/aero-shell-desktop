# Install policy checker evidence

**Date:** 2026-10-16  
**Environment:** Current Manjaro development host; no package was installed and no KDE or display-manager files were changed.

## Checks

- `python tests/test-install-policy.py` — 17 fixture checks passed after adding the exact AeroShell-specific portal selector and its parent directory. Fixtures accept the private prefix and the unique Wayland entry, launcher, and portal selector; reject KDE and SDDM destinations (including SDDM-named paths under `/opt`), non-directory private-prefix roots, unexpected `/usr` files, escaping and cross-prefix symlinks, symlinked session integration entries, package hooks, target-root symlink escapes, and unsafe ownership outcomes.
- `python scripts/audit-install-manifests.py --help` — command-line entry point loads.
- `git diff --check` — passed.
- The earlier `/tmp` stages passed for 8,326 and then 8,442 entries as lock-screen and Qt Quick Controls assets were added.
- The current stage at `build/stage-final-20261016a` passed `python scripts/audit-install-manifests.py --stage-root build/stage-final-20261016a --check-target /` for 8,474 entries, including the Windows 7 sound theme, private AeroThemePlasma-derived lock-screen package, font/icon settings, Qt Quick Controls defaults, and current launcher. No staged file collides with a package-owned path on the host.
- The local unpublished Manjaro package at `build/local-package/aeroshell-desktop-0.1.0.r1.g8a440d9-1-x86_64.pkg.tar.zst` was built and recognized by `pacman -Qp`. Its archive was checked for SDDM, global Plasma/KWin configuration, and home-profile destinations. It is a workspace snapshot, not a published or reproducible clean checkout.
- Read-only pacman ownership check for `/usr/bin/pacman` returned owner `pacman`. The checker reported `/usr/bin/pacman: already owned by package 'pacman'` when given that staged destination.

## Scope limits

The ownership behavior was exercised against the current Manjaro pacman database, using an existing unrelated system executable as a collision fixture. Package installation, upgrade, and removal still need a disposable Arch/Manjaro system. Target-root symlink escape and missing-root behavior use fixture directories. No package was installed and no host files were written.
