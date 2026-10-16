#!/usr/bin/env python3
"""Validate pinned upstream source metadata before any AeroShell build."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path, PurePosixPath
from typing import Any
from urllib.parse import urlsplit


SHA_RE = re.compile(r"^(?:[0-9a-f]{40}|[0-9a-f]{64})$", re.IGNORECASE)
NAME_RE = re.compile(r"^[a-z0-9][a-z0-9-]*$")


def is_relative_repo_path(value: Any) -> bool:
    if not isinstance(value, str) or not value or value.startswith("/"):
        return False
    path = PurePosixPath(value)
    return ".." not in path.parts and "." not in path.parts


def validate_lock(data: Any) -> list[str]:
    errors: list[str] = []
    if not isinstance(data, dict):
        return ["source lock root must be a JSON object"]
    if data.get("schema_version") != 1:
        errors.append("schema_version must be 1")
    sources = data.get("sources")
    if not isinstance(sources, list) or not sources:
        return errors + ["sources must be a non-empty list"]

    by_name: dict[str, dict[str, Any]] = {}
    for index, source in enumerate(sources):
        label = f"sources[{index}]"
        if not isinstance(source, dict):
            errors.append(f"{label} must be an object")
            continue

        name = source.get("name")
        if not isinstance(name, str) or not NAME_RE.fullmatch(name):
            errors.append(f"{label}.name must be a lowercase package-style name")
            continue
        if name in by_name:
            errors.append(f"duplicate source name: {name}")
            continue
        by_name[name] = source

        excluded = source.get("excluded", False)
        required = source.get("required")
        if not isinstance(excluded, bool):
            errors.append(f"{name}.excluded must be a boolean")
            excluded = False
        if not isinstance(required, bool):
            errors.append(f"{name}.required must be a boolean")
            required = False
        if excluded and required:
            errors.append(f"{name} cannot be both excluded and required")

        dependencies = source.get("build_dependencies")
        if not isinstance(dependencies, list) or not all(isinstance(dep, str) for dep in dependencies):
            errors.append(f"{name}.build_dependencies must be a list of source names")
        elif len(dependencies) != len(set(dependencies)):
            errors.append(f"{name}.build_dependencies contains duplicates")
        elif name in dependencies:
            errors.append(f"{name} cannot depend on itself")

        if excluded:
            if any(source.get(key) is not None for key in ("canonical_url", "commit", "license_file")):
                errors.append(f"{name} is excluded and must not carry fetchable source metadata")
            if dependencies:
                errors.append(f"{name} is excluded and must not have build dependencies")
            continue

        canonical_url = source.get("canonical_url")
        if not isinstance(canonical_url, str):
            errors.append(f"{name}.canonical_url must be an HTTPS URL")
        else:
            parsed = urlsplit(canonical_url)
            if parsed.scheme != "https" or not parsed.hostname or parsed.username or parsed.password:
                errors.append(f"{name}.canonical_url must use HTTPS and contain no credentials")
            if parsed.query or parsed.fragment:
                errors.append(f"{name}.canonical_url must not contain a query or fragment")

        commit = source.get("commit")
        if not isinstance(commit, str) or not SHA_RE.fullmatch(commit) or set(commit.lower()) == {"0"}:
            errors.append(f"{name}.commit must be a full 40- or 64-character hexadecimal commit")

        # Optional source provenance only. The user has explicitly removed asset-rights
        # verification from the completion criteria, so it must not gate source pins.
        license_file = source.get("license_file")
        if license_file is not None and not is_relative_repo_path(license_file):
            errors.append(f"{name}.license_file must be null or a safe repository-relative path")

        patch_directory = source.get("patch_directory")
        if patch_directory is not None and not is_relative_repo_path(patch_directory):
            errors.append(f"{name}.patch_directory must be null or a safe repository-relative path")

        if "sddm" in name.casefold() or (isinstance(canonical_url, str) and "sddm" in canonical_url.casefold()):
            errors.append(f"{name} references SDDM and must be marked excluded")

    selected = {name: source for name, source in by_name.items() if not source.get("excluded", False)}
    for name, source in selected.items():
        dependencies = source.get("build_dependencies", [])
        if not isinstance(dependencies, list):
            continue
        for dependency in dependencies:
            if dependency not in by_name:
                errors.append(f"{name} depends on unknown source {dependency!r}")
            elif dependency not in selected:
                errors.append(f"{name} depends on excluded source {dependency!r}")

    visiting: set[str] = set()
    visited: set[str] = set()

    def visit(name: str, chain: list[str]) -> None:
        if name in visiting:
            start = chain.index(name) if name in chain else 0
            errors.append("source dependency cycle: " + " -> ".join(chain[start:] + [name]))
            return
        if name in visited or name not in selected:
            return
        visiting.add(name)
        source = selected[name]
        dependencies = source.get("build_dependencies", [])
        if isinstance(dependencies, list):
            for dependency in dependencies:
                if dependency in selected:
                    visit(dependency, chain + [name])
        visiting.remove(name)
        visited.add(name)

    for name in selected:
        visit(name, [])
    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "lock",
        nargs="?",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "upstream" / "sources.lock.json",
        help="source lock JSON (defaults to upstream/sources.lock.json)",
    )
    args = parser.parse_args(argv)
    try:
        data = json.loads(args.lock.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(f"ERROR: cannot read {args.lock}: {exc}", file=sys.stderr)
        return 2
    errors = validate_lock(data)
    if errors:
        for error in errors:
            print(f"REJECT: {error}", file=sys.stderr)
        return 1
    print(f"PASS: validated {len(data['sources'])} pinned source records")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
