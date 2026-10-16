# Implementation task ledger

Task states: `pending`, `in-progress`, `blocked`, `done`. A blocked task records the observed failure and the action needed to unblock it. Do not report a check as passed unless its evidence exists.

## Task handoff template

```text
Task: Txx — title
Status: pending | in-progress | blocked | done
Dependencies:
Inputs inspected: paths and pinned revisions
Files changed:
Checks: command, environment, exit code, evidence path
Result: behavior demonstrated
Limitations / blocker:
Next task:
```

## Tasks

| ID | Task | Dependencies | Status | Evidence / blocker |
|---|---|---|---|---|
| T00 | Establish repository and task ledger | None | done | README, ignore rules, task ledger, and three decision records added. `git diff --check` and scaffold assertions passed. |
| T01 | Discover and pin upstream sources | T00 | blocked | Partial web audit in `docs/upstream-audit.md`; exact pins and checkout blocked by DNS failures for GitHub/GitGud. Retry `git ls-remote` and inspect full source trees when network is available. |
| T02 | Inventory installation and Wayland feasibility | T01 | pending | |
| T03 | Define staged-file policy and checker | T02 | pending | |
| T04 | Build the smallest dependency closure privately | T01–T03 | pending | |
| T05 | Prove library and plugin selection | T04 | pending | |
| T06 | Implement private profile provisioning | T02, T05 | pending | |
| T07 | Resolve service, activation, and environment ownership | T05, T06 | pending | |
| T08 | Implement the Wayland session launcher | T05–T07 | pending | |
| T09 | Prove a real minimal Wayland desktop | T08 | pending | Requires disposable Arch system with working Plasma |
| T10 | Add Wayland-safe appearance and settings | T09 | pending | |
| T11 | Complete Wayland integration and security behavior | T09, T07 | pending | Requires graphical test facility |
| T12 | Implement version compatibility checks | T05, T09 | pending | |
| T13 | Create managed Arch packages | T03–T12 | pending | |
| T14 | Exercise installation and failure recovery | T13 | pending | Requires disposable system |
| T15 | Add repeatable build and static checks | T03, T04, T12, T13 | pending | |
| T16 | Release candidate review and documentation | T10–T15 | pending | |
