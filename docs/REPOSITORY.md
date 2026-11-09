# Repository layout

Animate is a standalone Git repository on `main`. Everything needed to build,
simulate, package and test the firmware is inside it; compilers are installed
separately (see [BUILDING.md](BUILDING.md)).

## Local dependencies

The build, simulator, packaging and regression checks use copies kept in this
repository:

- `vendor/melodee/`: preserved board HAL, loader, DSP/reference source, build
  helpers, regression oracle, web updater modules, generated lookup tables and
  original license/attribution files. `vendor/melodee/build/gen` contains frozen
  source inputs, not disposable Animate build output; keep these files tracked.
- `vendor/tlbx-display/`: the shared display adapter and visualizer source.
- `assets/factory/`: the 285 source WAVs selected by the curation manifest.
- `assets/stock/`: where you place M-VAVE's official Pro V016 firmware for
  packaging (not included; see its README).
- `tests/fixtures/`: the saved USER VECTOR session used by the legacy oracle.

Required resources must stay in this repository so that a fresh clone builds.
Line endings are fixed by `.gitattributes` (LF, with CRLF for `.cmd`, `.bat` and
`.ps1`).

## Publication

Local independence does not establish redistribution permission. The licence
and attribution for everything in this tree are in [LICENSING.md](../LICENSING.md);
the top-level [LICENSE](../LICENSE) is GPL-3.0-only. The curated wavetables
(`assets/factory/`, and `src/generated/wave_bank.h` built from them), the four
original Animate waves (Korg MS2000 data inside `vendor/melodee/build/gen/`) are attributed there but **not licensed**. M-VAVE's stock firmware is not
included at all (`assets/stock/README.md`): attribution is a credit, not permission. Confirm the Spectral Growl
source and decide on redistribution before the first public push. Retain
all imported license notices. No remote publication was performed in this change.

Device backups, temporary build products and downloaded compilers stay ignored.
The owner-approved binary copy operation and hashes are listed in
[IMPORTED_ASSETS.md](IMPORTED_ASSETS.md).
