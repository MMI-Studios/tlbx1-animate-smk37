# Preserved Animate host bridge

The shipping source unit is `bridge.c`; the other C files here are included by
that unit. Do not compile this directory with a wildcard. Link `animate_math.c`
for explicit control extensions. The unchanged engine and real table assets are
included from the preserved Melodee sources in `vendor/melodee/` and generated headers.
No native Melodee engine is included in this bridge.

`core.h`, `mod.c`, and `params.c` are GPL-3.0-only extractions of their
same-named donor sources. Native engine dispatch and old UI formatting were
removed. `voice.c` now includes the shared donor implementation directly, with
optional hooks for independent note ownership, extension initialization and pan
scratch reset. Its existing engine-cap callback supplies the chosen voice limit.
The shared `audio_shed_voice.h` prioritizes released notes, protects the lowest
held note and requests a one-block fade; it replaces the adapter's abrupt kill.
`fx.c` and `perform.c` preserve the general effects and mixer; the
performance mute initializer now has one part. Slot balance adds a dry stereo
correction; shared chorus/delay/reverb sends remain mono as in the donor.

`ab_init` initializes a fresh module once. The bridge runs at 44100 Hz in blocks
of 32 frames, yielding interleaved stereo Q15 int32 samples. It is single-owner,
not thread-safe. It is not a sequencer, UI, persistence layer or board driver.
Rendering an arbitrary number of frames requires a caller-side block adapter.

The default cap is the original eight voices. Selecting sixteen is an explicit
allocation extension, available only with no active voices. `ab_note_on` retains
original same-pitch reuse; `ab_owned_note_on` permits independently owned notes
of the same pitch in POLY mode. Release the returned handle, not the MIDI pitch.
A stolen handle cannot release its replacement. Mono/unison require separate
group ownership and are not accepted by the owned-note entry point.

## Compatibility and extension state

With base/voice extension masks clear, the original `animate_render` executes
unchanged. Native differential tests match all six complete presets, including
USER VECTOR, against the preserved host for the fixture's chord/release/mixer
sequence. WASM reproduces those same hashes. This scoped comparison is not a
claim about all possible input sequences or physical audio hardware.

Canonical `ab_slot` IDs/units are the public `AB_*` enum in `bridge.h`. The newer
`ab_slot_parameter` and `ab_voice_parameter` APIs use the twenty logical IDs in
`animate_parameters.h` and retain float values. Editing a logical field activates
its mask bit; untouched preset values keep the old integer behavior. Save these
base masks with project values. To restore, set values first and then restore
the saved masks. Do not blindly activate all fields at initialization.
Bits 0–19 activate logical edits; bit 20 preserves explicit filter bypass.
Bypassing a chosen new filter retains its type/settings for re-enabling it.
`ab_tempo` updates float LFO sync and the inherited effects' nearest integer BPM.
`ab_master` keeps the original Q12 gain convention. `ab_release_all` releases
voices while allowing effects tails to continue.

Explicit extensions include fractional tuning, dry stereo balance, five filter
modes and five position-LFO shapes, tempo sync, independently owned source-window
starts, and per-voice parameter overlays. Four source IDs still identify the
original wavetable cycles; arbitrary sample import is not implemented here.
Envelope seconds control linear durations, retaining the baseline envelope family,
not the desktop engine's exponential time constants. New filter edits enable
their filter path; untouched filter bypass retains the original preset behavior.
New free LFO rates use .01–20 Hz logarithmic mapping, while an untouched original
rate retains the original stopped/linear 0–4 Hz behavior. These are explicit
extensions around the sonic baseline, not a claim of desktop DSP equivalence.

Run `python3 tools/build_bridge.py` under Linux/WSL for native comparisons and
ownership checks. `python3 tools/build_bridge.py wasm`, then Node tests
`tests/bridge_wasm.mjs` and `tests/bridge_extensions.mjs`, check WASM parity and
control behavior. The reference-only native build includes donor host simulation
and may emit pointer-width warnings from its unused hardware USB declarations.
