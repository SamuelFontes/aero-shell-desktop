#!/usr/bin/env python3
"""Build pinned original AeroThemePlasma components into a private staging tree.

Never invokes upstream installers, sudo, or system package installation.
The small integration patches are applied to disposable build checkouts only.
"""
import argparse
import hashlib
import json
import os
import shutil
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def run(args, log, **kwargs):
    print("+ " + " ".join(map(str, args)), flush=True)
    with log.open("w") as output:
        result = subprocess.run(list(map(str, args)), stdout=output, stderr=subprocess.STDOUT, **kwargs)
    if result.returncode:
        print(log.read_text()[-12000:], file=sys.stderr)
        raise SystemExit(result.returncode)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-root", type=Path, required=True)
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--vulkan-include", default="")
    args = parser.parse_args()
    root = args.build_root.resolve()
    root.mkdir(parents=True, exist_ok=True)
    stage = root / "native-stage"
    prefix = stage / "opt/aeroshell"
    sources = json.loads((ROOT / "packaging/native-sources.json").read_text())
    env = dict(os.environ, DESTDIR=str(stage))
    for name, spec in sources.items():
        source = root / ("upstream-" + name)
        build = root / "native" / {"aeroshell-kwin-components": "effects", "uac-polkit-agent": "polkit"}.get(name, name)
        log = root / (name + ".log")
        if not source.exists():
            source.parent.mkdir(parents=True, exist_ok=True)
            run(["git", "init", source], log)
            run(["git", "-C", source, "remote", "add", "origin", spec["url"]], log)
            run(["git", "-C", source, "fetch", "--depth", "1", "origin", spec["revision"]], log)
            run(["git", "-C", source, "checkout", "--detach", "FETCH_HEAD"], log)
        actual = subprocess.check_output(["git", "-C", source, "rev-parse", "HEAD"], text=True).strip()
        if actual != spec["revision"]:
            raise SystemExit(f"Unexpected revision in {source}: {actual}")
        patch = ROOT / "patches/native" / (name + ".patch")
        if patch.exists():
            applied = subprocess.run(["git", "-C", source, "apply", "--reverse", "--check", patch], capture_output=True)
            if applied.returncode:
                run(["git", "-C", source, "apply", patch], log)
        expected = patch.read_bytes() if patch.exists() else b""
        actual_patch = subprocess.check_output(["git", "-C", source, "diff", "--binary"])
        if actual_patch != expected:
            raise SystemExit(f"Unrecorded source changes in {source}; update or remove the disposable checkout")
        licenses = prefix / "share/aeroshell/LICENSES" / name
        licenses.mkdir(parents=True, exist_ok=True)
        if (source / "LICENSES").is_dir():
            shutil.copytree(source / "LICENSES", licenses, dirs_exist_ok=True)
        for candidate in source.iterdir():
            if candidate.is_file() and candidate.name.startswith(("LICENSE", "COPYING")):
                shutil.copy2(candidate, licenses / candidate.name)
        if name == "gtk":
            destination = prefix / "share/themes/Windows-7-Better"
            destination.mkdir(parents=True, exist_ok=True)
            for toolkit in ("gtk-2.0", "gtk-3.0", "gtk-4.0"):
                shutil.copytree(source / toolkit, destination / toolkit, dirs_exist_ok=True)
            for asset in ("index.theme", "LICENSE"):
                shutil.copy2(source / asset, destination / asset)
            continue
        flags = [
            "-DCMAKE_INSTALL_PREFIX=/opt/aeroshell", "-DCMAKE_INSTALL_LIBDIR=lib",
            "-DKDE_INSTALL_LIBDIR=lib", "-DKDE_INSTALL_LIBEXECDIR=libexec",
            "-DKDE_INSTALL_QTPLUGINDIR=lib/qt6/plugins", "-DKDE_INSTALL_QMLDIR=lib/qt6/qml",
            "-DKDE_INSTALL_SYSCONFDIR=etc", "-DCMAKE_BUILD_TYPE=Release",
            "-DBUILD_TESTING=OFF", "-DBUILD_QCH=OFF", "-DBUILD_EXAMPLES=OFF",
            f"-DCMAKE_PREFIX_PATH={prefix}",
        ]
        if args.vulkan_include:
            flags.append(f"-DVulkan_INCLUDE_DIR={args.vulkan_include}")
        if name == "aeroshell-kwin-components":
            flags.append("-DKWIN_BUILD_WAYLAND=ON")
        run(["cmake", "-S", source, "-B", build, "-G", "Ninja", *flags], log)
        run(["cmake", "--build", build, "--parallel", args.jobs], root / (name + "-build.log"))
        run(["cmake", "--install", build], root / (name + "-install.log"), env=env)
        # Staging must never accumulate files outside our private prefix.
        for entry in stage.rglob("*"):
            if entry.is_file() and not entry.is_relative_to(prefix):
                raise SystemExit(f"Upstream installed outside private prefix: {entry}")
    (prefix / "share/aeroshell").mkdir(parents=True, exist_ok=True)
    (prefix / "share/aeroshell/native-sources.json").write_text(json.dumps(sources, indent=2) + "\n")
    receipt = {"sources": sources, "patches": {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                for p in sorted((ROOT / "patches/native").glob("*.patch"))}}
    (prefix / "share/aeroshell/native-build.json").write_text(json.dumps(receipt, indent=2) + "\n")


if __name__ == "__main__":
    main()
