# Animate control map

This guide covers the new Animate-only firmware, not the preserved four-engine
or Melodee experiments. The controls are connected to the shared audio runtime;
physical validation is still outstanding.

## Startup and screen layout

Power-on shows the TLBX-1 splash for about 1.6 seconds while the pads sweep. Keys
already play during it; any button or encoder skips straight to Home. Every
screen's header shows the view on the left, then the lane (orange), slot (purple)
and step page; `*` marks unsaved edits. Parameter and mixer rows show their
value on the right with a position bar underneath. Bipolar controls such as
coarse/fine tuning and pan fill from a centre mark.

## Home and sound patches

The firmware starts on Home after the splash. **Shift + GLOBE** returns Home. Its header identifies
the selected slot; Home shows the loaded sound, Hold/arp status and an unsaved-edit
indicator. Pads 1–4 open slots A–D, 5/6 open X/Y modulation, 7 opens Mixer and 8
opens Sequencer. Pads 9, 10, 11, 12 and 13 open Patches, Global, Masks, Routing and
Effects; Pad 15 requests Save and Pad 16 opens the visualizer.

**PATCH** opens the sound browser without changing the sound. Left/right or K1
browses 22 factory entries and sixteen user slots: the six original sounds
(VECTOR … USER VECTOR), then sixteen factory sounds **with sequences** (WAVE
STATION … ENDLESS), then User 01–16. Pad 1 (or PATCH again) requests
Load; Pad 2 returns Home; Pad 3 requests Save. **Shift + PATCH** requests Save
directly, using the selected user slot or User 01 when a factory sound is selected.
In either confirmation screen, **Pad 1 or PARA confirms**, and **Pad 2 or Left
cancels**. Loading replaces the sound; saving replaces the selected user slot.

A sound patch includes all four slots, the bank wave held by each wave register,
their rendering modes, vector modulation, effects and musical settings. It does
**not** save the sequence or its step locks; loading a sound leaves those intact.
The sixteen factory sounds with sequences are the exception: their confirmation
reads LOAD SOUND + SEQ, and loading one replaces all ten lanes (steps and locks)
and sets tempo, length and division. Transport keeps playing or stays stopped.
Patches saved before the wave bank load with the original four waves. Browser project export remains the full
session backup. Browser user patches live in this site's local browser storage,
separate from hardware flash. Keep the same browser/profile and simulator address;
clearing site data removes those browser patches. Use project export for a portable
backup. A failed save leaves the unsaved indicator set.
Hardware storage initialization and its backup procedure are covered in INSTALL.md.
Device audio pauses during flash writes; saving is not a seamless live-performance
operation. Patch loading preserves the sequence but releases the previous voices.

## Direct sound navigation

In the simulator, use the visible **Slot A**, **Slot B**, **Slot C**, **Slot D**,
**X Mod** and **Y Mod** buttons above the hardware panel. One click opens the
chosen controls; the active button stays highlighted.

On the hardware, press **PARA** to enter sound parameters:

- Pads 1–4 select A–D.
- Pad 5 opens X modulation; Pad 6 opens Y modulation.
- The same six pads work from the modulation views, so returning to any slot is
  one pad press. They also work from the randomization-mask view.
- **Shift + PARA** opens X modulation directly from any view. Existing
  **Shift + Pad 8** also opens X modulation; arrows reach Y and the slot WT LFO.

Startup loads different wavetables into A–D with identical volume and other slot
settings. Navigation does not reset or randomize those settings. Named factory
presets, saved sessions and deliberate edits may have different slot settings.

## Mixer: Shift + Pad 4

The eight knobs always address the four sound slots in alternating volume/pan
pairs. Selecting a different lane, slot or fader bank does not change this order.

| Knob | Sound control |
| --- | --- |
| K1 | Slot A volume |
| K2 | Slot A pan |
| K3 | Slot B volume |
| K4 | Slot B pan |
| K5 | Slot C volume |
| K6 | Slot C pan |
| K7 | Slot D volume |
| K8 | Slot D pan |

The faders retain sequencing-lane levels. Left/right selects lanes 1–4, 5–8 or
9–10 for those faders; the last bank has two active faders. Pads 1–4 toggle the
corresponding lane mutes in that bank. These lane controls are separate from the
four sound slots. The screen identifies the current fader lanes below the eight
slot controls.

## Sound parameters: PARA or Shift + Pad 3

Pads 1–4 select slots A–D. Left/right switches between two knob banks. Faders
1–4 control the selected slot's Attack, Decay, Sustain and Release.

| Knob | Bank 1 | Bank 2 |
| --- | --- | --- |
| K1 | Wave (bank browser) | Wavetable LFO sync |
| K2 | Coarse tuning | Wavetable LFO division |
| K3 | Fine tuning | Sample start |
| K4 | Volume | Loop start |
| K5 | Pan | Loop end |
| K6 | Wavetable LFO amount | Filter type |
| K7 | Wavetable LFO shape | Filter cutoff |
| K8 | Wavetable LFO rate | Filter resonance |

Shift + Pad 5 cycles the selected slot and opens its parameter/envelope view.
Changing the editing slot does not replace its sound or reset its values.

## Sequencing and modulation

- Shift + Pad 1 opens lane selection; pads 1–10 select the editing lane.
- Shift + Pad 2 opens the sequencer. Pads toggle the displayed sixteen steps;
  left/right selects one of eight pages. Hold a step pad and use K1–K4 to edit
  note, velocity, gate and probability. Editing does not require stopping playback.
- Shift + Pad 6 opens per-slot randomization masks; pads 1–4 select the slot and
  left/right switches mask banks.
- Shift + Pad 7 opens randomization routing. Left/right changes the step page;
  Shift + left/right switches routing control banks. Shift + Arp applies the
  selected randomization operation.
- Shift + Pad 8 opens X modulation; left/right selects Vector X, Vector Y or the
  selected slot's wavetable LFO.
- Shift + Pad 16 opens the visualizer; left/right switches its view.
- Shift + Note Repeat toggles Hold. Shift + Chord toggles wheel-to-XY routing.
- GLOBE opens global pages; left/right switches pages and GLOBE returns to the
  previous view.

The intended sound reference is the earlier Melodee Animate adaptation. New
parameter access must preserve untouched preset sound. The four source choices
are its original wavetable assets; arbitrary sample import is not implemented.

## Wave bank

The firmware carries 289 waves: the original four (SINE C3, SAW C3, BELL2 C3,
VOICE2 C3) plus the 285 curated tables in `assets/wavetable-curation.json`
(Korg DWGS and MS2000 waves, Virus tables, Spectral Growl tables), stored as
256-sample frames exactly as the wavetable browser's 256-sample preview plays them.

- **K1 (WAVE)** in the slot parameter view browses the whole bank for the selected
  slot and shows the wave's name. Browsing gives the slot its own wave register
  (slot A uses register A, and so on), so it never changes another slot.
- **Shift + K1** still chooses which of the four registers the slot plays
  (the original SOURCE control); step locks and randomization use registers too,
  so a sequence can switch between a patch's four waves per step.
- Multi-frame tables currently play their first frame.
- Factory sounds VECTOR … USER VECTOR always use the original four waves.

Large randomization scopes are processed incrementally during playback. GLOBE
Audio's RANDOM LEFT readout counts remaining steps. Let it finish before pressing
Apply again; additional Apply presses during that operation are ignored.
Stop/Play is not required to publish edits.
Browser export is available after the pending operation completes.

## Polyphony diagnostics

GLOBE opens Audio; **Left once** opens the read-only **Overload** page (also
reachable after Chord with Right). It freezes the first buffer that requests
overload voice removal: total, render, scanner, USB and other microseconds,
voice count at the start of rendering, the budget limit and SCAN IRQS (scanner
interrupts during that buffer, separating interrupt rate from per-interrupt
cost). Unavailable values show --. The record lasts until reboot. It is a
trigger snapshot, not an average and not a diagnosis of which subsystem is faulty.
Knobs on this page do not edit sound parameters.

GLOBE Audio row 2, **POLYPHONY**, sets the voice ceiling from 1 to 16 in steps of one
(default 8). It changes only while no voice is sounding, including release tails.
With on-device patch storage set up (any patch saved once), the value is written
about 1.5 s after the last change, once no voice sounds, and restored at power-on.
Without owned storage the setting returns to 8 at restart.
GLOBE Audio also shows current active voices, average audio CPU load and cumulative
late buffers. Press Right once for Timing: HELD KEYS counts down keys received
from keyboard and MIDI (not sustained or Hold-latched notes); SHED VOICES counts
voices selected for a one-block fade by the hardware overload guard since boot.
The shared Melodee policy prioritizes released voices and protects the lowest held
note. Global polyphony is an allocation ceiling, not a measured CPU guarantee. These are
read-only diagnostics and do not change sound settings. Hold three physical keys:
HELD KEYS=3 with a rising SHED VOICES count identifies overload removal; a smaller
held-key count points toward input delivery or release handling. Three held keys
with no shedding but one active voice requires further allocator/voice-lifecycle
investigation. ARP intentionally changes the relationship between keys and voices.

The final three Timing rows are **RENDER US**, **IRQ US**, **OTHER US**:
smoothed microseconds per128-frame buffer, with a nominal2902us deadline.
RENDER covers the shared runtime/render call excluding measured scanner/USB
handler bodies; IRQ covers those bodies without double-counting nested scanning;
OTHER covers remaining audio-handler work. IRQ assembly entry/exit overhead is
not separately measured. These diagnostic values are unavailable in the simulator.
Record them while the clicking occurs; they do not change any sound setting.
