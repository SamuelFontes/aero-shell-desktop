# Source lock validator evidence

**Date:** 2026-10-16  
**Environment:** Local repository; no upstream source was fetched.

## Checks

- `python tests/test-source-lock.py` — 11 fixture checks passed for valid pins, optional license metadata, unsafe optional provenance paths, empty lock, mutable/short refs, URL credentials, unknown dependencies, dependency cycles, selected SDDM component, excluded SDDM record, and path traversal.
- `python -m py_compile scripts/validate-sources-lock.py tests/test-source-lock.py` — passed.
- `git diff --check` — passed.

## Scope limits

The tool validates lock structure only. It does not prove that a commit exists, that a URL is canonical, that the license text is valid, or that the source builds. `upstream/sources.lock.json` is absent until GitHub/GitGud can be resolved and source revisions can be inspected.
