#!/usr/bin/env python3
"""Exact cached/uncached extension audio comparison; no hardware access."""
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]
DONOR = ROOT / 'vendor' / 'melodee'
OUT = ROOT / 'build' / 'host'
OUT.mkdir(parents=True, exist_ok=True)
results = []
for name, defines in [('cached', []), ('uncached', ['-DAB_UNCACHED_COEFFICIENTS=1'])]:
    target = OUT / ('coefficients-' + name)
    subprocess.run(['cc', '-std=gnu11', '-O2', '-fwrapv', *defines,
                    '-I'+str(DONOR/'firmware/src'), '-I'+str(DONOR/'build/gen'),
                    str(ROOT/'tests/bridge_coefficients.c'), str(ROOT/'src/animate_math.c'),
                    '-o', str(target)], check=True)
    results.append(subprocess.check_output([target]))
assert results[0] == results[1], (results[0], results[1])
print('Cached/uncached audio matched exactly: 30 live-edit stages, 16 voices, all 20 slot fields,')
print('per-voice locks, cache eviction, tempo, vector modulation, masks, filter changes and release.')
