#!/usr/bin/env python3
"""Reject stale prebuilt staging trees before packaging them."""
import hashlib
import json
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
prefix = Path(sys.argv[1]) / "opt/aeroshell"
expected = {"sources": json.loads((root / "packaging/native-sources.json").read_text()),
            "patches": {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                        for p in sorted((root / "patches/native").glob("*.patch"))}}
try:
    actual = json.loads((prefix / "share/aeroshell/native-build.json").read_text())
except (OSError, ValueError) as error:
    raise SystemExit(f"Missing or invalid native build receipt: {error}")
if actual != expected:
    raise SystemExit("Native staging tree does not match the pinned sources and integration patches; rebuild it")
for relative in ("lib/libPlasmaQuick.so.7", "lib/qt6/plugins/org.kde.kdecoration3/org.smod.smod.so",
                 "lib/qt6/plugins/plasma/applets/io.gitgud.wackyideas.SevenStart.so",
                 "share/plasma/shells/io.gitgud.wackyideas.desktop/metadata.json",
                 "share/themes/Windows-7-Better/gtk-3.0/gtk.css",
                 "share/Kvantum/Windows7Aero/Windows7Aero.svg", "libexec/uac-polkit-agent"):
    if not (prefix / relative).is_file():
        raise SystemExit(f"Native runtime component missing: {relative}")
print("PASS: native staging tree matches the source pins and patches")
