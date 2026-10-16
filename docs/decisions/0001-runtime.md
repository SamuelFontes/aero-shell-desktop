# Decision 0001: runtime and compositor

**Status:** Unresolved; evidence required before T05 implementation.

**Question:** Which audited Wayland startup chain and compositor arrangement can run AeroShell's required shell while keeping stock Plasma libraries and plugins unchanged?

**Constraints:** Wayland is the MVP. Do not replace distro KDE packages, globally inject incompatible loader paths, or change Plasma launchers. Reuse the distro compositor only if the source and runtime audit proves that required AeroShell components work without modifying its shared installation or configuration.

**Evidence to collect:** Pinned upstream revisions; startup source paths; ELF dependencies and plugin lookup paths; nested Wayland results; ordinary application loading results; supported Plasma/KWin/Qt versions.

**Alternatives and consequences:** To be completed from source inspection and tests. Do not select based on convenience alone.
