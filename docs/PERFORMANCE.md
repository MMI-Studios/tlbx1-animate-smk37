# Animate hardware performance investigation

## Unified block-rate kernel with parallel voices (2026-11-07)

Root cause of the polyphony ceiling: the per-sample Animate loop did all four
slots' control work every sample, about 455 MCU instructions per voice-sample.
That covered vector LFO, smoothing, envelope state machines, window offsets and
five bookkeeping stores per slot. Editing any extended control moved every voice
to a heavier float path. CPU1 received whole blocks, so the two cores never
rendered voices at the same time.

`src/baseline/voice_kernel.c` replaces both paths:
- `ak_prepare` (CPU0) resolves each slot per 32-sample block with the same
  per-parameter rules and advances block-rate control.
- `ak_slot_run` renders each audible slot in tight loops. MCU disassembly:
  generation 26 and envelope/gain/mix 19 instructions per slot-sample, plus 29
  with the native filter; final voice mix 41 per sample; no divides or calls in
  per-sample loops. Silent slots are skipped.
- Voices pair across cores as in Melodee's Prophet. CPU0 submits a voice to
  CPU1 through the donor mailbox when another voice will render, renders the
  next one itself, then joins and adds. Whole-block dispatch
  (`render_dispatch.h`) is no longer used. The worker, mailbox and kernel are
  SRAM-resident.

Estimated work: about 220 instructions per voice-sample against 455, before the
second core. This is an estimate from instruction counts, not a hardware
measurement. GLOBE Engine now reports CPU1 ONLINE, CPU1 VOICES (cumulative
voice jobs) and CPU1 TIMEOUTS. A failed join drops that voice's block, never
re-renders it.


## Measured failure

Owner's latest installed-candidate capture: 2 voices, total 2916 us,
render 1835 us, scanner 1032 us, USB 12 us, other 37 us. Guard threshold
2466 us; nominal 128-frame/44.1 kHz deadline approximately 2902 us.
These are triggering-buffer measurements, not a benchmark of maximum polyphony.

## Melodee source comparison

- `vendor/melodee/firmware/src/eng_fm6.c` and
  `eng_prophet_test.c` submit independent voice jobs through `audio_worker.h`.
  Animate now uses that mailbox to render a complete control block on CPU1
  while CPU0 services scanner/USB interrupts. It does not split voices between
  cores: shared caches, pan buffers and sequencing have one owner at a time.
- Donor `voice.c` caches global tuning and updates modulation in 32-sample
  control blocks. The Animate bridge already inherits much of that host.
- Donor `tools/build.py` compiles annotated LLVM IR with O2 while
  `tools/size_fns.py` marks cold functions for size. Its comment still says Os;
  the actual command selects O2. Our board and bridge use O2, other units Os.
- FM-1 `main.c` schedules input at 10 kHz. SMK's different board scanner chains
  SPI completions. This is not evidence that the FM-1 scan cadence can simply
  be copied: contact debounce, velocity timing, LEDs and analog acquisition
  must remain correct on the SMK.

## Implemented next step

The custom bridge caches the four integer-path slot filter coefficients by
exact cutoff/resonance inputs. Each voice retains its own integrators. Actual
inputs are checked on each invocation, including alternating step-lock values;
there is no reliance on UI invalidation. Other donor builds use the original
calculation through a default macro. No quality, timing or UI contract changed.

All 128 x 128 cutoff/resonance pairs across four slots, repeated cache hits and
alternating keys match uncached coefficients. Runtime/reference fixtures and
hardware compile passed. Physical performance improvement is unmeasured.

## Complete-block worker integration

CPU0 consumes inputs, submits one 32-frame runtime block, and joins before
touching runtime state, preparing USB samples or proceeding to the next block.
The main loop cannot run during this audio ISR. Scanner and USB interrupts
publish input queues, not synth edits. Storage starts from main with no job
outstanding; the idle worker remains in SRAM while flash is unavailable.
The existing worker checks publication, faults and timeouts. Offline submission
renders serially. Failed joins stop the worker, silence the partial block and
release notes; partially advanced state is never rendered a second time.

Host serial/threaded runtime checks produce identical PCM hashes with eight
held notes, source changes, extended filters/pan and transport. Dispatch failure
checks cover unavailable worker, success, partial failure and serial fallback.
These checks do not establish physical CPU1 speed, stack margin or bus behavior.

GLOBE's Engine page has read-only CPU1 ONLINE, CPU1 BLOCKS and CPU1 TIMEOUTS
in its previously unused rows. ONLINE should be 1 and BLOCKS should increase.
RENDER US retains its CPU0 elapsed-minus-interrupt accounting: with the worker
it measures waiting/dispatch time excluding CPU0 interrupts, not exclusive CPU1
DSP time. Scanner and DSP now overlap; use TOTAL US and late/shedding counters
to assess deadline success, not addition of independent CPU costs.

Analog conversion dispatch now indexes pad/fader destinations directly rather
than searching 20 entries. All mappings match the existing tables; ADC, scan
cadence, debounce and velocity behavior are unchanged.

## Remaining hardware validation

1. Measure scanner invocation rate and bound its work without changing key
   velocity or losing controls. Do not guess a new SPI divider from CPU load.
2. Confirm CPU1 stays online and deadlines improve on the physical SMK. Further
   per-voice parallelism would require separate caches and accumulation buffers;
   this implementation intentionally transfers the complete runtime block.
3. Measure hardware worst-case polyphony with modulation, effects and UI active.
   Simulator correctness cannot establish the MCU deadline. Do not remove the
   overload guard or lower quality to disguise missed deadlines.
