#!/usr/bin/env python3
"""Fixture checks for the default-deny staged-file policy."""

from __future__ import annotations

import importlib.util
import tempfile
import unittest
from subprocess import CompletedProcess
from unittest.mock import patch
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("install_audit", ROOT / "scripts" / "audit-install-manifests.py")
assert SPEC and SPEC.loader
AUDIT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUDIT)
POLICY = AUDIT.load_policy(ROOT / "packaging" / "install-policy.json")


class InstallPolicyTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name) / "stage"
        self.root.mkdir()

    def tearDown(self) -> None:
        self.temp.cleanup()

    def stage(self, dest: str, content: str = "x") -> Path:
        path = self.root / dest.lstrip("/")
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
        return path

    def test_accepts_private_tree_and_exact_integration_files(self) -> None:
        self.stage("/opt/aeroshell/bin/shell")
        self.stage("/usr/bin/aeroshell-session-wayland")
        self.stage("/usr/share/wayland-sessions/aeroshell.desktop")
        manifest, errors = AUDIT.audit(self.root, POLICY, [])
        self.assertEqual(errors, [])
        self.assertEqual(len(manifest), 10)  # Seven parent directories plus three files.

    def test_rejects_kde_library_overwrite(self) -> None:
        self.stage("/usr/lib/libplasma.so")
        _, errors = AUDIT.audit(self.root, POLICY, [])
        self.assertTrue(any("/usr/lib/libplasma.so" in error for error in errors))

    def test_rejects_sddm_configuration(self) -> None:
        self.stage("/etc/sddm.conf.d/aeroshell.conf")
        _, errors = AUDIT.audit(self.root, POLICY, [])
        self.assertTrue(any("/etc/sddm.conf.d/aeroshell.conf" in error for error in errors))

    def test_rejects_sddm_asset_even_inside_private_prefix(self) -> None:
        self.stage("/opt/aeroshell/share/sddm/themes/aeroshell/theme.conf")
        _, errors = AUDIT.audit(self.root, POLICY, [])
        self.assertTrue(any("/opt/aeroshell/share/sddm/" in error for error in errors))

    def test_rejects_sddm_reference_case_insensitively(self) -> None:
        self.stage("/opt/aeroshell/share/SDDM/aeroshell.conf")
        _, errors = AUDIT.audit(self.root, POLICY, [])
        self.assertTrue(any("/opt/aeroshell/share/SDDM/" in error for error in errors))

    def test_rejects_unexpected_usr_executable(self) -> None:
        self.stage("/usr/bin/other-shell")
        _, errors = AUDIT.audit(self.root, POLICY, [])
        self.assertTrue(any("/usr/bin/other-shell" in error for error in errors))

    def test_private_prefix_root_must_be_a_directory(self) -> None:
        self.stage("/opt/aeroshell")
        _, errors = AUDIT.audit(self.root, POLICY, [])
        self.assertTrue(any("/opt/aeroshell: destination is not allowed" in error for error in errors))

    def test_rejects_symlink_escape(self) -> None:
        link = self.root / "opt" / "aeroshell" / "bin" / "escape"
        link.parent.mkdir(parents=True)
        link.symlink_to("../../../../etc/passwd")
        _, errors = AUDIT.audit(self.root, POLICY, [])
        self.assertTrue(any("symlink target" in error for error in errors))

    def test_private_prefix_symlink_cannot_point_at_system_entry(self) -> None:
        link = self.root / "opt" / "aeroshell" / "bin" / "session"
        link.parent.mkdir(parents=True)
        link.symlink_to("/usr/bin/aeroshell-session-wayland")
        _, errors = AUDIT.audit(self.root, POLICY, [])
        self.assertTrue(any("private-prefix symlink must stay within" in error for error in errors))

    def test_session_integration_entry_must_be_a_regular_file(self) -> None:
        link = self.root / "usr/bin/aeroshell-session-wayland"
        link.parent.mkdir(parents=True)
        link.symlink_to("/opt/aeroshell/bin/session")
        _, errors = AUDIT.audit(self.root, POLICY, [])
        self.assertTrue(any("/usr/bin/aeroshell-session-wayland: destination is not allowed" in error for error in errors))

    def test_rejects_any_package_hook(self) -> None:
        self.stage("/opt/aeroshell/bin/shell")
        hook = self.root / "install.hook"
        hook.write_text("systemctl enable aeroshell.service", encoding="utf-8")
        _, errors = AUDIT.audit(self.root, POLICY, [hook])
        self.assertTrue(any("hooks are forbidden" in error for error in errors))

    def test_does_not_follow_staged_symlink_directories(self) -> None:
        self.stage("/opt/aeroshell/bin/shell")
        link = self.root / "opt" / "aeroshell" / "alias"
        link.symlink_to("/usr/share")
        _, errors = AUDIT.audit(self.root, POLICY, [])
        self.assertTrue(any("symlink target" in error for error in errors))
        self.assertFalse(any("/usr/share/wayland-sessions/aeroshell.desktop" in error for error in errors))

    def test_target_owner_check_rejects_other_package(self) -> None:
        self.stage("/usr/bin/aeroshell-session-wayland")
        installed = self.root / "usr/bin/aeroshell-session-wayland"
        installed.parent.mkdir(parents=True, exist_ok=True)
        installed.write_text("old", encoding="utf-8")
        manifest, errors = AUDIT.audit(self.root, POLICY, [])
        with patch.object(AUDIT.subprocess, "run", return_value=CompletedProcess([], 0, "sddm\n", "")):
            target_errors = AUDIT.check_target_collisions(manifest, self.root, {"aeroshell-desktop-git"})
        self.assertTrue(any("owned by package 'sddm'" in error for error in target_errors))
        self.assertEqual(errors, [])

    def test_target_owner_check_allows_same_package_upgrade(self) -> None:
        self.stage("/usr/bin/aeroshell-session-wayland")
        installed = self.root / "usr/bin/aeroshell-session-wayland"
        installed.parent.mkdir(parents=True, exist_ok=True)
        installed.write_text("old", encoding="utf-8")
        manifest, _ = AUDIT.audit(self.root, POLICY, [])
        with patch.object(AUDIT.subprocess, "run", return_value=CompletedProcess([], 0, "aeroshell-desktop-git\n", "")):
            target_errors = AUDIT.check_target_collisions(manifest, self.root, {"aeroshell-desktop-git"})
        self.assertEqual(target_errors, [])

    def test_target_owner_check_rejects_unowned_file(self) -> None:
        self.stage("/usr/bin/aeroshell-session-wayland")
        installed = self.root / "usr/bin/aeroshell-session-wayland"
        installed.parent.mkdir(parents=True, exist_ok=True)
        installed.write_text("manual", encoding="utf-8")
        manifest, _ = AUDIT.audit(self.root, POLICY, [])
        with patch.object(AUDIT.subprocess, "run", return_value=CompletedProcess([], 1, "", "")):
            target_errors = AUDIT.check_target_collisions(manifest, self.root, set())
        self.assertTrue(any("unowned target file already exists" in error for error in target_errors))

    def test_target_owner_check_rejects_path_redirected_outside_root(self) -> None:
        self.stage("/usr/bin/aeroshell-session-wayland")
        outside = Path(self.temp.name) / "outside"
        outside.mkdir()
        manifest, _ = AUDIT.audit(self.root, POLICY, [])
        (self.root / "usr/bin/aeroshell-session-wayland").unlink()
        (self.root / "usr/bin").rmdir()
        (self.root / "usr").rmdir()
        (self.root / "usr").symlink_to(outside, target_is_directory=True)
        with patch.object(AUDIT.subprocess, "run") as run:
            target_errors = AUDIT.check_target_collisions(manifest, self.root, set())
        self.assertTrue(any("resolves outside the target root" in error for error in target_errors))
        run.assert_not_called()

    def test_target_owner_check_reports_missing_target_root_cleanly(self) -> None:
        with self.assertRaisesRegex(ValueError, "target root does not exist"):
            AUDIT.check_target_collisions([], self.root / "missing", set())


if __name__ == "__main__":
    unittest.main()
