#!/usr/bin/env python3
"""Pack the owner's curated wavetables into the firmware wave bank.

Reads assets/wavetable-curation.json (exported by tools/wavetable-browser.html)
and the source .wav files, and writes src/generated/wave_bank.h plus
assets/wave-bank-manifest.json.

Each frame is produced exactly as the browser's "256 samples / frame, 16-bit"
preview with "Normalize each table" on, which is what the owner auditioned:
first channel, frames of the curated frame size, linear resampling to 256
samples (wrapping within the frame), per-table peak normalisation, rounding to
16-bit codes. The codes are then scaled to the peak of the original four
Animate waves (18700) so new waves sit at the same level as existing presets.

Bank entries 0..3 are the original four waves, rendered by the firmware from
its preserved tables; this file only lists their names.
"""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'assets' / 'factory'
FRAME = 256
PEAK = 18700          # peak of the original Animate sine table in Q15
ORIGINALS = ['SINE C3', 'SAW C3', 'BELL2 C3', 'VOICE2 C3']
NAME_LEN = 12


def read_wav(path):
    blob = path.read_bytes()
    if blob[:4] != b'RIFF' or blob[8:12] != b'WAVE':
        raise ValueError(f'not a WAVE file: {path}')
    chunks, pos = {}, 12
    while pos + 8 <= len(blob):
        cid = blob[pos:pos + 4]
        size = struct.unpack_from('<I', blob, pos + 4)[0]
        chunks[cid] = (pos + 8, size)
        if cid == b'data':
            break
        pos += 8 + size + (size & 1)
    off, _ = chunks[b'fmt ']
    fmt, channels, _rate, _bps, align, bits = struct.unpack_from('<HHIIHH', blob, off)
    if fmt == 0xFFFE:
        fmt = struct.unpack_from('<H', blob, off + 24)[0]
    doff, dsize = chunks[b'data']
    frames = min(dsize, len(blob) - doff) // align
    raw = np.frombuffer(blob, dtype=np.uint8, count=frames * align, offset=doff).reshape(frames, align)
    b = bits // 8
    first = raw[:, :b].copy()
    if fmt == 3:
        values = first.view('<f4' if b == 4 else '<f8').reshape(-1).astype(np.float64)
    elif b == 1:
        values = (first.reshape(-1).astype(np.float64) - 128) / 128
    elif b == 2:
        values = first.view('<i2').reshape(-1) / 32768
    elif b == 3:
        x = first[:, 0].astype(np.int32) | first[:, 1].astype(np.int32) << 8 | first[:, 2].astype(np.int8).astype(np.int32) << 16
        values = x / 8388608
    else:
        values = first.view('<i4').reshape(-1) / 2147483648
    return values.astype(np.float32)          # the browser keeps samples in a Float32Array


def frames_256(samples, fsz, count):
    """Browser preview: data[k*256+j] = a + (b-a)*q, stored as float32."""
    out = np.empty(count * FRAME, dtype=np.float32)
    for k in range(count):
        base = k * fsz
        for j in range(FRAME):
            x = j * fsz / FRAME
            i0 = int(np.floor(x))
            q = x - i0
            a = float(samples[base + i0])
            b = float(samples[base + (i0 + 1) % fsz])
            out[k * FRAME + j] = a + (b - a) * q
    return out


def preview_codes(data):
    pk = float(np.max(np.abs(data))) if len(data) else 0.0
    if pk > 1e-6:
        data = (data.astype(np.float64) / pk).astype(np.float32)
    clamped = np.clip(data.astype(np.float64), -1, 1)
    return np.floor(clamped * 32767 + 0.5).astype(np.int32)     # JS Math.round


def display_name(name, folder=''):
    if folder.upper().endswith('DWGS'):
        # Korg DWGS waves: drop the catalogue number and the recording pitch.
        name = re.sub(r'^\d+ ', '', re.sub(r'-C\d$', '', name))
    n = re.sub(r'-C(\d)$', r' C\1', name)
    n = re.sub(r'^Spectral Growl ', 'GROWL ', n, flags=re.I)
    n = re.sub(r'^Virus_WT_', 'VIRUS ', n, flags=re.I)
    n = n.upper().replace('_', ' ')
    n = re.sub(r'[^A-Z0-9 .+-]', '', n)
    n = re.sub(r' +', ' ', n).strip()
    return n[:NAME_LEN].rstrip() or 'WAVE'


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--curation', type=Path, default=ROOT / 'assets/wavetable-curation.json')
    ap.add_argument('--source', type=Path, default=SOURCE)
    ap.add_argument('--out', type=Path, default=ROOT / 'src/generated/wave_bank.h')
    ap.add_argument('--manifest', type=Path, default=ROOT / 'assets/wave-bank-manifest.json')
    args = ap.parse_args()
    cur = json.loads(args.curation.read_text())
    if cur.get('format') != 'tlbx1-wavetable-curation':
        raise SystemExit('unexpected curation format')
    entries, pool, report = [], [], []
    for item in cur['items']:
        path = args.source / item['path']
        samples = read_wav(path)
        fsz, count = int(item['frameSize']), int(item['frameCount'])
        if fsz < 2 or count < 1 or fsz * count > len(samples):
            raise SystemExit(f'bad frame layout for {item["path"]}')
        codes = preview_codes(frames_256(samples, fsz, count))
        scaled = np.floor(codes.astype(np.float64) * PEAK / 32767 + 0.5).astype(np.int16)
        start = sum(len(p) for p in pool) // FRAME
        entries.append((display_name(item['name'], item.get('folder', '')), start, count))
        pool.append(scaled)
        report.append({'name': entries[-1][0], 'path': item['path'], 'frameSize': fsz, 'frames': count,
                       'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
    data = np.concatenate(pool)
    total_frames = len(data) // FRAME
    lines = ['/* Generated by tools/build_wavebank.py from assets/wavetable-curation.json. Do not edit.',
             ' * Third-party sound data (Korg MS2000/DWGS, Access Virus, Spectral Growl), resampled;',
             ' * not covered by this repository\'s GPL-3.0-only licence. See LICENSING.md. */',
             '#pragma once', '#include <stdint.h>',
             f'#define AK_BANK_FRAME {FRAME}u',
             f'#define AK_BANK_ORIGINALS {len(ORIGINALS)}u',
             f'#define AK_BANK_COUNT {len(ORIGINALS) + len(entries)}u',
             f'#define AK_BANK_FRAMES {total_frames}u',
             'typedef struct {uint16_t first, frames; char name[13];} ak_bank_entry_t;',
             '/* Flash only: copied into SRAM wave registers on selection, never read while rendering. */',
             'static const int16_t ak_bank_data[AK_BANK_FRAMES * AK_BANK_FRAME] = {']
    for i in range(0, len(data), 16):
        lines.append('    ' + ','.join(str(int(v)) for v in data[i:i + 16]) + ',')
    lines.append('};')
    lines.append('static const ak_bank_entry_t ak_bank_entries[AK_BANK_COUNT] = {')
    for n in ORIGINALS:
        lines.append(f'    {{0, 0, "{n}"}},')
    for name, first, count in entries:
        lines.append(f'    {{{first}, {count}, "{name}"}},')
    lines.append('};')
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text('\n'.join(lines) + '\n')
    manifest = {'format': 'tlbx1-wave-bank', 'frame': FRAME, 'peak': PEAK,
                'curation_sha256': hashlib.sha256(args.curation.read_bytes()).hexdigest(),
                'originals': ORIGINALS, 'entries': report, 'frames': total_frames,
                'bytes': int(len(data) * 2), 'data_sha256': hashlib.sha256(data.tobytes()).hexdigest()}
    args.manifest.write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'{len(ORIGINALS) + len(entries)} entries, {total_frames} frames, {len(data) * 2} bytes')


if __name__ == '__main__':
    main()
