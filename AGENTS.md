# Repository Guidelines

## Scope and continuity

This is the Animate-only SMK-37 firmware repository. Read `docs/REPOSITORY.md`,
`docs/SOUND_CONTRACT.md`, and `docs/INSTALL.md` before changing architecture or
installation behavior. If this checkout sits inside a larger workspace, also
follow that workspace's own agent instructions and handoff notes; newer owner
acceptance records supersede older claims.

## Implementation boundaries

The shared runtime drives the simulator and hardware. The active voice kernel
is `src/baseline/voice_kernel.c`; preserve block-rate preparation and parallel
voice pairing. `reference/desktop_voice.c` is not the active engine. Preserve
the agreed sound contract, live sequence editing, slot controls and independent
note ownership. Automatic overload shedding is disabled by owner request.

State the intended behavior and affected ownership boundary before editing.
Use `apply_patch` for edits; obtain approval before using an alternative. Keep
changes scoped, and distinguish implementation, automated checks and actual
hardware acceptance. Do not flash hardware, erase storage or replace bootloaders
without authorization. Do not restore automatic voice shedding implicitly.

## Development and verification

`python3 tools/check_runtime.py` runs native runtime and audio comparisons;
`python3 tools/build.py smk` builds the board application in the existing
WSL/toolchain environment. `node tools/package.mjs` packages only the reviewed
hash-pinned candidate. Dependencies are local under `vendor/` and `assets/`;
use `docs/BUILDING.md` for standalone setup. Run builds and tests only
within the owner's authorized scope.

## Repository and publication

Do not commit build output, device backups, toolchains or history from other repositories.
Do not publish or add a remote without the owner's destination and authorization.
Use past-tense, type-prefixed commit summaries, such as `feat: Added wave browsing`.
Keep build/test status out of commit messages. Reconcile current-version
changelog entries rather than appending contradictory implementation history.
