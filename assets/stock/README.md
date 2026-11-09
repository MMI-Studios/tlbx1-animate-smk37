# M-VAVE stock firmware (not included)

Packaging needs M-VAVE's official **SMK-37 Pro firmware V016**. It is M-VAVE's
(Cuvave's) software, so it is **not** distributed with this repository; each
builder downloads it from M-VAVE.

1. Go to <https://www.m-vave.com/appdownload> and open **Download → PC Firmware →
   SMK-37 PRO**. Download the V016 firmware file.
2. Do not modify, rename-repack or re-save it. Save it in this folder as:

   ```
   assets/stock/SMK-37 Pro_016.fwsc
   ```

3. Check it. Its SHA-256 must be:

   ```
   2d98ce72530e71d617384963423820536f094501299e2cfec0a8d3c3fb6bcfb0
   ```

   Windows: `certutil -hashfile "assets\stock\SMK-37 Pro_016.fwsc" SHA256`
   macOS/Linux: `shasum -a 256 "assets/stock/SMK-37 Pro_016.fwsc"`

`tools/package.mjs` stops with these instructions if the file is missing or
different, and `tests/package_test.mjs` is skipped. Building, simulating and all
other tests work without it.

If M-VAVE's page offers a newer version only, packaging will refuse it: the
package layout was verified against V016 exactly. The file is ignored by Git
(`.gitignore`), so it is never committed.

How it is used: the update package keeps the stock image's protected bytes
(boot area and device data), so packages built here contain M-VAVE data. Do not
publish built `.fwsc` or `animate-update.bin` files without considering that.
See [`LICENSING.md`](../../LICENSING.md#third-party-data-no-licence-granted).
