# Animate 982 hardware build

The dedicated board entry uses the preserved SMK scanner, audio DMA, USB/MIDI,
USB audio, LCD transport, guarded patch storage and RAM updater. The Animate bridge is compiled as a
separate translation unit. No native Melodee engine, sequencer, UI or project
store core is included in the board entry. The bridge includes Melodee's shared
voice lifecycle and overload policy directly, with default-neutral embedding hooks
in the preserved voice.c. The compact patch store adapts the audited root-last A/B
commit method with its own namespace. The previous application's default voice
behavior is retained; the four-engine project is unchanged.

Run `python3 tools/build.py smk` under the configured Linux/WSL toolchain.
`board`, `bridge` and `objects` are intermediate compilation targets and do not
produce a complete firmware image. Generated files live in `build/smk/`.

The build runs the preserved public updater's synthetic-flash tests, checks its
RAM address bounds, retains original flash/stack/boot boundaries and the 152 KiB main
audio arena minimum, and audits every flash routine as RAM-resident leaf code
without external calls or branches while XIP is unavailable. Synthetic tests
exercise interrupted commits, corrupt-copy fallback, protected ranges and the
backup/provisioning protocol. These checks
do not demonstrate physical update recovery or musical performance.

The reviewed complete link is 142020 bytes, with BSS ending at
`0x1c199d0`, pool ending at `0x1c3f198`, and 159328 bytes remaining in the main
audio resource arena after CPU1's private 8 KiB stacks. DSPHOT is 48 KiB at
`0x1c68000` (46696 bytes used); boot-copied constant tables occupy 12276 bytes
of low SRAM. Voices render through `src/baseline/voice_kernel.c`, paired across
CPU0/CPU1 (see PERFORMANCE.md).

Builds default to `AF_DUAL_CORE=1`, matching the Melodee 980 (`poc1-dc`), the
only build reported to perform well on hardware. The HAL then holds CPU1 at boot
and starts it in `fm1_core1_main`/`audio_worker_loop`, which poll only SRAM; the
donor `smk_multicore_audit` runs on every build. With `AF_DUAL_CORE=0`,
`fm1_core1_stop` compiles to nothing and CPU1 is never touched. That reproduces
raw `d6f2d50d…` exactly. Animate submits no CPU1 jobs, so audio output is
identical. Whether CPU1 left running by the bootloader was contending for the
flash/SRAM bus is unproven until hardware comparison. The compiler/linker reported no warnings. Read the
generated manifest and [installation record](INSTALL.md) for exact hashes. Packed project state is
placed in the pool; the bridge uses linker-backed audio resource banks instead
of allocating a duplicate host arena. Shared effects allocate approximately
64 KiB for delay, 4 KiB for chorus and 11.6 KiB for reverb, alongside Animate state.
The source allocation inventory for one part totals 100704 bytes: delay 65536,
chorus 4096, reverb 11872, Animate state 2816, optional slicer 8192 and optional
performance buffer 8192. This inventory is not a measured running peak; it
explains why the unchanged 155648-byte minimum remains conservative without
depending on the additional low/data/retained banks.

The local `app.ld` reserves40KiB at0x1c6a000..0x1c74000 for DSP code, within
the MCU's direct-call range of XIP; the prior low-SRAM location was out of range
for some calls. The region ends at the user-stack guard and is taken from the
upper pool, leaving the arena above its 152 KiB minimum. It holds the native and
extended renderers and every routine reached per audio block: `af_render_q15`,
sequencer advance, voice rendering and modulation, slicer/distortion, mix/spread,
delay, all reverbs, performance effects, master output and visual capture. It
also holds the audio, scanner and USB interrupt entry stubs and bodies, so the
audio deadline no longer depends on flash instruction-cache misses caused by UI
and display code. 39044 bytes are used.

The pi32v2 compiler emits a hardware divide even for constant divisors. The
Animate voice loops (`eng_animate.c` and `extended_render.c`) therefore compute
loop-invariant control terms once per block and use exact reciprocal/shift
replacements (`ANIMATE_DIV127`, `ANIMATE_DIV1000`, `ANIMATE_UDIV65535`,
`ANIMATE_SDIV_POW2`). These were verified against C division for every 32-bit
input. Disassembly shows no divide in either per-sample loop. crt0 occupies a leading `.boot` section
so these named input sections are claimed before the generic `.text` wildcard;
DSP source files are unchanged. Event-time code (note-on, dispatch, controls,
randomization setup) stays in flash. The build asserts the key routines'
placement and verifies every section's load image. Startup copies the section and the build asserts renderer
placement. The bridge uses O2 and inlines small DSP primitives; no fast-math,
gain increase or overload-threshold change is enabled. Physical throughput is
not established by the linked placement or host tests.

Hardware audio consumes the bridge's Q15 output and retains the inherited final
DMA multiplication by 128. USB capture currently carries master left/right in
the first two channels of the inherited four-channel descriptor; the remaining
two channels are zero. This is not ten-lane stem capture. TRS MIDI is not wired
in this board entry. Sixteen compact sound patches can be saved on the device;
complete sequence/projects remain browser exports. There is no automatic storage
initialization on boot. Confirmed first Save can claim a completely erased area;
unrecognized nonblank bytes require the explicit backup/provisioning utility.
Save/load and provisioning pause audio while storage work runs. Flash-driver
interrupt-masked durations and physical save latency have not been measured.

Boot draws the first splash frame (`src/splash.c`) before the scanner and audio
start, without added delay. The main loop then redraws only each frame's changed
rectangles at the 40 ms UI cadence, alongside a pad-light sweep, for 1.6 s or
until a button or encoder event. It then invalidates the LCD cache and repaints
the UI. Splash rendering never runs in an interrupt.

LCD updates use exact command caching and changed-row transfers. Normal screens
are checked every 40 ms; visual analysis and plots run at most every 100 ms.
Unchanged normal screens generate no LCD pixel traffic. Physical responsiveness
and CPU capacity remain unmeasured for this new application.

`node tools/package.mjs` requires a deliberately reviewed
`tools/package-review.json` containing the exact product, version, application
byte count/SHA-256 and loader SHA-256. It also requires the pinned official
Pro016 file and completed build manifest. There is no candidate override.
Packaging creates a Pro-model `.fwsc` and a generic-982 portable update packet;
it neither opens a device nor flashes it. M-UPGRADE may time out waiting for a
model-specific identity after reboot. Interrupted programming may require a
hardware recovery adapter; software-only recovery is not established.
