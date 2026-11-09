# Standalone development

Run commands from this repository's root. No TLBX desktop or sibling firmware
checkout is needed. Use Linux x86-64 or WSL for the firmware compiler.

## Tools

Install a C compiler (`cc`), Python 3.10 or newer with venv support, Node.js 22
or newer, curl and xz. Python audio analysis and bank generation use NumPy:

```sh
python3 -m venv build/venv
. build/venv/bin/activate
python -m pip install -r requirements-dev.txt
```

For the simulator, install Zig 0.13.0 and put `zig` on PATH, or export `ZIG`
as the full executable path. The hardware compiler is JieLi's Linux pi32v2
toolchain (clang 4.0.1). The preserved fetch helper installs it locally:

```sh
sh vendor/melodee/tools/get_toolchain.sh "$PWD/tools-local/jieli"
export JIELI_TOOLCHAIN="$PWD/tools-local/jieli/toolchain"
```

The build respects an existing `JIELI_TOOLCHAIN`; otherwise it uses that local
directory. Compiler executables are development tools and are not committed.
The upstream download helper retrieves its current archive rather than pinning
an archive hash; use the compiler version above for release reproduction.

## Build and check

```sh
python tools/check_runtime.py
python tools/build_bridge.py
node tests/wavebank_parity.mjs
node tests/baseline_audio_test.mjs
python tools/build_wasm.py
python tools/build.py smk
node tests/package_test.mjs
node tools/package.mjs
```

`build_wasm.py` writes `web/animate.wasm`. Serve the repository with
`python -m http.server 8795 --bind 127.0.0.1`, then open
`http://127.0.0.1:8795/web/` and enable audio.

The board application and manifests are written under `build/smk/`. Packaging
needs M-VAVE's official SMK-37 Pro V016 firmware, which is not included: download
it from M-VAVE and place it as described in
[assets/stock/README.md](../assets/stock/README.md). Packaging checks its hash and intentionally accepts only the reviewed hashes in
`tools/package-review.json`. A synthesis change will require a new candidate
review; do not remove that check to make packaging pass. These commands do not
flash hardware. Follow [INSTALL.md](INSTALL.md) separately for device operations.

## Wave bank

`src/generated/wave_bank.h` is committed and is sufficient for firmware builds.
`python tools/build_wavebank.py` regenerates it from the child-local
`assets/factory/` WAVs and `assets/wavetable-curation.json`. The parity check above
compares every sample against the browser preview. The wavetable browser can
open `assets/factory/wavetables` directly. Other desktop library assets are not
required or included.

## Imported source maintenance

The active application remains `src/`, including the unified voice kernel.
`vendor/melodee/` is a preserved dependency snapshot; its upstream build and
documentation describe the donor project, not this application's build entry.
Use the root `tools/` commands above. Donor engines remain as reference-test
dependencies and are not exposed as additional Animate instruments.
