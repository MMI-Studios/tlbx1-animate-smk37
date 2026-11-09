# Sound preservation comes first

## Revision 2026-11-07: near-identical, not bit-identical

The owner approved replacing bit-exact preservation with near-identical output,
in exchange for polyphony. The firmware now renders with the unified block-rate
kernel (`src/baseline/voice_kernel.c`). The per-sample renderers
(`extended_render.c` and the donor `eng_animate.c`) remain as the reference
oracle (`-DAB_REFERENCE_KERNEL`) and the source of every parameter's meaning.

What stays the same: tables and interpolation, per-parameter choice (native value,
edited value or per-voice lock), filter types and their coefficients, envelope
rates/stages/sustain, windows, loop windows, the edit-offset declick, position
modulation (still evaluated per sample when its depth is non-zero), velocity,
amplitude ramps, pan and levels.

What changed: the vector LFO target and envelope stage transitions update once
per 32-sample block (0.73 ms). The vector smoothing still runs per sample. Slot
gains ramp linearly across the block, and slots silent for a whole block are
skipped. Voices are paired across both cores; that pairing is exact.

Acceptance replaces the exact-hash requirement for this firmware:
`tools/kernel_compare.py --check` (run by `check_runtime.py`) requires every
scenario within 0.5 dB level and 1.5 dB mean STFT magnitude difference of the
reference. That covers presets, native and all five extended filters,
pan/level/pitch, windows, position LFOs (including S&H), extended envelopes,
vector LFO and solo, live window edits and eight voices. At introduction the
results were 49.6-72.8 dB waveform SNR and at most 0.10 dB spectral difference.
`build_bridge.py` requires each preset's energy within 1% of the preserved donor
(measured 0.008-0.024%). Parallel rendering must equal serial rendering exactly
(`render_worker_test`). The preserved donor's own hashes below remain unchanged
and still apply to the Melodee adaptation.

The rest of this document is the original contract, kept for reference.


Owner clarification,2026-11-05: Animate should sound the same as the adaptation
already implemented in Melodee. The new UI and parameter access
should follow TLBX desktop. This supersedes the earlier desktop-DSP port plan.

Confirmed: one Animate instrument, ten lanes,128steps/eightpages; initially the
same four real wavetables. All-slot editing, modulation and independently targeted
randomization remain required. Preserve TLBX-style shortcuts rather than donor menus.

## Canonical implementation

The preserved adaptation's references under `vendor/melodee/firmware/src/`:

- `eng_animate.c`: integer source lookup/interpolation, vector weights/smoothing,
  slot envelopes/filters, playback windows/position modulation and edit correction.
- `animate_controls.h`, `animate_user_vector.h` and generated `animate_poc_tables.h`:
  controls, exact captured USER VECTOR state and source data.
- `voice.c`, `fx.c` and DSP helpers: velocity/amplitude scaling, host modulation,
  track processing, headroom, DC removal and output.

The four tables alone do not preserve the sound. The kernel owns integer linear
slot envelopes (`ownenv=1`), not desktop exponential envelopes. Filter bypass
defaults, dual slot-level factors, velocity scaling and host amplitude ramps
matter. USER VECTOR includes host settings and a B-slot window; rebuilding it
from XY coordinates alone is not equivalent.

## Verification requirement

1. Run the preserved reference WASM test without changing expected hashes.
   `firmware/tests/animate_poc_test.mjs` records four corner hashes:
   `1782912505,2011708317,1877176961,3477105355`.
2. Drive the new integration with matching presets/notes/velocities/timing.
   Compare full rendered Q15 output, not just oscillator samples. Cover corners,
   moving vector, USER VECTOR, windows, modulation, chords and releases.
3. Navigation alone must not change the patch/audio. Equivalent parameter edits
   through the new UI must reach the preserved processing path.
4. Additional controls must have neutral defaults. Desktop ranges/semantics
   sometimes disagree internally; the inventory is not an initialization preset
   and does not authorize replacing existing DSP while calling its sound unchanged.
5. Validate physical output and performance separately. Host equality does not
   establish hardware scaling, timing or analog behavior.

The desktop voice experiment under `reference/` is excluded from the baseline.
Standalone support/math tests do not prove full sonic equivalence. This document
does not establish that a new installable firmware is ready.
