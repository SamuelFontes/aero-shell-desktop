# Decision 0003: service and session lifecycle

**Status:** Unresolved; evidence required before T07 implementation.

**Question:** How are the compositor, shell, portals, policy agent, screen locker, and other required services started and stopped without leaking environment or affecting KDE?

**Constraints:** Never globally enable AeroShell services, kill processes by broad name, restart shared user targets, or rely only on an exit trap to undo shared activation state after forced termination. Do not modify SDDM or other display-manager settings.

**Evidence to collect:** Pinned service/activation definitions, D-Bus names, systemd user-manager interactions, process ownership, normal logout and forced-termination results, same-user concurrent-session behavior.

**Alternatives and consequences:** To be completed from source inspection and tests.
