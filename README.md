# AeroShell Desktop

AeroShell is a planned independent desktop environment built from AeroThemePlasma components. The first release targets Wayland on Arch Linux and Manjaro and aims to coexist with an existing KDE Plasma installation.

**Project status: planning and source audit.** This repository does not yet contain a working session or installer. Do not run upstream installation scripts on a host system.

AeroShell must keep its desktop settings, processes, and services separate from Plasma. The project registers a standard Wayland session entry; display-manager configuration and login-screen themes remain user-managed. See [plan.md](plan.md) for requirements, task order, evidence rules, and acceptance criteria.

## Development status

Task progress and evidence are tracked in [docs/tasks.md](docs/tasks.md). Upstream source pins and install behavior must be verified before build or package work begins.
