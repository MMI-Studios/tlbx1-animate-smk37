# TLBX Animate firmware baseline

Animate is a four-slot vector wavetable synthesizer firmware for the M-VAVE
SMK-37 Pro, with a 289-wave bank, a ten-lane sequencer and a browser simulator.
It is unofficial and not affiliated with M-VAVE.

## Quick start

Linux x86-64 or WSL; details and alternatives are in [docs/BUILDING.md](docs/BUILDING.md).

```sh
git clone <this repository> animate && cd animate

# Tools: a C compiler, Python 3.10+, Node.js 22+, curl, xz
python3 -m venv build/venv && . build/venv/bin/activate
python -m pip install -r requirements-dev.txt

# 1. Check the engine (native runtime, audio and sound-contract tests)
python tools/check_runtime.py

# 2. Simulator (needs Zig 0.13.0 on PATH)
python tools/build_wasm.py
python -m http.server 8795 --bind 127.0.0.1   # open http://127.0.0.1:8795/web/

# 3. Hardware application (fetches JieLi's pi32v2 compiler once)
sh vendor/melodee/tools/get_toolchain.sh "$PWD/tools-local/jieli"
export JIELI_TOOLCHAIN="$PWD/tools-local/jieli/toolchain"
python tools/build.py smk
```

Packaging an installable update also needs M-VAVE's official SMK-37 Pro V016
firmware, which is not included; see [assets/stock/README.md](assets/stock/README.md).
Installing on hardware is covered in [docs/INSTALL.md](docs/INSTALL.md); back up
your keyboard first.

## Standalone repository

Build inputs, curated WAVs, board support and reference-test dependencies are
included in this repository. Start with [standalone build instructions](docs/BUILDING.md)
and the [repository layout](docs/REPOSITORY.md). No other checkout is
required. Compiler tools are installed separately.

Current accepted baseline: the 2026-11-07 wave-bank build, portable hash
`9788f87a550247c53a0e12abe94dd67171e1da35106fb28cc04be33ef4fcefde`.
The owner confirmed that build runs on hardware; overload in some factory
sequences remains an open, deferred issue. Older development notes below are
historical and do not supersede this acceptance record or `docs/SOUND_CONTRACT.md`.

## Development history

Dedicated Animate-only successor, started2026-11-05. The owner rejected the
independent four-engine port's Animate fidelity and requested a complete engine
baseline before adding other engines. Earlier Melodee-based builds are not part of this repository.

**The shared runtime and hardware-layout simulator are implemented. Physical
hardware acceptance remains outstanding.** This is the new Animate-only 982
application; earlier 981 artifacts are the separate four-engine firmware.

Open the [local simulator](http://127.0.0.1:8795/web/) and click **Enable audio**.
The sequencer starts empty; play the keyboard or enable steps. See the
[control guide](docs/CONTROLS.md) for the physical mapping and
[installation guide](docs/INSTALL.md) for the packaged 982 candidate and limits.

Use the visible **Slot A–D / X Mod / Y Mod** buttons for direct simulator access.
On hardware, **PARA**, then pads **1–4** selects A–D; pads **5/6** opens X/Y
modulation. **Shift + PARA** jumps directly to X modulation from any view.
All four startup slots share the same volume and parameter defaults, with a
different original wavetable in each slot.

## Required behavior

- Animate is the only synthesis engine. Its sonic baseline is the existing
  Melodee Animate adaptation, including its four real wavetable assets,
  integer DSP and host gain/modulation behavior. Desktop TLBX is the parameter/UI
  access reference, not permission to replace that established sound.
- Expose every slot A–D parameter, its applicable modulation, and independent
  per-slot randomization. Hidden or unwired parameters do not count as supported.
- Preserve the current TLBX-style screen, tactile shortcuts and live editing.
  Keep128 steps across eight16-step pages, one Animate instrument and ten note lanes.
- Adapt Melodee's general output/headroom, efficient performance patterns and
  scale/chord/tuning integration. Do not adopt its engines or menu workflow.
- Keep note ownership/release correct across scale, chord, track/lane and mode
  changes. Preserve agreed Repeat behavior rather than silently inheriting a
  different donor shortcut.
- Treat desktop/simulator/hardware verification as separate evidence. Physical
  output levels and real-time processing must be validated before musical acceptance.

## Architecture boundaries

Read [SOUND_CONTRACT.md](docs/SOUND_CONTRACT.md) before integration.

`src/support/` contains independently testable general music/output primitives
adapted with provenance from Melodee. `src/animate_parameters.*` is a descriptive
parameter inventory, not an engine. The parity document records implementation
status and desktop inconsistencies before choices are made.

`reference/desktop_voice.c/.h` is an abandoned, untested desktop-port experiment.
It is not the canonical kernel, is not built, and must not be wildcard-included
as the instrument implementation. The retained FunDSP attribution applies only
to that reference experiment. Preserve the existing Animate kernel first.

The target is a dedicated musical runtime and explicit board-service interface,
using existing SMK HAL/USB/scanner/LCD support and the existing toolchain without
copying toolchains. A simulator must run that same runtime. Old engine code and
installer manifests are not copied or relabeled as this implementation.

## Source set and capacity

The desktop factory audio library occupies about131MB. Existing raw application
bounds are581564 bytes total, with roughly435KB left in the preceding146212-byte
candidate; the complete engine will change that budget. The entire library cannot
be embedded. The owner selected the same four real wavetables from the Melodee
adaptation as the starting set. Additional source import is separate unfinished
work; do not claim a complete desktop library/import workflow. No silent truncation,
analytic substitution, loss of slots or randomization scope is authorized.

Ten lanes ×128 steps ×4 slots need explicit worst-case parameter-storage design.
Preserve parameter ranges and resolution deliberately; assuming one7-bit byte
per desktop value would already lose coarse/fine tuning granularity. Sparse
storage alone does not guarantee that randomizing every step will fit.

## Current delivery status

- The preserved engine and general host audio path run in the shared C runtime.
  Voice allocation/lifecycle and graceful overload handling include the Melodee
  implementation directly; Animate supplies ownership and extension hooks.
  Hardware visual preparation runs outside audio IRQs, and unchanged extension
  coefficients are cached. Previous hardware polyphony failed; this integration
  still requires physical validation and does not guarantee16 voices for every patch.
  Native fixtures match all six original presets exactly, including USER VECTOR.
- All twenty logical parameters are accessible for each slot, including explicit
  extensions for fractional tuning, stereo balance, five filter modes and
  wavetable LFO sync. Vector X/Y modulation also supports five shapes and sync.
  Untouched presets retain their original DSP path. See
  [the bridge contract](src/baseline/README.md) for envelope/filter differences
  from the desktop engine.
- Ten live-editable 128-step lanes, per-slot masks and targeted randomization are
  integrated with actual playback. Large randomization operations progress one
  step per audio block without requiring Stop/Play; GLOBE Audio shows work remaining.
- Mixer K1–K8 pairs are **A volume, A pan, B volume, B pan, C volume, C pan,
  D volume, D pan**. The four faders retain banked sequencing-lane levels.
- Keyboard/MIDI note ownership, Hold, arp/repeat, scale/chord mapping, wheel XY,
  actual-output visualization and browser project files are connected.
- Boot shows the TLBX-1 icon splash with a pad-light sweep, from the same C
  rasterizer in hardware and simulator. Screens use the TLBX palette with value
  bars, pad-numbered legends and a beat-grouped step grid.
- Voices render through one block-rate kernel (`src/baseline/voice_kernel.c`) for
  every parameter state, paired across CPU0 and CPU1 like Melodee's Prophet. Code,
  constant tables and the CPU1 mailbox are SRAM-resident. Output is near-identical
  to the original per-sample renderer (see SOUND_CONTRACT.md). Physical polyphony
  is not yet measured.
- Home provides direct pad navigation, Hold/arp status and unsaved-change feedback.
  PATCH browses without changing the sound; Load and Save require confirmation.
  Sixteen sound-patch slots retain slots, modulation, effects and musical settings
  in browser storage or the device's dedicated flash bank. See the
  [control guide](docs/CONTROLS.md) and [storage setup](docs/INSTALL.md).
- Browser checks confirmed actual destination audio and controls. The complete
  board application links within the unchanged memory/flash limits. Physical
  output level, responsiveness and sustained workload have not been measured.

## Remaining limits

This is not complete desktop parity. Sources are the four agreed wavetable assets;
there is no arbitrary sample import. Full device project saving and TRS MIDI remain
unimplemented. Browser project files preserve the new instrument/session state;
they are not desktop project files or old firmware session files. Audio currently
renders at 44.1 kHz with 16-bit PCM; those readouts do not offer the previous
four-engine firmware's rate/depth selectors. A 289-wave bank (the original four plus 285 curated tables) is browsable per
slot with K1, and the patch browser adds sixteen factory sounds with sequences.
Polyphony is adjustable from 1 to 16 voices in steps of one
(GLOBE Audio, second row; default 8) and can change only when no voices are active.
It is remembered across restarts once on-device patch storage is set up. Hardware musical acceptance is pending.

## License

The firmware is free software under the GNU General Public License, version 3
only ([`LICENSE`](LICENSE)); it builds on Melodee/Felucca (GPL-3.0-only). Third-party
sound data (Korg, Access Virus and Spectral Growl wavetables) and the M-VAVE stock
firmware used for packaging are **not** covered by that licence. See
[`LICENSING.md`](LICENSING.md) for full attribution.
