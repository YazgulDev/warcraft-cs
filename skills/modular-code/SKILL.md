---
name: modular-code
description: Organize source code by cohesive modules and apply clean code, SOLID and concrete design patterns when adding behavior or restructuring a project. Preserve public entry points and existing runtime contracts.
---

# Modular code

Read the project's AGENTS.md and current source/build structure before choosing module boundaries. Group files by the behavior they own, not by file extension or generic names such as Helpers. Keep each public type in its own file; keep its implementation alongside it.

- Domain modules own rules and data. Input and presentation translate events and state; platform adapters own native APIs, files, devices and engine hooks. The application layer coordinates these modules through explicit dependencies.
- Apply SOLID to actual change boundaries: one responsibility per collaborator, narrow contracts, substitutable implementations and dependency direction toward domain rules. Prefer composition. Introduce an interface only when a real alternate implementation or testing boundary needs it.
- Use Strategy for varying rules, State for explicit lifecycle transitions, Adapter for engine/platform integration, and a dispatcher/mailbox for input-to-game-thread coordination when those solve a concrete problem. Explain non-obvious ownership and timing constraints in nearby comments. Avoid service locators, speculative factories and arbitrary partial-class splits.
- During moves, update includes/imports, build manifests, tests, packaging and documentation together. Preserve public launcher/setup script paths, asset identifiers, saved data and configuration keys unless the user requests changes. Do not leave duplicate compatibility source files that can compile twice.
- Test independent rules without game/platform state where practical. Verify structural changes with the real build and packaging path; rendering/hook changes also need a disposable native scene. Keep verification fixtures out of ordinary client builds.
- Report the resulting modules, relevant patterns and verification. A file move alone does not prove a large controller has been decomposed; state remaining coupling accurately.

For Warcraft CS, use `src/{runtime,platform,input,application,config,audio,movement,combat,economy,inventory,squad,geometry,presentation}`. Native hook adapters belong in `platform`; portable geometry and movement rules do not depend on presentation. Launcher modules live beneath `launcher`, while its PowerShell entry points keep their existing paths.
