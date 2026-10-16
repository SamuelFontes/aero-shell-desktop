# Pinned source lock format

`upstream/sources.lock.json` is the only input file that selects upstream source revisions. It must be created from inspected upstream repositories and must not contain credentials. Do not write placeholder or guessed SHAs into the lock.

Each selected record has these fields:

| Field | Requirement |
|---|---|
| `name` | Unique lowercase package-style component name. |
| `canonical_url` | Credential-free HTTPS clone URL of the canonical upstream repository, not a read-only mirror. |
| `commit` | Full 40- or 64-character hexadecimal commit ID. Branch names and abbreviated IDs are rejected. |
| `license_file` | Optional safe repository-relative provenance path. Asset-rights verification is not required and the field does not gate pins. |
| `required` | Boolean. `true` means a build failure blocks the MVP desktop. |
| `excluded` | Boolean. Excluded components carry no URL, pin, or build dependencies and are never fetched. |
| `patch_directory` | Repository-relative patch-series directory or `null`. |
| `build_dependencies` | Names of other selected source records; the graph must be acyclic. |

SDDM-related components are represented only as `excluded: true` records. They must not enter the build graph or package. The validator rejects missing pins, URL credentials, path traversal, duplicate components, unknown/excluded dependencies, and dependency cycles.

Validate a completed lock with:

```sh
python scripts/validate-sources-lock.py
```

The lock is intentionally absent while the required Git hosts cannot be resolved. The validator's fixtures do not constitute upstream source verification.
