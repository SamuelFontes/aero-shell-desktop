# Runtime library loading

The AeroShell shell is built against the distro's Qt 6, KDE Frameworks, and KWayland libraries. It does not bundle or load an AeroThemePlasma `libplasma` fork. Its staged ELF RUNPATH is `$ORIGIN:$ORIGIN/../lib`, allowing only package-private libraries beside the shell if one is added later.

Run the static staged-binary audit after configuring, building, and installing into `stage/`:

```sh
scripts/verify-library-loading.sh stage/opt/aeroshell/bin/aeroshell-shell
```

The script rejects missing shared libraries, a RUNPATH that differs from the private-prefix contract, dependencies outside `/usr/lib` and `/lib`, and any `libplasma` dependency. If an AeroShell process is already running, pass its PID to inspect its `/proc/<pid>/maps` entries for Qt, KF, and KWayland libraries:

```sh
scripts/verify-library-loading.sh /opt/aeroshell/bin/aeroshell-shell <aeroshell-shell-pid>
```

The staged-binary audit passes in the current Manjaro build environment. This does not prove plugin lookup paths, loaded process mappings, ordinary application behavior, or a nested compositor session; those require a real AeroShell Wayland session. The sandbox currently prevents its private D-Bus socket from binding, so no live process mapping or compositor evidence is available.
