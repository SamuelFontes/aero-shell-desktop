#!/usr/bin/env python3
"""Exercise the production launcher against a disposable home and fake compositor."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class ProfileTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.home = self.root / "home"
        self.home.mkdir()
        self.prefix = self.root / "prefix"
        for name in ("share/plasma/shells/io.gitgud.wackyideas.desktop/metadata.json",
                     "share/plasma/desktoptheme/Seven-Black/metadata.json",
                     "share/Kvantum/Windows7Aero/Windows7Aero.kvconfig",
                     "share/Kvantum/Windows7Aero/Windows7Aero.svg",
                     "share/aeroshell/native-sources.json", "libexec/aeroshell-native-desktop"):
            path = self.prefix / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.touch()
        shutil.copytree(ROOT / "defaults/native", self.prefix / "share/aeroshell/native-defaults")
        self.config = self.home / ".config"
        self.config.mkdir()
        (self.config / "kdeglobals").write_text("[General]\nColorScheme=ExistingKDE\n")
        (self.config / "plasmashellrc").write_text("existing Plasma layout\n")
        (self.config / "user-dirs.dirs").write_text('XDG_DESKTOP_DIR="$HOME/Work Desktop"\n')
        runtime = self.root / "runtime"
        runtime.mkdir(mode=0o700)
        self.env = {"HOME": str(self.home), "AEROSHELL_PREFIX": str(self.prefix),
                    "XDG_RUNTIME_DIR": str(runtime), "PATH": os.environ["PATH"]}
        self.private = self.config / "aeroshell/native"

    def launch(self, *args):
        return subprocess.run(["python3", str(ROOT / "session/aeroshell-native-session"), *args],
                              env=self.env, capture_output=True, text=True)

    def test_first_login_and_existing_preferences(self):
        before = {p.name: p.read_bytes() for p in self.config.iterdir()}
        result = self.launch("--prepare-only")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("widgetStyle=kvantum", (self.private / "kdeglobals").read_text())
        self.assertIn("library=org.smod.smod", (self.private / "kwinrc").read_text())
        self.assertIn(str(self.prefix), (self.private / "kscreenlockerrc").read_text())
        self.assertEqual((self.private / "user-dirs.dirs").read_bytes(), before["user-dirs.dirs"])
        (self.private / "kwinrc").write_text("user customization\n")
        self.assertEqual(self.launch("--prepare-only").returncode, 0)
        self.assertEqual((self.private / "kwinrc").read_text(), "user customization\n")
        for name, content in before.items():
            self.assertEqual((self.config / name).read_bytes(), content)

    def test_refuse_redirected_profile(self):
        (self.config / "aeroshell").symlink_to(self.config, target_is_directory=True)
        self.assertNotEqual(self.launch("--prepare-only").returncode, 0)
        self.assertEqual((self.config / "kdeglobals").read_text(), "[General]\nColorScheme=ExistingKDE\n")

    def test_refuse_linked_settings(self):
        self.assertEqual(self.launch("--prepare-only").returncode, 0)
        (self.private / "kwinrc").unlink()
        os.link(self.config / "kdeglobals", self.private / "kwinrc")
        self.assertNotEqual(self.launch("--prepare-only").returncode, 0)

    def test_production_environment_is_private(self):
        fakebin = self.root / "bin"
        fakebin.mkdir()
        for name in ("kwin_wayland", "dbus-run-session"):
            p = fakebin / name
            p.write_text('#!/usr/bin/python3\nimport json,os,sys\nprint(json.dumps({"env":dict(os.environ),"argv":sys.argv}))\n')
            p.chmod(0o755)
        self.env.update({"PATH": str(fakebin) + ":" + self.env["PATH"],
                         "DBUS_SESSION_BUS_ADDRESS": "original-bus", "QT_QUICK_CONTROLS_CONF": "prototype-style"})
        result = self.launch()
        self.assertEqual(result.returncode, 0, result.stderr)
        child = json.loads(result.stdout)
        self.assertEqual(child["env"]["XDG_CONFIG_HOME"], str(self.private))
        self.assertNotIn("DBUS_SESSION_BUS_ADDRESS", child["env"])
        self.assertNotIn("QT_QUICK_CONTROLS_CONF", child["env"])
        self.assertEqual(child["env"]["QT_STYLE_OVERRIDE"], "kvantum")
        self.assertEqual(child["env"]["PLASMA_DEFAULT_SHELL"], "io.gitgud.wackyideas.desktop")
        self.assertIn(str(self.prefix / "libexec/aeroshell-native-desktop"), child["argv"])
        self.assertNotIn("startplasma-wayland", child["argv"])


if __name__ == "__main__":
    unittest.main()
