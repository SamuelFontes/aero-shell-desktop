#!/usr/bin/env python3
"""Audit a DESTDIR staging tree against AeroShell's default-deny path policy."""

from __future__ import annotations

import argparse
import json
import os
import posixpath
import stat
import subprocess
import sys
from pathlib import Path, PurePosixPath
from typing import Any


def load_policy(path: Path) -> dict[str, Any]:
    try:
        policy = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ValueError(f"cannot read policy {path}: {exc}") from exc
    if policy.get("schema_version") != 1:
        raise ValueError("unsupported or missing policy schema_version")
    for key in ("allow_exact", "allow_prefixes", "allow_parent_directories", "deny_prefixes"):
        values = policy.get(key)
        if not isinstance(values, list) or not all(isinstance(x, str) for x in values):
            raise ValueError(f"policy field {key!r} must be a list of paths")
        for value in values:
            p = PurePosixPath(value)
            if not value.startswith("/") or ".." in p.parts:
                raise ValueError(f"policy contains a non-absolute or unsafe path: {value!r}")
    substrings = policy.get("deny_path_substrings", [])
    if not isinstance(substrings, list) or not all(isinstance(x, str) and x for x in substrings):
        raise ValueError("policy field 'deny_path_substrings' must be a list of non-empty strings")
    return policy


def normalized(path: str) -> str:
    """Normalize a destination path without consulting or following host paths."""
    if not path.startswith("/"):
        raise ValueError(f"destination is not absolute: {path!r}")
    clean = posixpath.normpath(path)
    if clean != path.rstrip("/") and clean != "/":
        raise ValueError(f"destination is not normalized: {path!r}")
    if ".." in PurePosixPath(path).parts:
        raise ValueError(f"destination contains traversal: {path!r}")
    return clean


def is_denied(dest: str, policy: dict[str, Any]) -> bool:
    if any(dest == prefix.rstrip("/") or dest.startswith(prefix) for prefix in policy["deny_prefixes"]):
        return True
    lowered = dest.casefold()
    return any(token.casefold() in lowered for token in policy.get("deny_path_substrings", []))


def is_allowed(dest: str, is_dir: bool, policy: dict[str, Any], is_symlink: bool = False) -> bool:
    if is_denied(dest, policy):
        return False
    if dest in policy["allow_exact"]:
        return not is_dir and not is_symlink
    for prefix in policy["allow_prefixes"]:
        if dest == prefix.rstrip("/"):
            return is_dir and not is_symlink
        if dest.startswith(prefix):
            return True
    if is_dir and dest in policy["allow_parent_directories"]:
        return True
    return False


def symlink_target_destination(dest: str, target: str) -> str:
    if not target:
        raise ValueError("symlink has an empty target")
    joined = target if target.startswith("/") else posixpath.join(posixpath.dirname(dest), target)
    clean = posixpath.normpath(joined)
    if ".." in PurePosixPath(clean).parts:
        raise ValueError(f"symlink target escapes destination root: {target!r}")
    return clean


def check_target_collisions(
    manifest: list[str], target_root: Path, allowed_owners: set[str]
) -> list[str]:
    """Reject staged files that already exist outside this package in target root."""
    errors: list[str] = []
    try:
        root = target_root.resolve(strict=True)
    except OSError as exc:
        raise ValueError(f"target root does not exist: {target_root}: {exc}") from exc
    for line in manifest:
        kind, dest = line.split("\t", 1)
        if kind == "directory":
            continue
        candidate = root / dest.lstrip("/")
        try:
            resolved = candidate.resolve(strict=False)
            resolved.relative_to(root)
        except (OSError, ValueError):
            errors.append(f"{dest}: target path resolves outside the target root")
            continue
        if not os.path.lexists(candidate):
            continue
        try:
            result = subprocess.run(
                ["pacman", "--root", str(root), "-Qqo", "--", str(candidate)],
                check=False,
                capture_output=True,
                text=True,
                timeout=10,
            )
        except (OSError, subprocess.TimeoutExpired) as exc:
            errors.append(f"{dest}: cannot query pacman ownership: {exc}")
            continue
        owner = result.stdout.strip()
        if result.returncode == 0 and owner in allowed_owners:
            continue
        if result.returncode == 0:
            errors.append(f"{dest}: already owned by package {owner!r}")
        elif result.returncode == 1 and not owner:
            errors.append(f"{dest}: an unowned target file already exists")
        else:
            detail = result.stderr.strip() or f"pacman exited with {result.returncode}"
            errors.append(f"{dest}: pacman ownership query failed: {detail}")
    return errors


def audit(stage_root: Path, policy: dict[str, Any], hooks: list[Path]) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    manifest: list[str] = []
    try:
        root = stage_root.resolve(strict=True)
    except OSError as exc:
        raise ValueError(f"staging root does not exist: {stage_root}: {exc}") from exc
    if not root.is_dir():
        raise ValueError(f"staging root is not a directory: {root}")

    # Walk with followlinks disabled: package symlinks are inspected as entries,
    # never traversed into arbitrary host or staging locations.
    for current, dirnames, filenames in os.walk(root, topdown=True, followlinks=False):
        current_path = Path(current)
        names = list(dirnames) + list(filenames)
        for name in names:
            entry = current_path / name
            rel = entry.relative_to(root).as_posix()
            dest = normalized("/" + rel)
            try:
                info = entry.lstat()
            except OSError as exc:
                errors.append(f"{dest}: cannot inspect staged entry: {exc}")
                continue

            if stat.S_ISLNK(info.st_mode):
                kind = "symlink"
                try:
                    target = os.readlink(entry)
                    target_dest = symlink_target_destination(dest, target)
                    if not is_allowed(target_dest, False, policy):
                        errors.append(f"{dest}: symlink target is outside approved destinations: {target!r}")
                    if dest.startswith("/opt/aeroshell/") and not target_dest.startswith("/opt/aeroshell/"):
                        errors.append(f"{dest}: private-prefix symlink must stay within /opt/aeroshell: {target!r}")
                except ValueError as exc:
                    errors.append(f"{dest}: {exc}")
            elif stat.S_ISDIR(info.st_mode):
                kind = "directory"
            elif stat.S_ISREG(info.st_mode):
                kind = "file"
            else:
                kind = "special file"
                errors.append(f"{dest}: special files are forbidden")

            manifest.append(f"{kind}\t{dest}")
            if not is_allowed(dest, stat.S_ISDIR(info.st_mode), policy, stat.S_ISLNK(info.st_mode)):
                errors.append(f"{dest}: destination is not allowed by policy")

            # Do not recurse into staged symlinked directories.
            if name in dirnames and stat.S_ISLNK(info.st_mode):
                dirnames.remove(name)

    for hook in hooks:
        errors.append(f"{hook}: package/install hooks are forbidden by the initial policy")
    return sorted(manifest), errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage-root", required=True, type=Path, help="DESTDIR staging root to inspect")
    parser.add_argument(
        "--policy",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "packaging" / "install-policy.json",
        help="JSON install path policy (defaults to the repository policy)",
    )
    parser.add_argument("--hook-file", action="append", default=[], type=Path, help="package/install hook; any hook fails the initial policy")
    parser.add_argument("--check-target", type=Path, help="also reject existing target files unless pacman says they belong to an allowed package")
    parser.add_argument("--allow-target-owner", action="append", default=[], help="package name allowed to own an existing target file during an upgrade; repeat as needed")
    parser.add_argument("--print-manifest", action="store_true", help="print the audited staged file inventory")
    args = parser.parse_args(argv)
    try:
        policy = load_policy(args.policy)
        manifest, errors = audit(args.stage_root, policy, args.hook_file)
        if args.check_target:
            errors.extend(check_target_collisions(manifest, args.check_target, set(args.allow_target_owner)))
    except ValueError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2

    if args.print_manifest:
        for line in manifest:
            print(line)
    if errors:
        for error in errors:
            print(f"REJECT: {error}", file=sys.stderr)
        return 1
    print(f"PASS: audited {len(manifest)} staged entries")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
