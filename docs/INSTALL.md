# Animate 982 installation candidate

## Latest prepared candidate: wave bank and factory sequences (2026-11-07)

Builds on the remembered-polyphony candidate. Adds the 289-wave bank (K1 browses
it per slot), sixteen factory sounds with sequences, wave registers in patches
and sessions (older ones load with the original four waves), and fixes patch and
session validation that rejected polyphony other than 8 or 16. Original sounds
and their audio are unchanged (all exact runtime fixtures pass). Application
433028 bytes of the 581564-byte slot; SRAM +688 bytes, arena unchanged (159328).

| File | SHA-256 |
| --- | --- |
| `animate-candidate.bin` (433028 bytes) | `646b924bcde11839d9fea759065816c40aa2c3e4a128b9bd03cf8f2c3d5779e6` |
| `animate-update.bin` | `9788f87a550247c53a0e12abe94dd67171e1da35106fb28cc04be33ef4fcefde` |
| `Animate-SMK-37-Pro_982.fwsc` | `290a37ca958a9d29449a70d36bd40d82db0e65b68c601d1b4d333d5ed6b82521` |
| `Animate-SMK-37_982.fwsc` | `d9c74e45e2032c1f23ea37044926be6c65042817ce1d0d2e458eb428d5c34b46` |

## Latest prepared candidate: remembered polyphony 1–16 (2026-11-07)

Same as the owner-accepted unified-kernel build (`eef8b8f4…`), plus GLOBE Audio
POLYPHONY adjustable from 1 to 16 in steps of one (default 8), changing only while
no voice is sounding. The value is stored as a 16-byte record after the ownership
header in the storage marker sector and restored at power-on; this needs owned
on-device patch storage. Patch slots are not touched. After 240 changes the
marker sector is erased and its header rewritten first; power loss in that brief
window would leave storage unowned (patch data stays in place).

| File | SHA-256 |
| --- | --- |
| `animate-candidate.bin` (142772 bytes) | `1776b7c016d29db4fb44617ef47471bb9e04e7b27d0fc4715fd67eacc78b5d90` |
| `animate-update.bin` | `ae71f8486fe1225993f78239d20c0147fea79505eaa005a450980fc5f3a9cc36` |
| `Animate-SMK-37-Pro_982.fwsc` | `d0285d5a91cd1a747f0b242adeee8bced1514374c2fae4ebcb33e57ba64f4381` |
| `Animate-SMK-37_982.fwsc` | `d4184e301cc9bd5095e59d6bf27954ad59d57935105a864d29de374f9a38b2a3` |

## Latest prepared candidate: unified voice kernel with parallel voices (2026-11-07)

Replaces the per-sample Animate renderer and its separate edited-control path
with one block-rate kernel. CPU0 and CPU1 now render voices in parallel, as
Melodee's Prophet does, and editing a control no longer switches voices to a
more expensive path. Sound is near-identical rather than bit-identical (owner
decision; see SOUND_CONTRACT.md). Measured against the reference renderer: 49.6–72.8 dB
SNR, ≤ 0.10 dB spectral difference, preset energy within 0.024% of the donor.
Includes the SRAM table placement and SCAN IRQS readout from the previous
candidate; the guard remains disabled.

| File | SHA-256 |
| --- | --- |
| `animate-candidate.bin` (142020 bytes) | `3f2ba7aaec6a1c9bc68704f26feaecb90ce6f36fdb20743b21e3b736713bce70` |
| `animate-update.bin` | `eef8b8f46c99d4c5fdcfe178fc62fbb8eb4d4d3c23b673746d688134df570a02` |
| `Animate-SMK-37-Pro_982.fwsc` | `c7f8ba1d54ec93269aec532e2e54ad5f365feb6d56a70fd496a3ac7cdbaab908` |
| `Animate-SMK-37_982.fwsc` | `29a245f96cf7ca02e354b8608101fbd5ffc7f71143e489c715deae5bb6d23730` |

Hardware polyphony is unmeasured. After installing, check GLOBE → Right twice
(Engine): CPU1 ONLINE should be 1 and CPU1 VOICES should rise while playing
chords. If notes still fail, photograph OVERLOAD (GLOBE → Left).

## Latest prepared candidate: scanner/audio data in SRAM (2026-11-07)

Builds on the installed slot-edit repair (`b0954d16…` portable); guard still
disabled. Measured trigger captures showed scanner time of 1032–1163 µs per
2902 µs buffer at two voices, and 148 µs idle versus 652 µs with notes held.
Every constant table the scanner, audio, USB and CPU1 render paths read (key
contact map, scan-select words, pad/encoder/mux maps, wavetables, SINE, PITCH_OCT,
TANH, LFO/level/filter tables, `DIV_DEN`, …; 12276 bytes) is now copied to SRAM at
boot. Previously all of it was flash `.rodata` behind the XIP data cache, shared
by both cores while voices render. The CPU1 block handoff (`af_render_job_run`,
`audio_worker_join/submit`) and small per-block helpers
(`bridge_filter_coefficients`, `spread_sum`, `tune_cache_update`,
`af_voice_count`) moved into the SRAM code window, which grew 44→48 KiB. The audio
arena is 159328 bytes, above its 155648 minimum. No source logic changed; the
build adds `-fdata-sections` and linker placement only. The OVERLOAD page's last
row is now **SCAN IRQS**, the scanner interrupt count during the captured buffer,
separating interrupt rate from per-interrupt cost.

| File | SHA-256 |
| --- | --- |
| `animate-candidate.bin` (140356 bytes) | `2ff31e8bbe87e1068b54ace2890a7f6524f5661a40d52294eab4aef01c34b777` |
| `animate-update.bin` | `dfb2b9b224ef3bb5e326b2f581ad263ae0bfe26ba9bae2292affe3941b2d14e1` |
| `Animate-SMK-37-Pro_982.fwsc` | `b2eca5e0510c28b581191652924bc0f7eb00175f127579f518e7b75072e32013` |
| `Animate-SMK-37_982.fwsc` | `50b69c8d3b2611f1f6453616695261f78d414070666aa62d8d317d260e1b1365` |

Hardware effect is unmeasured. The contention explanation is an inference from
the idle/held scanner difference, not a measured cause. The recovery wrapper
is still pinned to an older package. Install through the 8795 page's explicit
loader entry (the owner-verified route for `b0954d16…`).

## Latest prepared repair: restored slot values clear edit flags

The current `build/smk` files now contain an uninstalled follow-up to the
guard-disabled build described below. Slot edits restored exactly to native
values clear their extension flags. Automatic shedding remains disabled.
Runtime regressions, hardware build and packaging passed; hardware behavior is
unverified and the cost of genuinely extended settings remains unresolved.

- Raw application: `6bab5c3583ce8226d49e8681e1f4a813108cb1f5c3bf1a2e172ca9fe96f38410`
- Portable: `b0954d166d5703dd5b04698f8c2bd6a5e429457c45f794d7073f67d54247019f`
- Pro `.fwsc`: `bde466c94226cf7637aa9e21f28ff46fb6869f8d08ee98331a005617efdfe69f`

The recovery wrapper still pins the previous installed package; it intentionally
rejects this newer candidate until its installation is selected.

## Current comparison build: automatic overload shedding disabled

The owner requested this experiment after two notes worked but additional notes
and live edits degraded polyphony. The original guard is preserved unchanged in
`vendor/melodee/firmware/src/smk37_audio_budget.h`; the active
adapter uses `src/audio_budget_monitor.h` instead. Overload capture and late-buffer
diagnostics remain, but they no longer remove voices. SHED VOICES should stay zero.
Output limiting and CPU1 fault handling are unchanged. Actual missed audio
deadlines may still glitch; this experiment does not establish a performance fix.

Previous candidate (installed and device-CRC verified November 7,
17:20; hardware sound comparison pending):

| File | SHA-256 |
| --- | --- |
| `animate-candidate.bin` | `83e8c717dc5429b1134cf8db2639b9e4f9543a18e9489a6bc68fe82e89365ecd` |
| `animate-update.bin` | `f45e9cdc1007527b24e5c53fe39fb42316d8546391f391c10dbbd82f8881d50e` |
| `Animate-SMK-37-Pro_982.fwsc` | `fcbf8adc2615cb5accdc24065a2f41cc95a9d77913a360f05a38b2fb20968b4c` |
| `Animate-SMK-37_982.fwsc` | `0fae79374f3d89fa4d448e127bac562281433bac3681e84ea539f3ef04faa455` |

Focused no-shedding regression, SMK compilation and package checks passed.
The recovery-install script is pinned to the guard-disabled Pro package above.
USB identity replies still fail on the installed candidate; use the recovery
route for this device, not repeated browser discovery attempts.
The hashes later in this document identify the previous, now-installed build.

This is the new Animate-only application, not the preserved four-engine 981
firmware. Earlier982 builds booted on the owner's SMK-37 Pro but failed polyphony
and note-continuity acceptance. The owner selected this custom-UI firmware again
on November 7 as the active repair target. Returning to it is not an audio fix.
Compiling and packaging do not establish sound, CPU
headroom, USB behavior or recovery on the physical device.

## Updating from Melodee 980 or our custom 981/982 firmware

### Explicit entry when application identity does not reply

This alternate browser route is implemented and mock-tested, but has not yet
been verified on the physical SMK. It does not repair the application identity
response itself.

1. Hard-refresh http://127.0.0.1:8795/web/update.html and select `animate-update.bin`.
2. Click **Refresh MIDI ports** and select the SMK's **Melodee** output.
3. Save any sound you need, then check the saved-state confirmation.
4. Click **Start device updater**. This stops sound and discards unsaved edits;
   it sends no firmware payload and does not erase flash.
5. Wait for **Verified ota-SMK-37_982**, then click **Install / resume Animate 982**.

If the loader does not reply, programming stays disabled. Preserve the log;
the recovery route remains available. Do not bypass loader verification.

**Current device exception:** ordinary browser discovery repeatedly receives no
application identity reply, even though Melodee MIDI ports are visible. Use the
explicit entry above rather than repeating discovery. The verified alternative
is download mode followed by the hash-pinned M-Flasher recovery install.
The ordinary browser workflow below requires a compatible identity reply.

Use http://127.0.0.1:8795/web/update.html in Chrome or Edge. Close M-UPGRADE and
other MIDI applications, keep the controller powered and connected, select
`build/smk/animate-update.bin`, then Connect and check. After the loader is
identified, tick the confirmation and choose Install / resume. The same flow
handles normal `SMK-37_980` / `SMK-37_981` / `SMK-37_982` and their
already-active `ota-SMK-37` loader identities.
Do not give this loader either .fwsc variant: it expects the portable format.

Owner's M-UPGRADE attempt transferred1024 header bytes, then the loader reported
SMKU code2 with writing=false (header/profile rejection). No application erase
or programming had begun in that attempt. Changing the wrapper model name fixed
only the initial model check, not the protocol mismatch. The local updater uses
the existing SMK transport and verifies the exact reviewed portable package.
Browser file-validation/gating passed without MIDI access; actual installation
through this page was subsequently reported by the owner; this new candidate's
physical performance remains unverified.

The offline build outputs are in `build/smk/`:

- `Animate-SMK-37-Pro_982.fwsc`: Pro-model package for the official M-UPGRADE
  workflow, reconstructed from the exact official Pro016 container.
- `Animate-SMK-37_982.fwsc`: same payload, labelled for an SMK-37 Pro already
  reporting generic `SMK-37`. This wrapper is not compatible with our981 portable
  RAM loader; use the browser flow above. It remains an offline packaging artifact,
  not a validated custom-firmware installation route.
- `animate-update.bin`: generic `SMK-37_982` portable packet for a compatible
  custom RAM-loader updater. It is not an M-UPGRADE file.
- `animate-candidate.bin`: raw linked application; do not give this to the
  official updater.
- `manifest.json` and `package-report.json`: byte counts, SHA-256 hashes and
  memory/package evidence for the generated files.

Reviewed scanner-overhead build (2026-11-07), still identified as982:

| File | Bytes | SHA-256 |
| --- | ---: | --- |
| `Animate-SMK-37-Pro_982.fwsc` | 705252 | `8cdf7800abf107fbd9a562938187d54092efd5b990c33cc9f2fc1b02126e0bf9` |
| `Animate-SMK-37_982.fwsc` | 705252 | `545dd5b7801d629b5b7539dc42bd1ffed0084bbab578c81bd8a49802174a0fca` |
| `animate-update.bin` | 627712 | `9ca291f172b3f42723c83c95ef049539a8126fa7a2a3179dbcbc4e0bf10ebb1f` |
| `animate-candidate.bin` | 140396 | `9728be6593d5cf439ef3df2f44778b58efc6da1c3dc3ed171039dbad64ae4f28` |

This candidate uses CPU1 for complete audio blocks while CPU0 handles input/USB
interrupts. After boot, GLOBE → Right twice → Engine shows CPU1 ONLINE (expected
1), CPU1 BLOCKS (increasing) and CPU1 TIMEOUTS (expected 0). Host audio equivalence,
worker failure handling, hardware compilation and package checks passed; physical
polyphony and deadline improvement remain unverified. Test a held chord and source
changes; if clicking remains, record these counters plus GLOBE Overload. RENDER US
now includes CPU0 waiting for CPU1, excluding CPU0 interrupts; it is not CPU1's
exclusive DSP time. TOTAL US still measures the full audio deadline.

The preceding CPU1-held build (`44b8b799…`) still failed on hardware. This build
adds a frozen first-trigger capture under **GLOBE → Left → Overload**. Reboot,
play the failing held chord, then photograph that page. It records stages and
voice count before removal rather than the average after voices have disappeared.
The captured two-voice failure measured3092us total,1876us render,1163us scanner,
14us USB and38us other, with2466us guard limit. This build removes unused
scanner diagnostic histories and compiles board services at O2. It
also uses cached button-light masks, eliminating a 19-indicator calculation
on every other SPI transfer. All524288 combinations passed equivalence checks.
The preceding scanner-cleanup build still failed (2916us total,1032us scanner).
Fast-code SRAM
is44KiB, with163424bytes audio arena (152KiB minimum preserved). Gain, synthesis,
scan cadence and guard policy are unchanged. Physical improvement is unverified.
CPU1 remains parked; Animate submits no CPU1 work.

The SRAM/splash candidate (`10245156…` portable) still dropped a second held note:
the overload guard sheds the newer voice when two halves exceed 85%. This build
removes every hardware divide from the per-sample voice loops (about thirty per
sample and voice), bit-identically, on top of the earlier change. That change moves every per-block audio routine and the audio, scanner and USB
interrupt bodies from flash execute-in-place into SRAM. That removes flash
instruction-cache misses from the audio deadline. It is a targeted, unmeasured
change: confirm on hardware with GLOBE → Right (RENDER US, IRQ US, OTHER US,
SHED VOICES) and GLOBE Audio (AUDIO %, VOICES, LATE BUFFERS) under held chords.
It also adds the boot splash and restyled screens.

The complete SMK link and synthetic updater/packaging tests passed. Packaging
verified protected-byte reconstruction and rejected altered stock/candidate
data, wrong identities/versions and truncation. No physical device was accessed.
This package includes Home, guarded patch navigation and on-device sound-patch
storage. Navigation checks preserve
the patch values and edit masks; the six reference audio fixtures remain exact.
This build reuses the shared Melodee voice lifecycle and graceful overload policy,
caches unchanged extended-control coefficients and moves visual preparation out
of the audio IRQ. Renderers/helpers now execute in reserved DSP SRAM with
throughput-optimized compilation and exact, cheaper wavetable lookup arithmetic.
Its physical16-voice capacity is not established. First assess
plain held chords and live source changes, then edited/modulated patches; watch
active voices and whether late-buffer/shedding counts continue rising.

Use the official Pro016 firmware as the stock baseline. Keep the stock Pro016
file available. A Pro running our981 custom firmware uses the portable browser
flow above. Offline checks prove the two .fwsc files differ only in wrapper identity
bytes, but neither is the format expected by that custom loader.
The generic application identity can
make M-UPGRADE time out after the controller reboots; a timeout by itself does
not identify what is running. Read the device identity again before retrying.
Do not repeatedly start another write while an update remains in progress.

The custom packet requires an already compatible updater/loader. Merely naming
it `.bin` does not make it acceptable to a stock firmware or arbitrary uploader.
No automatic install, USB connection or flashing is performed by the build or
packaging tools. An interrupted application write can require a hardware
recovery adapter; a software-only recovery guarantee is not available.

Current hardware-specific limits:

- Sixteen on-device sound-patch slots save all four slots, modulation, effects
  and musical settings. Sequence/step locks remain in browser project exports.
  No storage area is erased or claimed on boot.
- USB audio capture carries master L/R in channels 1–2; channels 3–4 are zero.
  TRS MIDI is not connected in this board entry.
- Parameter-rich audio performance and full-scope live randomization have not
  been measured on the physical MCU. Randomization is processed incrementally
  while audio continues, rather than performing a large interrupt-stack copy.
- Physical controls, display responsiveness, audio level and update/recovery
  behavior still require hardware acceptance. The earlier 981 boot result does
  not validate this replacement application.

See [hardware build details](HARDWARE_BUILD.md) for memory and validation scope.

## First on-device patch save

A confirmed Save can claim storage only if all 135168 bytes reserved for patch
slots and their ownership marker are erased. The official Pro016 update file
does not include this area, so its contents cannot be inferred from that file.
If Save reports a storage error on an uninitialized device, open
`web/provision.html` on the same local server as the simulator.

1. Explicitly allow MIDI access, select the SMK ports and read storage status.
2. Read and download the complete backup. Keep it outside the temporary download
   area. Select the saved file in the utility; every byte must match the backup.
3. Type `ERASE` and press **Initialize patch storage** only if you intend to erase
   existing data in the dedicated patch area. Keep the controller connected and
   wait for the verified result; the utility allows up to 60 seconds.
4. Return to Home on the controller and save a user patch.

The device requires the complete backup, matching nonce/CRC and an unchanged
fresh reread before provisioning. It erases only `0x9d000..0xbd000` and the
ownership sector `0xfe000..0xff000`. Calibration, app, boot and all other sectors
remain outside the write policy. Patch replacements commit the new header last,
preserving the previous verified copy if a replacement is interrupted.

Audio pauses during storage operations; initialization may take several seconds.
Actual hardware timing and power-loss recovery remain untested. The included C
protocol tests use simulated flash; the browser tests use a mock device. No
controller was connected or provisioned during development. The backup utility
does not yet provide a one-click restoration operation; retain its raw backup.
