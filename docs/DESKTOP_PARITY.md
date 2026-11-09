# Desktop Animate inventory and proposed firmware contract

**Sound-reference correction,2026-11-05:** owner requires the preserved Melodee
Animate adaptation's sound with desktop-style access. This inventory does not
authorize replacing DSP/envelopes/defaults. See [SOUND_CONTRACT.md](SOUND_CONTRACT.md).
One instrument, ten lanes and the existing four real wavetables are confirmed.

Status: **desktop control inventory, not the sonic baseline specification.**
The owner subsequently confirmed one ten-lane instrument and the same four real
wavetables from the earlier Melodee adaptation; that adaptation is the sound
reference. Independent state kernels are described in [STATE_KERNEL.md](STATE_KERNEL.md).
Audio/UI integration and the dedicated 982 build are now implemented; physical
hardware validation remains pending. See the project README and installation
guide for current delivered behavior and limitations. The metadata C files describe observed
desktop controls. They do not implement an engine, sequencer, asset loader,
hardware UI, randomization, persistence, or a parameter codec. Passing their
test proves metadata consistency only. Earlier firmware implementations are not evidence of parity here.

The owner requested a separate Animate-only firmware, the existing firmware UI
shortcuts, 128 steps/eight pages, all Animate slot/modulation/randomization
features, and Melodee hardware/output/performance and scale/chord infrastructure
without replacing the confirmed Animate sound with other Melodee engines. One
instrument with ten lanes and the four existing real sources is now confirmed;
future imported asset limits remain undecided. This document does not authorize a reduced lane
count, coarser parameters, fewer randomizable fields, or narrower edit scopes.

## Reference behaviour

Audited on 2026-11-05 against the TLBX-1 Animate engine:

- Ten lanes, 16 steps per page, eight pages, 128 steps per lane, four slots per
  step, and default lane notes.
- Ten sets of four Moog filter states plus a separate four-slot keybed state.
- Fifteen float and five integer per-step-slot override fields and their
  accepted ranges.
- Unset overrides inherit global slot values; clearing resets float overrides
  to unset and integer overrides to -1.
- Slot parameter ranges and control sensitivities (slot A; B–D are identical).
- Reset defaults used in the metadata.
- Filter, LFO shape and division labels.

## Slot inventory

Each row is independently applicable to slots A, B, C and D. Logical IDs below
are defined in `src/animate_parameters.h`; they are **not serialized offsets**.
The main-panel increment is an interaction sensitivity, not evidence that the
desktop float state is quantized. Numeric imports, randomization, MIDI mappings
and pointer movement can produce intermediate values. A firmware codec must
therefore state its precision policy explicitly before implementation.

| ID | Field | Main-panel range; sensitivity | Persisted override / DSP range | Default | Random bit |
|---:|---|---|---|---:|---:|
| 0 | Source | Catalogue selection | Dynamic source ID, inherit separately | First source | 0 |
| 1 | Coarse | -24…24 semitones; 0.1 | -48…48; step editor exposes -36…36 | 0 | 1 |
| 2 | Fine | -100…100 cents; 0.5 | -100…100 | 0 | 2 |
| 3 | Level | 0…1 linear; 0.01 | 0…1 | 1 | 3 |
| 4 | Pan | -1…1 balance; 0.01 | -1…1 | 0 | 4 |
| 5 | WT LFO amount | 0…1; 0.01 | 0…1 | 0 | 5 |
| 6 | WT LFO shape | Five choices | 0…4 | Sine | 6 |
| 7 | WT LFO rate | 0…1 normalized; **0.001** | 0…1 maps exponentially to 0.01…20 Hz | 0.5 | 7 |
| 8 | WT LFO sync | Off/on | Boolean | Off | 8 |
| 9 | WT LFO division | Eight choices | 0…7 | 1/16 | 9 |
| 10 | Sample start | 0…1; **0.005** | 0…1 | 0 | 10 |
| 11 | Loop start | State/callback exists; no main-panel knob found | 0…1; currently unused by renderer | 0 | 11 |
| 12 | Loop end | 0…1; **0.005** | 0…1 | 1 | 11 |
| 13 | Filter type | Five choices | 0…4 | LP24 | 12 |
| 14 | Cutoff | 0…1; 0.01 | Override 0.001…1; `20 + x² × 19980` Hz | 0.5 | 13 |
| 15 | Resonance | 0…1; 0.01 | Override/render clamp 0…0.95 | 0 | 14 |
| 16 | Attack | 0…5 seconds; 0.01 | 0…5 | 0.01 | 15 |
| 17 | Decay | **0…10 seconds**; 0.01 | 0…10 | 0.1 | 16 |
| 18 | Sustain | 0…1; 0.01 | 0…1 | 0.8 | 17 |
| 19 | Release | 0…10 seconds; 0.01 | 0…10 | 0.3 | 18 |

Coarse/fine combine as `2^((coarse + fine/100)/12)`.
Pan uses unity center balance: left `min(1,1-pan)`, right `min(1,1+pan)`;
it is not a -3 dB equal-power pan law.

Filter labels are LP24, LP12, HP, BP, Moog. The first four are state-variable
filter outputs; LP24 cascades two lowpass stages. Moog follows the FunDSP Moog
recurrence (MIT). Replacing it with the old port's shared two-pole filter would lose a
requested capability. All filters belong to each source voice, before slot
envelope, level, velocity, vector weight and stereo pan mixing.

ADSR uses exponential coefficients `exp(-1/(seconds × sample_rate))`, immediate
transition for time ≤1 ms, attack toward1.1 until reaching1, decay tolerance.001,
and release silence threshold.0001. Thus seconds are time constants, not exact
stage completion durations. The previous reduced port's decay ceiling5s
does not match this desktop inventory.

## Global modulation and performance controls

- Vector X/Y each0…1; bilinear weights `(1-X)(1-Y), X(1-Y), (1-X)Y, XY`.
  Desktop smoothing is `.999 × previous + .001 × target` per audio sample.
- Independent X and Y LFO: shape, sync, division, rate, amount. Main-panel rate
  control exposes normalized.01…1, while shared DSP accepts0…1; amount0…1.
- Four WT LFOs operate independently, with per-lane/slot phase and sample-and-hold
  state when sequence overrides are active. WT modulation offsets source lookup
  inside its playback window; it is not merely oscillator vibrato.
- Shapes: sine, triangle, square, rising saw, sample-and-hold.
- Divisions: 1/16,1/8,1/4,1/3,1/2,1,2,4 correspond to
  .25,.5,1,4/3,2,4,8,16 beats; synced Hz = `(BPM/60)/beats`.
- Engine output UI is -24…+24 dB, with numeric step.1 dB and scroll.5 dB
  This is separate from per-slot levels and
  the donor's required signed-Q15 output format.
- Desktop keybed has separate source/filter/envelope state and a Hold option;
  it is not automatically the same ownership/polyphony behavior as the current
  firmware's generic four-voice-per-track allocation. Preserve the owner's
  firmware Hold, wheel and musical input decisions explicitly during adaptation.

## Sequencer and randomization contract

Desktop has10 lanes ×128steps =1280 note cells per Animate instance. Each cell
has note activation, note, probability, velocity and gate plus four independently
overridable20-field slot records. Lane controls include enable, probability and
glide; normalized glide0…1 maps up to.5seconds. Default lane notes are
48,50,52,55,57,60,62,64,67,69. These are lanes within one engine instance.

Randomization has separate note amount and parameter amount0…1. Parameter amount
interpolates from global slot values toward random targets. A19-bit mask controls
the20 slot fields above: loop start/end share one bit. Default mask enables all19.
Masked-out existing overrides remain unchanged. An unset override inherits its
global slot value rather than permanently copying it.

Desktop targeted modes are Params/Notes/Both, scopes page/all pages/one edited
step, lane target all or one of10, and slot target all or A–D. The owner's firmware
workflow additionally established Current sound and selected individual pads.
Keep those explicit UI choices while providing desktop lane/slot targeting;
do not substitute a generic four-track lock editor. All edits/randomization must
work during playback without requiring Stop/Play to publish them.

Desktop note randomization is scale-aware, between MIDI36 and84. Amount changes
activation density and blends probability/velocity/gate from1 toward randomized
targets; gate clamps.05…1. This differs from independent uniform note/velocity
randomization in the previous reduced firmware.
Firmware UI should retain Shift+Pad5 envelopes, Shift+Pad6 parameter
selection, Shift+Pad7 routing, and Shift+ARP apply, while new lane/slot selection
and main-view mapping require the coordinated control specification.

## Source playback and import

The Animate engine reads factory `wavetables/` and `samples/` with cached loading
and stable source references. The firmware needs real source data and stable IDs; four
analytic waveforms are not a replacement for these capabilities.

Current renderer treats a wavetable playback window as one cycle, advances by
`frequency × window_length / sample_rate` and interpolates adjacent samples.
Sample mode reads the first channel without interpolation and advances by
`frequency/440`. Both wrap within the window, whose end is exclusive.
`sample_start`, not `loop_start`, currently supplies its lower bound. Loading
ignores the source file's sample rate, so the
sample playback rate behavior needs an explicit compatibility decision when
building a host asset converter. There is no basis here for promising arbitrary
desktop file sizes will fit device memory/flash.

Proposed implementation: host-side import/validation/conversion, bounded catalogue
and source windows in device flash, small preallocated read caches, stable asset
references, and source selection/overrides identical in engine semantics.
Catalogue size, maximum source length, precision, resampling policy and ownership
of writable flash remain unapproved design values. No device write is authorized
by this inventory.

## Desktop mismatches requiring a deliberate policy

| Evidence | Observed mismatch | Recommended intended contract, pending agreement |
|---|---|---|
| UI982,2940; persistence6 | Coarse main panel±24, step editor±36, storage/random±48 | One documented range preserving existing projects; never silently clip to±24 |
| UI1147; randomizer12175; audio-core20 | WT rate stores normalized0…1 but randomizer draws.01…32 | Randomize normalized values and display converted Hz |
| Renderer359,391; persistence13 | Loop start exists but renderer ignores it | Decide separate initial start vs repeat-loop start behavior explicitly |
| Randomizer12256; filter options | Randomization chooses types0…3 despite Moog type4 | Include all enabled filter choices unless owner requests exclusion |
| UI1043/1063; persistence15/16 | Cutoff/resonance UI bounds differ from accepted override bounds | Share validated metadata and display effective limits |
| Renderer391–414; loader687–732 | Sample playback discards source rate and uses nearest sample | Define reference pitch/rate and interpolation policy; do not claim exact parity after changing it |
| Keybed envelope156–191 | Hold=false auto-releases after decay, unlike ordinary sustained MIDI notes | Reconcile desktop keybed behavior with approved physical-key/Hold/sustain contract |

Recommendations are not implementation decisions. The metadata preserves
observed disagreements with flags and contains no clamping or DSP policy.

## Resource architecture and precision

Proposed engine state is one coherent Animate instance: ten lane voice states,
four source/filter/envelope states each, separate keybed state, shared vector
modulation, and per-lane/slot WT modulation. Cache coefficients, source windows,
pitch increments and interpolation constants at control/step boundaries. Do not
transplant desktop atomics, mutexes, dynamic allocations or per-sample f64 modulo
into the MCU IRQ. Borrow donor output formatting, audio scheduling, music-input
primitives and dirty-region display work; do not import donor synthesis engines.

Memory calculations must cover the fully populated randomization case:

- 10 ×128 ×4 =5120 step-slot records per instance.
- Twenty8-bit fields would be102400 bytes, but cannot preserve the observed
  coarse/fine/WT-rate/ADSR interaction resolution. This is **not an acceptable
  automatic7-bit parameter codec**.
- At main-panel sensitivity: coarse±24/.1 needs481 values (9bits); preserving
  stored±48/.1 needs961 (10bits). Fine±100/.5 needs401 (9bits). WT rate/.001
  needs1001 (10bits). Start/end/.005 need201 (8bits). A/.01 needs501 (9bits);
  D/R/.01 need1001 (10bits). Unset/inherit needs an additional representation.
- Twenty16-bit fields cost204800 bytes. A separate20-bit presence mask per record
  adds12800 bytes (217600 total); 32-bit-aligned masks add20480 instead
  (225280 total). Reserved sentinels can avoid separate masks only when field
  domains leave codes free. Catalogue capacity remains unknown.
- Even a variable-width codec preserving these interaction increments does not
  preserve arbitrary32-bit desktop randomized/imported values losslessly.
  That precision distinction requires agreement. Sparse storage saves typical
  space but cannot guarantee worst-case fully randomized capacity.

Do not reuse the old four-track generic step-lock allocation and label it parity.
Plan full worst-case memory and persistent storage, including assets and import
validation, before committing the representation. Preserve the existing64KiB
reserve unless a separately justified and approved memory architecture replaces
it. The new project has no measured CPU, memory, audio or hardware results yet.

## Validation status

The initial audit implemented descriptive parameter metadata and its consistency test.
The focused C11 metadata test passed with `-Wall -Wextra -Werror` and
AddressSanitizer/UndefinedBehaviorSanitizer on the native WSL host. It checks
logical IDs, unique keys, all19 random categories, paired loop-mask membership,
range-disagreement flags, corrected decay range and division endpoints. No
firmware build or musical test was performed for this metadata task.
Independent sequencer, packed state and serialization kernels were subsequently
added and tested as scoped in [STATE_KERNEL.md](STATE_KERNEL.md). They are not yet
integrated audio/UI behavior. DSP adaptation, UI wiring, asset transport, device
persistence, output integration, firmware packaging and physical acceptance
remain pending in this new project.
Tests must eventually cover all20 fields on all four slots, ten independent lanes,
step127/page8, per-step inheritance, masks and lane/slot routing, editing while
playing, source boundaries/import, filter choices, timing and overload/release
behavior. Native tests cannot establish hardware sound quality or responsiveness.
