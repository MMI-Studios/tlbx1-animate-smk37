# Dependency relocation: 2026-11-08

The active application, simulator, packaging and native checks use dependencies
inside this repository. Installed compilers remain external development tools.
The following checks ran in place, not from a fresh clone. No device was flashed or otherwise written.

- Native runtime, patch, visualizer and serial/parallel voice checks passed.
- All 22 unified/reference kernel scenarios passed the existing sound contract.
- Six preserved-host bridge comparisons and independent note ownership passed.
- Legacy WASM four-corner hashes and USER VECTOR session comparison passed.
- All 285 curated WAVs matched the browser preview: 136704 bank samples.
- Simulator WASM and full SMK application compiled successfully.
- Loader, storage, protected ranges and packaging checks passed.
- All 287 imported binary files matched their source SHA-256 hashes.

The hardware application remains 433028 bytes and is byte-identical to the
accepted baseline:

| Artifact | SHA-256 |
| --- | --- |
| Application | `646b924bcde11839d9fea759065816c40aa2c3e4a128b9bd03cf8f2c3d5779e6` |
| Portable update | `9788f87a550247c53a0e12abe94dd67171e1da35106fb28cc04be33ef4fcefde` |
| Pro package | `290a37ca958a9d29449a70d36bd40d82db0e65b68c601d1b4d333d5ed6b82521` |
| Custom package | `d9c74e45e2032c1f23ea37044926be6c65042817ce1d0d2e458eb428d5c34b46` |

The first runtime run reached its spectral check but lacked NumPy. A local
ignored virtual environment was provisioned, subprocess invocation was changed
to use the same interpreter, and the complete runtime suite then passed.
The preserved host oracle emits integer-to-pointer-size warnings from dormant
hardware register helpers on the native host; it completed successfully.

Ignore rules retain source/assets/generated dependency
headers while excluding build output, device backups and toolchain installs.
No initial commit, remote or public publication has been made. Redistribution
provenance for stock firmware and curated wave assets still needs review.
