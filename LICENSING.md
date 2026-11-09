# Animate firmware licensing and attribution

## The firmware

Animate firmware for the M-VAVE SMK-37 Pro is free software, licensed under the
GNU General Public License, version 3 only (`GPL-3.0-only`; full text in
[`LICENSE`](LICENSE)). It is built on Melodee, which is GPL-3.0-only, so the
firmware as a whole is GPL-3.0-only. If you distribute the firmware, or a device
or package containing it, you must also offer its complete corresponding source
under the same licence.

Copyright (C) 2026 the TLBX-1 Animate firmware authors.

This licence covers the code and the project's own documents and assets. It does
**not** cover the third-party sound data and vendor firmware listed under
[Third-party data](#third-party-data-no-licence-granted); attribution there is a
credit, not a grant of rights.

## Code from other projects

| What | Licence | Where |
| --- | --- | --- |
| Melodee by Kerem Kilic (Ellic Studio), a modified version of Felucca by Leo Kuroshita (@kurogedelic), Hügelton Instruments (<https://github.com/hugelton/Felucca>): board HAL, RAM loader, voice allocation, effects, scales, the original Animate engine (`eng_animate.c`), build helpers, web updater modules and generated tables. Copied unchanged except where noted in `vendor/melodee/`; the Animate-only extraction in `src/baseline/` adapts these files. | GPL-3.0-only | `vendor/melodee/` (with its own [`LICENSING.md`](vendor/melodee/LICENSING.md) and `LICENSES/`), `src/baseline/` |
| Parts of Melodee that keep their own licences (DaisySP, Rings, MSFA/Dexed, reSID, schwung-space-delay, fm1-x0x, MAME uPD933, fonts) | as listed in [`vendor/melodee/LICENSING.md`](vendor/melodee/LICENSING.md) | `vendor/melodee/` |
| Sound data bundled inside Melodee's generated tables (Casio CZ-1 preset tones, Sequential Prophet-5/10 factory programs) — vendored with Melodee, not used by Animate's sounds; rights stay with Casio and Sequential as stated in Melodee's licensing | Rights holders' (no licence granted) | `vendor/melodee/build/gen/` |
| FunDSP 0.20.0 `src/moog.rs` by Sami Perttu and FunDSP contributors: the Moog filter recurrence | MIT | `reference/desktop_voice.c`, [`LICENSES/FunDSP-MIT`](LICENSES/FunDSP-MIT) |
| TLBX-1 display adapter and visualizer (the TLBX-1 authors' own work) | GPL-3.0-only | `vendor/tlbx-display/` |
| Behavioural reference of the TLBX-1 desktop Animate engine, rewritten in C (the TLBX-1 authors' own work) | GPL-3.0-only | `reference/` |

## Third-party data (no licence granted)

The following sound data are **not** covered by this repository's licence; the
M-VAVE firmware listed with them is not included at all. The sound data are included so that the firmware can be built and
audited exactly as released; the rights remain with their owners, and no licence
to them is granted or implied. If you are a rights holder and want material
removed, open an issue and it will be taken out.

| What | Rights holder | Where | Used for |
| --- | --- | --- | --- |
| Korg MS2000 / DWGS single-cycle waveforms (64 DWGS waves and the SINE, SAW, SQUARE, TRIANGLE, VOX and NOISE waves), including the **four original Animate waves** SINE C3, SAW C3, BELL2 C3 and VOICE2 C3 | Korg Inc. | `assets/factory/wavetables/ms2000/`; the original four also as data in `vendor/melodee/build/gen/animate_poc_tables.h` | Wave bank entries 0–73 |
| Access Virus wavetables (`Virus_WT_001`–`200`) | Access Music Electronics GmbH | `assets/factory/wavetables/virus/` | Wave bank entries 74–273 |
| "Spectral Growl" wavetables (`Spectral Growl 001`–`015`) | **Source not yet confirmed** — to be completed by the maintainer before publication | `assets/factory/wavetables/growl/` | Wave bank entries 274–288 |
| M-VAVE SMK-37 Pro stock firmware V016 — **not included**; each builder downloads it from M-VAVE (see `assets/stock/README.md`) | M-VAVE (Cuvave) | `assets/stock/` (user-supplied, ignored by Git) | Packaging input only: the update package keeps the stock image's protected bytes, so built `.fwsc` / `animate-update.bin` packages contain M-VAVE data |

Derived files carry the same status as their source data:

- `src/generated/wave_bank.h` and `assets/wave-bank-manifest.json` are built from
  the wavetables above by `tools/build_wavebank.py` (resampled, normalised and
  re-quantised; the waveforms remain the rights holders').
- Firmware images and update packages built from this repository embed that wave
  bank and, for packages, the M-VAVE stock firmware.

The selection of tables (`assets/wavetable-curation.json`), the factory patches
and sequences (`tools/factory_patches.py`, `src/generated/factory_patches.h`) and
all other code and documents are the project's own work under GPL-3.0-only.

## Trademarks

Korg, MS2000 and DWGS are trademarks of Korg Inc.; Access and Virus of Access
Music Electronics GmbH; M-VAVE and SMK-37 of Cuvave; Wavestation of Korg Inc.
Names are used only to identify the source of data or compatible hardware. This
project is not affiliated with or endorsed by any of them.
