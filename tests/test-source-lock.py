#!/usr/bin/env python3
"""Fixture checks for immutable and safe upstream source metadata."""

from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("source_lock", ROOT / "scripts" / "validate-sources-lock.py")
assert SPEC and SPEC.loader
LOCK = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(LOCK)


def source(name: str, dependencies: list[str] | None = None) -> dict[str, object]:
    return {
        "name": name,
        "canonical_url": f"https://gitgud.io/aeroshell/{name}.git",
        "commit": "a" * 40,
        "license_file": "LICENSE",
        "required": True,
        "excluded": False,
        "patch_directory": f"patches/{name}",
        "build_dependencies": dependencies or [],
    }


class SourceLockTests(unittest.TestCase):
    def valid_lock(self) -> dict[str, object]:
        return {"schema_version": 1, "sources": [source("libplasma"), source("aeroshell-workspace", ["libplasma"])]}

    def test_accepts_full_pins_and_acyclic_dependencies(self) -> None:
        self.assertEqual(LOCK.validate_lock(self.valid_lock()), [])

    def test_asset_rights_metadata_is_not_required(self) -> None:
        data = self.valid_lock()
        for record in data["sources"]:  # type: ignore[union-attr]
            record.pop("license_file", None)
        self.assertEqual(LOCK.validate_lock(data), [])

    def test_rejects_unsafe_optional_license_path(self) -> None:
        data = self.valid_lock()
        data["sources"][0]["license_file"] = "../outside/LICENSE"  # type: ignore[index]
        self.assertTrue(any("license_file" in e for e in LOCK.validate_lock(data)))

    def test_rejects_empty_lock(self) -> None:
        self.assertTrue(any("non-empty list" in e for e in LOCK.validate_lock({"schema_version": 1, "sources": []})))

    def test_rejects_mutable_branch_or_short_commit(self) -> None:
        data = self.valid_lock()
        data["sources"][0]["commit"] = "Plasma/6.7"  # type: ignore[index]
        self.assertTrue(any("full 40- or 64-character" in e for e in LOCK.validate_lock(data)))

    def test_rejects_credentials_in_url(self) -> None:
        data = self.valid_lock()
        data["sources"][0]["canonical_url"] = "https://user:secret@gitgud.io/aeroshell/libplasma.git"  # type: ignore[index]
        self.assertTrue(any("contain no credentials" in e for e in LOCK.validate_lock(data)))

    def test_rejects_unknown_dependency(self) -> None:
        data = {"schema_version": 1, "sources": [source("workspace", ["missing"])]}
        self.assertTrue(any("unknown source" in e for e in LOCK.validate_lock(data)))

    def test_rejects_dependency_cycle(self) -> None:
        data = {"schema_version": 1, "sources": [source("one", ["two"]), source("two", ["one"])]}
        self.assertTrue(any("dependency cycle" in e for e in LOCK.validate_lock(data)))

    def test_rejects_selected_sddm_component(self) -> None:
        data = {"schema_version": 1, "sources": [source("aeroshell-sddm-kcm")]}
        self.assertTrue(any("must be marked excluded" in e for e in LOCK.validate_lock(data)))

    def test_accepts_sddm_component_only_as_nonfetchable_exclusion(self) -> None:
        excluded = {
            "name": "aeroshell-sddm-kcm",
            "canonical_url": None,
            "commit": None,
            "license_file": None,
            "required": False,
            "excluded": True,
            "patch_directory": None,
            "build_dependencies": [],
        }
        self.assertEqual(LOCK.validate_lock({"schema_version": 1, "sources": [excluded]}), [])

    def test_rejects_path_traversal_in_patch_directory(self) -> None:
        data = self.valid_lock()
        data["sources"][0]["patch_directory"] = "patches/../outside"  # type: ignore[index]
        self.assertTrue(any("patch_directory" in e for e in LOCK.validate_lock(data)))


if __name__ == "__main__":
    unittest.main()
