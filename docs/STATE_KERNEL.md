# Independent state and sequencer kernels

These modules do not synthesize audio and are not yet a functioning hardware or
simulator integration. The sound reference remains the earlier Melodee Animate
adaptation, protected by `tests/baseline_audio_test.mjs`. They contain no calls
to the abandoned desktop-derived voice implementation.

`sequence.h` provides ten lanes of128 live-editable steps in one instrument
pattern, with shared instrument tempo/division/length and independent lane note
gates, probability RNG and generation-tagged note ownership. It is not four outer
tracks. Events carry lane, generation, timestamp, note, velocity and originating
step. The audio bridge must retain each event's allocated voice handle and release
that handle only; pitch-only note-off would incorrectly release another lane or
a keybed note of the same pitch. Editing does not require a transport restart.
Step velocity is0…127, probability0…100, gate1…100; note0…127. These step encodings
are explicit and separate from the finer slot parameter codec.

`parameter_codec.h` packs all20 slot fields into151 bits/19bytes. Each field's
all-ones code represents inheritance, so no separate presence bitmap is needed.
The unused final bit must be zero. Four source IDs are supported, matching the
confirmed initial real-wavetable catalogue; expanding it requires a schema change.
No parameter is automatically reduced to generic7-bit precision.

| Field | Bits | Stored precision/domain |
|---|---:|---|
| Source | 3 | IDs0…3 plus inherit |
| Coarse | 10 | -48…48 semitones, .1 |
| Fine | 9 | -100…100 cents, .5 |
| Level | 7 | 0…1, .01 |
| Pan | 8 | -1…1, .01 |
| WT amount | 7 | 0…1, .01 |
| WT shape | 3 | 0…4 |
| WT rate | 10 | 0…1 normalized, .001 |
| WT sync | 2 | Boolean |
| WT division | 4 | 0…7 |
| Sample start | 8 | 0…1, .005 |
| Loop start | 16 | 0…1, 1/65534; no main-panel knob sensitivity established |
| Loop end | 8 | 0…1, .005 |
| Filter type | 3 | 0…4 |
| Cutoff | 10 | .001…1, .001 |
| Resonance | 7 | 0….95, .01 |
| Attack | 9 | 0…5s, .01 |
| Decay/release | 10 each | 0…10s, .01 |
| Sustain | 7 | 0…1, .01 |

Base slot values remain floats supplied by the actual sound bridge, not desktop
initial presets. Setting a step override rounds to the nearest declared interval;
maximum error is half an interval plus float rounding. This preserves the audited
main-panel interaction resolution but is not lossless arbitrary-float desktop
project interchange. Invalid/out-of-range values are rejected, not silently
clipped. No mapping from these physical controls into the preserved sound kernel
is implemented here.

`project_state.h` combines base slots, dense locks, sequencer, per-slot19-bit
random masks, selected step bits, lane glide amounts and randomization amounts.
It accepts Steps/Params/Both and Current sound/Selected/Page/All128 scopes, with
one/all lane and one/all slot targets. Current sound changes only base floats;
masked-out step locks remain untouched. Notes are generated as MIDI36…84; the
shared musical-input mapper must apply scale/chord semantics when integrated.
The kernel itself does not claim scale-aware sound rendering. Global vector/LFO,
master/output and host UI/music settings are not yet included in this state format.

The dense lock allocation is97280bytes for all5120 step-slot records. On the native
host the complete state is104336bytes and the version1 serialized payload104128
bytes. This is a measured object-size result, not proof of MCU linker fit. Device
link/layout and remaining audio/assets/UI budget must still be checked.

Exports are chunked without duplicating the lock array; the owner must serialize
all chunks before allowing edits. Imports require immutable complete input bytes,
validate header/checksum, settings, every step and every packed field before
changing live state, retain host sample rate, and restore stopped transport.
No hardware flash transport, allocation of a full MCU import buffer, or device
save capability is implied. A wrapper must supply an appropriate transport.

Validation: C11 `-Wall -Wextra -Werror`, AddressSanitizer and UBSan passed for all
5120×20 field roundtrips and half-step error bounds, all ten lanes through step127,
same-pitch lane generation isolation, live edit at the next event, independent
probability RNG, selection/page/lane/slot masks, copy/clear, full export/import,
invalid checksum, valid-checksum malformed packed data and truncated imports.
The shared runtime now connects these kernels to the preserved audio bridge.
`tools/check_runtime.py` verifies all six preset fixtures both directly and after
outer session roundtrip, all eight mixer controls, Hold/XY, live edits and bounded
random jobs. The browser wrapper in `session.c` adds musical, UI, FX and modulation
settings and rendering masks; the kernel payload alone is not a complete session.
`aps_randomize_range` applies bounded step chunks with explicit note-field masks;
chunked selected/page/all operations match the full-scope results exactly in
sanitizer tests. Read the hardware build record for measured linker fit. Physical
performance remains unverified.
